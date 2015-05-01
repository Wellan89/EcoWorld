uniform sampler2D texture0;

void main()
{
	float depth = texture2D(texture0, gl_TexCoord[0].xy).a;
	gl_FragColor = vec4(depth, depth, depth, 1.0);
}
