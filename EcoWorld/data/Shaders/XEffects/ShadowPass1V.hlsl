float4x4 mWorldViewProj;

struct VS_OUTPUT
{
	float4 Position :	POSITION0;
	float3 Texcoords :	TEXCOORD0;
};

VS_OUTPUT vertexMain(float4 Position : POSITION0, float2 Texcoords : TEXCOORD0)
{
	VS_OUTPUT OUT;

	float4 hpos = mul(Position, mWorldViewProj);
	OUT.Position = hpos;

	OUT.Texcoords.xy = Texcoords;
	OUT.Texcoords.z = hpos.z;

	return OUT;
}
