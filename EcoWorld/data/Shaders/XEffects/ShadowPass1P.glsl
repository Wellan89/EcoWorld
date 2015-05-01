uniform float MaxD;

void main() 
{
	float depth = gl_TexCoord[0].z / MaxD;

#ifdef XEFFECTS_USE_VSM_SHADOWS
	gl_FragColor = vec4(depth, depth * depth, 0.0, 0.0);
#else
	gl_FragColor = vec4(depth, 0.0, 0.0, 0.0);
#endif
}
