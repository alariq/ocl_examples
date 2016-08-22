#define BLK_SIZ 256
#define GLOB_BLK_SIZ	256

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
	__global const unsigned char*	src,
	__global int* 	loc_sum,
	__global int*	block_hist
)
{
	int idx = get_global_id(0);
	int li = get_local_id(0);
	int blockId = get_group_id(0);

	__local int our_h[2*BLK_SIZ];
	our_h[2*li + 0] = src[2*BLK_SIZ*blockId + 2*li + 0];
	our_h[2*li + 1] = src[2*BLK_SIZ*blockId + 2*li + 1];

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
	{
		block_hist[blockId] = our_h[2*BLK_SIZ-1];
		our_h[2*BLK_SIZ-1] = 0; // clear last element
	}
	
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
	loc_sum[2*BLK_SIZ*blockId + 2*li + 0] = our_h[2*li + 0];
	loc_sum[2*BLK_SIZ*blockId + 2*li + 1] = our_h[2*li + 1];
}

__kernel void blelloch2(
	__global int*	src,
	__global int*	block_hist,
	int		count
)
{
	int idx = get_global_id(0);
	int li = get_local_id(0);
	int blockId = get_group_id(0);

	__local int our_h[2*GLOB_BLK_SIZ];
	if(2*GLOB_BLK_SIZ*blockId + 2*li + 0 < count)
		our_h[2*li + 0] = src[2*GLOB_BLK_SIZ*blockId + 2*li + 0];
	else
		our_h[2*li + 0] = 0;
	if(2*GLOB_BLK_SIZ*blockId + 2*li + 1 < count)
		our_h[2*li + 1] = src[2*GLOB_BLK_SIZ*blockId + 2*li + 1];
	else
		our_h[2*li + 1] = 0;

	// sweep up
	int width = 2;
	int num_el = 2*GLOB_BLK_SIZ/width;
	int wby2 = width>>1;
	for(int i=2*GLOB_BLK_SIZ>>1;i>0;i>>=1)
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
		block_hist[blockId] = our_h[2*GLOB_BLK_SIZ-1];
		our_h[2*GLOB_BLK_SIZ-1] = 0; // clear last element
	}
	
	num_el = 1;
	width = 2*GLOB_BLK_SIZ/num_el;
	wby2 = width>>1;
	for(int i=2*GLOB_BLK_SIZ>>1;i>0;i>>=1)
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
	if(2*GLOB_BLK_SIZ*blockId + 2*li + 0 < count)
		src[2*GLOB_BLK_SIZ*blockId + 2*li + 0] = our_h[2*li + 0];
	if(2*GLOB_BLK_SIZ*blockId + 2*li + 1 < count)
		src[2*GLOB_BLK_SIZ*blockId + 2*li + 1] = our_h[2*li + 1];
}

__kernel void calc_remap(
	__global unsigned char* mask,
	__global int* loc_scans,
	__global int* block_scans,
	__global int* indices,
	int count)
{
	int idx = get_global_id(0);

	int bi = idx / (2*BLK_SIZ);
	int loc_idx = idx % (2*BLK_SIZ);

	if(mask[idx]!=0)
	{
		int remap_idx = loc_scans[loc_idx + 2*BLK_SIZ*bi] + block_scans[bi];
		indices[remap_idx] = idx;
//		if(idx<1000)
//		printf("%d / %d , ", idx, remap_idx);
	}
}

