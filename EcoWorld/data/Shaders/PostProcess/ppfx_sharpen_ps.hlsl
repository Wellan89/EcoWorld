uniform sampler2D SceneBuffer : register(s0);

uniform float BufferWidth;
uniform float BufferHeight;

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float2 texSize = float2(1.0/BufferWidth, 1.0/BufferHeight);
	float2 texSamples[8] = {
		-1.0, -1.0,
		 0.0, -1.0,
		 1.0, -1.0,
		-1.0,  0.0,
		 1.0,  0.0,
		-1.0,  1.0,
		 0.0,  1.0,
	     1.0,  1.0,
	};

	float4 finalCol = 9.0*tex2D(SceneBuffer, texCoord);

	for(int i=0; i<8; i++)
		finalCol -= tex2D(SceneBuffer, texCoord+texSize*texSamples[i]);

	return finalCol;
}
