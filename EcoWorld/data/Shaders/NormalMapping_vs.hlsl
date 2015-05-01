float3 lightDirection;
float3x3 worldTransposed;
float4x4 worldViewProj;

struct VS_INPUT
{
	float4 Position :		POSITION0;
	float2 Texcoord :		TEXCOORD0;
	float3 Normal :			NORMAL0;
	float3 Tangent :		TEXCOORD1;	// Attention : Sous Irrlicht, les donn�es des tangentes sont stock�es dans les coordonn�es de texture 1 !
	float3 Binormal :		TEXCOORD2;	// Attention : Sous Irrlicht, les donn�es des binormales sont stock�es dans les coordonn�es de texture 2 !
};

struct VS_OUTPUT
{
	float4 Position :		POSITION0;
	float2 Texcoord :		TEXCOORD0;
	float3 LightDirection :	TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.Position = mul(Input.Position, worldViewProj);
	Output.Texcoord = Input.Texcoord;

	float3 normal = mul(Input.Normal, worldTransposed);
	float3 tangent = mul(Input.Tangent, worldTransposed);
	float3 binormal = mul(Input.Binormal, worldTransposed);

	float3 transLightDir;
	transLightDir.x = dot(tangent, lightDirection);
	transLightDir.y = dot(binormal, lightDirection);
	transLightDir.z = dot(normal, lightDirection);
	Output.LightDirection = -transLightDir;

	return Output;
}
