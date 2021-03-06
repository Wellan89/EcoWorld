// nvidia shader library
// http://developer.download.nvidia.com/shaderlibrary/webpages/shader_library.html

uniform sampler2D SceneBuffer : register(s0);

uniform float NumColors;
uniform float Gamma;

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float4 texCol = tex2D(SceneBuffer, texCoord);
	float3 col0 = texCol.rgb;
	col0 = pow(col0, Gamma);
	col0 = col0*NumColors;
	col0 = floor(col0);
	col0 = col0/NumColors;
	col0 = pow(col0, 1.0/Gamma);
	return float4(col0, texCol.a);
}
