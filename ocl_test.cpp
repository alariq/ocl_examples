#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <CL/cl.h>
#include "utils/dump_utils.h"
#include <string.h>
#include <assert.h>

//#define NVIDIA_SDK

bool DWS4X = true;

#define min(a, b)  (a) < (b) ? (a) : (b)

bool dws_createBuffers(dws_data* pdata)
{
	cl_int status;
	int numEl = pdata->w*pdata->h;
	pdata->in_buf = clCreateBuffer(ocl_get_context(), CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY, sizeof(float)*pdata->w*pdata->h, 0, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed.");

	pdata->min = FLT_MAX;
	pdata->max = -FLT_MAX;

	{
	ScopedMap in_buf(pdata->in_buf);
	float* pin = (float*)in_buf.map(CL_MAP_WRITE, sizeof(float)*pdata->w*pdata->h);
	for(int y=0;y<pdata->h;++y)
		for(int x=0;x<pdata->w;++x)
		{
			//float v = ((float)(rand()%10000))/10.0f;
			float v = y*pdata->w + x;
			pin[y*pdata->w + x] = v;
			pdata->max = v > pdata->max ? v : pdata->max;
			pdata->min = v < pdata->min ? v : pdata->min;
		}

	pdata->range = pdata->max - pdata->min;
	//calc ref. hist on cpu
	pdata->hist = new int[pdata->numBins];
	pdata->pdbg = new int[pdata->w*pdata->h];
	memset(pdata->hist, 0, pdata->numBins * sizeof(int));
	for(int i=0;i<pdata->h*pdata->w;++i)
	{
		int bin = min(pdata->numBins-1, (int)((pin[i] - pdata->min)/pdata->range * pdata->numBins));
		assert(bin < pdata->numBins && bin >=0);
		pdata->pdbg[i] = bin;
		pdata->hist[bin]++;
	}
	pdata->incl_sum = new int[pdata->numBins];
	pdata->incl_sum[0] = pdata->hist[0];
	int n=pdata->hist[0];
	for(int i=1;i<pdata->numBins;++i)
	{
		pdata->incl_sum[i] = pdata->incl_sum[i-1] + pdata->hist[i];
		n+=pdata->hist[i];
	}
	assert(n == pdata->h*pdata->w);
	}
	
	int numBlocks = (numEl + BLK_SIZ - 1) / BLK_SIZ;
	int hist_count = numBlocks*pdata->numBins;
	pdata->out_buf = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE/*|CL_MEM_HOST_READ_ONLY*/, sizeof(int)*hist_count, 0, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed.");
	{
		ScopedMap out_buf(pdata->out_buf);
		int* pout = (int*)out_buf.map(CL_MAP_WRITE, sizeof(int)*pdata->numBins);
		memset(pout, 0, sizeof(int)*pdata->numBins); 
	}

	hist_count = (hist_count + BLK_SIZ - 1) / BLK_SIZ;
	pdata->out_buf2 = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE/*|CL_MEM_HOST_READ_ONLY*/, sizeof(int)*hist_count, 0, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed.");

	pdata->dbg = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, sizeof(int)*pdata->w*pdata->h, 0, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed.");

	return true;
}

bool dws_loadKernel(dws_data* pdata)
{
	char pflags[1024];
	pflags[0] = '\0';

	if(!pdata->program.load(DATA_ROOT"calc.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	bool b_success = true;
	b_success = b_success && pdata->program.create_kernel("calc_hist_single_pass");
	b_success = b_success && pdata->program.create_kernel("hillis_steele");
	b_success = b_success && pdata->program.create_kernel("blelloch");
	b_success = b_success && pdata->program.create_kernel("loc2glob");
	return b_success;
}

bool dws_executeKernel(dws_data* pdata, float* exec_time, cl_kernel k, int blockSize, int gridSize)
{
	cl_int status;
	size_t globalWorkSize[3] = {1,1,1};
    size_t localWorkSize[3] = {1, 1, 1};
	
    // Set local and global work group sizes
	globalWorkSize[0] = gridSize * blockSize;
	localWorkSize[0] = blockSize;

    // Execute kernel on given device
    cl_event  eventND[1];
	status = clEnqueueNDRangeKernel(ocl_get_queue(), k, 1, NULL, globalWorkSize, localWorkSize, 0, 0, eventND );
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(ocl_get_queue());
    CHECK_OPENCL_ERROR(status, "clFlush() failed");

    status = ocl_wait_for_event(&eventND[0], false);
    if(false == status)
		return false;

	static float time_ms = 0.0f;

	if(exec_time)
	{
		ocl_get_exec_time_in_ms(&eventND[0], exec_time);
		time_ms += *exec_time;
		printf("Time: %.3f\n", time_ms);
	}

	clReleaseEvent(eventND[0]);

    status = clFinish(ocl_get_queue());
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

	return true;
}
bool dws_setKernelParams(dws_data* pdata)
{
	cl_int status;
	const cl_kernel kernel = pdata->program.kernels_["downsample"];

    status = clSetKernelArg(kernel, 1, sizeof(cl_uint), (void *)&pdata->w);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");
    status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&pdata->h);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (w)");

	status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&pdata->out_buf);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (out_buf)");

	return true;
}
