sampler2D texture0 : register(s0);	// Current screen buffer
sampler2D texture1 : register(s1);	// Original screen buffer
uniform float OriginalScreenPercent;

float4 main(float2 TexCoords : TEXCOORD0) : COLOR0
{
	float4 t0Col = tex2D(texture0, TexCoords);
	float4 t1Col = tex2D(texture1, TexCoords);
	return lerp(t0Col, t1Col, OriginalScreenPercent);
}
