// nvidia shader library
// http://developer.download.nvidia.com/shaderlibrary/webpages/shader_library.html

uniform sampler2D texture0;

uniform float Desaturation;

void main()
{
	// digital ITU Recommendations
	const vec3 ITU_R_601 = vec3(0.2990, 0.5870, 0.1140);
	const vec3 ITU_R_709 = vec3(0.2126, 0.7152, 0.0722);

	vec3 texColor = texture2D(texture0, gl_TexCoord[0].xy).rgb;
	
    float gray = dot(texColor, ITU_R_601);
    vec3 result = mix(texColor, gray.xxx, Desaturation);
    gl_FragColor = vec4(result, 1.0);
}
