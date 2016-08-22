#include "dump_utils.h"
#include "bmp_save.h"
#include <stdio.h>

const static bool enable_dump = true;

void dump2BMP(const char* fname, int w, int h, char* pdata)
{
	if(!enable_dump)
		return;

	const int bpp = 3;
	char* bmp_data = new char[w*h*bpp];

	BITMAPINFO bi = {0};
	bi.bmiHeader.biBitCount = bpp*8;
	bi.bmiHeader.biSize = 40;
	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = h;
	bi.bmiHeader.biCompression = 0;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biSizeImage = w*h*3;

	for(int j=0;j<h; j++)
	{
		for(int i=0;i<w; i++)
		{
			int pos = i + w*j;
			char Y = pdata[pos];
			bmp_data[bpp*i + w*bpp*j + 0] = Y;
			bmp_data[bpp*i + w*bpp*j + 1] = Y;
			bmp_data[bpp*i + w*bpp*j + 2] = Y;
		}
	}
	SaveDIBitmap(fname, &bi, bmp_data);
	delete[] bmp_data;
}


int dumpRaw(const char* fname, int len, char* pdata, int scale, int offset)
{
	FILE* fp;
    if ((fp = fopen(fname, "wb")) == NULL)
        return (-1);

	char c;
	for(int i=0;i<len;++i)
	{
		c = pdata[i]*scale + offset;
		fwrite(&c, 1, 1, fp);	
	}
	
	fclose(fp);

	return 0;
}