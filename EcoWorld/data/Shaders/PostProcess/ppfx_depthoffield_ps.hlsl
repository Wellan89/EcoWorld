uniform sampler2D SceneBuffer : register(s0);
uniform sampler2D BlurBuffer : register(s1);
uniform sampler2D DistanceBuffer : register(s2);

float4 main(float2 texCoord : TEXCOORD0):COLOR0
{
	float4 sharp = tex2D(SceneBuffer, texCoord);
	float4 blur  = tex2D(BlurBuffer, texCoord);
	float dist   = tex2D(DistanceBuffer, texCoord).a;

	float factor = 0.0;
	if (dist < 0.04)
		factor = 30.0 * (0.04 - dist);
	else if (dist > 0.06)
		factor = 3.0 * (dist - 0.06);

	factor = clamp(factor, 0.0, 0.90);
	return lerp(sharp, blur, factor);
}
