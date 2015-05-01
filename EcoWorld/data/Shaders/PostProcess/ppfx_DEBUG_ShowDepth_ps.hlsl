uniform sampler2D DepthBuffer : register(s0);

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float depth = tex2D(DepthBuffer, texCoord).a;
	return float4(depth, depth, depth, 1.0);
}
