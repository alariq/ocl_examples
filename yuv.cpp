#include <stdio.h>
#include <memory.h>
#include <cstdlib>
#include <windows.h>

#include "yuv.h"
#include "utils/dump_utils.h"

static const char FRAME[] = {'F','R','A','M','E','\n' };
static const char HEADER[]  = {'Y','U','V','4','M','P','E','G','2'};

const int g_num_frames = 150;

char clip(int i )
{
	return i > 255 ? 255 : (i<0 ? 0 : (char)i);
}

int get_safe_index(int i, int dim)
{
	return i >= dim ? dim-1 : (i<0 ? 0 : i);
}

void upsample_h(unsigned char* in, char* out, int w, int h, bool /*isVertical*/)
{
	for(int j=0;j<w;++j)
	{
		for(int i=0;i<h;++i)
		{
			out[2*i*w + j] = in[w*i + j];

			int a = in[w*i + j];
			a += in[ get_safe_index(w*(i+1), h) + j ];
			a = 9*a;

			int b = in[ get_safe_index(w*(i-1), h) + j ];
			b += in[ get_safe_index(w*(i+2), h) + j];

			int t = ( a - b + 8 ) >> 4;

			out[w*(2*i+1) + j] = clip(t);
			if(out[w*(2*i+1) + j] == 0)
			{
				int asdfsd=0;
				asdfsd;
			}
		}
	}
}

void upsample_w(unsigned char* in, char* out, int w, int h, bool /*isVertical*/)
{
	for(int j=0;j<h;++j)
	{
		for(int i=0;i<w;++i)
		{
			out[2*i + 2*w*j] = in[i + w*j];

			int a = in[i + w*j];
			a += in[ get_safe_index((i+1), w) + w*j ];
			a = 9*a;

			int b = in[ get_safe_index((i-1), w) + w*j ];
			b += in[ get_safe_index((i+2), w) + w*j];

			int t = ( a - b + 8 ) >> 4;

			out[2*i+1 + 2*w*j] = clip(t);

			if(out[2*i+1 + 2*w*j] == 0)
			{
				int asdfsd=0;asdfsd;
			}
		}
	}
}



//DATA_ROOT"test.y4m"
int parseYUVFile(char* fname, YUV_params& outParams)
{
	FILE * pFile = fopen(fname,"rb");
	if (0 == pFile)
	{
		printf ("can't open input file");
		fclose (pFile);
		return 1;
	}

	char header[sizeof(HEADER)] = {0};
	fread(&header[0], 1, sizeof(HEADER), pFile);
	if(0 != memcmp(&header[0], &HEADER[0], sizeof(HEADER)))
	{
		printf ("input file is not a valid Y4m file");
		fclose (pFile);
		return 1;
	}

	int pos = ftell(pFile);
	char c = 0;
	int param_len = 0;
	while(c!='\n')
	{
		fread(&c, 1, 1, pFile);
		param_len++;
	} 
	param_len--;

	int frame_pos = ftell(pFile);

	fseek(pFile, pos, SEEK_SET);
	
	char* cparam = new char[param_len+1];
	cparam[param_len] = '\0';
	fread(cparam, 1, param_len, pFile);

	int i=0;
	
	int w=0;
	int h=0;
	char* buf = new char[param_len+1];
	while(cparam[i])
	{
		switch(cparam[i++])
		{
			case 'W':
			{
				sscanf(&cparam[i], "%d", &w);
				break;
			}
			case 'H':
			{
				sscanf(&cparam[i], "%d ", &h);
				break;
			}

		}
	}

	printf("Source image W=%d H=%d\n", w, h);

	fseek(pFile, frame_pos, SEEK_SET);

	delete[] cparam;

	fread(buf,1,sizeof(FRAME),pFile);
	if(0!=memcmp(buf, FRAME, sizeof(FRAME)))
	{
		printf ("input file is not a valid Y4m file");
		fclose (pFile);
		return 1;
	}

	// step_back at the beginning of a frame
	long offset = sizeof(FRAME);
	fseek(pFile, -offset, SEEK_CUR);

	outParams.w = w;
	outParams.h = h;
	outParams.handle = pFile;
		
	return 0;

}

int parseYUVFile_raw(char* fname, int w, int h, YUV_params& outParams)
{
	FILE * pFile = fopen(fname,"rb");
	if (0 == pFile)
	{
		printf ("can't open input file");
		fclose (pFile);
		return 1;
	}
	outParams.w = w;
	outParams.h = h;
	outParams.handle = pFile;

	return 0;
}

int read_frame(const YUV_params& params, char* pY, char* pCb, char* pCr, bool bDump)
{
	int frame_length = params.w * params.h * 3 / 2; // (4:2:0)
	char* frame_data = new char[frame_length];

	fseek(params.handle, sizeof(FRAME), SEEK_CUR);
	int read = fread(frame_data, frame_length, 1, params.handle);

	//pY = new char[params.w * params.h];
	memcpy(pY, frame_data, params.w * params.h);

	if(bDump) dump2BMP(DATA_ROOT"Y_out.bmp", params.w, params.h, frame_data);

	//int Cb_off = (((g_image_params.h + 15) & ~15) * g_image_params.w);
	//int Cr_off = (((((g_image_params.h * 3) / 2) + 15) & ~15) * g_image_params.w);

	//const int Csize = g_image_params.w * g_image_params.h / 16;
	const int Cw = params.w/2;
	const int Ch = params.h/2;

	const int Cb_offset = params.w * params.h;
	const int Cr_offset = Cb_offset + params.w/2 * params.h/2;

	if(bDump) dump2BMP(DATA_ROOT"Cb_out.bmp", params.w/2, params.h/2, frame_data + Cb_offset);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out.bmp", params.w/2, params.h/2, frame_data + Cr_offset);

	int upsample_h_size = Cw*(2*Ch);
	int upsample_w_size = (2*Cw)*(2*Ch);

	char* upsampled_h = new char[upsample_h_size];
	char* upsampled_w = new char[upsample_w_size];
	memset(upsampled_h, 0, upsample_h_size);
	memset(upsampled_w, 0, upsample_w_size);

	upsample_h((unsigned char*)(frame_data + Cb_offset), upsampled_h, Cw, Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cb_out_upsampled_h.bmp", Cw, 2*Ch, upsampled_h);

	upsample_w((unsigned char*)upsampled_h, upsampled_w, Cw, 2*Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cb_out_upsampled_w.bmp", 2*Cw, 2*Ch, upsampled_w);

	//pCb = new char[upsample_w_size];
	memcpy(pCb, upsampled_w, upsample_w_size);

	memset(upsampled_h, 0, upsample_h_size);
	memset(upsampled_w, 0, upsample_w_size);

	upsample_h((unsigned char*)(frame_data + Cr_offset), upsampled_h, Cw, Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out_upsampled_h.bmp", Cw, 2*Ch, upsampled_h);

	upsample_w((unsigned char*)upsampled_h, upsampled_w, Cw, 2*Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out_upsampled_w.bmp", 2*Cw, 2*Ch, upsampled_w);

	//pCr = upsampled_w;
	memcpy(pCr, upsampled_w, upsample_w_size);

	delete[] upsampled_h;
	delete[] upsampled_w;
	delete[] frame_data;

	//read = fread(frame_data, frame_length, 1, pFile);
					
	return 0;
}

int read_frame_raw(const YUV_params& params, char* pY, char* pCb, char* pCr, bool bDump)
{
	int frame_length = params.w * params.h * 3 / 2; // (4:2:0)
	char* frame_data = new char[frame_length];

	int read = fread(frame_data, frame_length, 1, params.handle);

	//pY = new char[params.w * params.h];
	memcpy(pY, frame_data, params.w * params.h);

	if(bDump) dump2BMP(DATA_ROOT"Y_out.bmp", params.w, params.h, frame_data);

	//int Cb_off = (((g_image_params.h + 15) & ~15) * g_image_params.w);
	//int Cr_off = (((((g_image_params.h * 3) / 2) + 15) & ~15) * g_image_params.w);

	//const int Csize = g_image_params.w * g_image_params.h / 16;
	const int Cw = params.w/2;
	const int Ch = params.h/2;

	const int Cb_offset = params.w * params.h;
	const int Cr_offset = Cb_offset + params.w/2 * params.h/2;

	if(bDump) dump2BMP(DATA_ROOT"Cb_out.bmp", params.w/2, params.h/2, frame_data + Cb_offset);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out.bmp", params.w/2, params.h/2, frame_data + Cr_offset);

	int upsample_h_size = Cw*(2*Ch);
	int upsample_w_size = (2*Cw)*(2*Ch);

	char* upsampled_h = new char[upsample_h_size];
	char* upsampled_w = new char[upsample_w_size];
	memset(upsampled_h, 0, upsample_h_size);
	memset(upsampled_w, 0, upsample_w_size);

	upsample_h((unsigned char*)(frame_data + Cb_offset), upsampled_h, Cw, Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cb_out_upsampled_h.bmp", Cw, 2*Ch, upsampled_h);

	upsample_w((unsigned char*)upsampled_h, upsampled_w, Cw, 2*Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cb_out_upsampled_w.bmp", 2*Cw, 2*Ch, upsampled_w);

	//pCb = new char[upsample_w_size];
	memcpy(pCb, upsampled_w, upsample_w_size);

	memset(upsampled_h, 0, upsample_h_size);
	memset(upsampled_w, 0, upsample_w_size);

	upsample_h((unsigned char*)(frame_data + Cr_offset), upsampled_h, Cw, Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out_upsampled_h.bmp", Cw, 2*Ch, upsampled_h);

	upsample_w((unsigned char*)upsampled_h, upsampled_w, Cw, 2*Ch, true);
	if(bDump) dump2BMP(DATA_ROOT"Cr_out_upsampled_w.bmp", 2*Cw, 2*Ch, upsampled_w);

	//pCr = upsampled_w;
	memcpy(pCr, upsampled_w, upsample_w_size);

	delete[] upsampled_h;
	delete[] upsampled_w;
	delete[] frame_data;

	//read = fread(frame_data, frame_length, 1, pFile);
					
	return 0;
}



