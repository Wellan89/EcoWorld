uniform sampler2D texture0; //SceneBuffer
uniform sampler2D texture1; //BlurBuffer
uniform sampler2D texture2; //DistanceBuffer

void main()
{
	vec4 sharp = texture2D(texture0, gl_TexCoord[0].xy);
	vec4 blur  = texture2D(texture1, gl_TexCoord[0].xy);
	float dist = texture2D(texture2, gl_TexCoord[0].xy).a;

	float factor = 0.0;
	if (dist < 0.04)
		factor = 30.0 * (0.04 - dist);
	else if (dist > 0.06)
		factor = 3.0 * (dist - 0.06);

	factor = clamp(factor, 0.0, 0.90);
	gl_FragColor = mix(sharp, blur, factor);
}
