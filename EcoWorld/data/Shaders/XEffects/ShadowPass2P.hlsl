sampler2D ShadowMapSampler : register(s0);
float4 AmbientColour, LightColour;
float MaxD;
float MapResInv;

#ifdef XEFFECTS_USE_VSM_SHADOWS
float calcShadow(float2 texCoords, float2 offset, float RealDist)
{
	float4 shadTexCol = tex2D(ShadowMapSampler, texCoords + offset);

	float ret = 0.0f;
	if (RealDist < shadTexCol.x)
	{
		float E_x2 = shadTexCol.y;
		float Ex_2 = shadTexCol.x * shadTexCol.x;
		float variance = min(max(E_x2 - Ex_2, 0.00001) + 0.000001, 1.0);
		float m_d = (shadTexCol.x - RealDist);
		float p = variance / (variance + m_d * m_d);
		ret = (1.0 - p) / XEFFECTS_SAMPLES_AMOUNT;
	}
	return ret;
}
#else
float calcShadow(float2 texCoords, float2 offset, float RealDist)
{
	float4 shadTexCol = tex2D(ShadowMapSampler, texCoords + offset);
	float extractedDistance = shadTexCol.r;

	return (extractedDistance <= RealDist ? (1.0 / XEFFECTS_SAMPLES_AMOUNT) : 0.0);
}
#endif

float4 pixelMain(float4 SMPos : TEXCOORD0, float LightAtt : TEXCOORD1) : COLOR0
{
	const float2 offsetArray[16] = 
	{
		float2(0.0, 0.0),
		float2(0.0, 1.0),
		float2(1.0, -1.0),
		float2(-1.0, 1.0),
		float2(-2.0, 0.0),
		float2(0.0, -2.0),
		float2(-2.0, -2.0),
		float2(2.0, 2.0),
		float2(3.0, 0.0),
		float2(0.0, 3.0),
		float2(3.0, -3.0),
		float2(-3.0, 3.0),
		float2(-4.0, 0.0),
		float2(0.0, -4.0),
		float2(-4.0, -4.0),
		float2(4.0, 4.0)
	};

	SMPos.x = (SMPos.x / SMPos.w) + 0.5;
	SMPos.y = (SMPos.y / SMPos.w) + 0.5;

	float4 finalCol = AmbientColour;

	// If this point is within the light's frustum.
#ifdef XEFFECTS_USE_ROUND_SPOTLIGHTS
	float lengthToCenter = length(SMPos.xy - float2(0.5, 0.5));
	if (lengthToCenter < 0.5 && SMPos.z > 0.0 && SMPos.z < MaxD)
#else
	float2 clampedSMPos = saturate(SMPos.xy);

	// Vérifie que le pixel est dans le champ de vision de la lampe (largeur et profondeur)
	// => Que la profondeur du pixel est bien disponible dans la texture et que sa distance réelle est bien comparable avec celle stockée dans la texture
	if (clampedSMPos.x == SMPos.x && clampedSMPos.y == SMPos.y && SMPos.z > 0.0 && SMPos.z < MaxD)
#endif
	{
		float realDistance = SMPos.z / MaxD - 0.002;

		float lightFactor = 1.0;
		for (int i = 0; i < XEFFECTS_SAMPLES_AMOUNT; ++i)
			lightFactor -= calcShadow(SMPos.xy, offsetArray[i] * MapResInv, realDistance);

		// Multiply with diffuse.
#ifdef XEFFECTS_USE_ROUND_SPOTLIGHTS
		finalCol += LightColour * lightFactor * LightAtt * clamp(5.0 - 10.0 * lengthToCenter, 0.0, 1.0);
#else
		finalCol += LightColour * lightFactor * LightAtt;
#endif
	}

	return finalCol;
}
