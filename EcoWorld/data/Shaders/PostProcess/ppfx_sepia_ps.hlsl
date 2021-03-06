// nvidia shader library
// http://developer.download.nvidia.com/shaderlibrary/webpages/shader_library.html

uniform sampler2D SceneBuffer : register(s0);

uniform float Toning;
uniform float Desaturation;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	// digital ITU Recommendations
	const float3 ITU_R_601 = float3(0.2990, 0.5870, 0.1140);
	const float3 ITU_R_709 = float3(0.2126, 0.7152, 0.0722);

	const float3 PaperTone = float3(1.0, 0.95, 0.7);
	const float3 StainTone = float3(0.2, 0.05, 0.0);
	
	float3 texColor = PaperTone*tex2D(SceneBuffer, texCoord).rgb;
    float gray = dot(texColor.rgb, ITU_R_601);
    float3 muted = lerp(texColor, gray.xxx, Desaturation);
    float3 sepia = lerp(StainTone, PaperTone, gray);
    float3 result = lerp(muted, sepia, Toning);
   
    return float4(result, 1.0);
}