float4x4 WorldMatrix;
float4x4 WorldViewProjMatrix;

float MaxDistanceInv;

struct VS_INPUT
{
	float4 Pos			: POSITION0;
    float3 Normal		: NORMAL0;
	float2 Tex			: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos			: POSITION;
    float4 FinalColor	: TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// default positions
	float4 pos = mul(input.Pos, WorldViewProjMatrix);
    output.Pos = pos;

	// world normal
    output.FinalColor.rgb = mul(input.Normal, WorldMatrix) * 0.5 + 0.5;

	// depth
	output.FinalColor.a = pos.z * MaxDistanceInv;

	return output;
}
