#include "yuvVideo.h"
#include <iostream>

using namespace std;

int openInputFile(char* filename, AVFormatContext** pFormatCtx,
	AVCodecContext** pCodecCtx, AVCodec** pCodec, int* videoStream, char* resolution)
{
	AVInputFormat * input = NULL;
	AVDictionary * opts = NULL;

	input = av_find_input_format("rawvideo");
	av_dict_set(&opts, "video_size", resolution, 0);

	if (avformat_open_input(pFormatCtx, filename, input, &opts) != 0)
	{
		cout << "Couldn't open input videofile" << endl;
		return -1;
	}
	if (avformat_find_stream_info(*pFormatCtx, NULL) < 0)
	{
		cout << "Couldn't find stream info" << endl;
		return -1;
	}

	av_dump_format(*pFormatCtx, 0, filename, 0);

	// Find the video stream
	*videoStream = -1;
	for (int i = 0; i < (*pFormatCtx)->nb_streams; i++)
	{
		if ((*pFormatCtx)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			*videoStream = i;
			break;
		}
	}
	if (*videoStream == -1)
	{
		cout << "Couldn't find videostream" << endl;
		return -1;
	}
	// Get a pointer to the codec context for the video stream
	*pCodecCtx = (*pFormatCtx)->streams[*videoStream]->codec;

	// Find the decoder for the video stream
	*pCodec = avcodec_find_decoder((*pCodecCtx)->codec_id);
	if (*pCodec == NULL) 
	{
		cout << "Unsupported codec!" << endl;
		return -1;
	}

	return 1;
}

int openOutputFile(char* filename, AVCodecContext *inputCodecCtx,
	AVFormatContext **outputFormatCtx,
	AVCodecContext **outputCodecCtx)
{
	AVCodec *outputCodec = NULL;
	AVStream *stream = NULL;

	//Create and edit output videofile
	*outputFormatCtx = avformat_alloc_context();
	if (((*outputFormatCtx)->oformat = av_guess_format(NULL, filename, NULL)) == NULL)
	{
		cout << "Couldn't guess output video format" << endl;
		return -1;
	}
	if (avio_open2(&(*outputFormatCtx)->pb, filename, AVIO_FLAG_WRITE, NULL, NULL) < 0)
	{
		cout << "Couldn't initialize IO context" << endl;
		return -1;
	}
	if (!(outputCodec = avcodec_find_encoder(AV_CODEC_ID_RAWVIDEO))) {
		cout << "Couldn't find encoder" << endl;
		return -1;
	}

	if (!(stream = avformat_new_stream(*outputFormatCtx, NULL))) {
		cout << "Couldn't create new stream" << endl;
		return -1;
	}

	//Copying codec context
	*outputCodecCtx = avcodec_alloc_context3(outputCodec);
	(*outputCodecCtx) = inputCodecCtx;

	if (avcodec_open2(*outputCodecCtx, outputCodec, NULL) < 0)
	{
		cout << "Couldn't open output codec" << endl;
		return -1;
	}

	if (avcodec_parameters_from_context(stream->codecpar, *outputCodecCtx) < 0) 
	{
		cout << "Couldn't initialize stream parameters" << endl;
		return -1;
	}
	
	if (avformat_write_header(*outputFormatCtx, NULL) < 0) 
	{
		cout << "Couldn't write output header" << endl;
		return -1;
	}

	return 1;
}

int addFrame(AVFormatContext *outFormatContex, AVCodecContext *outCodecContex, AVFrame *frame)
{
	int err;
	AVPacket *outputPacket = av_packet_alloc();

	if ((err = avcodec_send_frame(outCodecContex, frame)) < 0) 
	{
		cout << "Failed to send frame" << endl;
		return err;
	}

	//Place frame into output media
	if (avcodec_receive_packet(outCodecContex, outputPacket) == 0) 
	{
		av_interleaved_write_frame(outFormatContex, outputPacket);
	}
	av_packet_unref(outputPacket);
	av_packet_free(&outputPacket);

	return 1;
}