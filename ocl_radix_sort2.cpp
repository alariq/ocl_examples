#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <assert.h>
#include <string>
#include <string.h>

#include "ocl_radix_sort2.h"

#define DEBUG_OUTPUT 0
#define DEBUG_CHECK 1

bool rs_create_ocl_buffers(RadixSort2_t* r, int* pel, int* pval, size_t num_el)
{
	const int el_siz = sizeof(int);
	cl_int status;

	r->num_el_per_wg = r->NUM_EL_PER_WI * r->RADIX_BLK_SIZ;

	r->num_el = num_el;//r->num_el_per_wg * ((num_el + r->num_el_per_wg - 1) / r->num_el_per_wg);
	r->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pel, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	r->src_v = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pval, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	
	r->dst = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	r->dst_v = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->num_blocks = (r->num_el + r->num_el_per_wg - 1) / r->num_el_per_wg;

	r->local_scan_num_el = r->NUM_BINS * r->num_blocks * r->RADIX_BLK_SIZ;
	r->local_scan = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->local_scan_num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->scan_num_el = r->num_blocks * r->NUM_BINS;
	r->scan = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->scan_num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	r->bin_offsets = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, r->NUM_BINS*el_siz, 0, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");



	return true;
}

bool rs_load_kernel(RadixSort2_t* r)
{
	char pflags[1024];
	pflags[0] = '\0';

	if(!r->program.load(DATA_ROOT"radix_sort2.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	bool b_success = true;
	b_success = b_success && r->program.create_kernel("radix_hist");
	b_success = b_success && r->program.create_kernel("radix_glob_scan");
	b_success = b_success && r->program.create_kernel("excl_scan_bins");
	b_success = b_success && r->program.create_kernel("remap");
	b_success = b_success && r->program.create_kernel("matmul");
	return b_success;
}


template<typename T> void swap(T* a, T* b)
{
	T t = *a;
	*a = *b;
	*b = t;
}

// max amount of data which can be sorted by this func: (RADIX_BLK_SIZ*NUM_EL_PER_WI)*LOC2GLOB_BLK_SIZ
bool rs_do2()
{
	RadixSort2_t r;
	size_t num_el = //64*8*200 + 45;
                    220480;
	int* pel = new int[num_el];
	int* pval = new int[num_el];

	for(size_t i=0;i<num_el;++i)
	{
		//pel[i] = (i+230567)/(2*num_el - i);//rand()%0xFFFFFFFF;
		pel[i] = rand()%0xFFFFFFFF;
		pval[i] = pel[i] + 256;
	}

	if(!rs_create_ocl_buffers(&r, pel, pval, num_el))
		return false;

	if(!rs_load_kernel(&r))
		return false;

	cl_int status;

	int mask_width = 4; // 4 bits
	int mask = 0xF;
	int shift = 0;
	int num_iter = sizeof(int)*8 / mask_width;

	float total_exec_time = 0;
#if 0
	float cpu_m1[16] = {
		0.99114645f,  0.09327769f,  0.90075564f,  0.8913309f,
		0.59739089f,  0.13906649f,  0.94246316f,  0.65673178f,
		0.24535166f,  0.68942326f,  0.41361505f,  0.5789603f,
		0.31962237f,  0.17714553f,  0.49025267f,  0.21861202
	};

	float cpu_m2[16] = {
		0.41509482f,  0.82779616f,  0.74143827f,  0.37681136f,
		0.88058949f,  0.01039944f,  0.4342753f,   0.45752665f,
		0.60375261f,  0.21243185f,  0.88312167f,  0.97394323f,
		0.60855824f,  0.69482827f,  0.61627114f,  0.57155776
	};

	cl_mem m1 = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, 16*sizeof(float), &cpu_m1, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	cl_mem m2 = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, 16*sizeof(float), &cpu_m2, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	cl_mem o = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, 16*sizeof(float), 0, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	cl_kernel k = r.program.kernels_["matmul"];
	int stride = 4;
	status = clSetKernelArg(k, 0, sizeof(cl_int), &stride);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &m1);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &m2);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &o);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	if(!ocl_execute_kernel_2d_sync(k, 4,4, 1,1, 0))
		return false;

	// test
	//status = clEnqueueWriteBuffer(ocl_get_queue(), o, TRUE, 0, 16*sizeof(float), cpu_m2, 0, 0, 0);  CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");

	{
	ScopedMap sm(o);
	float* p = (float*)sm.map(CL_MAP_READ, sizeof(float)*16);
	for(int j=0;j<4;++j)
	{
		for(int i=0;i<4;++i)
		{
			printf("%f ", p[4*j + i]);
		}
		printf("\n");
	}
	}
#endif

	for(int i=0;i<num_iter;++i, shift+=mask_width)
	{

	cl_kernel k = r.program.kernels_["radix_hist"];

	mask = 0xF << shift;
	
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.src);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.src_v);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &r.scan);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &r.local_scan);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 4, sizeof(cl_int), &mask);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 5, sizeof(cl_int), &shift);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 6, sizeof(cl_int), &r.num_el);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	float exec_time;
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, r.num_blocks, &exec_time))
		return false;

	total_exec_time += exec_time;
#if DEBUG_OUTPUT
    const int our_bins_size = r.num_blocks * r.NUM_BINS;
    int* our_bins = new int[our_bins_size];

    memset(our_bins, 0, sizeof(int)*our_bins_size);
    for(int b=0; b<r.num_blocks;++b)
    {
        for(int li=0; li<r.RADIX_BLK_SIZ; ++li) {
            int offset = r.NUM_EL_PER_WI*(b*r.RADIX_BLK_SIZ + li);
            for(int i=0; i<r.NUM_EL_PER_WI;++i) {
				if(offset + i < r.num_el) {
                	int bin = (pel[offset + i] & mask) >> shift;
					assert(bin < 16 && bin>=0);
                	our_bins[r.NUM_BINS*b + bin]++;
				}
            }
        }
    }

	{
	ScopedMap sm(r.local_scan);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.NUM_BINS*r.RADIX_BLK_SIZ);
	for(int j=0;j<r.RADIX_BLK_SIZ;++j)
	{
		for(int i=0;i<r.NUM_BINS;++i)
		{
			printf("%d ", p[i*r.RADIX_BLK_SIZ + j]);
		}
		printf("\n");
	}
	}

	{
	ScopedMap sm(r.scan);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.scan_num_el);
    // check with CPU version
	if(i == 0) { // check only works on first iteration because on second, our input array is already sorted according to higher 4 bits.
    	for(int i=0;i<our_bins_size;++i)
    	{
       		if(p[i] != our_bins[i])
			{
				printf("error!\n");
			}
    	}
	}
    delete our_bins;
    our_bins = NULL;

	for(int j=0;j<r.num_blocks;++j)
	{
		for(int i=0;i<r.NUM_BINS;++i)
		{
			printf("%d ", p[j*r.NUM_BINS + i]);
		}
		printf("\n");
	}
	}
#endif

	k = r.program.kernels_["radix_glob_scan"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.scan);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_int), &r.num_blocks);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	// each WG processes up to 2*BLK_SIZ elements from one bin 
	int num_loc2glob_blocks = r.NUM_BINS * ((r.num_blocks + 2*r.LOC2GLOB_BLK_SIZ - 1) / (2*r.LOC2GLOB_BLK_SIZ));
	if(!ocl_execute_kernel_1d_sync(k, r.LOC2GLOB_BLK_SIZ, num_loc2glob_blocks, &exec_time))
		return false;

	total_exec_time += exec_time;

#if DEBUG_OUTPUT
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
#endif

#if DEBUG_CHECK
	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.NUM_BINS);
	for(int i=0;i<r.NUM_BINS;++i)
	{
		printf("%d ", p[i]);
	}
	printf("\n");
	}
#endif

	k = r.program.kernels_["excl_scan_bins"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, 1, &exec_time))
		return false;

	total_exec_time += exec_time;

#if DEBUG_OUTPUT
	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*r.NUM_BINS);
	for(int i=0;i<r.NUM_BINS;++i)
	{
		printf("%d ", p[i]);
	}
	printf("\n");
	}
#endif

	k = r.program.kernels_["remap"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.src);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.src_v);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &r.dst);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &r.dst_v);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 4, sizeof(cl_mem), &r.local_scan);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 5, sizeof(cl_mem), &r.scan);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 6, sizeof(cl_mem), &r.bin_offsets);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 7, sizeof(cl_int), &mask);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 8, sizeof(cl_int), &shift);			CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 9, sizeof(cl_int), &r.num_el);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	
	if(!ocl_execute_kernel_1d_sync(k, r.RADIX_BLK_SIZ, r.num_blocks, &exec_time))
		return false;

	total_exec_time += exec_time;

	if(i==num_iter-1) // last iter should sort everything
	{
	ScopedMap sm(r.dst);
	ScopedMap smv(r.dst_v);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*num_el);
	int* pv = (int*)smv.map(CL_MAP_READ, sizeof(int)*num_el);
	int prev = p[0];
	int num_err = 0;
	for(size_t i=0;i<num_el;++i)
	{
#if DEBUG_OUTPUT
		printf("%d ", p[i]);
#endif
		if(prev > p[i] || pv[i]!=p[i]+256)
		{
			int adsfa=0;
			num_err++;
		}
		prev = p[i];

	}
	if(num_err)
		printf("\nNum errors: %d\n", num_err);
	}

#if 0
	{
	ScopedMap sm(r.scan);
	int* p = (int*)sm.map(CL_MAP_WRITE, sizeof(int)*r.scan_num_el);
	memset(p, 0, sizeof(int)*r.scan_num_el);
	}
	{
	ScopedMap sm(r.bin_offsets);
	int* p = (int*)sm.map(CL_MAP_WRITE, sizeof(int)*r.NUM_BINS);
	memset(p, 0, sizeof(int)*r.NUM_BINS);
	}
#endif

	swap(&r.src, &r.dst);
	swap(&r.src_v, &r.dst_v);
	
	}

	printf("Total exec time: %f", total_exec_time);

	return true;
}



