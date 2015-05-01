uniform mat4 WorldMatrix;
uniform float MaxDistanceInv;

varying vec4 FinalColor;

void main()
{
	// default positions
	vec4 pos = ftransform();
    gl_Position = pos;

	// world normal
    FinalColor.rgb = (WorldMatrix * vec4(gl_Normal, 0.0)).xyz * 0.5 + 0.5;

	// depth
	FinalColor.a = pos.z * MaxDistanceInv;
}
