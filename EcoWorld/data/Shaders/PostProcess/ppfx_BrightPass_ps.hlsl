sampler2D texture0 : register(s0);

float4 main(float4 Texcoords : TEXCOORD0) : COLOR0
{
	float4 finalVal = tex2D(texture0, Texcoords.xy);
	finalVal *= finalVal;
	return finalVal;
}
