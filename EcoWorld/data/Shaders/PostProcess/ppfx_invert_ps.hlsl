uniform sampler2D SceneBuffer : register(s0);

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	return float4(1.0-tex2D(SceneBuffer, texCoord).rgb, 1.0);
}
