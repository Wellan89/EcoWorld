uniform sampler2D SceneBuffer : register(s0);

uniform float BufferWidth;
uniform float BufferHeight;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
	// digital ITU Recommendations
	const float3 ITU_R_601 = float3(0.2990, 0.5870, 0.1140);
	const float3 ITU_R_709 = float3(0.2126, 0.7152, 0.0722);

	float2 texSize = float2(1.0/BufferWidth, 1.0/BufferHeight);
	float4 texCol = 0.0;
	float average = 0.0;
    for (int v=0; v<2; v++)
	{
		for (int u=0; u<2; u++)
		{
			texCol = tex2D(SceneBuffer, texCoord+texSize*float2(u, v));
			
            // There are a number of ways we can try and convert the RGB value into
            // a single luminance value:
            
            // 1. Do a very simple mathematical average:
            //float lumValue = dot(texCol.rgb, float3(0.33f, 0.33f, 0.33f));
            
            // 2. Perform a more accurately weighted average:
            //float lumValue = dot(texCol.rgb, ITU_R_709);
            
            // 3. Take the maximum value of the incoming, same as computing the
            //    brightness/value for an HSV/HSB conversion:
            //float lumValue = max(texCol.r, max(texCol.g, texCol.b));
            
            // 4. Compute the luminance component as per the HSL colour space:
            float lumValue = 0.5*(max(texCol.r, max(texCol.g, texCol.b))+min(texCol.r, min(texCol.g,texCol.b)));
            
            // 5. Use the magnitude of the colour
            //float lumValue = length(texCol.rgb);	
			
			average += lumValue;
		}
	}
    average *= 0.25;
    return float4(average, 0.0, 0.0, 1.0);	
}

