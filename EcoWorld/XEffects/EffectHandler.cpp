#include "EffectHandler.h"
#include "EffectCB.h"
#include "..\CShaderPreprocessor.h"
#include "..\CScreenQuad.h"
#include "..\GameConfiguration.h"
#ifdef _DEBUG
// Inclus Game.h pour pouvoir gérer les touches du clavier pour l'affichage d'informations de débogage
#include "../Game.h"
#endif

//#include <string>
//#include <iostream>
//#include <fstream>

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

#define SHADOW_PASS_1V		"ShadowPass1V"
#define SHADOW_PASS_1P		"ShadowPass1P"
#define SHADOW_PASS_1PT		"ShadowPass1PT"
#define SHADOW_PASS_2V		"ShadowPass2V"
#define SHADOW_PASS_2P		"ShadowPass2P"
#define WHITE_WASH_P		"WhiteWashP"
#define WHITE_WASH_P_ADD	"WhiteWashAddP"
#define SCREEN_QUAD_V		"ScreenQuadV"
#define LIGHT_MODULATE_P	"LightModulateP"
#define SIMPLE_P			"SimpleP"
#define VSM_BLUR_P			"VSMBlurP"

EffectHandler::EffectHandler(IrrlichtDevice* dev, const irr::core::dimension2du& screenRTTSize, const bool useVSMShadows, const bool useRoundSpotLights, const bool use32BitDepthBuffers,
	E_FILTER_TYPE FilterType, CShaderPreprocessor* shaderPreprocessor, CScreenQuad* screenQuad)
 : device(dev), smgr(dev->getSceneManager()), driver(dev->getVideoDriver()),
 ScreenRTTSize(screenRTTSize.getArea() == 0 ? dev->getVideoDriver()->getScreenSize() : screenRTTSize),
 ClearColour(0x0), shadowsUnsupported(false), depthMC(0), shadowMC(0),
 AmbientColour(0x0), use32BitDepth(use32BitDepthBuffers), useVSM(useVSMShadows), filterType(FilterType), ScreenQuad(screenQuad)
{
	Depth = EMT_SOLID;
	DepthT = EMT_SOLID;
	WhiteWash = EMT_SOLID;
	WhiteWashTRef = EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	WhiteWashTAdd = EMT_TRANSPARENT_ADD_COLOR;
	WhiteWashTAlpha = EMT_TRANSPARENT_ALPHA_CHANNEL;
	Simple = EMT_SOLID;
	Shadow = EMT_SOLID;
	VSMBlurH = EMT_SOLID;
	VSMBlurV = EMT_SOLID;

	//const bool tempTexFlagMipMaps = driver->getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
	//const bool tempTexFlag32 = driver->getTextureCreationFlag(ETCF_ALWAYS_32_BIT);
	//driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);
	//driver->setTextureCreationFlag(ETCF_ALWAYS_32_BIT, true);

	screenRTT[0] = driver->addRenderTargetTexture(ScreenRTTSize);
	screenRTT[1] = driver->addRenderTargetTexture(ScreenRTTSize);

	//driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, tempTexFlagMipMaps);
	//driver->setTextureCreationFlag(ETCF_ALWAYS_32_BIT, tempTexFlag32);

	//E_SHADER_EXTENSION shaderExt = (driver->getDriverType() == EDT_DIRECT3D9) ? ESE_HLSL : ESE_GLSL;
	const io::path shaderExt = (driver->getDriverType() != EDT_OPENGL) ? ".hlsl" : ".glsl";

	const bool neededFeatures = (driver->getDriverType() != EDT_OPENGL ? driver->queryFeature(EVDF_PIXEL_SHADER_2_0) : driver->queryFeature(EVDF_ARB_GLSL));
	if (shaderPreprocessor && neededFeatures)
	{
		// Ajouté : Désactive le filtre des ombres si les pixels shaders 3.0 ne sont pas supportés
		const bool newPixelModel = driver->queryFeature(video::EVDF_PIXEL_SHADER_3_0);
		if (!newPixelModel && filterType != EFT_NONE)
		{
			gameConfig.xEffectsFilterType = EFT_NONE;
			filterType = EFT_NONE;

			LOG("XEffects: Pixel shader model 3 is not avalaible for this driver : Shadows filtering is disabled.", ELL_WARNING);
		}

		// Met à jour le header de XEffects du préprocesseur de shaders
		shaderPreprocessor->generateXEffectsShadersHeader();

		depthMC = new DepthShaderCB();
		shadowMC = new ShadowShaderCB(FilterType);

		io::path tmpVS = io::path(SHADOW_PASS_1V) + shaderExt;
		io::path tmpPS = io::path(SHADOW_PASS_1P) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\".", ELL_INFORMATION);
		Depth = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_SOLID,
			0, true, false);

		tmpPS = io::path(SHADOW_PASS_1PT) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\".", ELL_INFORMATION);
		DepthT = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF,
			0, true, false);

		tmpPS = io::path(WHITE_WASH_P) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (1 / 3).", ELL_INFORMATION);
		WhiteWash = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_SOLID,
			0, true, false);

		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (2 / 3).", ELL_INFORMATION);
		WhiteWashTRef = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF,
			0, true, false);

		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (3 / 3).", ELL_INFORMATION);
		WhiteWashTAlpha = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_TRANSPARENT_ALPHA_CHANNEL,
			0, true, false);

		tmpPS = io::path(WHITE_WASH_P_ADD) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\".", ELL_INFORMATION);
		WhiteWashTAdd = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", video::EVST_VS_2_0,
			tmpPS.c_str(), "pixelMain", video::EPST_PS_2_0,
			depthMC, video::EMT_TRANSPARENT_ALPHA_CHANNEL,
			0, true, false);

		tmpVS = io::path(SHADOW_PASS_2V) + shaderExt;
		tmpPS = io::path(SHADOW_PASS_2P) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (samples amount : " << (filterType == 0 ? 1 : filterType * 4) << ") .", ELL_INFORMATION);
		Shadow = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", EVST_VS_1_1,
			tmpPS.c_str(), "pixelMain", newPixelModel ? EPST_PS_3_0 : EPST_PS_2_0,
			shadowMC, video::EMT_SOLID,
			0, true, false);

		// Create screen quad shader callback.
		ScreenQuadCB* const SQCB = new ScreenQuadCB(this);

		// Light modulate.
		tmpVS = io::path(SCREEN_QUAD_V) + shaderExt;
		tmpPS = io::path(LIGHT_MODULATE_P) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\".", ELL_INFORMATION);
		LightModulate = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", EVST_VS_1_1,
			tmpPS.c_str(), "pixelMain", EPST_PS_2_0,
			SQCB, video::EMT_SOLID,
			0, true, false);

		// Simple present.
		tmpPS = io::path(SIMPLE_P) + shaderExt;
		LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\".", ELL_INFORMATION);
		Simple = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
			tmpVS.c_str(), "vertexMain", EVST_VS_1_1,
			tmpPS.c_str(), "pixelMain", EPST_PS_1_1,
			SQCB, video::EMT_SOLID,
			0, true, false);

		// VSM blur.
		if (useVSMShadows)
		{
			tmpPS = io::path(VSM_BLUR_P) + shaderExt;
			LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (1 / 2 : horizontal blur).", ELL_INFORMATION);
			VSMBlurH = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
				tmpVS.c_str(), "vertexMain", EVST_VS_1_1,
				tmpPS.c_str(), "pixelMain", EPST_PS_2_0,
				SQCB, video::EMT_SOLID,
				0, true, false);

			LOG("XEffects: Compiling vertex shader \"" << tmpVS.c_str() << "\" and pixel shader \"" << tmpPS.c_str() << "\" (2 / 2 : vertical blur).", ELL_INFORMATION);
			VSMBlurV = shaderPreprocessor->addHighLevelShaderMaterialFromFiles(
				tmpVS.c_str(), "vertexMain", EVST_VS_1_1,
				tmpPS.c_str(), "pixelMain", EPST_PS_2_0,
				SQCB, video::EMT_SOLID,
				0, true, true);
		}

		// Drop the screen quad callback.
		SQCB->drop();
	}
	else
	{
		LOG("XEffects: Shader effects not supported on this system.", ELL_WARNING);
		shadowsUnsupported = true;
	}
}
EffectHandler::~EffectHandler()
{
	if (screenRTT[0])
		driver->removeTexture(screenRTT[0]);

	if (screenRTT[1])
		driver->removeTexture(screenRTT[1]);

	// Ajouté :
	if (depthMC)	depthMC->drop();
	if (shadowMC)	shadowMC->drop();
}
void EffectHandler::setScreenRenderTargetResolution(const irr::core::dimension2du& resolution)
{
	//const bool tempTexFlagMipMaps = driver->getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
	//const bool tempTexFlag32 = driver->getTextureCreationFlag(ETCF_ALWAYS_32_BIT);
	//driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);
	//driver->setTextureCreationFlag(ETCF_ALWAYS_32_BIT, true);

	if (screenRTT[0])
		driver->removeTexture(screenRTT[0]);
	screenRTT[0] = driver->addRenderTargetTexture(resolution);

	if (screenRTT[1])
		driver->removeTexture(screenRTT[1]);
	screenRTT[1] = driver->addRenderTargetTexture(resolution);

	//driver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, tempTexFlagMipMaps);
	//driver->setTextureCreationFlag(ETCF_ALWAYS_32_BIT, tempTexFlag32);

	ScreenRTTSize = resolution;
}
void EffectHandler::addShadowToNode(irr::scene::ISceneNode *node, E_SHADOW_MODE shadowMode)
{
	// Ajouté :
	if (!node)	return;

	SShadowNode snode = {node, shadowMode};
	ShadowNodeArray.push_back(snode);
}
void EffectHandler::update(irr::video::ITexture* outputTarget)
{
	if (shadowsUnsupported)
		return;

	ICameraSceneNode* const activeCam = smgr->getActiveCamera();
	if (!activeCam)
		return;

	// Pass 1 : Rendu réel de la scène avec les lumières d'Irrlicht désactivées
	// TODO : Pour le bon fonctionnement de XEffects, il faut activer les lumières d'Irrlicht ici !
	driver->setRenderTarget(screenRTT[0], true, true, ClearColour);
	smgr->drawAll();

#ifdef _DEBUG
	// NumPad 1 : Rendu réel de la scène (éclairages désactivés)
	if (game->keys[KEY_NUMPAD1])
	{
		screenQuadMat.setTexture(0, screenRTT[0]);
		screenQuadMat.MaterialType = (E_MATERIAL_TYPE)Simple;
		driver->setRenderTarget(outputTarget, false, false, SColor(0x0));
		ScreenQuad->fullRender(screenQuadMat);
		return;
	}
#endif



	// Pass 2 : Rendu des objets suivant le point de vue de chaque lumière : Stockage de la distance la plus courte de la lumière au premier objet

	// Ajouté : Désactive le brouillard
	video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
	overrideMat.EnableFlags |= video::EMF_FOG_ENABLE;
	overrideMat.Material.FogEnable = false;

	const u32 LightListSize = LightList.size();
	const u32 ShadowNodeArraySize = ShadowNodeArray.size();
	if (ShadowNodeArraySize || LightListSize)
	{
		//driver->setRenderTarget(screenRTT[1], true, true, AmbientColour);
		shadowMC->AmbientColour = AmbientColour;

		activeCam->render();

		for (u32 l = 0; l < LightListSize; ++l)
		{
			// Set max distance constant for depth shader.
			depthMC->FarLink = LightList[l].getFarValue();

			driver->setTransform(ETS_VIEW, LightList[l].getViewMatrix());
			driver->setTransform(ETS_PROJECTION, LightList[l].getProjectionMatrix());

			ITexture* const currentShadowMapTexture = getShadowMapTexture(LightList[l].getShadowMapResolution());
			if (currentShadowMapTexture)
			{
				driver->setRenderTarget(currentShadowMapTexture, true, true, SColor(0xffffffff));

				for (u32 i = 0; i < ShadowNodeArraySize; ++i)
				{
					if (ShadowNodeArray[i].shadowMode == ESM_CAST || ShadowNodeArray[i].shadowMode == ESM_BOTH)
					{
						scene::ISceneNode* const shadowNode = ShadowNodeArray[i].node;
						if (!shadowNode->isVisible())
							continue;

						SNodeMaterialBuffer matBuffer(shadowNode);
						matBuffer.changeNodeMaterialType_1((E_MATERIAL_TYPE)Depth, (E_MATERIAL_TYPE)DepthT);

						matBuffer.renderNodeAndChildren();

						matBuffer.restoreMaterialType();
					}
				}

				// Blur the shadow map texture if we're using VSM filtering.
				if (useVSM)
				{
					ITexture* const currentSecondaryShadowMap = getShadowMapTexture(LightList[l].getShadowMapResolution(), true);

					screenQuadMat.setTexture(0, currentShadowMapTexture);
					screenQuadMat.MaterialType = (E_MATERIAL_TYPE)VSMBlurH;
					driver->setRenderTarget(currentSecondaryShadowMap, true, true, SColor(0xffffffff));
					ScreenQuad->preRender();	// Prépare le driver pour les rendus du screen quad que demandera PostProcess
					driver->setMaterial(screenQuadMat);
					ScreenQuad->render();

					screenQuadMat.setTexture(0, currentSecondaryShadowMap);
					screenQuadMat.MaterialType = (E_MATERIAL_TYPE)VSMBlurV;
					driver->setRenderTarget(currentShadowMapTexture, true, true, SColor(0xffffffff));
					driver->setMaterial(screenQuadMat);
					ScreenQuad->render();
					ScreenQuad->postRender();	// Termine les rendus du screen quad
				}
			}

#ifdef _DEBUG
			// NumPad 2 : Vue de la dernière shadow map (distances par rapport à la lampe)
			if (game->keys[KEY_NUMPAD2])
			{
				screenQuadMat.setTexture(0, currentShadowMapTexture);
				screenQuadMat.MaterialType = (E_MATERIAL_TYPE)Simple;
				driver->setRenderTarget(outputTarget, false, false, SColor(0x0));
				ScreenQuad->fullRender(screenQuadMat);
				return;
			}
#endif



			// Pass 3 : Calcul des ombres depuis la vue de la caméra

			//driver->setRenderTarget(screenRTT[0], true, true, SColor(0xffffffff));
			driver->setRenderTarget(screenRTT[1], true, true, SColor(0xffffffff));

			activeCam->render();

			shadowMC->LightColour = LightList[l].getLightColor();
			shadowMC->LightDir = LightList[l].getDirectionOpp();
			shadowMC->FarLink = LightList[l].getFarValue();
			shadowMC->ViewLink = LightList[l].getViewMatrix();
			shadowMC->ProjLink = LightList[l].getProjectionMatrix();
			if (shadowMC->sendMapResInv)
				shadowMC->MapResInv = 1.0f / (f32)LightList[l].getShadowMapResolution();

			for (u32 i = 0; i < ShadowNodeArraySize; ++i)
			{
				if (ShadowNodeArray[i].shadowMode == ESM_RECEIVE || ShadowNodeArray[i].shadowMode == ESM_BOTH)
				{
					scene::ISceneNode* const shadowNode = ShadowNodeArray[i].node;
					if (!shadowNode->isVisible())
						continue;

					SNodeMaterialBuffer matBuffer(shadowNode);
					matBuffer.changeNodeMaterialTypeAndTexture0((E_MATERIAL_TYPE)Shadow, currentShadowMapTexture);

					matBuffer.renderNodeAndChildren();

					matBuffer.restoreMaterialTypeAndTexture0();
				}
			}
		}

		// Render all the excluded and casting-only nodes.
		for (u32 i = 0; i < ShadowNodeArraySize; ++i)
		{
			if (ShadowNodeArray[i].shadowMode == ESM_CAST || ShadowNodeArray[i].shadowMode == ESM_EXCLUDE)
			{
				scene::ISceneNode* const shadowNode = ShadowNodeArray[i].node;
				if (!shadowNode->isVisible())
					continue;

				SNodeMaterialBuffer matBuffer(shadowNode);
				matBuffer.changeNodeMaterialType_2((E_MATERIAL_TYPE)WhiteWash, (E_MATERIAL_TYPE)WhiteWashTRef, (E_MATERIAL_TYPE)WhiteWashTAdd, (E_MATERIAL_TYPE)WhiteWashTAlpha);

				matBuffer.renderNodeAndChildren();

				matBuffer.restoreMaterialType();
			}
		}

#ifdef _DEBUG
		// NumPad 3 : Vue de la light map (multiplicateur d'assombrissement dans la vue de la caméra)
		if (game->keys[KEY_NUMPAD3])
		{
			screenQuadMat.setTexture(0, screenRTT[1]);
			screenQuadMat.MaterialType = (E_MATERIAL_TYPE)Simple;
			driver->setRenderTarget(outputTarget, false, false, SColor(0x0));
			ScreenQuad->fullRender(screenQuadMat);
			return;
		}
#endif
	}
	else
		driver->setRenderTarget(screenRTT[0], true, true, SColor(0xffffffff));




	// Pass 4 : Rendu final de l'éclairage par multiplication entre l'assombrissement dû aux ombres et le rendu normal de la scène
	screenQuadMat.setTexture(0, screenRTT[0]);
	screenQuadMat.setTexture(1, screenRTT[1]);
	screenQuadMat.MaterialType = (E_MATERIAL_TYPE)LightModulate;
	driver->setRenderTarget(outputTarget, false, false, SColor(0x0));
	ScreenQuad->fullRender(screenQuadMat);

	// Ajouté : Restaure le brouillard
	overrideMat.EnableFlags &= ~video::EMF_FOG_ENABLE;
}
irr::video::ITexture* EffectHandler::getShadowMapTexture(const irr::u32 resolution, const bool secondary)
{
	// Using Irrlicht cache now.
	core::stringc shadowMapName = core::stringc("XEFFECTS_SM_") + core::stringc(resolution);
	if(secondary)
		shadowMapName += "_2";

	// Modifié : Contournement du problème du warning non désiré : on modifie le niveau de log à ELL_NONE, puis on le rétablit :
	ILogger* const logger = device->getLogger();
	const ELOG_LEVEL lastLogLevel = logger->getLogLevel();
	logger->setLogLevel(ELL_NONE);
	ITexture* shadowMapTexture = driver->getTexture(shadowMapName);
	logger->setLogLevel(lastLogLevel);

	if (shadowMapTexture)
		return shadowMapTexture;
	//LOG("XEffects: Please ignore previous warning, it is harmless.", ELL_WARNING);

	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, use32BitDepth ? ECF_G32R32F : ECF_G16R16F);
	if (shadowMapTexture)
		return shadowMapTexture;



	// Ajouté : Essaie différents formats de texture lorsque driver->addRenderTargetTexture retourne NULL (arrive fréquemment en mode OpenGL) :

	LOG("XEffects: Cannot get shadow map texture (" << shadowMapName.c_str() << ") (32 bits depth " << (use32BitDepth ? "enabled" : "disabled") << ") :" << endl
		<< "          -> Trying to create it using color format : " << (use32BitDepth ? "ECF_G32R32F." : "ECF_G16R16F.") << endl
		<< "             => Failure !" << endl
		<< "          -> Trying to create it using color format : " << (use32BitDepth ? "ECF_A32B32G32R32F." : "ECF_A16B16G16R16F."), ELL_WARNING);
	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, use32BitDepth ? ECF_A32B32G32R32F : ECF_A16B16G16R16F);
	if (shadowMapTexture)
	{
		LOG("             => Success !", ELL_WARNING);
		return shadowMapTexture;
	}

	if (use32BitDepth)	// Essaie en mode 16 bits
	{
		LOG("             => Failure !" << endl
			<< "          -> Trying to create it using color format : ECF_G16R16F.", ELL_WARNING);
		shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_G16R16F);
		if (shadowMapTexture)
		{
			LOG("             => Success !", ELL_WARNING);
			return shadowMapTexture;
		}

		LOG("             => Failure !" << endl
			<< "          -> Trying to create it using color format : ECF_A16B16G16R16F.", ELL_WARNING);
		shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_A16B16G16R16F);
		if (shadowMapTexture)
		{
			LOG("             => Success !", ELL_WARNING);
			return shadowMapTexture;
		}
	}
	else				// Essaie en mode 32 bits
	{
		LOG("             => Failure !" << endl
			<< "          -> Trying to create it using color format : ECF_G32R32F.", ELL_WARNING);
		shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_G32R32F);
		if (shadowMapTexture)
		{
			LOG("             => Success !", ELL_WARNING);
			return shadowMapTexture;
		}

		LOG("             => Failure !" << endl
			<< "          -> Trying to create it using color format : ECF_A32B32G32R32F.", ELL_WARNING);
		shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_A32B32G32R32F);
		if (shadowMapTexture)
		{
			LOG("             => Success !", ELL_WARNING);
			return shadowMapTexture;
		}
	}

	LOG("             => Failure !" << endl
		<< "          -> Trying to create it using color format : ECF_R8G8B8.", ELL_WARNING);
	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_R8G8B8);
	if (shadowMapTexture)
	{
		LOG("             => Success !", ELL_WARNING);
		return shadowMapTexture;
	}

	LOG("             => Failure !" << endl
		<< "          -> Trying to create it using color format : ECF_A8R8G8B8.", ELL_WARNING);
	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_A8R8G8B8);
	if (shadowMapTexture)
	{
		LOG("             => Success !", ELL_WARNING);
		return shadowMapTexture;
	}

	LOG("             => Failure !" << endl
		<< "          -> Trying to create it using color format : ECF_R5G6B5.", ELL_WARNING);
	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_R5G6B5);
	if (shadowMapTexture)
	{
		LOG("             => Success !", ELL_WARNING);
		return shadowMapTexture;
	}

	LOG("             => Failure !" << endl
		<< "          -> Trying to create it using color format : ECF_A1R5G5B5.", ELL_WARNING);
	shadowMapTexture = driver->addRenderTargetTexture(dimension2du(resolution, resolution), shadowMapName, ECF_A1R5G5B5);
	if (shadowMapTexture)
	{
		LOG("             => Success !", ELL_WARNING);
	}
	else
	{
		LOG("             => Failure !", ELL_WARNING);
	}

	return shadowMapTexture;
}

// Copyright (C) 2007-2009 Ahmed Hilali
