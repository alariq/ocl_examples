//--------------------------------------------------------------------------------------
// File: BasicCompute11.hlsl
//
// This file contains the Compute Shader to perform array A + array B
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


cbuffer ImageParams: register(b0)
{
	uint4 whxx;
};


#ifdef USE_STRUCTURED_BUFFERS

struct BufType
{
    int i;
    float f;
};

StructuredBuffer<BufType> Buffer0 : register(t0);
StructuredBuffer<BufType> Buffer1 : register(t1);
RWStructuredBuffer<BufType> YPlane : register(u0);
//RWStructuredBuffer<BufType> UVPlane : register(u1);

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    BufferOut[DTid.x].i = Buffer0[DTid.x].i + Buffer1[DTid.x].i;
    BufferOut[DTid.x].f = Buffer0[DTid.x].f + Buffer1[DTid.x].f;
#ifdef TEST_DOUBLE
    BufferOut[DTid.x].d = Buffer0[DTid.x].d + Buffer1[DTid.x].d;
#endif 
}

#else // The following code is for raw buffers

ByteAddressBuffer Buffer0 : register(t0);
ByteAddressBuffer Buffer1 : register(t1);
//RWByteAddressBuffer BufferOut : register(u0);
//RWByteAddressBuffer BufferOut : register(t2);
RWTexture2D<float> YPlaneTex : register(u0);
RWTexture2D<float2> UVPlaneTex : register(u1);

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{

    int i0 = asint( Buffer0.Load( DTid.x*8 ) );
    float f0 = asfloat( Buffer0.Load( DTid.x*8+4 ) );
    int i1 = asint( Buffer1.Load( DTid.x*8 ) );
    float f1 = asfloat( Buffer1.Load( DTid.x*8+4 ) );
    
    //BufferOut.Store( DTid.x*8, asuint(i0 + whxx.x*i1) );
    //BufferOut.Store( DTid.x*8+4, asuint(f0 + whxx.y*f1) );
    YPlaneTex[DTid.xy] = float(DTid.y%2);	
    //UVPlaneTex[DTid.xy] = float2(0.5f, 0.5);	
}

#endif // USE_STRUCTURED_BUFFERS
