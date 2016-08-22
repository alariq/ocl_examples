#define NUM_EL_PER_WI 	8
#define NUM_BINS 	16
#define BLK_SIZ		64
#define LOC2GLOB_BLK_SIZ 256

// our bins
//			BIN_ID 0 - 15
//		0 1 2 3 4 5 6 7 8 9 a b c d e f
// wo		x
// rk		x	
// ite		x
// ms		x
// from		x
// 0 		x
// to		.
// BNLK_SIZ-1	.
//		.
//
//		x
// x - may contain number from 0 to NUM_EL_PER_WI


// our_h - sum of all x values in column
// we then do exclusive scan on it



__kernel void radix_hist(
	__global const int* src
	,__global const int* src_v
	,__global int* scan // used to calc global scan ( global offset of each bin in a work group)
	,__global int* local_scan
	,int mask
	,int shift
	,int num_el)
{
	
	__local unsigned short our_bins[NUM_BINS][BLK_SIZ];
	__local unsigned short our_h[NUM_BINS];

	int li = get_local_id(0);
	int gi = get_global_id(0);
	int bi = get_group_id(0);

	// clear local mem
	for(int i=0; i<NUM_BINS;++i)
		our_bins[i][li] = 0;

	if(li < NUM_BINS)
	{
		our_h[li] = 0;
	}

	int offset = NUM_EL_PER_WI*(bi*BLK_SIZ + li);

	for(int i=0;i<NUM_EL_PER_WI;++i)
	{
		if(offset + i < num_el)
		{
			int bin = (src[offset + i] & mask) >> shift;
			our_bins[bin][li]++;
			//atomic_add(&our_h[bin], 1);
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	
	// perform exclusive scan on our_bins
	for(int bin=0;bin<NUM_BINS;++bin)
	{
		// sweep up
		int width = 2;
		int num_el = BLK_SIZ/width;
		int wby2 = width>>1;
		for(int i=BLK_SIZ>>1;i>0;i>>=1)
		{
		    	barrier(CLK_LOCAL_MEM_FENCE);
			if(li < num_el)
			{
			    int idx = width*(li+1) - 1;
			    our_bins[bin][idx] = our_bins[bin][idx] + our_bins[bin][(idx - wby2)];
			}
			width<<=1;
			wby2 = width>>1;
			num_el>>=1;
		}
		// down-sweep
		if(0 == li)
		{
			int v = our_bins[bin][BLK_SIZ-1];
			our_h[bin] = v;
			our_bins[bin][BLK_SIZ-1] = 0; // clear last element
		}
	
		num_el = 1;
		width = BLK_SIZ/num_el;
		wby2 = width>>1;
		for(int i=BLK_SIZ>>1;i>0;i>>=1)
		{
		   	barrier(CLK_LOCAL_MEM_FENCE);
			if(li < num_el) 
			{
				int idx = (width)*(li+1) - wby2 - 1;
				int tmp = our_bins[bin][idx];
	
				our_bins[bin][idx] = our_bins[bin][(idx + wby2)];
				our_bins[bin][(idx + wby2)] = tmp + our_bins[bin][(idx + wby2)];
			}
			width>>=1;
			wby2 = width>>1;
			num_el<<=1;
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		local_scan[NUM_BINS*BLK_SIZ*bi + bin*BLK_SIZ + li] = our_bins[bin][li];
	}

	

	if(li < NUM_BINS)
		scan[NUM_BINS*bi + li] = our_h[li];
}

__kernel void radix_glob_scan(
	__global int* scan
	,__global int* bin_offsets // will contain global offset for each bin
	,const int num_blocks
	)
{
	
	int bin2sort = get_group_id(0);
	int li = get_local_id(0);

	__local int our_h[2*LOC2GLOB_BLK_SIZ];

	if(li + 0 < num_blocks)
		our_h[li + 0] = scan[NUM_BINS*(li + 0) + bin2sort];
	else
		our_h[li + 0] = 0;
	if(li + LOC2GLOB_BLK_SIZ < num_blocks)
		our_h[li + LOC2GLOB_BLK_SIZ] = scan[NUM_BINS*(li + LOC2GLOB_BLK_SIZ) + bin2sort];
	else
		our_h[li + LOC2GLOB_BLK_SIZ] = 0;

	// sweep up
	int width = 2;
	int num_el = 2*LOC2GLOB_BLK_SIZ/width;
	int wby2 = width>>1;
	for(int i=2*LOC2GLOB_BLK_SIZ>>1;i>0;i>>=1)
	{
	    	barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el)
		{
		    int idx = width*(li+1) - 1;
		    our_h[idx] = our_h[idx] + our_h[(idx - wby2)];
		}
		width<<=1;
		wby2 = width>>1;
		num_el>>=1;
	}
	// down-sweep
	if(0 == li)
	{
		int v = our_h[2*LOC2GLOB_BLK_SIZ-1];
		bin_offsets[bin2sort] = v;

		our_h[2*LOC2GLOB_BLK_SIZ-1] = 0; // clear last element
	}
	
	num_el = 1;
	width = 2*LOC2GLOB_BLK_SIZ/num_el;
	wby2 = width>>1;
	for(int i=2*LOC2GLOB_BLK_SIZ>>1;i>0;i>>=1)
	{
	   	barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el) 
		{
			int idx = (width)*(li+1) - wby2 - 1;
			int tmp = our_h[idx];

			our_h[idx] = our_h[(idx + wby2)];
			our_h[(idx + wby2)] = tmp + our_h[(idx + wby2)];
		}
		width>>=1;
		wby2 = width>>1;
		num_el<<=1;
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	if(2*li + 0 < num_blocks)
		scan[NUM_BINS*(2*li + 0) + bin2sort] = our_h[2*li + 0];
	if(2*li + 1 < num_blocks)
		scan[NUM_BINS*(2*li + 1) + bin2sort] = our_h[2*li + 1];

}

__kernel void excl_scan_bins(
	__global int* bin_offsets // will contain global offset for each bin
	)
{
	int li = get_local_id(0);

	__local int our_h[NUM_BINS];
	
	if(li < NUM_BINS)
		our_h[li] = bin_offsets[li];

	int width = 2;
	int num_el = NUM_BINS/width;
	int wby2 = width>>1;
	for(int i=NUM_BINS>>1;i>0;i>>=1)
	{
	    	barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el)
		{
		    int idx = width*(li+1) - 1;
		    our_h[idx] = our_h[idx] + our_h[(idx - wby2)];
		}
		width<<=1;
		wby2 = width>>1;
		num_el>>=1;
	}
	// down-sweep
	if(0 == li)
	{
		our_h[NUM_BINS-1] = 0; // clear last element
	}
	
	num_el = 1;
	width = NUM_BINS/num_el;
	wby2 = width>>1;
	for(int i=NUM_BINS>>1;i>0;i>>=1)
	{
	   	barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el) 
		{
			int idx = (width)*(li+1) - wby2 - 1;
			int tmp = our_h[idx];

			our_h[idx] = our_h[(idx + wby2)];
			our_h[(idx + wby2)] = tmp + our_h[(idx + wby2)];
		}
		width>>=1;
		wby2 = width>>1;
		num_el<<=1;
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if(li < NUM_BINS)
		bin_offsets[li] = our_h[li];
}

__kernel void remap(
	__global const int* src
	,__global const int* src_v
	,__global int* dst
	,__global int* dst_v
	,__global const int* local_scan
	,__global const int* scan 
	,__global const int* bin_offsets
	,int mask
	,int shift
	,int num_el
)
{
	int li = get_local_id(0);
	int gi = get_global_id(0);
	int bi = get_group_id(0);

	__local unsigned char our_bins[NUM_BINS][BLK_SIZ];
	__local unsigned int our_h[NUM_BINS];

	__local unsigned int our_bo[NUM_BINS];
	

	if(li < NUM_BINS)
	{
		our_h[li] = scan[NUM_BINS*bi + li];
		our_bo[li] =  bin_offsets[li];
	}

	for(int i=0;i<NUM_BINS;++i)
	//	our_bins[li][i] = local_scan[BLK_SIZ*NUM_BINS*bi + i*BLK_SIZ + li];
		our_bins[i][li] = 0;

	barrier(CLK_LOCAL_MEM_FENCE);
	
	int offset = NUM_EL_PER_WI*(bi*BLK_SIZ + li);

	#pragma unroll NUM_EL_PER_WI
	for(int i=0;i<NUM_EL_PER_WI;++i)
	{
		if(offset + i < num_el)
		{
			int key = src[offset + i];
			int v = src_v[offset + i];
			int bin = (key & mask) >> shift;
			//int loc_idx = our_bins[li][bin]++;
			int loc_idx = local_scan[BLK_SIZ*NUM_BINS*bi + bin*BLK_SIZ + li] + our_bins[bin][li];
			our_bins[bin][li]++;
			//int idx = bin_offsets[bin] + our_h[bin] + loc_idx;
			int idx = our_bo[bin] + our_h[bin] + loc_idx;
			dst[idx] = key;
			dst_v[idx] = v;
		}
	}
}

__kernel void matmul(const unsigned int size, __global float * matrix1, __global float * matrix2, __global float * res) {

    int i = get_global_id(1); 
    int j = get_global_id(0);

    res[i + size * j] = 0;

    for (int k = 0; k < size; k++)
    {
        //res[i + size * j] += matrix1[i + size * k] * matrix2[k + size * j];
        res[i + size * j] += matrix1[k + size * i] * matrix2[j + size * k];
    }

}

