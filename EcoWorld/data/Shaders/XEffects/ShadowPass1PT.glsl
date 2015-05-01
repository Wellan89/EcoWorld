uniform sampler2D ColorMapSampler;
uniform float MaxD;

void main() 
{
	float depth = gl_TexCoord[0].z / MaxD;
	float alpha = texture2D(ColorMapSampler, gl_TexCoord[0].xy).a;

#ifdef XEFFECTS_USE_VSM_SHADOWS
	gl_FragColor = vec4(depth, depth * depth, 0.0, alpha);
#else
	gl_FragColor = vec4(depth, 0.0, 0.0, alpha);
#endif
}
