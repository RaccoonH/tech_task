#define _CRT_SECURE_NO_WARNINGS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "bmpHeader.h"
#include "yuvVideo.h"

using namespace std;

int main(int argc, char* argv[]) {

	//Initializing for BMP file
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFOHEADER bmpInfoHeader;

	//Initalizing for YUV videofile
	AVFormatContext   *pFormatCtx = NULL;
	AVFormatContext	*outputFormatContext = NULL;
	AVCodecContext	*outputCodecContext = NULL;
	int               videoStream;
	AVCodecContext    *pCodecCtxOrig = NULL;
	AVCodecContext    *pCodecCtx = NULL;
	AVCodec           *pCodec = NULL;
	AVFrame           *pFrame = NULL;
	AVPacket          packet;

	if (argc < 5)
	{
		cout << "Too few arguments" << endl;
		return -1;
	}
	
	char* fileBMP = argv[1];
	char* inputFilename = argv[2];
	char* outputFilename = argv[3];
	char* resolution = argv[4];

	//Open BMP file and convert RGB to YUV
	ifstream fileStream;
	fileStream.open(fileBMP, ifstream::binary);
	if (!fileStream) 
	{
		cout << "Error opening file" << endl;
		return -1;
	}
	if (readHeader(fileStream, bmpHeader, bmpInfoHeader) < 1)
	{
		cout << "Error with image file" << endl;
		return -1;
	}

	RGB **rgb = new RGB*[bmpInfoHeader.biHeight];
	for (unsigned int i = 0; i < bmpInfoHeader.biHeight; i++)
	{
		rgb[i] = new RGB[bmpInfoHeader.biWidth];
	}
	readRGB(fileStream, bmpInfoHeader, rgb);

	YUV **yuv = new YUV*[bmpInfoHeader.biWidth];
	for (unsigned int i = 0; i < bmpInfoHeader.biHeight; i++)
	{
		yuv[i] = new YUV[bmpInfoHeader.biWidth];
	}

	convertRGBtoYUV(rgb, yuv, bmpInfoHeader.biHeight, bmpInfoHeader.biWidth);

	//Work with YUV video
	av_register_all();

	// Open video file
	if (openInputFile(inputFilename, &pFormatCtx, &pCodecCtxOrig, &pCodec, &videoStream, resolution) < 1)
	{
		cout << "Error with input videofile" << endl;
		return -1;
	}

	if ((pCodecCtxOrig->width < bmpInfoHeader.biWidth) ||
		(pCodecCtxOrig->height < bmpInfoHeader.biHeight))
	{
		cout << "BMP file resolution is bigger than YUV file" << endl;
		return -1;
	}

	if (openOutputFile(outputFilename, pCodecCtxOrig, &outputFormatContext, &outputCodecContext) < 1)
	{
		cout << "Error with output videofile" << endl;
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) 
	{
		cout << "Couldn't copy codec context" << endl;
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		cout << "Couldn't open codec" << endl;
		return -1;
	}

	//Fill YUV colors buffer
	unsigned int size_total = bmpInfoHeader.biHeight*bmpInfoHeader.biWidth;

	uint8_t *inputBufferY = new uint8_t[size_total];
	uint8_t *inputBufferU = new uint8_t[size_total/2];
	uint8_t *inputBufferV = new uint8_t[size_total/2];

	YUV *yuv2 = new YUV[size_total];
	int index = 0;

	for (unsigned int i = bmpInfoHeader.biHeight-1; i > 0; i--)
	{
		for (unsigned int j = 0; j < bmpInfoHeader.biWidth; j++)
		{
			yuv2[index] = yuv[i][j];
			index++;
		}
	}

	for (unsigned int i = 0; i < size_total; i++)
	{
		inputBufferY[i] = uint8_t(yuv2[i].yuvY);
	}

	index = 0;
	for (unsigned int i = 0; i < size_total; i = i+2)
	{
		inputBufferU[index] = uint8_t(yuv2[i].yuvU);
		inputBufferV[index] = uint8_t(yuv2[i].yuvV);
		index++;
	}

	//Place picture into frames
	int err;
	pFrame = av_frame_alloc();
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			err = avcodec_send_packet(pCodecCtx, &packet);
			if (err < 0) 
			{
				cout << "avcodec_send_packet error: " << err << endl;
				break;
			}
			while (err >= 0) 
			{
				err = avcodec_receive_frame(pCodecCtx, pFrame);
				if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) 
				{
					break;
				}
				else if (err < 0) 
				{
					cout << "avcodec_receive_frame: " << err << endl;
					return err;
				}
				if (err >= 0) {
					//Copying inputBuffers to frame
					av_image_copy_plane(pFrame->data[0],
						pFrame->linesize[0],
						inputBufferY,
						bmpInfoHeader.biWidth,
						bmpInfoHeader.biWidth,
						bmpInfoHeader.biHeight
					);

					av_image_copy_plane(pFrame->data[1],
						pFrame->linesize[1],
						inputBufferU,
						bmpInfoHeader.biWidth,
						bmpInfoHeader.biWidth / 2,
						bmpInfoHeader.biHeight / 2
					);

					av_image_copy_plane(pFrame->data[2],
					pFrame->linesize[2],
					inputBufferV,
					bmpInfoHeader.biWidth,
					bmpInfoHeader.biWidth / 2,
					bmpInfoHeader.biHeight / 2
					);

					addFrame(outputFormatContext, outputCodecContext, pFrame);	
				}
				av_frame_unref(pFrame);
			}
			av_packet_unref(&packet);
		}
		av_free_packet(&packet);
	}

	//Free and close objects
	av_frame_free(&pFrame);

	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);
	avcodec_close(outputCodecContext);

	avformat_close_input(&pFormatCtx);
	avformat_free_context(outputFormatContext);

	return 0;
}

