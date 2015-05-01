uniform sampler2D texture0;

void main()
{
	vec4 finalVal = texture2D(texture0, gl_TexCoord[0].xy);
	finalVal *= finalVal;
	gl_FragColor = finalVal;
}
