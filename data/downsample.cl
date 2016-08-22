constant sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

//__attribute((reqd_work_group_size(64,1,1)))
__kernel void downsample(__read_only image2d_t Y, 
			int w, int h,
                       __global uchar* dst)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int scale = 2;
#ifdef DWS4X
	float img_w = (float)w/4.0;
	float img_h = (float)h;

	
	uint16 v;
#ifdef BY_FOUR
	v.s0123 = read_imageui(Y, smp, (float2)((4*x*scale + 0.5f), (y*scale+0.5f)));
	v.s4567 = read_imageui(Y, smp, (float2)(((4*x+1)*scale + 0.5f), (y*scale+0.5f)));
	v.s89ab = read_imageui(Y, smp, (float2)(((4*x+2)*scale + 0.5f), (y*scale+0.5f)));
	v.scdef = read_imageui(Y, smp, (float2)(((4*x+3)*scale + 0.5f), (y*scale+0.5f)));
#else
	v.s0123 = read_imageui(Y, smp, (float2)((x*scale + 0.5f), (y*scale+0.5f)));
#endif

	// img_w = group size
	int rgs = img_w/2; // dws'ed group size

	int yoff = (w/scale)*y;

#ifdef BY_FOUR
	int xoff = min(0*rgs + 4*x, (w/scale)-1);
	*((__global uint*)(dst + yoff + xoff)) = (0xFF & v.s0) | ((0xFF & v.s4)<<8) | ((0xFF & v.s8)<<16) | ((0xFF & v.sC)<<24);

	xoff = min(1*rgs + 4*x, (w/scale)-1);
	*((__global uint*)(dst + yoff + xoff)) = (0xFF & v.s1) | ((0xFF & v.s5)<<8) | ((0xFF & v.s9)<<16) | ((0xFF & v.sD)<<24);

	xoff = min(2*rgs + 4*x, (w/scale)-1);
	*((__global uint*)(dst + yoff + xoff)) = (0xFF & v.s2) | ((0xFF & v.s6)<<8) | ((0xFF & v.sA)<<16) | ((0xFF & v.sE)<<24);

	xoff = min(3*rgs + 4*x, (w/scale)-1);
	*((__global uint*)(dst + yoff + xoff)) = (0xFF & v.s3) | ((0xFF & v.s7)<<8) | ((0xFF & v.sB)<<16) | ((0xFF & v.sF)<<24);
#else
	int xoff = min(0*rgs + x, (w/scale)-1);
	*((__global uchar*)(dst + yoff + xoff)) = (0xFF & v.s0);

	xoff = min(1*rgs + x, (w/scale)-1);
	*((__global uchar*)(dst + yoff + xoff)) = (0xFF & v.s1);

	xoff = min(2*rgs + x, (w/scale)-1);
	*((__global uchar*)(dst + yoff + xoff)) = (0xFF & v.s2);

	xoff = min(3*rgs + x, (w/scale)-1);
	*((__global uchar*)(dst + yoff + xoff)) = (0xFF & v.s3);
#endif

/*	*((__global uchar4*)(dst + yoff + xoff)) = (uchar4)(v.s0 , v.s4, v.s8, v.sC);

	xoff = min(1*rgs + 4*x, (w/scale)-1);
	*((__global uchar4*)(dst + yoff + xoff)) = (uchar4)(v.s1, v.s5, v.s9, v.sD);

	xoff = min(2*rgs + 4*x, (w/scale)-1);
	*((__global uchar4*)(dst + yoff + xoff)) = (uchar4)(v.s2, v.s6, v.sA, v.sE);

	xoff = min(3*rgs + 4*x, (w/scale)-1);
	*((__global uchar4*)(dst + yoff + xoff)) = (uchar4)(v.s3, v.s7, v.sB, v.sF);
*/
#else
	uchar v = read_imageui(Y, smp, (float2)(x*scale+0.5, y*scale+0.5)).s0;
	dst[x + (w/scale)*y] = v;
#endif

}
