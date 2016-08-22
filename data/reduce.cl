#define WG_SIZE 64

__kernel void reduce(
	 __global int* pdata
	,__global int* sum
	,int count
	)
{
	
	int li = get_local_id(0);
	int block_id = get_group_id(0);
	
	__local int our_data[2*WG_SIZE];

	if(block_id*2*WG_SIZE + li + 0 < count)
		our_data[li + 0] = pdata[block_id*2*WG_SIZE + li + 0];
	else
		our_data[li + 0] = 0;

	if(block_id*2*WG_SIZE + li + WG_SIZE < count)
		our_data[li + WG_SIZE] = pdata[block_id*2*WG_SIZE + li + WG_SIZE];
	else
		our_data[li + WG_SIZE] = 0;

	
	int width = 2;
	int num_el = 2*WG_SIZE/width;
	int wby2 = width>>1;
	for(int i=2*WG_SIZE>>1;i>0;i>>=1)
	{
	    	barrier(CLK_LOCAL_MEM_FENCE);
		if(li < num_el)
		{
		    int idx = width*(li+1) - 1;
		    our_data[idx] = our_data[idx] + our_data[(idx - wby2)];
		}
		width<<=1;
		wby2 = width>>1;
		num_el>>=1;
	}

	if(0 == li)
	{
		int v = our_data[2*WG_SIZE-1];
		sum[block_id*2*WG_SIZE] = v;
	}
}

