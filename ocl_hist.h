#ifndef OCL_HIST_H
#define OCL_HIST_H

struct Hist_t {
	cl_mem src;
	cl_mem bins;
	cl_mem loc_bins;
	cl_mem loc_bins2;

	ocl_program program;

	const static int NUM_EL_PER_WI = 16;
	const static int HIST_BLK_SIZ = 256;
	const static int SUM_BLK_SIZ = 64;
	
	int num_el;
	int num_bins;

	int num_el_per_wg;
	int num_blocks;
	int num_loc_bin_els;
};

bool hist_do();

#endif // OCL_HIST_H