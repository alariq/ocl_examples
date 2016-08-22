#ifndef OCL_STREAM_COMPACT_H
#define OCL_STREAM_COMPACT_H

struct StreamCompact_t {
	cl_mem src;
	cl_mem loc_sums;
	cl_mem block_sums;
	cl_mem indices;
	cl_mem num_masked;

	ocl_program program;

	static const int SC_BLK_SIZ = 256;
	static const int GLOB_BLK_SIZ = 256;

	// calculated
	int num_el;
	int num_blocks;

	// gpu calculated
	int num_indices;
};

bool sc_do();

#endif // OCL_STREAM_COMPACT_H