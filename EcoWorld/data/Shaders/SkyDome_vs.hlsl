float4x4 worldViewProj;

struct VS_INPUT
{
	float4 Position :		POSITION0;
	float2 Texcoord0 :		TEXCOORD0;
	float2 Texcoord1 :		TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 Position :		POSITION0;
	float2 Texcoord0 :		TEXCOORD0;
	float2 Texcoord1 :		TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.Position = mul(Input.Position, worldViewProj);
	Output.Texcoord0 = Input.Texcoord0;
	Output.Texcoord1 = Input.Texcoord1;

	return Output;
}
