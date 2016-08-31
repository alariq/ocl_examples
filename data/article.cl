    __kernel void VecAdd(
            __global const int* a,
            __global const int* b,
            __global int* c,
            int numElements)
{
        int gid = get_global_id(0);
            if(gid < numElements)
                        c[gid] = a[gid] + b[gid];
}


__kernel void Transpose(
        __global const float* m,
        int width,
        int height,
        __global float* m_tr)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    m_tr[x*height + y] = m[y*width + x];

}

// passed from host side as -D compiler option
#ifndef DIM_X
    #define DIM_X 8 
#endif
#ifndef DIM_Y
    #define DIM_Y 8
#endif

__kernel void Transpose_lm(
        __global const float* m,
        int width,
        int height,
        __global float* m_tr)
{
    __local float block[DIM_X * DIM_Y];
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);

    block[ly*DIM_X + lx] = m[gy*width + gx];
    barrier(CLK_LOCAL_MEM_FENCE);

    int grp_x = get_group_id(0);
    int grp_y = get_group_id(1);

    int block_offset = (DIM_X*grp_x)*height + (DIM_Y*grp_y);

    int lidx = ly*DIM_X + lx;
    int tr_lx = lidx / DIM_Y;
    int tr_ly = lidx % DIM_Y;
    m_tr[block_offset + tr_lx*height + tr_ly] = block[tr_ly*DIM_X + tr_lx];  
    
    // intermediate code (can show this in slides, to easier transition to final variant)
    //int x = gx;
    //int y = gy;
    //m_tr[block_offset + lx*height + ly] = m[y*width + x];
}

#define NUM_BANKS 32

__kernel void Transpose_lm_padding(
        __global const float* m,
        int width,
        int height,
        __global float* m_tr)
{
    __local float block[(DIM_X+1)*DIM_Y];
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);

    const int lidx = ly*DIM_X + lx;
    const int offset = lidx / NUM_BANKS;
    block[lidx + offset] = m[gy*width + gx];
    barrier(CLK_LOCAL_MEM_FENCE);

    int grp_x = get_group_id(0);
    int grp_y = get_group_id(1);

    int block_offset = (DIM_X*grp_x)*height + (DIM_Y*grp_y);

    int tr_lx = lidx / DIM_Y;
    int tr_ly = lidx % DIM_Y;
    int tr_lidx = tr_ly*DIM_X + tr_lx;
    int tr_offset = tr_lidx / NUM_BANKS;
    m_tr[block_offset + tr_lx*height + tr_ly] = block[tr_lidx + tr_offset];  
}

__kernel void Transpose_lm_no_conf(
        __global const float* m,
        int width,
        int height,
        __global float* m_tr)
{
    __local float block[(DIM_X+1) * DIM_Y];
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);

    block[ly*(DIM_X+1) + lx] = m[gy*width + gx];
    barrier(CLK_LOCAL_MEM_FENCE);

    int grp_x = get_group_id(0);
    int grp_y = get_group_id(1);
    int block_offset = (DIM_X*grp_x)*height + (DIM_Y*grp_y);

    //m_tr[block_offset + ly*height + lx] = block[lx*(DIM_X+1) + ly];  

    int lidx = ly*DIM_X + lx;
    int tr_lx = lidx / DIM_Y;
    int tr_ly = lidx % DIM_Y;
    m_tr[block_offset + tr_lx*height + tr_ly] = block[tr_ly*(DIM_X+1) + tr_lx];  
    
    // intermediate code (can show this in slides, to easier transition to final variant)
    //int x = gx;
    //int y = gy;
    //m_tr[block_offset + lx*height + ly] = m[y*width + x];
}

