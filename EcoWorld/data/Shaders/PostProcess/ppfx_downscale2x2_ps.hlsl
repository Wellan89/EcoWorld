uniform sampler2D SceneBuffer : register(s0);

uniform float BufferWidth;
uniform float BufferHeight;

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float2 texSize = float2(1.0/BufferWidth, 1.0/BufferHeight);
	float4 average=0.0;
	const float2 samples[4] = {
		{-0.5, -0.5},
		{-0.5,  0.5},
		{ 0.5, -0.5},
		{ 0.5,  0.5}
    };
	for (int i=0; i<4; ++i)
	{
		average += tex2D(SceneBuffer, texCoord+texSize*samples[i]);
	}
	float4 finalCol = average*0.25;
	return finalCol;
}
