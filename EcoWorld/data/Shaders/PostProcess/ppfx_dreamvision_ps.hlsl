// based on http://www.geeks3d.com/20091112/shader-library-dream-vision-post-processing-filter-glsl/

uniform sampler2D SceneBuffer : register(s0);

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	float4 finalCol = tex2D(SceneBuffer, texCoord);

	finalCol += tex2D(SceneBuffer, texCoord+0.001);
	finalCol += tex2D(SceneBuffer, texCoord+0.003);
	finalCol += tex2D(SceneBuffer, texCoord+0.005);
	finalCol += tex2D(SceneBuffer, texCoord+0.007);
	finalCol += tex2D(SceneBuffer, texCoord+0.009);
	finalCol += tex2D(SceneBuffer, texCoord+0.011);

	finalCol += tex2D(SceneBuffer, texCoord-0.001);
	finalCol += tex2D(SceneBuffer, texCoord-0.003);
	finalCol += tex2D(SceneBuffer, texCoord-0.005);
	finalCol += tex2D(SceneBuffer, texCoord-0.007);
	finalCol += tex2D(SceneBuffer, texCoord-0.009);
	finalCol += tex2D(SceneBuffer, texCoord-0.011);
	finalCol = finalCol/9.5;
	
	return finalCol; 
}



