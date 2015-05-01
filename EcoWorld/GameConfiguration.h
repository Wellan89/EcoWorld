#ifndef DEF_GAME_CONFIGURATION
#define DEF_GAME_CONFIGURATION

#include "global.h"
#include "XEffects/EffectHandler.h"			// Pour les param�tres de XEffects

//#ifdef USE_IRRKLANG
#include <ik_ESoundOutputDrivers.h>	// Pour les types de driver audio d'IrrKlang
//#endif



// Classe pour stocker et comparer des informations sur un niveau gamma
// Note : Pour que le device puisse changer le niveau gamma d'apr�s les niveaux RGB, il faut que la luminosit� (Brightness) et le contraste (Contrast) soient � 0.0f
struct GammaInfos
{
	// Valeurs du niveau gamma
	float red;
	float green;
	float blue;
	float brightness;
	float contrast;

	// Op�rateur d'assignemet
	GammaInfos& operator=(const GammaInfos& other)
	{
		red = other.red;
		green = other.green;
		blue = other.blue;
		brightness = other.brightness;
		contrast = other.contrast;

		return *this;
	}

	// Op�rateurs d'�galit�  et de comparaison
#define GAMMA_TOLERANCE	0.08f	// Le niveau de la tol�rance pour le niveau gamma, car les valeurs retourn�es par le device ne sont pas constantes, mais tout de m�me assez proches de la r�alit�
#ifdef GAMMA_TOLERANCE
	bool operator==(const GammaInfos& other) const
	{
		if (!core::equals(red, other.red, GAMMA_TOLERANCE))					return false;
		if (!core::equals(green, other.green, GAMMA_TOLERANCE))				return false;
		if (!core::equals(blue, other.blue, GAMMA_TOLERANCE))				return false;
		if (!core::equals(brightness, other.brightness, GAMMA_TOLERANCE))	return false;
		if (!core::equals(contrast, other.contrast, GAMMA_TOLERANCE))		return false;

		return true;
	}
#else	// Comparaison exacte des valeurs (non recommand� car les valeurs retourn�es par le device ne sont pas exactes !)
	bool operator==(const GammaInfos& other) const
	{
		if (red != other.red)				return false;
		if (green != other.green)			return false;
		if (blue != other.blue)				return false;
		if (brightness != other.brightness)	return false;
		if (contrast != other.contrast)		return false;
		return true;
#endif
	bool operator!=(const GammaInfos& other) const	{ return (!((*this) == other)); }

	// Constructeurs
	GammaInfos() : red(1.0f), green(1.0f), blue(1.0f), brightness(0.0f), contrast(0.0f)	{ }
	GammaInfos(float Red, float Green, float Blue, float Brightness = 0.0f, float Contrast = 0.0f)
		: red(Red), green(Green), blue(Blue), brightness(Brightness), contrast(Contrast)	{ }
	// Constructeur de copie
	GammaInfos(const GammaInfos& other)
	{
		(*this) = other;
	}
};

// Classe pour g�rer la configuration et les options du jeu
class GameConfiguration
{
public:
	// Enum�ration d�finissant la qualit� des textures utilis�es
	enum E_TEXTURE_QUALITY
	{
		ETQ_VERY_LOW,
		ETQ_LOW,
		ETQ_MEDIUM,
		ETQ_HIGH
	};
	
	// Enum�ration contenant les diff�rents packs d'icones et logos contenus dans le jeu
	enum E_MAIN_ICONS_SET
	{
		EMIS_STANDART,	// Pack d'ic�nes standard, cr�� par Vincent GENTY
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
		EMIS_NEXT_GEN,	// Pack d'ic�nes next-gen, cr��s par Romain DARCEL
#endif
	};

	// Enum�ration contenant les diff�rents packs de musiques contenus dans le jeu
	enum E_GAME_MUSICS_SET
	{
		EGMS_MEDIEVAL,		// Musiques m�di�vales prises d'Anno 1404
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
		EGMS_FUTURISTIC,	// Musiques futuristes fournies par Romain DARCEL
#endif
	};



	// Constructeur par d�faut
	GameConfiguration();

	// Remet les valeurs par d�faut
	// keepAudioConfig :	Si true, les param�tres audio seront conserv�es (concerne seulement les param�tres suivants : audioEnabled, mainVolume, musicVolume, soundVolume)
	void resetDefaultValues(bool keepAudioConfig = false);

	// Enregistre la configuration dans un fichier
	void save(const io::path& adresse, io::IFileSystem* fileSystem) const;

	// Charge la configuration depuis un fichier
	void load(const io::path& adresse, io::IFileSystem* fileSystem);



	// Attributs :

	// Param�tres de cr�ation du device
	SIrrlichtCreationParameters deviceParams;

	// Param�tres graphiques
	bool deviceWindowResizable;
	bool drawWireframe;
	bool drawPointCloud;
	bool bilinearFilterEnabled;
	bool trilinearFilterEnabled;
	u8 anisotropicFilter;
	u8 antialiasingMode;
	bool usePowerOf2Textures;
	E_TEXTURE_QUALITY texturesQuality;
	float weatherVisibilityFactor;

	// Param�tres des shaders et des ombres d'Irrlicht
	bool shadersEnabled;
	bool waterShaderEnabled;
	core::dimension2du waterShaderRenderTargetSize;
	bool animatedWater;
	bool skyDomeShadersEnabled;
	bool normalMappingEnabled;
	bool terrainsShadersEnabled;
	bool stencilShadowsEnabled;

	// Param�tres des effets post-rendu
	bool usePostProcessEffects;
	bool postProcessShakeCameraOnDestroying;
	bool postProcessUseDepthRendering;
	video::SColor postProcessDefaultDepth;
	core::array<core::stringw> postProcessEffects;

	// Param�tres des ombres XEffects
	bool useXEffectsShadows;
	core::dimension2du xEffectsScreenRTTSize;
	bool xEffectsUseVSMShadows;
	bool xEffectsUseRoundSpotlights;
	bool xEffectsUse32BitDepthBuffers;
	E_FILTER_TYPE xEffectsFilterType;
	u32 xEffectsShadowMapResolution;

	// Param�tres de l'interface
	int guiTransparency;
	io::path guiSkinFile;
	E_MAIN_ICONS_SET mainIconsSet;

	// Param�tres vid�o Gamma
	GammaInfos gamma;

	// Param�tres du jeu
	u32 autoSaveFrequency;

	// Param�tres audio et d'IrrKlang
	bool audioEnabled;
	float mainVolume;
	float musicVolume;
	float soundVolume;
	irrklang::E_SOUND_OUTPUT_DRIVER irrKlangDriver;
	int irrKlangOptions;
	core::stringc irrKlangDeviceID;
	u32 gameMusicsSet;	// Les diff�rents packs de musique sont cod�s sous forme de u32 pour permettre au joueur d'ajouter simplement ses propres packs de musique au jeu

	// Op�rateur d'assignement
	GameConfiguration& operator=(const GameConfiguration& other)
	{
		// Param�tres de cr�ation du device
		deviceParams = other.deviceParams;

		// Param�tres graphiques
		deviceWindowResizable = other.deviceWindowResizable;
		drawWireframe = other.drawWireframe;
		drawPointCloud = other.drawPointCloud;
		bilinearFilterEnabled = other.bilinearFilterEnabled;
		trilinearFilterEnabled = other.trilinearFilterEnabled;
		anisotropicFilter = other.anisotropicFilter;
		antialiasingMode = other.antialiasingMode;
		usePowerOf2Textures = other.usePowerOf2Textures;
		texturesQuality = other.texturesQuality;

		// Param�tres des shaders et des ombres d'Irrlicht
		shadersEnabled = other.shadersEnabled;
		waterShaderEnabled = other.waterShaderEnabled;
		waterShaderRenderTargetSize = other.waterShaderRenderTargetSize;
		animatedWater = other.animatedWater;
		skyDomeShadersEnabled = other.skyDomeShadersEnabled;
		normalMappingEnabled = other.normalMappingEnabled;
		terrainsShadersEnabled = other.terrainsShadersEnabled;
		stencilShadowsEnabled = other.stencilShadowsEnabled;

		// Param�tres des effets post-rendu
		usePostProcessEffects = other.usePostProcessEffects;
		postProcessShakeCameraOnDestroying = other.postProcessShakeCameraOnDestroying;
		postProcessUseDepthRendering = other.postProcessUseDepthRendering;
		postProcessDefaultDepth = other.postProcessDefaultDepth;
		postProcessEffects = other.postProcessEffects;

		// Param�tres des ombres XEffects
		useXEffectsShadows = other.useXEffectsShadows;
		xEffectsScreenRTTSize = other.xEffectsScreenRTTSize;
		xEffectsUseVSMShadows = other.xEffectsUseVSMShadows;
		xEffectsUseRoundSpotlights = other.xEffectsUseRoundSpotlights;
		xEffectsUse32BitDepthBuffers = other.xEffectsUse32BitDepthBuffers;
		xEffectsFilterType = other.xEffectsFilterType;
		xEffectsShadowMapResolution = other.xEffectsShadowMapResolution;

		// Param�tres de l'interface
		guiTransparency = other.guiTransparency;
		guiSkinFile = other.guiSkinFile;
		mainIconsSet = other.mainIconsSet;

		// Param�tres vid�o Gamma
		gamma = other.gamma;

		// Param�tres du jeu
		autoSaveFrequency = other.autoSaveFrequency;

		// Param�tres audio et d'IrrKlang
		audioEnabled = other.audioEnabled;
		mainVolume = other.mainVolume;
		musicVolume = other.musicVolume;
		soundVolume = other.soundVolume;
		irrKlangDriver = other.irrKlangDriver;
		irrKlangOptions = other.irrKlangOptions;
		irrKlangDeviceID = other.irrKlangDeviceID;
		gameMusicsSet = other.gameMusicsSet;

		return *this;
	}
	// Constructeur de copie
	GameConfiguration(const GameConfiguration& other)
	{
		(*this) = other;
	}
};

/*
Options r�glables :
- Le type de driver (DirectX, OpenGL...) (ComboBox)
- R�solution (ComboBox)
- Plein �cran (CheckBox)
- Antialiasing (ComboBox)
- Synchronisation Verticale (CheckBox)
- Ombres (CheckBox)
- Shaders (CheckBox)
- Animation de l'eau (CheckBox)
- Filtrage anisotropique (CheckBox)
- Taille des textures (ComboBox)
- Transparence de la GUI (ScrollBar)

- Valeur gamma (ScrollBar)

- Fr�quence des sauvegardes automatiques (ComboBox)

- Musique et sons (CheckBox)
- Volume g�n�ral (ScrollBar)
- Volume de la musique (ScrollBar)
- Volume des sons (ScrollBar)

- device->setWindowResizable(bool)

Non encore fait : (TODO)
- Niveau de d�tail des objets 3D (ComboBox)
*/

#endif
