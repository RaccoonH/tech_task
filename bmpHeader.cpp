#include <fstream>
#include <iostream>
#include <thread>
#include <emmintrin.h>

#include "bmpHeader.h"

template <typename Type>
void read(std::ifstream &fp, Type &result, std::size_t size) {
	fp.read(reinterpret_cast<char*>(&result), size);
}

int readHeader(std::ifstream &fileStream, BITMAPFILEHEADER &bmpHeader, BITMAPINFOHEADER &bmpInfoHeader) {
	size_t sizeStruct = sizeof(bmpHeader.bfType) + sizeof(bmpHeader.bfSize) + sizeof(bmpHeader.bfReserved1) + sizeof(bmpHeader.bfReserved2) + sizeof(bmpHeader.bfOffBits);

	read(fileStream, bmpHeader, sizeStruct);
	read(fileStream, bmpInfoHeader, sizeof(bmpInfoHeader));

	if (bmpHeader.bfType != 0x4D42) {   // check format file - BM
		std::cout << "Error: file is not BMP file. " << std::endl;
		return 0;
	}
	if (bmpInfoHeader.biBitCount != 24) {
		std::cout << "Error: file is not 24 bit file. " << std::endl;
		return 0;
	}
	if (bmpInfoHeader.biSize != 40) {       // check size of header - 40 (without compression)
		std::cout << "Error: file is not BMP file or include compression. " << std::endl;
		return 0;
	}
	return 1;
}

void readRGB(std::ifstream &fileStream, BITMAPINFOHEADER &bmpInfoHeader, RGB **rgb) {

	int padding = (4 - (bmpInfoHeader.biWidth * (bmpInfoHeader.biBitCount / 8)) % 4) & 3;

	for (unsigned int i = 0; i < bmpInfoHeader.biHeight; i++) {
		for (unsigned int j = 0; j < bmpInfoHeader.biWidth; j++) {
			read(fileStream, rgb[i][j].rgbBlue, bmpInfoHeader.biBitCount / 24);
			read(fileStream, rgb[i][j].rgbGreen, bmpInfoHeader.biBitCount / 24);
			read(fileStream, rgb[i][j].rgbRed, bmpInfoHeader.biBitCount / 24);
		}
		fileStream.seekg(padding, std::ios_base::cur);
	}
}

void computeY(YUV **yuv, RGB **rgb, int width, int height)
{
	__m128d _mm_mul_rgb[3];
	__m128d _mm_rgb[3];
	__m128d _mm_16 = _mm_cvtepi32_pd(_mm_cvtsi32_si128(16));

	double mul_rgb[3] = { 0.257 , 0.504, 0.098};
	_mm_mul_rgb[0] = _mm_load1_pd(&mul_rgb[0]);
	_mm_mul_rgb[1] = _mm_load1_pd(&mul_rgb[1]);
	_mm_mul_rgb[2] = _mm_load1_pd(&mul_rgb[2]);
	for (unsigned int i = 0; i < height; i++)
	{
		for (unsigned int j = 0; j < width; j++)
		{
			_mm_rgb[0] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbRed));
			_mm_rgb[1] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbGreen));
			_mm_rgb[2] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbBlue));

			_mm_rgb[0] = _mm_mul_pd(_mm_rgb[0], _mm_mul_rgb[0]);
			_mm_rgb[1] = _mm_mul_pd(_mm_rgb[1], _mm_mul_rgb[1]);
			_mm_rgb[2] = _mm_mul_pd(_mm_rgb[2], _mm_mul_rgb[2]);

			 _mm_rgb[0] = _mm_add_pd(_mm_rgb[0], _mm_rgb[1]);
			 _mm_rgb[0] = _mm_add_pd(_mm_rgb[0], _mm_rgb[2]);
			 _mm_rgb[0] = _mm_add_pd(_mm_rgb[0], _mm_16);

			 yuv[i][j].yuvY = _mm_cvtsd_si32(_mm_rgb[0]);
			//Y = (R * 0.257) + (0.504 * G) + (0.098 * B) + 16;
		}
	}
}

void computeU(YUV **yuv, RGB **rgb, int width, int height)
{
	__m128d _mm_mul_rgb[3];
	__m128d _mm_rgb[3];
	__m128d _mm_128 = _mm_cvtepi32_pd(_mm_cvtsi32_si128(128));
	
	double mul_rgb[3] = { 0.148 , 0.291, 0.439 };
	_mm_mul_rgb[0] = _mm_load1_pd(&mul_rgb[0]);
	_mm_mul_rgb[1] = _mm_load1_pd(&mul_rgb[1]);
	_mm_mul_rgb[2] = _mm_load1_pd(&mul_rgb[2]);
	for (unsigned int i = 0; i < height; i++)
	{
		for (unsigned int j = 0; j < width; j++)
		{
			_mm_rgb[0] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbRed));
			_mm_rgb[1] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbGreen));
			_mm_rgb[2] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbBlue));

			_mm_rgb[0] = _mm_mul_pd(_mm_rgb[0], _mm_mul_rgb[0]);
			_mm_rgb[1] = _mm_mul_pd(_mm_rgb[1], _mm_mul_rgb[1]);
			_mm_rgb[2] = _mm_mul_pd(_mm_rgb[2], _mm_mul_rgb[2]);

			_mm_rgb[2] = _mm_sub_pd(_mm_rgb[2], _mm_rgb[0]);
			_mm_rgb[2] = _mm_sub_pd(_mm_rgb[2], _mm_rgb[1]);
			_mm_rgb[2] = _mm_add_pd(_mm_rgb[2], _mm_128);

			yuv[i][j].yuvU = _mm_cvtsd_si32(_mm_rgb[2]);
			//U = -(R * 0.148) - (0.291 * G) + (0.439 * B) + 128;
		}
	}
}

void computeV(YUV **yuv, RGB **rgb, int width, int height)
{
	__m128d _mm_mul_rgb[3];
	__m128d _mm_rgb[3];
	__m128d _mm_128 = _mm_cvtepi32_pd(_mm_cvtsi32_si128(128));

	double mul_rgb[3] = { 0.439 , 0.368, 0.071 };
	_mm_mul_rgb[0] = _mm_load1_pd(&mul_rgb[0]);
	_mm_mul_rgb[1] = _mm_load1_pd(&mul_rgb[1]);
	_mm_mul_rgb[2] = _mm_load1_pd(&mul_rgb[2]);
	for (unsigned int i = 0; i < height; i++)
	{
		for (unsigned int j = 0; j < width; j++)
		{
			_mm_rgb[0] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbRed));
			_mm_rgb[1] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbGreen));
			_mm_rgb[2] = _mm_cvtepi32_pd(_mm_cvtsi32_si128(rgb[i][j].rgbBlue));

			_mm_rgb[0] = _mm_mul_pd(_mm_rgb[0], _mm_mul_rgb[0]);
			_mm_rgb[1] = _mm_mul_pd(_mm_rgb[1], _mm_mul_rgb[1]);
			_mm_rgb[2] = _mm_mul_pd(_mm_rgb[2], _mm_mul_rgb[2]);

			_mm_rgb[0] = _mm_sub_pd(_mm_rgb[0], _mm_rgb[1]);
			_mm_rgb[0] = _mm_sub_pd(_mm_rgb[0], _mm_rgb[2]);
			_mm_rgb[0] = _mm_add_pd(_mm_rgb[0], _mm_128);

			yuv[i][j].yuvV = _mm_cvtsd_si32(_mm_rgb[0]);
			//V = (R * 0.439) - (0.368 * G) - (0.071 * B) + 128;
		}
	}
}

void convertRGBtoYUV(RGB **rgb, YUV **yuv, int height, int width)
{
	std::thread t1(computeY, yuv, rgb, width, height);
	std::thread t2(computeU, yuv, rgb, width, height);
	std::thread t3(computeV, yuv, rgb, width, height);

	t1.join();
	t2.join();
	t3.join();
}


