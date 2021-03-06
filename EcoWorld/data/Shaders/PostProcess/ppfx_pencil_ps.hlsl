// based on the work of Shawn Hargreaves
// http://www.talula.demon.co.uk/postprocessing/postprocessing.html

uniform sampler2D SceneBuffer : register(s0);
uniform sampler2D Pencil : register(s1);

uniform float Brightness;
uniform float EffectStrength;
uniform float ElapsedTime;

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float4 col0 = float4(1.0, 0.0, 0.0, 0.0);
	float4 col1 = float4(0.0, 0.0, 1.0, 0.0);
	
	// noise
	float2 noiseCoord;
	noiseCoord.x = 0.4*sin(ElapsedTime*50.0);
	noiseCoord.y = 0.4*cos(ElapsedTime*50.0);

	float4 texCol = Brightness*tex2D(SceneBuffer, texCoord);
	float4 pen0 = tex2D(Pencil, (texCoord*EffectStrength)+noiseCoord);
	
	float4 col2 = (1.0-texCol)*(pen0);
	
	float pen1 = dot(col2, col0);
	float pen2 = dot(col2, col1);
	
	float4 finalCol = float4(pen2, pen2, pen2, pen1);
	finalCol = (1.0-finalCol)*(1.0-finalCol.a);
	finalCol = saturate((finalCol-0.5)*2.0*texCol)*float4(1.0, 0.9, 0.8, 1.0);
	return finalCol;
}