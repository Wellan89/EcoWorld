uniform float screenX, screenY;
uniform vec3 LineStarts0, LineStarts1, LineStarts2, LineStarts3;
uniform vec3 LineEnds0, LineEnds1, LineEnds2, LineEnds3;

void main()  
{ 
	gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);

	vec2 tCoords;
	tCoords.x = 0.5 * (1.0 + gl_Vertex.x);
	tCoords.y = 0.5 * (1.0 + gl_Vertex.y);

	gl_TexCoord[0].xy = tCoords.xy;
	tCoords.y = 1.0 - tCoords.y;

	vec3 tLStart = mix(LineStarts0, LineStarts1, tCoords.x);
	vec3 bLStart = mix(LineStarts2, LineStarts3, tCoords.x);
	gl_TexCoord[1].xyz = mix(tLStart, bLStart, tCoords.y);

	vec3 tLEnd = mix(LineEnds0, LineEnds1, tCoords.x);
	vec3 bLEnd = mix(LineEnds2, LineEnds3, tCoords.x);
	gl_TexCoord[2].xyz = mix(tLEnd, bLEnd, tCoords.y);

	gl_TexCoord[3].xy = vec2(screenX, screenY);
}
