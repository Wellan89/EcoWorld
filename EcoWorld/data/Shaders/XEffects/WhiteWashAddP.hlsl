sampler2D ColorMapSampler : register(s0);

float luminance(float3 color)
{
	return clamp(color.r * 0.3 + color.g * 0.59 + color.b * 0.11, 0.0, 1.0);
}

float4 pixelMain(float4 Color : TEXCOORD0, float2 Texcoords : TEXCOORD1, float4 VColor : TEXCOORD2) : COLOR0
{
	float4 diffuseTex = tex2D(ColorMapSampler, Texcoords);
	diffuseTex *= VColor;

	return float4(1.0, 1.0, 1.0, luminance(diffuseTex.rgb));
}
