#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <assert.h>
#include <string.h>

#include "ocl_radix_sort.h"


bool rs_create_ocl_buffers(RadixSort_t* r, int* pel, int* pval, size_t num_el)
{
	const int el_siz = sizeof(int);
	cl_int status;

	r->num_el_per_wg = r->NUM_EL_PER_WI * r->RADIX_BLK_SIZ;

	r->num_el = r->num_el_per_wg * ((num_el + r->num_el_per_wg - 1) / r->num_el_per_wg);
	r->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pel, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	r->src_v = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pval, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->dst = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	r->dst_v = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->num_blocks = r->num_el / r->num_el_per_wg;

	r->hist_num_el = r->NUM_BINS * r->num_blocks;
	r->hist = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->hist_num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->scan_num_el = r->num_blocks * r->NUM_BINS;
	r->scan = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->hist_num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->bin_offsets = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->NUM_BINS*el_siz, 0, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	return true;
}

bool rs_load_kernel(RadixSort_t* r)
{
	char pflags[1024];
	pflags[0] = '\0';

	if(!r->program.load(DATA_ROOT"radix_sort.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	bool b_success = true;
	b_success = b_success && r->program.create_kernel("radix_hist");
	b_success = b_success && r->program.create_kernel("radix_glob_scan");
	b_success = b_success && r->program.create_kernel("excl_scan_bins");
	b_success = b_success && r->program.create_kernel("remap");
	return b_success;
}


bool rs_do()
{
	RadixSort_t r;
	size_t num_el = 1024;
	int* pel = new int[num_el];
	int* pval = new int[num_el];
	for(size_t i=0;i<num_el;++i)
	{
		pel[i] = rand()%16;
		pval[i] = pel[i] + 256;
	}

	if(!rs_create_ocl_buffers(&r, pel, pval, num_el))
		return false;

	if(!rs_load_kernel(&r))
		return false;

	cl_int status;

	int mask = 0xF;
	int shift = 0;

	for(int i=0;i<2;++i)
	{

	cl_kernel k = r.program.kernels_["radix_hist"];

	mask = 0xF << shift;
	
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.src);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.src_v);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &r.dst);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &r.dst_v);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 4, sizeof(cl_mem), &r.hist);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 5, sizeof(cl_mem), &r.scan);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 6, sizeof(cl_int), &mask);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 7, sizeof(cl_int), &shift);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	float total_exec_time = 0;
	float exec_time;
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, r.num_blocks, &exec_time))
		return false;

	{
	ScopedMap sm(r.hist);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.hist_num_el);
	for(int j=0;j<r.num_blocks;++j)
	{
		for(int i=0;i<r.NUM_BINS;++i)
		{
			printf("%d ", p[j*r.NUM_BINS + i]);
		}
		printf("\n");
	}
	}

	{
	ScopedMap sm(r.scan);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.scan_num_el);
	for(int j=0;j<r.num_blocks;++j)
	{
		for(int i=0;i<r.NUM_BINS;++i)
		{
			printf("%d ", p[j*r.NUM_BINS + i]);
		}
		printf("\n");
	}
	}

	{
	ScopedMap sm(r.dst);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.num_el);
	for(int j=0;j<r.num_blocks;++j)
	{
		for(int i=0;i<r.num_el_per_wg;++i)
		{
			printf("%d ", p[j*r.num_el_per_wg + i]);
		}
		printf("\n");
	}
	}

	k = r.program.kernels_["radix_glob_scan"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.scan);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &r.num_blocks);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	// each WG processes up to 2*BLK_SIZ elements from one bin 
	int num_loc2glob_blocks = r.NUM_BINS * (r.num_blocks + 2*BLK_SIZ - 1) / (2*BLK_SIZ);
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, num_loc2glob_blocks, &exec_time))
		return false;

	{
	ScopedMap sm(r.scan);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.scan_num_el);
	for(int j=0;j<r.num_blocks;++j)
	{
		for(int i=0;i<r.NUM_BINS;++i)
		{
			printf("%d ", p[j*r.NUM_BINS + i]);
		}
		printf("\n");
	}
	}

	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.NUM_BINS);
	for(int i=0;i<r.NUM_BINS;++i)
	{
		printf("%d ", p[i]);
	}
	printf("\n");
	}

	k = r.program.kernels_["excl_scan_bins"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, 1, &exec_time))
		return false;

	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.NUM_BINS);
	for(int i=0;i<r.NUM_BINS;++i)
	{
		printf("%d ", p[i]);
	}
	printf("\n");
	}

	k = r.program.kernels_["remap"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.dst);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.dst_v);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &r.src);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &r.src_v);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 4, sizeof(cl_mem), &r.scan);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 5, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 6, sizeof(cl_int), &mask);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 7, sizeof(cl_int), &shift);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, r.num_blocks, &exec_time))
		return false;

	{
	ScopedMap sm(r.src);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*num_el);
	int prev = p[0];
	for(size_t i=0;i<num_el;++i)
	{
		printf("%d ", p[i]);
		if(prev > p[i])
		{
			int adsfa=0;
		}
		prev = p[i];

	}
	printf("\n");
	}

	{
	ScopedMap sm(r.scan);
	int* p = (int*)sm.map(CL_MAP_WRITE, sizeof(int)*r.scan_num_el);
	memset(p, 0, sizeof(int)*r.scan_num_el);
	}
	{
	ScopedMap sm(r.hist);
	int* p = (int*)sm.map(CL_MAP_WRITE, sizeof(int)*r.hist_num_el);
	memset(p, 0, sizeof(int)*r.hist_num_el);
	}
	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_WRITE, sizeof(int)*r.NUM_BINS);
	memset(p, 0, sizeof(int)*r.NUM_BINS);
	}



	}

	return true;
}



