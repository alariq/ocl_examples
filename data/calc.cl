#define BLK_SIZ 64

__kernel void calc_hist_single_pass(
	__global const float* in
	,int numProcEl
	,__global int* hist
	,int numBins
	,float minVal
	,float range
	,__global int* dbg)
{
	int idx = get_global_id(0);
	if(idx < numProcEl)
	{
		int bucket = min(numBins-1, (int)((in[idx] - minVal)/range*numBins));
		dbg[idx] = bucket;
		atomic_inc(&hist[bucket]);
	}
}

// numBins MUST be multiple of 2*BLK_SIZ;

__kernel void hillis_steele(
	__global int* hist
	,__global int* out
	,int numBins
)
{
	int idx = get_global_id(0);
	int li = get_local_id(0);
	int blockId = get_group_id(0);

	__local int our_scan[BLK_SIZ];
	our_scan[li + 0] = hist[BLK_SIZ*blockId + li + 0];

	int width = 1;
	int num_el = BLK_SIZ - 1;
	int a,b;
	for(int d=BLK_SIZ>>1; d>0; d>>=1)
	{
		//if(blockId==0 && li==0) printf("ne: %d w: %d\n", num_el, width);
		barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el)
		{
			a = our_scan[li + 0];
			b = our_scan[li + width];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el)
		{
			our_scan[li + width] = a + b;
		}
		num_el -= width;
		width <<= 1;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	if(li==0)
	{
		out[blockId] = our_scan[BLK_SIZ - 1];
	}

	hist[BLK_SIZ*blockId + li + 0] = our_scan[li + 0];

}

__kernel void blelloch(
	__global int* hist
)
{
	int idx = get_global_id(0);
	int li = get_local_id(0);
	int blockId = get_group_id(0);

	__local int our_h[2*BLK_SIZ];
	our_h[2*li + 0] = hist[2*BLK_SIZ*blockId + 2*li + 0];
	our_h[2*li + 1] = hist[2*BLK_SIZ*blockId + 2*li + 1];

	// sweep up
	int width = 2;
	int num_el = 2*BLK_SIZ/width;
	int wby2 = width>>1;
	for(int i=2*BLK_SIZ>>1;i>0;i>>=1)
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
		our_h[2*BLK_SIZ-1] = 0; // clear last element
	
	num_el = 1;
	width = 2*BLK_SIZ/num_el;
	wby2 = width>>1;
	for(int i=2*BLK_SIZ>>1;i>0;i>>=1)
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
	hist[2*BLK_SIZ*blockId + 2*li + 0] = our_h[2*li + 0];
	hist[2*BLK_SIZ*blockId + 2*li + 1] = our_h[2*li + 1];
}

__kernel void loc2glob(
	__global int* loc_hists
	,__global int* offsets
	,int numProcEl)
{
	int idx = get_global_id(0);
	int li = get_local_id(0);
	int blockId = get_group_id(0);
	if(idx < numProcEl)
	{
		//if(blockId==1) printf("b:%d o:%d v: %d\n", blockId, offsets[blockId], loc_hists[BLK_SIZ*blockId + li]);
		loc_hists[BLK_SIZ*blockId + li] += offsets[blockId];
		//if(blockId==1) printf("v2: %d\n", loc_hists[BLK_SIZ*blockId + li]);
		
	}
}

