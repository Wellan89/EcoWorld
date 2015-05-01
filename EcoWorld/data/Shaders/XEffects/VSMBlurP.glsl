uniform sampler2D ColorMapSampler;

void main()
{
	vec2 offsetArray[5];
#ifdef XEFFECTS_VERTICAL_VSM_BLUR
	offsetArray[0] = vec2(0.0, 0.0);
	offsetArray[1] = vec2(0.0, -1.5 / gl_TexCoord[3].y);
	offsetArray[2] = vec2(0.0, 1.5 / gl_TexCoord[3].y);
	offsetArray[3] = vec2(0.0, -2.5 / gl_TexCoord[3].y);
	offsetArray[4] = vec2(0.0, 2.5 / gl_TexCoord[3].y);
#else
	offsetArray[0] = vec2(0.0, 0.0);
	offsetArray[1] = vec2(-1.5 / gl_TexCoord[3].x, 0.0);
	offsetArray[2] = vec2(1.5 / gl_TexCoord[3].x, 0.0);
	offsetArray[3] = vec2(-2.5 / gl_TexCoord[3].x, 0.0);
	offsetArray[4] = vec2(2.5 / gl_TexCoord[3].x, 0.0);
#endif

	vec4 BlurCol = vec4(0.0, 0.0, 0.0, 0.0);

	for(int i = 0;i < 5; ++i)
		BlurCol += texture2D(ColorMapSampler, clamp(gl_TexCoord[0].xy + offsetArray[i], vec2(0.001, 0.001), vec2(0.999, 0.999)));

	gl_FragColor = BlurCol * 0.2;
}
