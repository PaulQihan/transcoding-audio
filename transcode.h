
#pragma once
#include <iostream>

#include <string>
#include <stdio.h>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"
#include "libavutil/hwcontext.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libavutil/imgutils.h"
#include "libavdevice/avdevice.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/time.h"
}

using namespace std;

class DecodeAudio {

public:
    DecodeAudio();
    int Decode_Audio_Init(string InputUrl);
    int Decode_Audio_Destroy();
    int Decode_Audio_Frame(AVFrame *frames[], int *frame_count);
    ~DecodeAudio();
    AVFormatContext * getDecAVFCtx();

private:
    int Decode_Audio_Write(AVPacket *packet, AVFrame **frames, int *frame_count);
    AVCodecContext *audio_decoder_ctx = NULL;
    int audio_stream;
    AVFormatContext *input_ctx = NULL;

};
typedef struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;
class EncodeAudio {

public:
    EncodeAudio(AVFormatContext *input_ctx);
    int Encode_Audio_Init(const char* OutputUrl, const char *out_format);
    int Encode_Audio_Frame(AVFrame *frame);
    int Encode_Audio_Destory();
    AVPacket pkt;
    ~EncodeAudio();
private:
    void add_stream(OutputStream* ost, AVFormatContext* oc, AVCodec** codec, enum AVCodecID codec_id);
    void open_audio(AVFormatContext* oc, AVCodec* codec, OutputStream* ost, AVDictionary* opt_arg);
    int write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c, AVStream* st, AVFrame* frame);
	AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
    void close_stream(AVFormatContext* oc, OutputStream* ost);
    AVFormatContext *input_ctx = NULL;
    AVFormatContext *oc;

	bool flag;
	OutputStream audio_st;
};