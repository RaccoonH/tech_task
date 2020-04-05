extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int openInputFile(char* filename, AVFormatContext** pFormatCtx,
	AVCodecContext** pCodecCtx, AVCodec** pCodec, int* video_stream, char* resolution);

int openOutputFile(char* filename, AVCodecContext *inputCodecCtx,
	AVFormatContext **outputFormatCtx,
	AVCodecContext **outputCodecCtx);

int addFrame(AVFormatContext *outFormatContex, AVCodecContext *outCodecContex, AVFrame *frame);

