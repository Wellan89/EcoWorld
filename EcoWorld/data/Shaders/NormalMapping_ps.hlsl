float3 diffuseColor;
float3 ambientColor;

sampler2D baseMap;
sampler2D bumpMap;

struct PS_INPUT
{
	float2 Texcoord :		TEXCOORD0;
	float3 LightDirection :	TEXCOORD1;
};

float4 main(PS_INPUT Input) : COLOR0
{
	// Fonction d'éclairage avec le normal mapping :
	float3 normal = normalize((tex2D(bumpMap, Input.Texcoord).xyz * 2.0f) - 1.0f);
	float diffuseFactor = max(dot(normal, normalize(Input.LightDirection)), 0.0f);
	float3 totalDiffuseColor = diffuseColor * diffuseFactor;

	float4 baseColor = tex2D(baseMap, Input.Texcoord);
	return float4((ambientColor + totalDiffuseColor) * baseColor.rgb, baseColor.a);
}
