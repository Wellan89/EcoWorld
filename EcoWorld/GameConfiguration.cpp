#include "GameConfiguration.h"
//#ifdef USE_IRRKLANG
#include <ik_ESoundEngineOptions.h>	// Pour les options de création du device audio d'IrrKlang
//#endif

GameConfiguration::GameConfiguration()
{
	resetDefaultValues();
}
void GameConfiguration::resetDefaultValues(bool keepAudioConfig)
{
	// Paramêtres de création du device
	deviceParams.DeviceType = EIDT_BEST;
	deviceParams.DriverType = video::EDT_OPENGL;
	deviceParams.WindowSize.set(800, 600);
	deviceParams.Bits = 16;
	deviceParams.ZBufferBits = 16;
	deviceParams.Fullscreen = false;
	deviceParams.Stencilbuffer = false;
	deviceParams.Vsync = false;
	deviceParams.AntiAlias = 0;
	deviceParams.WithAlphaChannel = false;
	deviceParams.Doublebuffer = true;
	deviceParams.IgnoreInput = false;
	deviceParams.Stereobuffer = false;
	deviceParams.HighPrecisionFPU = false;
	deviceParams.EventReceiver = NULL;
	deviceParams.WindowId = NULL;
#ifdef _DEBUG
	deviceParams.LoggingLevel = ELL_DEBUG;
#else
	deviceParams.LoggingLevel = ELL_WARNING;
#endif

	// Paramêtres graphiques
	deviceWindowResizable = false;
	drawWireframe = false;
	drawPointCloud = false;
	bilinearFilterEnabled = true;
	trilinearFilterEnabled = false;
	anisotropicFilter = 0;
	antialiasingMode = video::EAAM_SIMPLE | video::EAAM_LINE_SMOOTH;
	usePowerOf2Textures = false;
	texturesQuality = ETQ_LOW;
	weatherVisibilityFactor = 1.0f;

	// Paramêtres des shaders et des ombres d'Irrlicht
	shadersEnabled = false;
	waterShaderEnabled = false;
	waterShaderRenderTargetSize.set(256, 256);
	animatedWater = true;
	skyDomeShadersEnabled = false;
	normalMappingEnabled = false;
	terrainsShadersEnabled = false;
	stencilShadowsEnabled = false;

	// Paramêtres des effets post-rendu
	usePostProcessEffects = false;
	postProcessShakeCameraOnDestroying = false;
	postProcessUseDepthRendering = false;
	postProcessDefaultDepth.set(201391872);	// 201391872 = 0x0C00ff00 = Depth(12/255 = 0.047) et WorldNormal(0.0f, 1.0f, 0.0f) : Non flouté par l'effet de Depth of Field
	postProcessEffects.clear();

	// Paramêtres des ombres XEffects
	useXEffectsShadows = false;
	xEffectsScreenRTTSize.set(0, 0);
	xEffectsUseVSMShadows = false;
	xEffectsUseRoundSpotlights = false;
	xEffectsUse32BitDepthBuffers = false;
	xEffectsFilterType = EFT_NONE;
	xEffectsShadowMapResolution = 512;

	// Paramêtres de l'interface
	guiTransparency = 175;
	guiSkinFile = "GUI Skin EcoWorld Default.xml";
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
	mainIconsSet = EMIS_NEXT_GEN;
#else
	mainIconSet = EMIS_STANDART;
#endif

	// Paramêtres vidéo Gamma
	// Note : Pour que le device puisse changer le niveau gamma d'après les niveaux RGB, il faut que la luminosité (Brightness) et le contraste (Contrast) soient à 0.0f
	gamma.red = 1.0f;
	gamma.green = 1.0f;
	gamma.blue = 1.0f;
	gamma.brightness = 0.0f;
	gamma.contrast = 0.0f;

	// Paramêtres du jeu
	// Note : La fréquence des sauvegardes automatiques est en secondes
	autoSaveFrequency = 900;	// 15 minutes par défaut

	// Paramêtres audio et d'IrrKlang
	if (!keepAudioConfig)
	{
		audioEnabled = false;
		mainVolume = 1.0f;
		musicVolume = 0.7f;
		soundVolume = 1.0f;
	}
	irrKlangDriver = irrklang::ESOD_AUTO_DETECT;
	irrKlangOptions = irrklang::ESEO_MULTI_THREADED | irrklang::ESEO_USE_3D_BUFFERS;
	irrKlangDeviceID = "";
	gameMusicsSet = EGMS_MEDIEVAL;
}
void GameConfiguration::load(const io::path& adresse, io::IFileSystem* fileSystem)
{
	// On remet les valeurs par défaut avant le chargement des données
	resetDefaultValues();

	// Crée les données pour le fichier
	if (!fileSystem)
		return;

	io::IXMLReader* reader = fileSystem->createXMLReader(adresse);
	if (!reader)
		return;

	io::IAttributes* in = fileSystem->createEmptyAttributes(NULL);
	if (!in)
	{
		reader->drop();
		return;
	}

	// Paramêtres de création du device
	//reader->resetPosition();
	if (in->read(reader, false, L"DeviceCreationParameters"))
	{
		if (in->existsAttribute("DeviceType"))					deviceParams.DeviceType = (E_DEVICE_TYPE)in->getAttributeAsInt("DeviceType");
		if (in->existsAttribute("DriverType"))					deviceParams.DriverType = (video::E_DRIVER_TYPE)in->getAttributeAsInt("DriverType");
		if (in->existsAttribute("WindowSizeWidth"))				deviceParams.WindowSize.Width = (u32)abs(in->getAttributeAsInt("WindowSizeWidth"));
		if (in->existsAttribute("WindowSizeHeight"))			deviceParams.WindowSize.Height = (u32)abs(in->getAttributeAsInt("WindowSizeHeight"));
		if (in->existsAttribute("Bits"))						deviceParams.Bits = in->getAttributeAsInt("Bits");
		if (in->existsAttribute("ZBufferBits"))					deviceParams.ZBufferBits = in->getAttributeAsInt("ZBufferBits");
		if (in->existsAttribute("Fullscreen"))					deviceParams.Fullscreen = in->getAttributeAsBool("Fullscreen");
		if (in->existsAttribute("Stencilbuffer"))				deviceParams.Stencilbuffer = in->getAttributeAsBool("Stencilbuffer");
		if (in->existsAttribute("Vsync"))						deviceParams.Vsync = in->getAttributeAsBool("Vsync");
		if (in->existsAttribute("AntiAlias"))					deviceParams.AntiAlias = (u8)core::clamp(in->getAttributeAsInt("AntiAlias"), 0, 255);
		if (in->existsAttribute("WithAlphaChannel"))			deviceParams.WithAlphaChannel = in->getAttributeAsBool("WithAlphaChannel");
		if (in->existsAttribute("Doublebuffer"))				deviceParams.Doublebuffer = in->getAttributeAsBool("Doublebuffer");
		if (in->existsAttribute("IgnoreInput"))					deviceParams.IgnoreInput = in->getAttributeAsBool("IgnoreInput");
		if (in->existsAttribute("Stereobuffer"))				deviceParams.Stereobuffer = in->getAttributeAsBool("Stereobuffer");
		if (in->existsAttribute("HighPrecisionFPU"))			deviceParams.HighPrecisionFPU = in->getAttributeAsBool("HighPrecisionFPU");
		if (in->existsAttribute("WindowId"))					deviceParams.WindowId = in->getAttributeAsUserPointer("WindowId"); // On lit quand même cette valeur, même si elle n'a pas été enregistrée, car l'utilisateur avancé pourrait vouloir qu'Irrlicht s'exécute dans la fenêtre qu'il a crée
		if (in->existsAttribute("LoggingLevel"))				deviceParams.LoggingLevel = (ELOG_LEVEL)in->getAttributeAsInt("LoggingLevel");

		in->clear();
	}

	// Paramêtres graphiques
	reader->resetPosition();
	if (in->read(reader, false, L"Graphics"))
	{
		if (in->existsAttribute("deviceWindowResizable"))		deviceWindowResizable = in->getAttributeAsBool("deviceWindowResizable");
		if (in->existsAttribute("drawWireframe"))				drawWireframe = in->getAttributeAsBool("drawWireframe");
		if (in->existsAttribute("drawPointCloud"))				drawPointCloud = in->getAttributeAsBool("drawPointCloud");
		if (in->existsAttribute("bilinearFilterEnabled"))		bilinearFilterEnabled = in->getAttributeAsBool("bilinearFilterEnabled");
		if (in->existsAttribute("trilinearFilterEnabled"))		trilinearFilterEnabled = in->getAttributeAsBool("trilinearFilterEnabled");
		if (in->existsAttribute("anisotropicFilter"))			anisotropicFilter = (u8)core::clamp(in->getAttributeAsInt("anisotropicFilter"), 0, 255);
		if (in->existsAttribute("antialiasingMode"))			antialiasingMode = (u8)core::clamp(in->getAttributeAsInt("antialiasingMode"), 0, 255);
		if (in->existsAttribute("usePowerOf2Textures"))			usePowerOf2Textures = in->getAttributeAsBool("usePowerOf2Textures");
		if (in->existsAttribute("texturesQuality"))				texturesQuality = (E_TEXTURE_QUALITY)in->getAttributeAsInt("texturesQuality");
		if (in->existsAttribute("weatherVisibilityFactor"))		weatherVisibilityFactor = in->getAttributeAsFloat("weatherVisibilityFactor");

		in->clear();
	}

	// Paramêtres des shaders et des ombres d'Irrlicht
	reader->resetPosition();
	if (in->read(reader, false, L"Shaders"))
	{
		if (in->existsAttribute("shadersEnabled"))				shadersEnabled = in->getAttributeAsBool("shadersEnabled");
		if (in->existsAttribute("waterShaderEnabled"))			waterShaderEnabled = in->getAttributeAsBool("waterShaderEnabled");
		if (in->existsAttribute("waterShaderRenderTargetSize"))	waterShaderRenderTargetSize = in->getAttributeAsDimension2d("waterShaderRenderTargetSize");
		if (in->existsAttribute("animatedWater"))				animatedWater = in->getAttributeAsBool("animatedWater");
		if (in->existsAttribute("skyDomeShadersEnabled"))		skyDomeShadersEnabled = in->getAttributeAsBool("skyDomeShadersEnabled");
		if (in->existsAttribute("normalMappingEnabled"))		normalMappingEnabled = in->getAttributeAsBool("normalMappingEnabled");
		if (in->existsAttribute("terrainsShadersEnabled"))		terrainsShadersEnabled = in->getAttributeAsBool("terrainsShadersEnabled");
		if (in->existsAttribute("stencilShadowsEnabled"))		stencilShadowsEnabled = in->getAttributeAsBool("stencilShadowsEnabled");

		in->clear();
	}

	// Paramêtres des effets post-rendu
	reader->resetPosition();
	if (in->read(reader, false, L"PostProcess"))
	{
		if (in->existsAttribute("usePostProcessEffects"))				usePostProcessEffects = in->getAttributeAsBool("usePostProcessEffects");
		if (in->existsAttribute("postProcessShakeCameraOnDestroying"))	postProcessShakeCameraOnDestroying = in->getAttributeAsBool("postProcessShakeCameraOnDestroying");
		if (in->existsAttribute("postProcessUseDepthRendering"))		postProcessUseDepthRendering = in->getAttributeAsBool("postProcessUseDepthRendering");
		if (in->existsAttribute("postProcessDefaultDepth"))				postProcessDefaultDepth = in->getAttributeAsColor("postProcessDefaultDepth");

		if (in->existsAttribute("PostProcessEffectsCount"))
		{
			const u32 postProcessEffectsCount = (u32)max(in->getAttributeAsInt("PostProcessEffectsCount"), 0);
			if (postProcessEffectsCount > 0)
			{
				postProcessEffects.reallocate(postProcessEffectsCount);
				for (u32 i = 0; i < postProcessEffectsCount; ++i)
				{
					// Obtient le prochain effet et l'ajoute à la liste :
					sprintf_SS("PostProcessEffect%u", i);
					if (in->existsAttribute(text_SS))
						postProcessEffects.push_back(in->getAttributeAsStringW(text_SS));
				}	
			}
		}
	}

	// Paramêtres ombres XEffects
	reader->resetPosition();
	if (in->read(reader, false, L"XEffects"))
	{
		if (in->existsAttribute("useXEffectsShadows"))				useXEffectsShadows = in->getAttributeAsBool("useXEffectsShadows"); 
		if (in->existsAttribute("xEffectsScreenRTTSize"))			xEffectsScreenRTTSize = in->getAttributeAsDimension2d("xEffectsScreenRTTSize");
		if (in->existsAttribute("xEffectsUseVSMShadows"))			xEffectsUseVSMShadows = in->getAttributeAsBool("xEffectsUseVSMShadows");
		if (in->existsAttribute("xEffectsUseRoundSpotlights"))		xEffectsUseRoundSpotlights = in->getAttributeAsBool("xEffectsUseRoundSpotlights");
		if (in->existsAttribute("xEffectsUse32BitDepthBuffers"))	xEffectsUse32BitDepthBuffers = in->getAttributeAsBool("xEffectsUse32BitDepthBuffers");
		if (in->existsAttribute("xEffectsFilterType"))				xEffectsFilterType = (E_FILTER_TYPE)(core::clamp(in->getAttributeAsInt("xEffectsFilterType"), 0, 4));
		if (in->existsAttribute("xEffectsShadowMapResolution"))		xEffectsShadowMapResolution = (u32)abs(in->getAttributeAsInt("xEffectsShadowMapResolution"));

		in->clear();
	}

	// Paramêtres de l'interface
	reader->resetPosition();
	if (in->read(reader, false, L"GUI"))
	{
		if (in->existsAttribute("guiTransparency"))				guiTransparency = in->getAttributeAsInt("guiTransparency");
		if (in->existsAttribute("guiSkinFile"))					guiSkinFile = in->getAttributeAsString("guiSkinFile");
		if (in->existsAttribute("mainIconsSet"))				mainIconsSet = (E_MAIN_ICONS_SET)in->getAttributeAsInt("mainIconsSet");

		in->clear();
	}

	// Paramêtres vidéo Gamma
	reader->resetPosition();
	if (in->read(reader, false, L"GammaRamp"))
	{
		if (in->existsAttribute("red"))							gamma.red = in->getAttributeAsFloat("red");
		if (in->existsAttribute("green"))						gamma.green = in->getAttributeAsFloat("green");
		if (in->existsAttribute("blue"))						gamma.blue = in->getAttributeAsFloat("blue");
		if (in->existsAttribute("brightness"))					gamma.brightness = in->getAttributeAsFloat("brightness");
		if (in->existsAttribute("contrast"))					gamma.contrast = in->getAttributeAsFloat("contrast");

		in->clear();
	}

	// Paramêtres du jeu
	reader->resetPosition();
	if (in->read(reader, false, L"AutoSave"))
	{
		if (in->existsAttribute("autoSaveFrequency"))			autoSaveFrequency = (u32)abs(in->getAttributeAsInt("autoSaveFrequency"));

		in->clear();
	}

#ifdef USE_IRRKLANG
	// Paramêtres audio et d'IrrKlang
	reader->resetPosition();
	if (in->read(reader, false, L"Audio"))
	{
		if (in->existsAttribute("audioEnabled"))				audioEnabled = in->getAttributeAsBool("audioEnabled");
		if (in->existsAttribute("mainVolume"))					mainVolume = in->getAttributeAsFloat("mainVolume");
		if (in->existsAttribute("musicVolume"))					musicVolume = in->getAttributeAsFloat("musicVolume");
		if (in->existsAttribute("soundVolume"))					soundVolume = in->getAttributeAsFloat("soundVolume");
		if (in->existsAttribute("irrKlangDriver"))				irrKlangDriver = (irrklang::E_SOUND_OUTPUT_DRIVER)in->getAttributeAsInt("irrKlangDriver");
		if (in->existsAttribute("irrKlangOptions"))				irrKlangOptions = in->getAttributeAsInt("irrKlangOptions");
		if (in->existsAttribute("irrKlangDeviceID"))			irrKlangDeviceID = in->getAttributeAsString("irrKlangDeviceID");
		if (in->existsAttribute("gameMusicsSet"))				gameMusicsSet = abs(in->getAttributeAsInt("gameMusicsSet"));

		in->clear();
	}
#endif

	// Supprime les données pour le fichier
	in->drop();
	reader->drop();

	// Vérifie que les shaders sont activées pour pouvoir utiliser les shaders de l'eau, de normal mapping, des terrains, des effets post-rendu et de XEffects
	if (!shadersEnabled)
	{
		waterShaderEnabled = false;
		normalMappingEnabled = false;
		terrainsShadersEnabled = false;
		usePostProcessEffects = false;
		useXEffectsShadows = false;
	}
}
void GameConfiguration::save(const io::path& adresse, io::IFileSystem* fileSystem) const
{
	// Crée les données pour le fichier
	if (!fileSystem)
		return;

	io::IXMLWriter* writer = fileSystem->createXMLWriter(adresse);	// TODO : Bug d'Irrlicht ici lorsque le programme n'a pas les droits d'écriture à l'adresse spécifiée !
	if (!writer)
		return;

	io::IAttributes* out = fileSystem->createEmptyAttributes(NULL);
	if (!out)
	{
		writer->drop();
		return;
	}

	// Ecrit le header XML
	writer->writeXMLHeader();

	// Paramêtres de création du device
	out->addInt("DeviceType", (int)deviceParams.DeviceType);
	out->addInt("DriverType", (int)deviceParams.DriverType);
	out->addInt("WindowSizeWidth", deviceParams.WindowSize.Width);
	out->addInt("WindowSizeHeight", deviceParams.WindowSize.Height);
	out->addInt("Bits", deviceParams.Bits);
	out->addInt("ZBufferBits", deviceParams.ZBufferBits);
	out->addBool("Fullscreen", deviceParams.Fullscreen);
	out->addBool("Stencilbuffer", deviceParams.Stencilbuffer);
	out->addBool("Vsync", deviceParams.Vsync);
	out->addInt("AntiAlias", deviceParams.AntiAlias);
	out->addBool("WithAlphaChannel", deviceParams.WithAlphaChannel);
	out->addBool("Doublebuffer", deviceParams.Doublebuffer);
	out->addBool("IgnoreInput", deviceParams.IgnoreInput);
	out->addBool("Stereobuffer", deviceParams.Stereobuffer);
	out->addBool("HighPrecisionFPU", deviceParams.HighPrecisionFPU);
	out->addUserPointer("WindowId", NULL); // deviceParams.WindowId // On n'enregistre pas ce paramêtre car l'ID d'une fenêtre n'est pas constant
	out->addInt("LoggingLevel", (int)deviceParams.LoggingLevel);

	out->write(writer, false, L"DeviceCreationParameters");
	out->clear();

	// Paramêtres graphiques
	out->addBool("deviceWindowResizable", deviceWindowResizable);
	out->addBool("drawWireframe", drawWireframe);
	out->addBool("drawPointCloud", drawPointCloud);
	out->addBool("bilinearFilterEnabled", bilinearFilterEnabled);
	out->addBool("trilinearFilterEnabled", trilinearFilterEnabled);
	out->addInt("anisotropicFilter", (int)anisotropicFilter);
	out->addInt("antialiasingMode", (int)antialiasingMode);
	out->addBool("usePowerOf2Textures", usePowerOf2Textures);
	out->addInt("texturesQuality", (int)texturesQuality);
	out->addFloat("weatherVisibilityFactor", weatherVisibilityFactor);

	out->write(writer, false, L"Graphics");
	out->clear();

	// Paramêtres des shaders et des ombres d'Irrlicht
	out->addBool("shadersEnabled", shadersEnabled);
	out->addBool("waterShaderEnabled", waterShaderEnabled);
	out->addDimension2d("waterShaderRenderTargetSize", waterShaderRenderTargetSize);
	out->addBool("animatedWater", animatedWater);
	out->addBool("skyDomeShadersEnabled", skyDomeShadersEnabled);
	out->addBool("normalMappingEnabled", normalMappingEnabled);
	out->addBool("terrainsShadersEnabled", terrainsShadersEnabled);
	out->addBool("stencilShadowsEnabled", stencilShadowsEnabled);

	out->write(writer, false, L"Shaders");
	out->clear();

	// Paramêtres des effets post-rendu
	out->addBool("usePostProcessEffects", usePostProcessEffects);
	out->addBool("postProcessShakeCameraOnDestroying", postProcessShakeCameraOnDestroying);
	out->addBool("postProcessUseDepthRendering", postProcessUseDepthRendering);
	out->addColor("postProcessDefaultDepth", postProcessDefaultDepth);
	{
		const u32 postProcessEffectsCount = postProcessEffects.size();
		out->addInt("PostProcessEffectsCount", postProcessEffectsCount);
		for (u32 i = 0; i < postProcessEffectsCount; ++i)
		{
			sprintf_SS("PostProcessEffect%u", i);
			out->addString(text_SS, postProcessEffects[i].c_str());
		}
	}

	out->write(writer, false, L"PostProcess");
	out->clear();

	// Paramêtres des ombres XEffects
	out->addBool("useXEffectsShadows", useXEffectsShadows);
	out->addDimension2d("xEffectsScreenRTTSize", xEffectsScreenRTTSize);
	out->addBool("xEffectsUseVSMShadows", xEffectsUseVSMShadows);
	out->addBool("xEffectsUseRoundSpotlights", xEffectsUseRoundSpotlights);
	out->addBool("xEffectsUse32BitDepthBuffers", xEffectsUse32BitDepthBuffers);
	out->addInt("xEffectsFilterType", (E_FILTER_TYPE)xEffectsFilterType);
	out->addInt("xEffectsShadowMapResolution", (int)xEffectsShadowMapResolution);

	out->write(writer, false, L"XEffects");
	out->clear();

	// Paramêtres de l'interface
	out->addInt("guiTransparency", guiTransparency);
	out->addString("guiSkinFile", guiSkinFile.c_str());
	out->addInt("mainIconsSet", (int)mainIconsSet);

	out->write(writer, false, L"GUI");
	out->clear();

	// Paramêtres vidéo Gamma
	out->addFloat("red", gamma.red);
	out->addFloat("green", gamma.green);
	out->addFloat("blue", gamma.blue);
	out->addFloat("brightness", gamma.brightness);
	out->addFloat("contrast", gamma.contrast);

	out->write(writer, false, L"GammaRamp");
	out->clear();

	// Paramêtres du jeu
	out->addInt("autoSaveFrequency", (int)autoSaveFrequency);

	out->write(writer, false, L"AutoSave");
	out->clear();

#ifdef USE_IRRKLANG
	// Paramêtres audio et d'IrrKlang
	out->addBool("audioEnabled", audioEnabled);
	out->addFloat("mainVolume", mainVolume);
	out->addFloat("musicVolume", musicVolume);
	out->addFloat("soundVolume", soundVolume);
	out->addInt("irrKlangDriver", (int)irrKlangDriver);
	out->addInt("irrKlangOptions", irrKlangOptions);
	out->addString("irrKlangDeviceID", irrKlangDeviceID.c_str());
	out->addInt("gameMusicsSet", (int)gameMusicsSet);

	out->write(writer, false, L"Audio");
	out->clear();
#endif

	// Supprime les données pour le fichier
	out->drop();
	writer->drop();
}
