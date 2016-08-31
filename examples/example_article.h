#ifndef EXAMPLE_ARTICLE_H
#define EXAMPLE_ARTICLE_H

struct MatrixTranspose_t {
	cl_mem src;
	cl_mem dst;
	cl_mem dst2;
	cl_mem dst3;
	ocl_program program;

	const static int WG_SIZE_X = 4;
	const static int WG_SIZE_Y = 64;
	int num_el;
};

bool article_example_do(void);

#endif // EXAMPLE_ARTICLE_H
