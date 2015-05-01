sampler2D ColorMapSampler : register(s0);

float4 pixelMain ( float4 Texcoords : TEXCOORD0, float2 ScreenSize : TEXCOORD3 ) : COLOR0
{
	float2 offsetArray[5];
#ifdef XEFFECTS_VERTICAL_VSM_BLUR
	offsetArray[0] = float2(0, 0);
	offsetArray[1] = float2(0, 1.5 / ScreenSize.y);
	offsetArray[2] = float2(0, -1.5 / ScreenSize.y);
	offsetArray[3] = float2(0, 2.5 / ScreenSize.y);
	offsetArray[4] = float2(0, -2.5 / ScreenSize.y);
#else
	offsetArray[0] = float2(0, 0);
	offsetArray[1] = float2(1.5 / ScreenSize.x, 0);
	offsetArray[2] = float2(-1.5 / ScreenSize.x, 0);
	offsetArray[3] = float2(2.5 / ScreenSize.x, 0);
	offsetArray[4] = float2(-2.5 / ScreenSize.x, 0);
#endif

	float4 finalVal = float4(0.0, 0.0, 0.0, 0.0);

	for(int i = 0;i < 5; ++i)
		finalVal += tex2D(ColorMapSampler, clamp(Texcoords.xy + offsetArray[i], float2(0.001, 0.001), float2(0.999, 0.999)));

	return finalVal * 0.2f;
}
