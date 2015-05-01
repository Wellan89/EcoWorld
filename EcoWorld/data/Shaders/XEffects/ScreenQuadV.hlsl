float screenX, screenY;
float3 LineStarts0, LineStarts1, LineStarts2, LineStarts3;
float3 LineEnds0, LineEnds1, LineEnds2, LineEnds3;

struct VS_OUTPUT 
{
	float4 Position		: POSITION0;
	float2 TexCoords	: TEXCOORD0;
	float3 LStart		: TEXCOORD1;
	float3 LEnd			: TEXCOORD2;
	float2 ScreenSize	: TEXCOORD3;
};

VS_OUTPUT vertexMain(float3 Position : POSITION0)
{ 
	VS_OUTPUT OUT;

	OUT.Position = float4(Position.xy, 0.0, 1.0);

	float texX = 0.5 * (1.0 + Position.x + (1.0 / screenX));
	float texY = 1.0 - 0.5 * (1.0 + Position.y - (1.0 / screenY));
	OUT.TexCoords.x = texX;
	OUT.TexCoords.y = texY;

	float3 tLStart = lerp(LineStarts0, LineStarts1, texX);
	float3 bLStart = lerp(LineStarts2, LineStarts3, texX);
	OUT.LStart = lerp(tLStart, bLStart, texY);

	float3 tLEnd = lerp(LineEnds0, LineEnds1, texX);
	float3 bLEnd = lerp(LineEnds2, LineEnds3, texX);
	OUT.LEnd = lerp(tLEnd, bLEnd, texY);

	OUT.ScreenSize = float2(screenX, screenY);

	return OUT;
}
