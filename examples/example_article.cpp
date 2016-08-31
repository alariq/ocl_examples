#include <CL/cl.h>

#include "ocl/ocl_utils.h"
#include "ocl/ocl_program.h"
#include "ocl_test.h"
#include <assert.h>
#include <string>

#include "example_article.h"

#define DEBUG_OUTPUT 1

static bool create_ocl_buffers(MatrixTranspose_t* in, float* pel, size_t num_el)
{
	const int el_siz = sizeof(float);
	cl_int status;

	in->num_el = num_el;
	in->src = clCreateBuffer(ocl_get_context(), CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, num_el*el_siz, pel, &status);	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	in->dst = clCreateBuffer(ocl_get_context(), CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, num_el*el_siz, 0, &status);    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	in->dst2 = clCreateBuffer(ocl_get_context(), CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, num_el*el_siz, 0, &status);    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	in->dst3 = clCreateBuffer(ocl_get_context(), CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, num_el*el_siz, 0, &status);    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. ");
	
	return true;
}

static void destroy_ocl_buffers(MatrixTranspose_t* in)
{
	clReleaseMemObject(in->src);
	clReleaseMemObject(in->dst);
	clReleaseMemObject(in->dst2);
	clReleaseMemObject(in->dst3);
}

static bool load_kernel(MatrixTranspose_t* in)
{
	char pflags[1024];
	sprintf(pflags, "-Werror -DDIM_X=%d -DDIM_Y=%d", in->WG_SIZE_X, in->WG_SIZE_Y);

	if(!in->program.load(DATA_ROOT"article.cl", pflags, ocl_get_context(), ocl_get_devices() + ocl_get_device_id(), 1))
		return false;

	return in->program.create_kernel("Transpose") && in->program.create_kernel("Transpose_lm") && in->program.create_kernel("Transpose_lm_no_conf") && 
        in->program.create_kernel("Transpose_lm_padding");
}

#define N_TRIES 100
#define CHECK_RESULTS 1

static bool transpose_do()
{
	MatrixTranspose_t t;
    int matrix_size_x = 80*t.WG_SIZE_X;
    int matrix_size_y = 60*t.WG_SIZE_Y;

	const int num_el = matrix_size_x*matrix_size_y;
	float* matrix = new float[num_el];
	float* tr_matrix = new float[num_el];
	for(int y=0;y<matrix_size_y;++y) {
        for(int x=0;x<matrix_size_x;++x) {

		    float v = ((float)(rand()%1024)) / 1024.0f;
            matrix[y*matrix_size_x + x] = v;
            tr_matrix[x*matrix_size_y + y] = v;
	    }
    }

	if(!create_ocl_buffers(&t, matrix, num_el))
		return false;

	if(!load_kernel(&t))
		return false;

	float total_exec_time = 0;
	float total_exec_time_lm = 0;
	float total_exec_time_lm_no_conf = 0;
	float exec_time;
	float exec_time_lm;
	float exec_time_lm_no_conf;

	for(int tries=0;tries<N_TRIES;tries++)
	{

	    cl_int status;
	    cl_kernel k = t.program.kernels_["Transpose"];

	    status = clSetKernelArg(k, 0, sizeof(cl_mem), &t.src);	        CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 1, sizeof(cl_int), &matrix_size_x);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 2, sizeof(cl_int), &matrix_size_y);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 3, sizeof(cl_mem), &t.dst);          CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

        int grid_size_x = (matrix_size_x + t.WG_SIZE_X - 1) / t.WG_SIZE_X ;
        int grid_size_y = (matrix_size_y + t.WG_SIZE_Y - 1) / t.WG_SIZE_Y ;
            
	    if(!ocl_execute_kernel_2d_sync(k, t.WG_SIZE_X, t.WG_SIZE_Y, grid_size_x, grid_size_y, &exec_time))
            return false;

	    if(tries)
		    total_exec_time += exec_time;

#if CHECK_RESULTS
	    {
            ScopedMap sm(t.dst);
            float* p = (float*)sm.map(CL_MAP_READ, sizeof(float)*matrix_size_x*matrix_size_y);
            for(int y=0;y<matrix_size_y;++y) {
                for(int x=0;x<matrix_size_x;++x) {
                    float v_cpu = tr_matrix[y*matrix_size_x + x];
                    float v_gpu = p[y*matrix_size_x + x];
                    assert(v_cpu == v_gpu);
                }
            }
        }
#endif
	    k = t.program.kernels_["Transpose_lm"];

	    status = clSetKernelArg(k, 0, sizeof(cl_mem), &t.src);	        CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 1, sizeof(cl_int), &matrix_size_x);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 2, sizeof(cl_int), &matrix_size_y);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 3, sizeof(cl_mem), &t.dst2);         CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	    if(!ocl_execute_kernel_2d_sync(k, t.WG_SIZE_X, t.WG_SIZE_Y, grid_size_x, grid_size_y, &exec_time_lm))
            return false;

	    if(tries)
		    total_exec_time_lm += exec_time_lm;

#if CHECK_RESULTS
	    {
            ScopedMap sm(t.dst2);
            float* p = (float*)sm.map(CL_MAP_READ, sizeof(float)*matrix_size_x*matrix_size_y);
            for(int y=0;y<matrix_size_y;++y) {
                for(int x=0;x<matrix_size_x;++x) {
                    float v_cpu = tr_matrix[y*matrix_size_x + x];
                    float v_gpu = p[y*matrix_size_x + x];
                    assert(v_cpu == v_gpu);
                }
            }
        }
#endif

	    //k = t.program.kernels_["Transpose_lm_no_conf"];
	    k = t.program.kernels_["Transpose_lm_padding"];

	    status = clSetKernelArg(k, 0, sizeof(cl_mem), &t.src);	        CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 1, sizeof(cl_int), &matrix_size_x);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 2, sizeof(cl_int), &matrix_size_y);	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");
	    status = clSetKernelArg(k, 3, sizeof(cl_mem), &t.dst3);         CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	    if(!ocl_execute_kernel_2d_sync(k, t.WG_SIZE_X, t.WG_SIZE_Y, grid_size_x, grid_size_y, &exec_time_lm_no_conf))
            return false;

	    if(tries)
		    total_exec_time_lm_no_conf += exec_time_lm_no_conf;

#if CHECK_RESULTS
	    {
            ScopedMap sm(t.dst3);
            float* p = (float*)sm.map(CL_MAP_READ, sizeof(float)*matrix_size_x*matrix_size_y);
            for(int y=0;y<matrix_size_y;++y) {
                for(int x=0;x<matrix_size_x;++x) {
                    float v_cpu = tr_matrix[y*matrix_size_x + x];
                    float v_gpu = p[y*matrix_size_x + x];
                    assert(v_cpu == v_gpu);
                }
            }
        }
#endif


	}

    delete[] matrix;
    delete[] tr_matrix;
    destroy_ocl_buffers(&t);

	printf("Exec time: %f\n", total_exec_time/(N_TRIES-1));
	printf("Exec time(lm): %f\n", total_exec_time_lm/(N_TRIES-1));
	printf("Exec time(lm_no_conf): %f\n", total_exec_time_lm_no_conf/(N_TRIES-1));

	return true;
}

bool article_example_do()
{
    return transpose_do();
}

