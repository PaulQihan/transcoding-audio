#include <iostream>
#include "transcode.h"

DecodeAudio::DecodeAudio() {

}

int DecodeAudio::Decode_Audio_Init(string InputUrl) {
	AVCodec* decoder_audio = NULL;
	AVStream* audio = NULL;
	avdevice_register_all();
	avformat_network_init();   
	AVDictionary* options = NULL;
    int ret = 0;
    if (avformat_open_input(&input_ctx, InputUrl.c_str(), NULL, &options) != 0) {
		fprintf(stderr, "Cannot open input file '%s'\n", InputUrl.c_str());
		return -1;
	}
	if (avformat_find_stream_info(input_ctx, NULL) < 0) {
		fprintf(stderr, "Cannot find input stream information.\n");
		return -1;
	}
	av_dump_format(input_ctx, 0, InputUrl.c_str(), 0);
	ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder_audio, 0);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a audio stream in the input file\n");
		//return -1;
	}
	audio_stream = ret;	
	if (!(audio_decoder_ctx = avcodec_alloc_context3(decoder_audio)))
		return AVERROR(ENOMEM);
	audio = input_ctx->streams[audio_stream];
	if (avcodec_parameters_to_context(audio_decoder_ctx, audio->codecpar) < 0)
		return -1;
    decoder_audio = avcodec_find_decoder(audio_decoder_ctx->codec_id);
	if ((ret = avcodec_open2(audio_decoder_ctx, decoder_audio, NULL)) < 0) {
		fprintf(stderr, "Failed to open codec for stream #%u\n", audio_stream);
		return -1;
	}
	return 0;
}

int DecodeAudio::Decode_Audio_Frame(AVFrame *frames[], int *frame_count){
    avcodec_flush_buffers(audio_decoder_ctx);

	AVPacket packet;

	int ret = 0;
	while (ret >= 0)
	{
		if (ret = av_read_frame(input_ctx, &packet) < 0)
			break;
        if(packet.size > 0){
            ret = Decode_Audio_Write(&packet, frames, frame_count);
		    av_packet_unref(&packet);
        }


		return ret;
	}
	/* flush the decoder */
	packet.data = NULL;
	packet.size = 0;
    ret = Decode_Audio_Write(&packet, frames, frame_count);
	av_packet_unref(&packet);

	return -22;    
}

int DecodeAudio::Decode_Audio_Write(AVPacket* pkt, AVFrame **frames, int *frame_count)
{
	int i, ch;
	int ret, data_size;
    AVFrame *frame = NULL;

	ret = avcodec_send_packet(audio_decoder_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error submitting the packet to the decoder\n");
            cout << ret << endl;

		exit(1);
	}


	while (ret >= 0) {
		if (!(frame = av_frame_alloc())) {
			fprintf(stderr, "audio Can not alloc frame\n");
			ret = AVERROR(ENOMEM);
			exit(1);
		}

		ret = avcodec_receive_frame(audio_decoder_ctx, frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_frame_free(&frame);
			return 1;
		}

		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}
		data_size = av_get_bytes_per_sample(audio_decoder_ctx->sample_fmt);
		if (data_size < 0) {
			/* This should not occur, checking just for paranoia */
			fprintf(stderr, "Failed to calculate data size\n");
			exit(1);
		}
        frames[*frame_count] = frame;
        *frame_count += 1;
        cout << frame->pts << endl;

	}
}

int DecodeAudio::Decode_Audio_Destroy() {
	avcodec_free_context(&audio_decoder_ctx);

	return 0;
}

AVFormatContext* DecodeAudio::getDecAVFCtx() {

	return input_ctx;
}

DecodeAudio::~DecodeAudio() {
    this->Decode_Audio_Destroy();
}

EncodeAudio::EncodeAudio(AVFormatContext *input_ctx) {
    this->input_ctx = input_ctx;
}

void EncodeAudio::add_stream(OutputStream* ost, AVFormatContext* oc, AVCodec** codec, enum AVCodecID codec_id) {
	AVCodecContext* c;
	int i;    
    *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!(*codec)) {
		fprintf(stderr, "Could not find encoder for '%s'\n",
			avcodec_get_name(codec_id));
        exit(1);
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	ost->enc = c;
    c->sample_fmt = (*codec)->sample_fmts ?
        (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = 64000;
    c->sample_rate = 48000;//44100;
    if ((*codec)->supported_samplerates) {
        c->sample_rate = (*codec)->supported_samplerates[0];
        for (i = 0; (*codec)->supported_samplerates[i]; i++) {
            if ((*codec)->supported_samplerates[i] == 48000)//44100)
                c->sample_rate = 48000;// 44100;
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    if ((*codec)->channel_layouts) {
        c->channel_layout = (*codec)->channel_layouts[0];
        for (i = 0; (*codec)->channel_layouts[i]; i++) {
            if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                c->channel_layout = AV_CH_LAYOUT_STEREO;
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    AVRational a_time_base = { 1, c->sample_rate };
    ost->st->time_base = a_time_base;
}

AVFrame* EncodeAudio::alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
	AVFrame* frame = av_frame_alloc();
	int ret;

	if (!frame) {
		fprintf(stderr, "Error allocating an audio frame\n");
		exit(1);
	}

	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0) {
			fprintf(stderr, "Error allocating an audio buffer\n");
			exit(1);
		}
	}

	return frame;
}


void EncodeAudio::open_audio(AVFormatContext* oc, AVCodec* codec, OutputStream* ost, AVDictionary* opt_arg) {
	AVCodecContext* c;
	int nb_samples;
	int ret;
	AVDictionary* opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
		exit(1);
	}

	/* init signal generator */
	ost->t = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_FLT, c->channel_layout,
		c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}

	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		exit(1);
	}

	/* set options */
	av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
	av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		exit(1);
	}
}

int EncodeAudio::Encode_Audio_Init(const char* OutputUrl, const char *out_format){
	AVOutputFormat* fmt;
	AVCodec* audio_codec;
	int ret;
	audio_st = { 0 };

	AVDictionary* opt = NULL;
	int i;

	/* Initialize libavcodec, and register all codecs and formats. */
	avdevice_register_all();
	avformat_network_init();
	av_dict_set(&opt, "fflags", "flush_packets", 0);

	avformat_alloc_output_context2(&oc, NULL, out_format, OutputUrl);
	if (!oc)
		return 1;

	fmt = oc->oformat;

    add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);

	open_audio(oc, audio_codec, &audio_st, opt);

	av_dump_format(oc, 0, OutputUrl, 1);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&oc->pb, OutputUrl, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open '%s': %s\n", OutputUrl,
				av_err2str(ret));
			return 1;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(oc, &opt);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file: %s\n",
			av_err2str(ret));
		return 1;
	}

	return 0;   
}

int EncodeAudio::write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c, AVStream* st, AVFrame* frame) {
	int ret;

	// send the frame to the encoder
	ret = avcodec_send_frame(c, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame to the encoder: %s\n",
			av_err2str(ret));
		exit(1);
	}

	while (ret >= 0) {
		pkt = { 0 };

		ret = avcodec_receive_packet(c, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
			exit(1);
		}

        ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);//to do 
        
        pkt.stream_index = st->index;

		ret = av_interleaved_write_frame(fmt_ctx, &pkt);
		av_packet_unref(&pkt);
		if (ret < 0) {
			fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
			return 1;
		}
	}

	return ret == AVERROR_EOF ? 1 : 0;    
}

int EncodeAudio::Encode_Audio_Frame(AVFrame* frame) {
	AVFormatContext* fmt_ctx = oc;
	OutputStream* ost = &audio_st;
	AVCodecContext* c;
	int ret;
	int dst_nb_samples;

	c = ost->enc;

	if (frame) {

		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);


		ret = av_frame_make_writable(ost->frame);
		if (ret < 0)
			exit(1);

		/* convert to destination format */
		ret = swr_convert(ost->swr_ctx,
			ost->frame->data, dst_nb_samples,
			(const uint8_t**)frame->data, frame->nb_samples);
		if (ret < 0) {
			fprintf(stderr, "Error while converting\n");
			exit(1);
		}

		AVRational at_timebase = { 1, c->sample_rate };
		frame->pts = av_rescale_q(ost->samples_count, at_timebase, c->time_base);
		ost->samples_count += dst_nb_samples;
	}

	return write_frame(oc, c, ost->st, frame);
}

void EncodeAudio::close_stream(AVFormatContext* oc, OutputStream* ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}

int EncodeAudio::Encode_Audio_Destory() {

	av_write_trailer(oc);

	/* Close each codec. */
	close_stream(oc, &audio_st);

	if (!(oc->oformat->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&oc->pb);

	/* free the stream */
	avformat_free_context(oc);
	return 0;
}

EncodeAudio::~EncodeAudio() {
    this->Encode_Audio_Destory();
};