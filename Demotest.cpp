#include "transcode.h"

AVFrame* frames[100]  = {NULL};
int frame_count = 0;
int main() {
    DecodeAudio * decoder = new DecodeAudio();
    decoder->Decode_Audio_Init("test2.aac");
    AVFormatContext* dec_AVFcontext = decoder->getDecAVFCtx();
    EncodeAudio * encoder = new EncodeAudio(dec_AVFcontext);
    encoder->Encode_Audio_Init("out.aac", "flv");
    int ret = 0;
    int count = 0;
    while(1) {
        frame_count = 0;
        ret = decoder->Decode_Audio_Frame(frames, &frame_count);
		if (ret < 0) {
			printf("error or end\n");
            for(int i = 0; i < frame_count; i++) {
                ret = encoder->Encode_Audio_Frame(frames[i]);
                cout << count++ << endl;

                av_frame_free(&frames[i]);
                if (ret < 0) {
                    printf("encode error\n");
                    std::cout << ret << std::endl;
                    continue;
                }
            }
			goto fail;
		}       
        for(int i = 0; i < frame_count; i++) {
            ret = encoder->Encode_Audio_Frame(frames[i]);
            cout << count++ << endl;

            av_frame_free(&frames[i]);
            if (ret < 0) {
                printf("encode error\n");
                std::cout << ret << std::endl;
                continue;
            }
        }

    }
    fail:
        delete decoder;
        delete encoder;
        return 0;
}