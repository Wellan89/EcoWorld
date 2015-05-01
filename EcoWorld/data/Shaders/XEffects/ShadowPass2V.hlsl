float4x4 mWorldViewProj;
float4x4 mWorldViewProj2;
float3 LightDir;

struct VS_OUTPUT 
{
	float4 Position				: POSITION0;
	float4 ShadowMapSamplingPos : TEXCOORD0;
	float LightAtt		  		: TEXCOORD1;
};

VS_OUTPUT vertexMain(float4 Pos : POSITION0, float3 Normal : NORMAL0)
{
	VS_OUTPUT OUT;

	OUT.Position = mul(Pos, mWorldViewProj);

	float4 SMPos = mul(Pos, mWorldViewProj2);
	SMPos.xy = float2(SMPos.x, -SMPos.y) * 0.5f;
	OUT.ShadowMapSamplingPos = SMPos;

	OUT.LightAtt = max(dot(Normal, LightDir), 0.0f);

	return OUT;
}
