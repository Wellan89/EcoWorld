float4x4 mWorldViewProj;

float3 lightDirection;
float3 diffuseColor;
float3 ambientColor;

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Diffuse : COLOR0;
	float2 TexCoord0 : TEXCOORD0;
	float2 TexCoord1 : TEXCOORD1;
};

VS_OUTPUT vertexMain(in float4 vPosition : POSITION, in float3 vNormal : NORMAL, float2 texCoord0 : TEXCOORD0, float2 texCoord1 : TEXCOORD1)
{
	VS_OUTPUT Output;
	Output.Position = mul(vPosition, mWorldViewProj);

	float diffuseFactor = max(dot(vNormal, lightDirection), 0.0f);
	Output.Diffuse = float4(diffuseColor * diffuseFactor, 0.0f);

	Output.TexCoord0 = texCoord0;
	Output.TexCoord1 = texCoord1;
	return Output;
}



sampler2D tex[4];

PS_OUTPUT pixelMain(float2 TexCoord0 : TEXCOORD0, float2 TexCoord1 : TEXCOORD1, float4 Diffuse : COLOR0) : COLOR0
{
	float4 dcol = {0.5f, 0.5f, 0.5f, 0.5f};

	float4 a  = tex2D(tex[0], TexCoord0);
	float4 c1 = tex2D(tex[1], TexCoord1);
	float4 c2 = tex2D(tex[2], TexCoord1);
	float4 d  = tex2D(tex[3], TexCoord1);
	float4 finalCol = ((lerp(c1, c2, 0.3f * a.r + 0.59f * a.g + 0.11f * a.b)) + d - dcol) * float4(Diffuse + ambientColor, 1.0f);

	return finalCol;
}
