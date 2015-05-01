uniform sampler2D ColorMapSampler;

void main() 
{		
	gl_FragColor = texture2D(ColorMapSampler, gl_TexCoord[0].xy);
}
