uniform sampler2D SceneBuffer : register(s0);

uniform float BufferWidth;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	float blurSize = 1.0/BufferWidth; 
	float4 finalCol = 0.0;
	
	// gauss distribution with  mean:0 std:2
	float weight[9] = {
		0.0295,
		0.0673,
		0.1235,
		0.1786,
		0.2022,
		0.1786,
		0.1235,
		0.0673,
		0.0295
	};
	
	// blur in x (horizontal)
	// take the samples with the distance blurSize between them
	for (int i=0; i<9; i++)
		finalCol += tex2D(SceneBuffer, float2(texCoord.x+(i-4)*blurSize, texCoord.y))*weight[i];

	return finalCol;
}


float4 main7x7(float2 texCoord : TEXCOORD0) : COLOR0
{
	float blurSize = 1.0/BufferWidth; 
	float4 finalCol = 0.0;
	
	// gauss distribution with mean:0 std:1
	float weight[7] = {
		0.0050,
		0.0540,
		0.2410,
		0.4000,
		0.2410,
		0.0540,
		0.0050
	};
	
	// blur in x (horizontal)
	// take the samples with the distance blurSize between them
	for (int i=0; i<7; i++)
		finalCol += tex2D(SceneBuffer, float2(texCoord.x+(i-3)*blurSize, texCoord.y))*weight[i];

	return finalCol;
}