float MaxD;

float4 pixelMain(float3 Texcoords : TEXCOORD0) : COLOR0
{
	float depth = Texcoords.z / MaxD;

#ifdef XEFFECTS_USE_VSM_SHADOWS
	return float4(depth, depth * depth, 0.0, 0.0);
#else
	return float4(depth, 0.0, 0.0, 0.0);
#endif
}
