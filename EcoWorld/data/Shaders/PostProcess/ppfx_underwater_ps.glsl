uniform sampler2D texture0; //SceneBuffer
uniform sampler2D texture1; //NormalTex
uniform sampler2D texture2; //DepthSampler

uniform float WaterColorR;
uniform float WaterColorG;
uniform float WaterColorB;
uniform float BlendFactor;

uniform float EffectStrength;
uniform float ElapsedTime;
uniform float Speed;

float getDepth(vec2 coords)
{
	vec4 texDepth = texture2D(texture2, coords);
	return texDepth.a;
}

void main()
{
	vec2 texCoord = gl_TexCoord[0].xy;
	texCoord.x += sin(ElapsedTime + texCoord.x * 10.0) * 0.005;
    texCoord.y += cos(ElapsedTime + texCoord.y * 10.0) * 0.005;	

	vec4 normalCol = 2.0 * texture2D(texture1, texCoord * 0.2 + ElapsedTime * Speed) - 1.0;
	vec4 sceneCol = texture2D(texture0, texCoord+normalCol.xy * EffectStrength);
	vec4 waterCol = vec4(WaterColorR, WaterColorG, WaterColorB, 1.0);

	gl_FragColor = mix(sceneCol, waterCol, BlendFactor);
}
