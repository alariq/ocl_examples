#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <assert.h>
#include <string>

#include "ocl_hist.h"

#define DEBUG_OUTPUT 1

bool hist_create_ocl_buffers(Hist_t* h, int* pel, size_t num_el, size_t num_bins)
{
	const int el_siz = sizeof(int);
	cl_int status;

	h->num_el_per_wg = h->NUM_EL_PER_WI * h->HIST_BLK_SIZ;

	h->num_el = num_el;
	h->num_bins = num_bins;
	h->num_blocks = (h->num_el + h->num_el_per_wg - 1) / h->num_el_per_wg;
	h->num_loc_bin_els = h->num_bins * h->num_blocks;
	
	h->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pel, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	h->loc_bins = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, h->num_loc_bin_els*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	int num_lb2 = h->num_bins * ((h->num_blocks + 2*h->SUM_BLK_SIZ - 1) / (2*h->SUM_BLK_SIZ));
	h->loc_bins2 = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_lb2*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	return true;
}

bool hist_load_kernel(Hist_t* h)
{
	char pflags[1024];
	sprintf(pflags, "-DSUM_BLK_SIZ=%d -DBLK_SIZ=%d -DNUM_EL_PER_WI=%d", h->SUM_BLK_SIZ, h->HIST_BLK_SIZ, h->NUM_EL_PER_WI);

	if(!h->program.load(DATA_ROOT"hist.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	bool b_success = true;
	b_success = b_success && h->program.create_kernel("hist");
	b_success = b_success && h->program.create_kernel("hist_sum");
	return b_success;
}

template<typename T> void swap(T* a, T* b)
{
	T t = *a;
	*a = *b;
	*b = t;
}

bool hist_do()
{
	Hist_t h;
	const size_t num_el = 10240000;
	const size_t num_bins = 1024;
	int* pel = new int[num_el];
	for(size_t i=0;i<num_el;++i)
	{
		pel[i] = rand()%num_bins;
	}

	if(!hist_create_ocl_buffers(&h, pel, num_el, num_bins))
		return false;

	if(!hist_load_kernel(&h))
		return false;

	for(int tries=0;tries<10;tries++)
	{

	cl_int status;
	
	cl_kernel k = h.program.kernels_["hist"];

	status = clSetKernelArg(k, 0, sizeof(cl_mem), &h.src);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &h.loc_bins);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_int), &h.num_el);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	float total_exec_time = 0;
	float exec_time;
	if(!ocl_execute_kernel_1d_sync(k, h.HIST_BLK_SIZ, h.num_blocks, &exec_time))
		return false;
	total_exec_time += exec_time;

	/*#if DEBUG_OUTPUT
	{
		ScopedMap sm(h.loc_bins2);
		int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*num_sum_blocks);
		int num_hist_el = 0;
		for(int j=0;j<num_sum_blocks;++j)
		{
			int v = p[j];
			num_hist_el += v;
		}*/

	k = h.program.kernels_["hist_sum"];
	int num_sum_el = h.num_blocks;
	int num_sum_blocks_per_bin = ((h.num_blocks + 2*h.SUM_BLK_SIZ - 1) / (2*h.SUM_BLK_SIZ));
	int num_sum_blocks = h.num_bins * num_sum_blocks_per_bin;
	while(1)
	{

		status = clSetKernelArg(k, 0, sizeof(cl_mem), &h.loc_bins);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
		status = clSetKernelArg(k, 1, sizeof(cl_mem), &h.loc_bins2);CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
		status = clSetKernelArg(k, 2, sizeof(cl_int), &num_sum_el);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

		if(!ocl_execute_kernel_1d_sync(k, h.SUM_BLK_SIZ, num_sum_blocks, &exec_time))
			return false;
		total_exec_time += exec_time;
#if DEBUG_OUTPUT
	{
		ScopedMap sm(h.loc_bins2);
		int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*num_sum_blocks);
		int num_hist_el = 0;
		for(int j=0;j<num_sum_blocks;++j)
		{
			int v = p[j];
			num_hist_el += v;
			//printf("%d ", v);
			if(num_hist_el!=num_el)
			{
				int asdf=0;
			}
			//printf("\n");
		}
		printf("num hist el: %d\n", num_hist_el);
	}
#endif

		swap(&h.loc_bins2, &h.loc_bins);

		if(num_sum_blocks_per_bin==1)
			break;
	
		num_sum_el = num_sum_blocks_per_bin;
		num_sum_blocks_per_bin =  ((num_sum_el + 2*h.SUM_BLK_SIZ - 1) / (2*h.SUM_BLK_SIZ));
		num_sum_blocks = h.num_bins * num_sum_blocks_per_bin;
	
	}

#if DEBUG_OUTPUT
	{
	ScopedMap sm(h.loc_bins);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*h.num_bins);
	int num_hist_el = 0;
	for(int i=0;i<h.num_bins;++i)
	{
		int v = p[i];
		num_hist_el += v;
		//printf("%d ", v);
	}
	if(num_hist_el!=num_el)
	{
		int asdf=0;
		printf("ERROR!\n");
	}
	printf("\n");
	}
#endif

	printf("Total exec time: %f\n", total_exec_time);

	}

	return true;
}



