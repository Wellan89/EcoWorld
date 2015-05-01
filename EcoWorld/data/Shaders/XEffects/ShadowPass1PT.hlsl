sampler2D ColorMapSampler : register(s0);
float MaxD;

float4 pixelMain(float3 Texcoords : TEXCOORD0) : COLOR0
{
	float depth = Texcoords.z / MaxD;
	float alpha = tex2D(ColorMapSampler, Texcoords.xy).a;

#ifdef XEFFECTS_USE_VSM_SHADOWS
	return float4(depth, depth * depth, 0.0, alpha);
#else
	return float4(depth, 0.0, 0.0, alpha);
#endif
}
