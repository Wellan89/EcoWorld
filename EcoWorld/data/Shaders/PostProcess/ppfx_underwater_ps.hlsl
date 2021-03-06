uniform sampler2D SceneBuffer  : register(s0);
uniform sampler2D NormalTex    : register(s1);
uniform sampler2D DepthSampler : register(s2);

uniform float WaterColorR;
uniform float WaterColorG;
uniform float WaterColorB;
uniform float BlendFactor;

uniform float EffectStrength;
uniform float ElapsedTime;
uniform float Speed;

float getDepth(float2 coords)
{
	float4 texDepth = tex2D(DepthSampler, coords);
	return texDepth.a;
}

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	texCoord.x += sin(ElapsedTime + texCoord.x * 10.0) * 0.005;
    texCoord.y += cos(ElapsedTime + texCoord.y * 10.0) * 0.005;	

	float4 normalCol = 2.0 * (tex2D(NormalTex, texCoord * 0.2 + ElapsedTime * Speed) - 0.5);
	float4 sceneCol = tex2D(SceneBuffer, texCoord+normalCol.xy*EffectStrength);
	float4 waterCol = float4(WaterColorR, WaterColorG, WaterColorB, 1.0);

	return lerp(sceneCol, waterCol, BlendFactor);
}
