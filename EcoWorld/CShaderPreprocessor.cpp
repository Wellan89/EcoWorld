#include "CShaderPreprocessor.h"
#include "GameConfiguration.h"

int CShaderPreprocessor::addHighLevelShaderMaterialFromFiles(const io::path& vertexShaderProgramFileName, const char* vertexShaderEntryPointName, video::E_VERTEX_SHADER_TYPE vsCompileTarget,
	const io::path& pixelShaderProgramFileName, const char* pixelShaderEntryPointName, video::E_PIXEL_SHADER_TYPE psCompileTarget, video::IShaderConstantSetCallBack* callback,
	video::E_MATERIAL_TYPE baseMaterial, int userData, bool xEffectsShader, bool xEffectsVerticalVSMBlur)
{
	if (!gpu)
		return -1;

	// Obtient le header complet de ces shaders
	core::stringc vertexShader;
	generateFullShaderHeader(vertexShader, vsCompileTarget, psCompileTarget, xEffectsShader, xEffectsVerticalVSMBlur);

	// Ajoute le texte des shaders depuis les fichiers
	core::stringc pixelShader(vertexShader);
	getShaderFile(vertexShaderProgramFileName, vertexShader);
	getShaderFile(pixelShaderProgramFileName, pixelShader);

	// Demande enfin à Irrlicht de compiler ces shaders, puis retourne le résultat
	return gpu->addHighLevelShaderMaterial(
		vertexShader.c_str(), vertexShaderEntryPointName, vsCompileTarget,
		pixelShader.c_str(), pixelShaderEntryPointName, psCompileTarget,
		callback, baseMaterial, userData);
}
/*void CShaderPreprocessor::generateShadersHeader()
{
	shadersHeader = "";

	shadersHeader.append("\r\n\r\n");
}*/
void CShaderPreprocessor::generateXEffectsShadersHeader()
{
	// Génère l'en-tête pour les shaders de XEffects
	const u32 sampleCounts[EFT_COUNT] = {1, 4, 8, 12, 16};
	sprintf_SS("#define XEFFECTS_SAMPLES_AMOUNT %d", gameConfig.xEffectsFilterType == 0 ? 1 : gameConfig.xEffectsFilterType * 4);
	xEffectsShadersHeader = text_SS;

	if (gameConfig.xEffectsUseVSMShadows)
		xEffectsShadersHeader.append("\r\n#define XEFFECTS_USE_VSM_SHADOWS");
	if (gameConfig.xEffectsUseRoundSpotlights)
		xEffectsShadersHeader.append("\r\n#define XEFFECTS_USE_ROUND_SPOTLIGHTS");

	xEffectsShadersHeader.append("\r\n\r\n");
}
void CShaderPreprocessor::generateFullShaderHeader(core::stringc& outHeader, video::E_VERTEX_SHADER_TYPE vsCompileTarget, video::E_PIXEL_SHADER_TYPE psCompileTarget, bool xEffectsShader, bool xEffectsVerticalVSMBlur)
{
	// Génère l'en-tête global pour ce shader
	outHeader = "";
	//outHeader = shadersHeader;
	if (xEffectsShader)
	{
		outHeader.append(xEffectsShadersHeader);

		if (xEffectsVerticalVSMBlur)
			outHeader.append("\r\n#define XEFFECTS_VERTICAL_VSM_BLUR");
	}

	// Ajoute les informations spécifiques pour ce shader (les versions vertex shader et pixel shader utilisées)
	switch (vsCompileTarget)
	{
	case video::EVST_VS_1_1:	outHeader.append("\r\n#define VS_1_1");	break;
	case video::EVST_VS_2_0:	outHeader.append("\r\n#define VS_2_0");	break;
	case video::EVST_VS_2_a:	outHeader.append("\r\n#define VS_2_a");	break;
	case video::EVST_VS_3_0:	outHeader.append("\r\n#define VS_3_0");	break;
	case video::EVST_VS_4_0:	outHeader.append("\r\n#define VS_4_0");	break;
	case video::EVST_VS_4_1:	outHeader.append("\r\n#define VS_4_1");	break;
	case video::EVST_VS_5_0:	outHeader.append("\r\n#define VS_5_0");	break;
	}
	switch (psCompileTarget)
	{
	case video::EPST_PS_1_1:	outHeader.append("\r\n#define PS_1_1");	break;
	case video::EPST_PS_1_2:	outHeader.append("\r\n#define PS_1_2");	break;
	case video::EPST_PS_1_3:	outHeader.append("\r\n#define PS_1_3");	break;
	case video::EPST_PS_1_4:	outHeader.append("\r\n#define PS_1_4");	break;
	case video::EPST_PS_2_0:	outHeader.append("\r\n#define PS_2_0");	break;
	case video::EPST_PS_2_a:	outHeader.append("\r\n#define PS_2_a");	break;
	case video::EPST_PS_2_b:	outHeader.append("\r\n#define PS_2_b");	break;
	case video::EPST_PS_3_0:	outHeader.append("\r\n#define PS_3_0");	break;
	case video::EPST_PS_4_0:	outHeader.append("\r\n#define PS_4_0");	break;
	case video::EPST_PS_4_1:	outHeader.append("\r\n#define PS_4_1");	break;
	case video::EPST_PS_5_0:	outHeader.append("\r\n#define PS_5_0");	break;
	}
	outHeader.append("\r\n\r\n");
}
void CShaderPreprocessor::getShaderFile(const io::path& filename, core::stringc& outShader)
{
#ifdef _DEBUG
	if (&filename == &outShader)
	{
		LOG_DEBUG("CShaderPreprocessor::getShaderFile(...) : Le cas filename = outShader n'est pas autorise : &filename = " << &filename << " ; &outShader = " << &outShader << " !", ELL_ERROR);
		return;
	}
#endif

	if (!fileSystem)
		return;

	io::IReadFile* const file = fileSystem->createAndOpenFile(filename);
	if (!file)
		return;

	const u32 fileSize = file->getSize();
	char* const buffer = new char[fileSize + 1];
	if (!buffer)
		return;

	file->read(buffer, fileSize);
	buffer[fileSize] = '\0';

	outShader.append(buffer);
	delete[] buffer;
}
