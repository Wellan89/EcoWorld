float4x4 mWorldViewProj;
float4x4 mWorld;
float TerrainHeight;

float3 lightDirection;
float3 diffuseColor;
float3 ambientColor;

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Diffuse : COLOR0;
	float2 TexCoord1 : TEXCOORD1;
};

VS_OUTPUT vertexMain(in float4 vPosition : POSITION, in float3 vNormal : NORMAL, float2 texCoord1 : TEXCOORD1)
{
	VS_OUTPUT Output;
	Output.Position = mul(vPosition, mWorldViewProj);
	float diffuseFactor = max(dot(vNormal, lightDirection), 0.0f);
	Output.Diffuse = float4(diffuseColor * diffuseFactor, vPosition.y / TerrainHeight);
	Output.TexCoord1 = texCoord1;
	return Output;
}

struct PS_OUTPUT
{
	float4 RGBColor : COLOR0;
};

sampler2D tex[4];
PS_OUTPUT pixelMain(float2 TexCoord1 : TEXCOORD1, float4 Position : POSITION, float4 Diffuse : COLOR0 )
{
	float4 dcol = {0.5f, 0.5f, 0.5f, 0.5f};

	PS_OUTPUT Output;
	float heightpercent = Diffuse.a;
	float4 grasscolor = tex2D(tex[0], TexCoord1) * pow((1.0f - heightpercent), 4.0f);
	float4 rockcolor = tex2D(tex[1], TexCoord1) * pow((1.0f - abs(0.5f - heightpercent)), 4.0f);
	float4 snowcolor = tex2D(tex[2], TexCoord1) * pow(heightpercent,4.0f);
	float4 detailcolor = tex2D(tex[3], TexCoord1) - dcol;
	Output.RGBColor = (grasscolor + rockcolor + snowcolor + detailcolor) * float4(Diffuse.rgb + ambientColor, 1.0f);

	return Output;
}
