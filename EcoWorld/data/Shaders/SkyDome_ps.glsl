uniform float tex1Percentage;
uniform sampler2D tex0, tex1;

void main()
{
	vec4 tex0Color = texture2D(tex0, gl_TexCoord[0].xy);

	if (tex1Percentage == 0.0f)
	{
		// Optimisation : Dans 90% des cas, tex1Percentage == 0.0f (aucune transition n'est en cours)
		// On évite donc ainsi le calcul d'une unité de texture supplémentaire
		gl_FragColor = tex0Color;
	}
	else
	{
		vec4 tex1Color = texture2D(tex1, gl_TexCoord[1].xy);
		gl_FragColor = mix(tex0Color, tex1Color, tex1Percentage);
	}
}
