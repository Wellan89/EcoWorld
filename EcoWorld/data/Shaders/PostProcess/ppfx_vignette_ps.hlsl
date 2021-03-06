uniform sampler2D SceneBuffer : register(s0);

uniform float EdgeDarkness;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
    float2 tc= texCoord;
    float4 finalCol = tex2D(SceneBuffer, texCoord);
    tc -= 0.5;
    float vignette = 1.0-(dot(tc, tc)*EdgeDarkness);
	return finalCol*vignette;
}
