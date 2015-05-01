uniform sampler2D ShadowMapSampler;
uniform vec4 AmbientColour, LightColour;
uniform float MaxD;
uniform float MapResInv;

#ifdef XEFFECTS_USE_VSM_SHADOWS
float calcShadow(vec2 texCoords, vec2 offset, float RealDist)
{
	vec4 shadTexCol = texture2D(ShadowMapSampler, texCoords + offset);

	float ret = 0.0;
	if (RealDist < shadTexCol.x)
	{
		float E_x2 = shadTexCol.y;
		float Ex_2 = shadTexCol.x * shadTexCol.x;
		float variance = min(max(E_x2 - Ex_2, 0.00001) + 0.000001, 1.0);
		float m_d = (shadTexCol.x - RealDist);
		float p = variance / (variance + m_d * m_d);
		ret = (1.0 - p) / float(XEFFECTS_SAMPLES_AMOUNT);
	}
	return ret;
}
#else
float calcShadow(vec2 smTexCoord, vec2 offset, float realDistance)
{
	vec4 texDepth = texture2D(ShadowMapSampler, vec2( smTexCoord + offset));
	float extractedDistance = texDepth.r;
	
	return (extractedDistance <= realDistance) ? (1.0 / float(XEFFECTS_SAMPLES_AMOUNT)) : 0.0;
}
#endif

void main() 
{
	vec2 offsetArray[16];
	offsetArray[0] = vec2(0.0, 0.0);
	offsetArray[1] = vec2(0.0, 1.0);
	offsetArray[2] = vec2(1.0, 1.0);
	offsetArray[3] = vec2(-1.0, -1.0);
	offsetArray[4] = vec2(-2.0, 0.0);
	offsetArray[5] = vec2(0.0, -2.0);
	offsetArray[6] = vec2(2.0, -2.0);
	offsetArray[7] = vec2(-2.0, 2.0);
	offsetArray[8] = vec2(3.0, 0.0);
	offsetArray[9] = vec2(0.0, 3.0);
	offsetArray[10] = vec2(3.0, 3.0);
	offsetArray[11] = vec2(-3.0, -3.0);
	offsetArray[12] = vec2(-4.0, 0.0);
	offsetArray[13] = vec2(0.0, -4.0);
	offsetArray[14] = vec2(4.0, -4.0);
	offsetArray[15] = vec2(-4.0, 4.0);

	vec4 SMPos = gl_TexCoord[0];
	SMPos.x = (SMPos.x / SMPos.w) + 0.5;
	SMPos.y = (SMPos.y / SMPos.w) + 0.5;

	vec4 finalCol = AmbientColour;

	// If this point is within the light's frustum.
#ifdef XEFFECTS_USE_ROUND_SPOTLIGHTS
	float lengthToCenter = length(SMPos.xy - vec2(0.5, 0.5));
	if (lengthToCenter < 0.5 && SMPos.z > 0.0 && SMPos.z < MaxD)
#else
	vec2 clampedSMPos;
	clampedSMPos.x = clamp(SMPos.x, 0.0, 1.0);
	clampedSMPos.y = clamp(SMPos.y, 0.0, 1.0);
	if (clampedSMPos.x == SMPos.x && clampedSMPos.y == SMPos.y && SMPos.z > 0.0 && SMPos.z < MaxD)
#endif
	{
		float realDist = SMPos.z / MaxD - 0.002;

		float lightFactor = 1.0;
		for (int i = 0; i < XEFFECTS_SAMPLES_AMOUNT; ++i)
			lightFactor -= calcShadow(SMPos.xy, offsetArray[i] * MapResInv, realDist);

		// Multiply with diffuse.
#ifdef XEFFECTS_USE_ROUND_SPOTLIGHTS
		finalCol += LightColour * lightFactor * gl_TexCoord[1].x * clamp(5.0 - 10.0 * lengthToCenter, 0.0, 1.0);
#else
		finalCol += LightColour * lightFactor * gl_TexCoord[1].x;
#endif
	}

	gl_FragColor = finalCol;
}
