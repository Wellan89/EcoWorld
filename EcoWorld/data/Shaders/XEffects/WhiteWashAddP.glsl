uniform sampler2D ColorMapSampler;

float luminance(vec3 color)
{
	return clamp(color.r * 0.3 + color.g * 0.59 + color.b * 0.11, 0.0, 1.0);
}

void main() 
{
	vec4 diffuseTex = texture2D(ColorMapSampler, gl_TexCoord[1].xy);
	//diffuseTex *= gl_TexCoord[2];

	gl_FragColor = vec4(1.0, 1.0, 1.0, luminance(diffuseTex.rgb));
}
