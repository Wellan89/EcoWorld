uniform sampler2D texture0;

uniform float ColorR;
uniform float ColorG;
uniform float ColorB;

void main()
{
	// digital ITU Recommendations
	const vec3 ITU_R_601 = vec3(0.2990, 0.5870, 0.1140);
	const vec3 ITU_R_709 = vec3(0.2126, 0.7152, 0.0722);

	vec4 texCol = texture2D(texture0, gl_TexCoord[0].xy);
	float lum = dot(texCol.rgb, ITU_R_601);	
	gl_FragColor = vec4
	(
		pow(lum, 1.0/ColorR),
		pow(lum, 1.0/ColorG),
		pow(lum, 1.0/ColorG), 
		texCol.a
	)*ColorB+texCol*(1.0-ColorB);
}
