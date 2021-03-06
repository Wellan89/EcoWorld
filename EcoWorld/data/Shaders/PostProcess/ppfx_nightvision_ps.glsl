// based on http://www.geeks3d.com/20091009/shader-library-night-vision-post-processing-filter-glsl/

uniform sampler2D texture0; //SceneBuffer
uniform sampler2D texture1; //NoiseTex
uniform sampler2D texture2; //MaskTex

uniform float RandomValue;
uniform float LuminanceThreshold;
uniform float ColorAmplification;
uniform float NoiseStrength;
uniform float VisionColorR;
uniform float VisionColorG;
uniform float VisionColorB;

void main ()
{
	vec4 finalCol;

	// noise
	vec2 noiseCoord;
	noiseCoord.x = RandomValue; // 0.4*sin(ElapsedTime*50.0);
	noiseCoord.y = RandomValue; // 0.4*cos(ElapsedTime*50.0);
	vec3 noiseCol = texture2D(texture1, (gl_TexCoord[0].xy*NoiseStrength) + noiseCoord).rgb;
	
	// light amplification
	vec3 texCol = texture2D(texture0, gl_TexCoord[0].xy + (noiseCol.xy*0.001)).rgb;
	texCol = clamp((texCol.xyz-vec3(LuminanceThreshold))/(ColorAmplification-LuminanceThreshold), 0.0, 1.0);

	// mask
	vec4 maskCol = texture2D(texture2, gl_TexCoord[0].xy);

	// calculate final color
	finalCol.rgb = mix((texCol+(noiseCol*0.2))*vec3(VisionColorR, VisionColorG, VisionColorB), maskCol.rgb, maskCol.a);
	finalCol.a = 1.0;

	gl_FragColor = finalCol;
}
