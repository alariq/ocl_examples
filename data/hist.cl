//#define NUM_EL_PER_WI 	4
#define NUM_BINS 	1024
//#define BLK_SIZ		64
//#define SUM_BLK_SIZ	64

















__kernel void hist(
	__global const int* src
	,__global int* loc_bins
	,int num_el)
{
	
	__local unsigned int our_h[NUM_BINS];

	int li = get_local_id(0);
	int gi = get_global_id(0);
	int bi = get_group_id(0);

	// clear local mem
	const int N = (NUM_BINS + BLK_SIZ - 1) / BLK_SIZ;
	for(int i=0;i<N;++i)
	{
		if(li + i*BLK_SIZ < NUM_BINS)
			our_h[li + i*BLK_SIZ] = 0;
	}

	int offset = NUM_EL_PER_WI*(bi*BLK_SIZ + li);

	#pragma unroll NUM_EL_PER_WI
	for(int i=0;i<NUM_EL_PER_WI;++i)
	{
		//if(offset + i < num_el) // needed only if nuber_of_elements/NUM_EL_PER_WI not multiple of number_of_workitems
		{
			int bin = src[offset + i];
			atomic_add(&our_h[bin], 1);
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i=0;i<N;++i)
	{
		if(li + i*BLK_SIZ < NUM_BINS)
			loc_bins[NUM_BINS*bi + li + i*BLK_SIZ] = our_h[li + i*BLK_SIZ];
	}
}

__kernel void hist_sum(
	__global int* loc_bins
	,__global int* out
	,const int num_loc_bins_blocks
	)
{
	
	int num_loc_bins_blocks_padded = (num_loc_bins_blocks + 2*SUM_BLK_SIZ - 1) / (2*SUM_BLK_SIZ);
	int bi = get_group_id(0) % num_loc_bins_blocks_padded;

	int bin2sort = get_group_id(0) / num_loc_bins_blocks_padded;
	int li = get_local_id(0);

	//if(li==0) printf("bidx: %d binidx: %d\n", bi, bin2sort);
	//return;

	__local int our_h[2*SUM_BLK_SIZ];

	if(bi*2*SUM_BLK_SIZ + li + 0 < num_loc_bins_blocks)
		our_h[li + 0] = loc_bins[NUM_BINS*(bi*2*SUM_BLK_SIZ + li + 0) + bin2sort];
	else
		our_h[li + 0] = 0;
	if(bi*2*SUM_BLK_SIZ + li + SUM_BLK_SIZ < num_loc_bins_blocks)
		our_h[li + SUM_BLK_SIZ] = loc_bins[NUM_BINS*(bi*2*SUM_BLK_SIZ + li + SUM_BLK_SIZ) + bin2sort];
	else
		our_h[li + SUM_BLK_SIZ] = 0;

	// sweep up
	int width = 2;
	int num_el = 2*SUM_BLK_SIZ/width;
	int wby2 = width>>1;
	for(int i=2*SUM_BLK_SIZ>>1;i>0;i>>=1)
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

	if(0 == li)
	{
		int v = our_h[2*SUM_BLK_SIZ-1];
		out[bi*NUM_BINS + bin2sort] = v;
	}
}

