// nvidia shader library
// http://developer.download.nvidia.com/shaderlibrary/webpages/shader_library.html

uniform sampler2D SceneBuffer : register(s0);

uniform float NumTilesX;
uniform float NumTilesY;
uniform float Threshhold;

float4 main(float2 texCoord : TEXCOORD0) : COLOR
{
	float3 edgeCol = float3(0.6, 0.6, 0.6);

    float2 texSize = float2(1.0/NumTilesX, 1.0/NumTilesY);
	float2 baseCoord = texCoord-fmod(texCoord, texSize);
    float2 centerCoord = baseCoord+texSize/2.0;
    float2 stCoord = (texCoord-baseCoord)/texSize;
    float4 col0 = float4(1.0-edgeCol, 1.0);
    float threshholdB = 1.0-Threshhold;
    
    float4 col1 = 0.0;
    float4 col2 = 0.0;
    if (stCoord.x > stCoord.y) 
    { 
		col1 = col0; 
	}
    if (stCoord.x > threshholdB) 
    { 
		col2 = col1; 
    }
    if (stCoord.y > threshholdB) 
    { 
		col2 = col1; 
    }
    float4 bottomCol = col2;
    
    col1 = 0.0;
    col2 = 0.0;
    if (stCoord.x > stCoord.y) 
    { 
		col1 = col0; 
    }
    if (stCoord.x < Threshhold) 
    { 
		col2 = col1; 
    }
    if (stCoord.y < Threshhold) 
    { 
		col2 = col1; 
    }
    float4 topCol = col2;
    
    float4 tileCol = tex2D(SceneBuffer, centerCoord);
    float4 finalCol = tileCol+topCol-bottomCol;
    return finalCol;
}
