#ifndef OCL_RADIX_SORT_H
#define OCL_RADIX_SORT_H

struct RadixSort_t {
	cl_mem src;
	cl_mem src_v;
	
	cl_mem dst;
	cl_mem dst_v;

	cl_mem hist;
	cl_mem scan;
	cl_mem bin_offsets;

	ocl_program program;

	static const int NUM_EL_PER_WI = 4;
	static const int NUM_BITS = 4;
	static const int NUM_BINS = 1<<NUM_BITS;
	static const int RADIX_BLK_SIZ = 64;

	// calculated
	int num_el_per_wg;
	int num_el;
	int num_blocks;
	int hist_num_el;
	int scan_num_el;

};

bool rs_do();

//void rs_create_ocl_buffers(RadixSort_t* r);


#endif // OCL_RADIX_SORT_H