uniform sampler2D ColorMapSampler;

void main() 
{
	gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(ColorMapSampler, gl_TexCoord[1].xy).a);
}
