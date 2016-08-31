/*
 * OpenCL downsample implementation example.
 * alariq@gmail.com
 *
 * Distributed under terms of the LGPL. 
 */

#include <stdio.h>
#include "utils/dump_utils.h"
#include "yuv.h"
#include "ocl_test.h"
#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include <string>
#include <string.h>
#include <cassert>

#include "ocl_radix_sort2.h"
#include "ocl_hist.h"
#include "ocl_stream_compact.h"

#include "examples/example1_reduce.h"
#include "examples/example_article.h"

YUV_params g_yuv_params;
char* Y_frame = 0;
char* Cb_frame = 0;
char* Cr_frame = 0;

dws_data g_dws_data;

extern bool DWS4X;

#define P_WIDTH "--width="
#define P_HEIGHT "--height="
#define P_INPUT "--input="
#define P_NUM_ITER "--num-iter="
#define P_PROFILE "--profile"
#define P_DUMPRES "--dumpresult="
#define P_DUMPSRC "--dumpsource="
#define P_OPTIMIZED "--optimized"
#define P_ALLOW4PIX "--allow4pixwrite"
#define PATH_SIZE	MAX_PATH


// generate hatch
void read_frame_null(const YUV_params& params, char* pdata)
{
    for(int y=0;y<params.h;++y)
    {
	for(int x=0;x<params.w;++x)
	{
	    pdata[x + y*params.w] = 255*(((x+y)>>3)&0x3);
	}
    }
}

unsigned char julia(int x, int y, int w, int h)
{
    static const float in_c = 0.5f;
    static const  float out_c1 = 0.0f;
    static const  float out_c2 = 1.0f;

    float xf = 2.0f*((float)x/(float)w - 0.5f);
    float yf = 2.0f*((float)y/(float)h - 0.5f);

    float time = 0.0f;
    float real  = xf;
    float imag  = yf;
    float Creal = 0.285f;
    float Cimag = 0.01f;

    float r2 = 0.0;
    int   iter;
    float k;
    static const int NUM_IT	= 40;

    for (iter = 0; iter < NUM_IT; ++iter)
    {
	float tempreal = real;

	real = (tempreal * tempreal) - (imag * imag) + Creal;
	imag = 2.0f * tempreal * imag + Cimag;
	r2   = (real * real) + (imag * imag);
	if(r2 >=2 )
	    break;
    }
    if(r2 < 4)
    {
	k = (float)iter/(float)NUM_IT;
	return (unsigned char)(in_c*k*255);
    }
    else
    {
	k = float(iter) * 0.05f;
	int ki = (int)k;
	k = k - ki; 
	return (unsigned char)((out_c1*(1-k) +  out_c2*k)*255);
    }

}


void read_frame_null2(const YUV_params& params, char* pdata)
{
    for(int y=0;y<params.h;++y)
    {
	for(int x=0;x<params.w;++x)
	{
	    pdata[x + y*params.w] = julia(x, y, params.w, params.h);
	}
    }
}

void usage(char* argv[])
{
    printf("usage: %s --width=N --height=N [ --input=file.yuv --num-iter=N --profile --dumpresult=out.bmp ]\n", argv[0]);
    printf("--width=N frame width\n");
    printf("--height=N frame height\n");
    printf("--input=file.yuv  YUV sample file, uses fake input if not specified (useful for tests)\n");
    printf("--optimized Use optimized approach, default: disabled\n");
    printf("--allow4pixwrite Write by 4 pixels if possible(texture width is multiple of 16), works only in --optimized mode, default: disabled\n");
    printf("--num_iter=N Number of time to run kernel (used to calculate average run time) default: 30 \n");
    printf("--profile Output kernel average run time, default: disabled\n");
    printf("--dumpsource=sout.bmp Output original frame to file , default: disabled\n");
    printf("--dumpresult=rout.bmp Output downsampled frame to file, default: disabled\n");
    printf("\n");
}

template <typename T> void swap(T* one, T* two)
{
    T t = *one;
    *one = two;
    *two = t;
}

int main( int argc, char* argv[] )
{
    cl_int status;
    int profile = 1;

	oclInitParams oclinitparams;
	memset(&oclinitparams, 0, sizeof(oclinitparams));

    oclinitparams.device_type_ = CL_DEVICE_TYPE_GPU;
	oclinitparams.display_ = 0;
	oclinitparams.glcontext_ = 0;
	oclinitparams.b_enable_interop = false;

    if(!ocl_init(&oclinitparams, profile ? CL_QUEUE_PROFILING_ENABLE : 0))
    {
        printf("Failed to init OpenCL\n");
        exit(1);
    }

    if(!article_example_do())
    {
        printf("Article examples failed\n");
        exit(1);
    }
     
    reduce_do();
    sc_do();
    //exit(0);
    rs_do2();
    hist_do();

    g_dws_data.numBins = 1024;
    g_dws_data.w = 256;
    g_dws_data.h = 384;

    if(!dws_loadKernel(&g_dws_data))
    {
	printf("Failed to load kernel\n");
	exit(1);
    }

    if(!dws_createBuffers(&g_dws_data))
    {
	printf("Failed to load kernel\n");
	exit(1);
    }

    int numEl = g_dws_data.w*g_dws_data.h;
    int numBlocks = (numEl + BLK_SIZ - 1) / BLK_SIZ;
    
    cl_kernel kernel = g_dws_data.program.kernels_["calc_hist_single_pass"];
    const int blockSize = BLK_SIZ;
    int gridSize = numBlocks;

    const int numProceEl = g_dws_data.w * g_dws_data.h;
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&g_dws_data.in_buf);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&numProceEl);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)"); 
    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&g_dws_data.out_buf);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (out_buf)");
    status = clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&g_dws_data.numBins);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 4, sizeof(cl_float), (void *)&g_dws_data.min);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 5, sizeof(cl_float), (void *)&g_dws_data.range);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&g_dws_data.dbg);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");

    float exec_time;
    bool b_ok = dws_executeKernel(&g_dws_data, &exec_time, kernel, blockSize, gridSize);
    assert(b_ok);
    {
	ScopedMap in_buf(g_dws_data.out_buf);
	int* pin = (int*)in_buf.map(CL_MAP_READ, sizeof(int)*g_dws_data.numBins);
	ScopedMap dbgbuf(g_dws_data.dbg);
	int* pgpudbg = (int*)dbgbuf.map(CL_MAP_READ, sizeof(int)*g_dws_data.w*g_dws_data.h);
	int num = 0;
	for(int i=0;i<g_dws_data.numBins;++i)
	{
	    num += pin[i];
	    if(pin[i] != g_dws_data.hist[i])
	    {
		int dsfa=0;
	    }
	    
	}
	for(int i=0;i<g_dws_data.w*g_dws_data.h;++i)
	{
	    if(pgpudbg[i] != g_dws_data.pdbg[i])
	    {
		int dsfa=0;
	    }
	}
	assert(num == g_dws_data.w*g_dws_data.h);
    }

    // make numBins multiple of BLK_SIZ and pad additional space with zeroes
    int padded_size = ((g_dws_data.numBins + 2*BLK_SIZ - 1) / (2*BLK_SIZ)) * 2*BLK_SIZ;
    if(g_dws_data.numBins != padded_size)
    {
	ScopedMap in_buf(g_dws_data.out_buf);
	int* pin = (int*)in_buf.map(CL_MAP_WRITE, sizeof(int)*padded_size);
	memset(pin + g_dws_data.numBins, 0, sizeof(int)*(padded_size - g_dws_data.numBins));
    }

    gridSize = (g_dws_data.numBins + BLK_SIZ - 1) / BLK_SIZ;
    kernel = g_dws_data.program.kernels_["hillis_steele"];
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&g_dws_data.out_buf);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&g_dws_data.out_buf2);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&g_dws_data.numBins);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    b_ok = dws_executeKernel(&g_dws_data, &exec_time, kernel, blockSize, gridSize);
    assert(b_ok);
    {
	ScopedMap in_buf(g_dws_data.out_buf2);
	int* pout = (int*)in_buf.map(CL_MAP_READ, sizeof(int)*gridSize);
	int num = 0;
	for(int i=0;i<gridSize;++i)
	    num += pout[i];
	assert(num == g_dws_data.w*g_dws_data.h);
	ScopedMap out_buf(g_dws_data.out_buf);
	int* pin = (int*)out_buf.map(CL_MAP_READ, sizeof(int)*gridSize*blockSize);
	int asfsa=0;
    }

    // make sure out_buf2 is multiple of 2*BLK_SIZ (blelloch WG processes this num of elements) and fill rest by zeroes
    padded_size = (( gridSize + 2*BLK_SIZ - 1) / (2*BLK_SIZ))*2*BLK_SIZ;
    if(gridSize!=padded_size)
    {
	ScopedMap in_buf(g_dws_data.out_buf2);
	int* pin = (int*)in_buf.map(CL_MAP_WRITE, sizeof(int)*padded_size);
	memset(pin + gridSize, 0, sizeof(int)*(padded_size - gridSize));
    }
    
    kernel = g_dws_data.program.kernels_["blelloch"];
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&g_dws_data.out_buf2);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    b_ok = dws_executeKernel(&g_dws_data, &exec_time, kernel, blockSize, 1);
    assert(b_ok);
    {
	ScopedMap in_buf(g_dws_data.out_buf2);
	int* pout = (int*)in_buf.map(CL_MAP_READ, sizeof(int)*gridSize);
	int num = 0;
    }



    gridSize = (g_dws_data.numBins + BLK_SIZ - 1) / BLK_SIZ;
    kernel = g_dws_data.program.kernels_["loc2glob"];
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&g_dws_data.out_buf);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&g_dws_data.out_buf2);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&g_dws_data.numBins);    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    b_ok = dws_executeKernel(&g_dws_data, &exec_time, kernel, blockSize, gridSize);
    assert(b_ok);
    {
	ScopedMap in_buf(g_dws_data.out_buf);
	int* pout = (int*)in_buf.map(CL_MAP_READ, sizeof(int)*g_dws_data.numBins);
	for(int i=0;i<g_dws_data.numBins;++i)
	{
	//	assert(pout[i] == g_dws_data.incl_sum[i]);
	}
    }
    
    return 0;
}

