#include <assert.h>
#include <string.h>
#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_stream_compact.h"

bool sc_create_ocl_buffers(StreamCompact_t* s, unsigned char* pel, size_t num_el)
{
	const int el_siz = sizeof(unsigned char);
	cl_int status;

	// 2*BLK_SIZ = num of el-s processed by WG
	s->num_blocks = (num_el + 2*s->SC_BLK_SIZ - 1) / (2*s->SC_BLK_SIZ);
	s->num_el = 2*s->SC_BLK_SIZ*s->num_blocks; // padded to be mutiple of 2*SC_BL_SIZ

	s->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_ONLY, s->num_el*el_siz, 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	unsigned char* p = (unsigned char*)clEnqueueMapBuffer(ocl_get_queue(), s->src, CL_TRUE, CL_MAP_WRITE, 0, s->num_el, 0, NULL, NULL, &status); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	for(int i=0;i<num_el;++i)
		p[i] = pel[i];
	memset(p+num_el, 0, s->num_el-num_el);
	status = clEnqueueUnmapMemObject(ocl_get_queue(), s->src, p, 0,NULL, NULL); CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	
	s->loc_sums = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*sizeof(int), 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	s->block_sums = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, s->num_blocks*sizeof(int), 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	s->num_masked = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, 128*sizeof(int), 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	s->indices = clCreateBuffer(ocl_get_context(), CL_MEM_READ_WRITE, num_el*sizeof(int), 0, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
    
	return true;
}

bool sc_load_kernel(StreamCompact_t* s)
{
	char pflags[1024];
	pflags[0] = '\0';

	if(!s->program.load(DATA_ROOT"stream_compact.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	bool b_success = true;
	b_success = b_success && s->program.create_kernel("blelloch");
	b_success = b_success && s->program.create_kernel("blelloch2");
	b_success = b_success && s->program.create_kernel("calc_remap");
	return b_success;
}

bool sc_do()
{
	StreamCompact_t s;
	int w = 500;
	int h = 333;
	int num_els = w*h;
	unsigned char* els = new unsigned char[num_els];
	int num_masked = 0;
	for(int i=0; i<num_els;++i)
	{
		int v = rand()%20 == 1 ? 1 : 0;
		els[i] = v;
		num_masked += v;	
	}
    
	if(!sc_create_ocl_buffers(&s, els, w*h))
	    return false;

	if(!sc_load_kernel(&s))
	    return false;
    
	float total_exec_time = 0;
	cl_int status;
	cl_kernel k = s.program.kernels_["blelloch"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &s.src);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &s.loc_sums);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &s.block_sums);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	//status = clSetKernelArg(k, 4, sizeof(cl_int), &s.num_el);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	float exec_time;
	if(!ocl_execute_kernel_1d_sync(k, s.SC_BLK_SIZ, s.num_blocks, &exec_time))
		return false;
	total_exec_time += exec_time;
    
	{
	ScopedMap sm(s.block_sums);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*s.num_blocks);
	int gpu_masked = 0;
	for(int i=0;i<s.num_blocks;++i)
	{
		gpu_masked += p[i];
	}
	if(gpu_masked!=num_masked)
	{
		int asdf=0;
	}
	}

	{
	ScopedMap sm(s.loc_sums);
	int* p = (int*)sm.map(CL_MAP_READ, 10*sizeof(int)*2*s.SC_BLK_SIZ);
	int asdf=0;
	}

	k = s.program.kernels_["blelloch2"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &s.block_sums);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &s.num_masked);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_int), &s.num_blocks);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
    
	if(!ocl_execute_kernel_1d_sync(k, s.GLOB_BLK_SIZ, 1, &exec_time))
		return false;
	total_exec_time += exec_time;

	{
	ScopedMap sm(s.num_masked);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*1);
	if(p[0]!=num_masked)
	{
		int adsgfa=0;
	}
	}
	{
	ScopedMap sm(s.block_sums);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*s.num_blocks);
	if(p[0]!=num_masked)
	{
	}
	}


	
	k = s.program.kernels_["calc_remap"];
	status = clSetKernelArg(k, 0, sizeof(cl_mem), &s.src);		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 1, sizeof(cl_mem), &s.loc_sums);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 2, sizeof(cl_mem), &s.block_sums);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 3, sizeof(cl_mem), &s.indices);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	status = clSetKernelArg(k, 4, sizeof(cl_int), &num_els);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	if(!ocl_execute_kernel_1d_sync(k, 64, s.num_el / 64, &exec_time))
		return false;
	total_exec_time += exec_time;
        
	{
	ScopedMap sm(s.indices);
	int* p = (int*)sm.map(CL_MAP_READ, sizeof(int)*num_masked);
	for(int i=0;i<num_masked;++i)
	{
		if(els[p[i]]==0)
		{
			int asfa=0;
		}
	}
	}
            
	return true;
}

