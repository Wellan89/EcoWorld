uniform sampler2D DepthBuffer : register(s0);

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	return float4(tex2D(DepthBuffer, texCoord).rgb, 1.0);
}
