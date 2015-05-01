uniform sampler2D texture0;	// Current screen buffer
uniform sampler2D texture1;	// Original screen buffer
uniform float OriginalScreenPercent;

void main()
{
	vec4 t0Col = texture2D(texture0, gl_TexCoord[0].xy);
	vec4 t1Col = texture2D(texture1, gl_TexCoord[0].xy);
	gl_FragColor = mix(t0Col, t1Col, OriginalScreenPercent);
}
