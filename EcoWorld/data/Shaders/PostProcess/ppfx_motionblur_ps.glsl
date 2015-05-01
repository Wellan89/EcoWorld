uniform sampler2D texture0; //SceneBuffer
uniform sampler2D texture1; //PrevBuffer

uniform float EffectStrength;

void main()
{
	gl_FragColor = mix(
		texture2D(texture0, gl_TexCoord[0].xy), 
		texture2D(texture1, gl_TexCoord[0].xy),	
		EffectStrength);
}
