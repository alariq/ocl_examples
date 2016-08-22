#ifndef OCL_REDUCE_H
#define OCL_REDUCE_H

struct Reduce_t {
	cl_mem src;
	cl_mem sum;
	ocl_program program;

	const static int WG_SIZE = 64;
	int num_el;
};

bool reduce_do();

#endif // OCL_REDUCE_H