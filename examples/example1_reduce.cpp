#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <assert.h>
#include <string>

#include "example1_reduce.h"

#define DEBUG_OUTPUT 1

bool reduce_create_ocl_buffers(Reduce_t* in, int* pel, size_t num_el)
{
	const int el_siz = sizeof(int);
	cl_int status;

	in->num_el = num_el;
	in->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pel, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	in->sum = clCreateBuffer(ocl_get_context(), CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, el_siz, 0, &status);    	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	
	return true;
}

void reduce_destroy_ocl_buffers(Reduce_t* in)
{
	clReleaseMemObject(in->src);
	clReleaseMemObject(in->sum);
}

bool reduce_load_kernel(Reduce_t* in)
{
	char pflags[1024];
	sprintf(pflags, "-DWG_SIZ=%d ", in->WG_SIZE);

	if(!in->program.load(DATA_ROOT"reduce.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	return in->program.create_kernel("reduce");
}

#define N_TRIES 10
#define CHECK_RESULTS 1

bool reduce_do()
{
	Reduce_t r;
	const size_t num_el = 128;
	int* pel = new int[num_el];
	int sum_cpu = 0;
	for(size_t i=0;i<num_el;++i)
	{
		pel[i] = rand()%1024;
		sum_cpu += pel[i];
	}

	if(!reduce_create_ocl_buffers(&r, pel, num_el))
		return false;

	if(!reduce_load_kernel(&r))
		return false;

	float total_exec_time = 0;
	float exec_time;
	for(int tries=0;tries<N_TRIES;tries++)
	{

	    cl_int status;
	    cl_kernel k = r.program.kernels_["reduce"];

	    status = clSetKernelArg(k, 0, sizeof(cl_mem), &r.src);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 1, sizeof(cl_mem), &r.sum);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 2, sizeof(cl_int), &r.num_el);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	    if(!ocl_execute_kernel_1d_sync(k, r.WG_SIZE, 1, &exec_time))
		return false;

	    if(tries)
		total_exec_time += exec_time;

#if CHECK_RESULTS
	    {
		ScopedMap sm(r.sum);
		int* p = (int*)sm.map(CL_MAP_READ, sizeof(int));
		int num_hist_el = 0;
		assert(*p == sum_cpu);
	    }
#endif
	}

	printf("Exec time: %f\n", total_exec_time/(N_TRIES-1));

	return true;
}



