#ifndef _BMP_SAVE_H_
#define _BMP_SAVE_H_

#include <stdint.h>

typedef struct {
   uint16_t bfType;                 /* Magic identifier            */
   uint32_t bfSize;                       /* File size in bytes          */
   uint16_t bfReserved1, bfReserved2;
   uint32_t bfOffBits;                     /* Offset to image data, bytes */
} BITMAPFILEHEADER;

//http://www.fourcc.org/rgb.php
#define BI_RLE8 	0x1
#define BI_RGB  	0x0
#define BI_BITFIELDS 	0x3
#define BI_RLE4		0x2
#  define BF_TYPE 0x4D42             /* "MB" */

typedef struct {
   unsigned int size;               /* Header size in bytes      */
   int width,height;                /* Width and height of image */
   unsigned short int planes;       /* Number of colour planes   */
   unsigned short int bits;         /* Bits per pixel            */
   unsigned int compression;        /* Compression type          */
   unsigned int imagesize;          /* Image size in bytes       */
   int xresolution,yresolution;     /* Pixels per meter          */
   unsigned int ncolours;           /* Number of colours         */
   unsigned int importantcolours;   /* Important colours         */
} INFOHEADER;

typedef struct tagBITMAPINFOHEADER {
	uint32_t biSize;
	long  biWidth;
	long  biHeight;
	uint16_t  biPlanes;
	uint16_t  biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	long  biXPelsPerMeter;
	long  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	    RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO;


int                                /* O - 0 = success, -1 = failure */
SaveDIBitmap(const char *filename, /* I - File to load */
             BITMAPINFO *info,     /* I - Bitmap information */
			char  *bits);     /* I - Bitmap data */

#endif // _BMP_SAVE_H_
