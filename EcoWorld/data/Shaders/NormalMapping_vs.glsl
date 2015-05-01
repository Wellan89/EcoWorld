uniform vec3 lightDirection;
uniform mat3 worldTransposed;

attribute vec3 Tangent;
attribute vec3 Binormal;

void main()
{
	gl_Position = ftransform();
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;

	vec3 Tangent = gl_MultiTexCoord1.xyz;
	vec3 Binormal = gl_MultiTexCoord2.xyz;

	vec3 fvNormal = worldTransposed * gl_Normal;
	vec3 fvTangent = worldTransposed * Tangent;		// Attention : Sous Irrlicht, les données des tangentes sont stockées dans les coordonnées de texture 1 !
	vec3 fvBinormal = worldTransposed * Binormal;	// Attention : Sous Irrlicht, les données des binormales sont stockées dans les coordonnées de texture 2 !

	vec3 transLightDir;
	transLightDir.x = dot(fvTangent, lightDirection.xyz);
	transLightDir.y = dot(fvBinormal, lightDirection.xyz);
	transLightDir.z = dot(fvNormal, lightDirection.xyz);
	gl_TexCoord[1].xyz = -transLightDir;
}
