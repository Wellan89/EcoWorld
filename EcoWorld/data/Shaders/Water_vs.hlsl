/*
 * Copyright (c) 2007, elvman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY elvman ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Modified version

float4x4	View;
float4x4	WorldViewProj;				// World * View * Projection transformation
float4x4	WorldReflectionViewProj;	// World * Reflection View * Projection transformation
float		WaveLengthInv;
float2		WindTime;

struct VS_OUTPUT
{
	float4 Position					: POSITION;

	float WaveHeightPos				: COLOR0;

	float2 BumpMapTexCoord			: TEXCOORD0;
	float3 RefractionMapTexCoord	: TEXCOORD1;
	float3 ReflectionMapTexCoord	: TEXCOORD2;

	float3 Position3D				: TEXCOORD3;
};

struct VS_INPUT
{
	float4 Position		: POSITION;
	float2 TexCoord0	: TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.WaveHeightPos = Input.Position.y * 0.02f;

	// transform position to clip space
	Output.Position = mul(Input.Position, WorldViewProj);

	// calculate wave coords
	Output.BumpMapTexCoord = Input.TexCoord0 * WaveLengthInv + WindTime;
	float4 RefractionMapTexCoord = mul(Input.Position, WorldViewProj);
	Output.RefractionMapTexCoord = RefractionMapTexCoord.xyw;	
	float4 ReflectionMapTexCoord = mul(Input.Position, WorldReflectionViewProj);
	Output.ReflectionMapTexCoord = ReflectionMapTexCoord.xyw;

	// position of the pixel
	Output.Position3D = mul(Input.Position, View);

	return Output;
}
