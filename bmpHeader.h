#pragma once
#ifndef BMP_HEADER
#define BMP_HEADER

typedef struct {
	unsigned short bfType;
	unsigned int   bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int   bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
	unsigned int   biSize;
	unsigned int   biWidth;
	unsigned int   biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	unsigned int   biXPelsPerMeter;
	unsigned int   biYPelsPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
	unsigned char  rgbRed;
	unsigned char  rgbGreen;
	unsigned char  rgbBlue;
	unsigned char  rgbReserved;
} RGB;

typedef struct {
	unsigned int yuvY;
	int yuvU;
	int yuvV;
} YUV;

template <typename Type>
void read(std::ifstream &fp, Type &result, std::size_t size);

int readHeader(std::ifstream &fileStream, BITMAPFILEHEADER &bmpHeader, BITMAPINFOHEADER &bmpInfoHeader);

void readRGB(std::ifstream &fileStream, BITMAPINFOHEADER &bmpInfoHeader, RGB **rgb);

void convertRGBtoYUV(RGB **rgb, YUV **yuv, int height, int width);

#endif // BMP_HEADER
