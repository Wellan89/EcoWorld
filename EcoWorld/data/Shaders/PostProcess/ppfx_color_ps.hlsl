uniform sampler2D SceneBuffer : register(s0);

uniform float ColorR;
uniform float ColorG;
uniform float ColorB;

float4 main(float2 texCoord: TEXCOORD0) : COLOR0
{
	// digital ITU Recommendations
	const float3 ITU_R_601 = float3(0.2990, 0.5870, 0.1140);
	const float3 ITU_R_709 = float3(0.2126, 0.7152, 0.0722);

	float4 texCol = tex2D(SceneBuffer, texCoord);
	float lum = dot(texCol.rgb, ITU_R_601);
	float4 finalCol = float4
	(
		pow(lum, 1.0/ColorR),
		pow(lum, 1.0/ColorG),
		pow(lum, 1.0/ColorG),
		texCol.a
	)*ColorB+texCol*(1.0-ColorB);
			
	return finalCol;
}
