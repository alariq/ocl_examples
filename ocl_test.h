#ifndef __OCL_DOWNSAMPLE_H__
#define __OCL_DOWNSAMPLE_H__

#include <CL/cl.h>
#include "ocl/ocl_program.h"

struct dws_data {
	cl_mem in_buf;
	cl_mem out_buf;
	cl_mem out_buf2;
	cl_mem dbg;
	int w, h;
	ocl_program program;
	int numBins;
	float min, max, range;
	int* hist;
	int* incl_sum;
	int* pdbg;
};

#define BLK_SIZ	64 

bool dws_loadKernel(dws_data* pdata);
bool dws_executeKernel(dws_data* pdata, float* exec_time, cl_kernel k, int blockSize, int gridSize);
bool dws_setKernelParams(dws_data* pdata);
bool dws_createBuffers(dws_data* pdata);

#endif //__OCL_DOWNSAMPLE_H__
