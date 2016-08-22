#ifndef __YUV_H__
#define __YUV_H__

#define DATA_ROOT "./data/"

struct YUV_params {
	int w,h;
	FILE* handle;
	//...
};

int parseYUVFile(char* fname, YUV_params& outParams);
int parseYUVFile_raw(char* fname, int w, int h, YUV_params& outParams);

int read_frame(const YUV_params& params, char* pY, char* pCb, char* pCr, bool bDump);
int read_frame_raw(const YUV_params& params, char* pY, char* pCb, char* pCr, bool bDump);

#endif // __YUV_H__