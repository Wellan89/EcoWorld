uniform float ElapsedTime;

void main()
{
	gl_Position =  gl_Vertex * 2.0 - 1.0;
	gl_TexCoord[0]  = gl_Vertex;
    
    vec2 r = vec2(sin(ElapsedTime), -sin(ElapsedTime))-1.0;
	r = 1.0 + 0.02*r;
	
	vec2 ctr = vec2(0.5, 0.5);
	vec2 t = r*(gl_TexCoord[0].xy-ctr);
	gl_TexCoord[0].xy = t+0.5;
}

