//--------------------------------------------------------------------------------------
// File: SimpleSample.hlsl
//
// The HLSL file for the SimpleSample sample for the Direct3D 11 device
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------------------
Texture2D txBackground : register( t0 );
Texture3D txLUT : register( t1 );
SamplerState samLinear : register( s0 );

//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( uint id : SV_VERTEXID )
{
    VS_OUTPUT Output;

	Output.Tex = float2((id << 1) & 2, id & 2);
    Output.Pos = float4(Output.Tex * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return Output;    
}

//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
// color with diffuse material color
//--------------------------------------------------------------------------------------
float4 PSMain( VS_OUTPUT input ) : SV_TARGET
{
	float3 index = txBackground.Sample( samLinear, input.Tex ).rgb;
	float4 lut = txLUT.SampleGrad( samLinear, index, 0, 0 ); // SampleGrad is used so only the top mip is used

	return pow(lut, 2.2f);
}
