#ifndef OCL_RADIX_SORT2_H
#define OCL_RADIX_SORT2_H

struct RadixSort2_t {
	cl_mem src;
	cl_mem src_v;
	
	cl_mem dst;
	cl_mem dst_v;

	cl_mem local_scan;
	cl_mem scan;
	cl_mem bin_offsets;

	ocl_program program;

	static const int NUM_EL_PER_WI = 2;
	static const int NUM_BITS = 4;
	static const int NUM_BINS = 1<<NUM_BITS;
	static const int RADIX_BLK_SIZ = 256;
	static const int LOC2GLOB_BLK_SIZ = 256;

	// calculated
	int num_el_per_wg;
	int num_el;
	int num_blocks;
	int local_scan_num_el;
	int scan_num_el;

};

bool rs_do2();

//void rs_create_ocl_buffers(RadixSort_t* r);


#endif // OCL_RADIX_SORT2_H