uniform sampler2D SceneBuffer : register(s0);
uniform sampler2D PrevBuffer  : register(s1);

uniform float EffectStrength;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	return lerp(
		tex2D(SceneBuffer, texCoord), 
		tex2D(PrevBuffer, texCoord), 
		EffectStrength);
}
