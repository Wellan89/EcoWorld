float tex1Percentage;

sampler2D tex[2];

float4 main(float2 Texcoord0 : TEXCOORD0, float2 Texcoord1 : TEXCOORD1) : COLOR0
{
	float4 tex0Color = tex2D(tex[0], Texcoord0);

	float4 ret;
	if (tex1Percentage == 0.0f)
	{
		// Optimisation : Dans 90% des cas, tex1Percentage == 0.0f (aucune transition n'est en cours)
		// On évite donc ainsi le calcul d'une unité de texture supplémentaire
		ret = tex0Color;
	}
	else
	{
		float4 tex1Color = tex2D(tex[1], Texcoord1);
		ret = lerp(tex0Color, tex1Color, tex1Percentage);
	}
	return ret;
}
