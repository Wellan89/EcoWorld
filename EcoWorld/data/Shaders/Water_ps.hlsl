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

float3		CameraPosition;	// Camera position
float		WaveHeight;
float4		WaterColor;
float		ColorBlendFactor;
sampler2D	WaterBump;		//coverage
sampler2D	RefractionMap;	//coverage
sampler2D	ReflectionMap;	//coverage

struct PS_INPUT
{
	float WaveHeightPos				: COLOR0;

	float2 BumpMapTexCoord			: TEXCOORD0;
	float3 RefractionMapTexCoord	: TEXCOORD1;
	float3 ReflectionMapTexCoord	: TEXCOORD2;

	float3 Position3D				: TEXCOORD3;
};

float4 main(PS_INPUT Input) : COLOR0
{ 
	// bump color
	float2 bumpTexCoords = Input.BumpMapTexCoord;
	bumpTexCoords.x += Input.WaveHeightPos;
	bumpTexCoords.y += Input.WaveHeightPos;
	float2 bumpColor = (tex2D(WaterBump, bumpTexCoords)).rg;
	float2 perturbation = (bumpColor - 0.5f) * WaveHeight;
	perturbation.x += Input.WaveHeightPos;
	perturbation.y += Input.WaveHeightPos;

	// refraction
	float doubleRefractionInv = 0.5f / Input.RefractionMapTexCoord.z;
	float2 ProjectedRefractionTexCoords;
	ProjectedRefractionTexCoords.x = Input.RefractionMapTexCoord.x * doubleRefractionInv + 0.5f;
	ProjectedRefractionTexCoords.y = -Input.RefractionMapTexCoord.y * doubleRefractionInv + 0.5f;
	float4 refractiveColor = tex2D(RefractionMap, ProjectedRefractionTexCoords + perturbation);

	// reflection
	float doubleReflectionInv = 0.5f / Input.ReflectionMapTexCoord.z;
	float2 ProjectedReflectionTexCoords;
    ProjectedReflectionTexCoords.x = Input.ReflectionMapTexCoord.x * doubleReflectionInv + 0.5f;
    ProjectedReflectionTexCoords.y = Input.ReflectionMapTexCoord.y * doubleReflectionInv + 0.5f;
	float4 reflectiveColor = tex2D(ReflectionMap, ProjectedReflectionTexCoords + perturbation);

	// fresnel
	float3 eyeVector = normalize(CameraPosition - Input.Position3D);
	float3 normalVector = {0.0f, 1.0f, 0.0f};
	float fresnelTerm = max(dot(eyeVector, normalVector), 0.0f);
	float4 combinedColor = lerp(refractiveColor, reflectiveColor * (1.0f - fresnelTerm), 0.25f); 

	return (ColorBlendFactor * WaterColor + (1.0f - ColorBlendFactor) * combinedColor);
}

