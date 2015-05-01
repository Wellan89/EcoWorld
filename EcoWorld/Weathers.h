#ifndef DEF_WEATHERS
#define DEF_WEATHERS

#include "global.h"
#include "GameConfiguration.h"

// Enumération définissant les ID des temps disponibles dans le jeu
enum WeatherID
{
	WI_sunny,		// Ensoleillé
	WI_cloudy,		// Nuageux
	WI_foggy,		// Brouillard
	WI_raining,		// Pluie

	WI_night,		// Nuit

	WI_COUNT
};

class WeatherInfos
{
public:
	// TODO : Autres paramêtres du temps ajoutables :
	// ----------------------------------------------
	// - Vent
	// - Neige
	// - Soleil : Pouvoir changer sa taille ?
	// - Nuages (blending)
	// - Etoiles (blending)
	// - Soleil ou Lune réelle (blending d'une image plus réaliste, surtout pour la lune)
	// - Arc-en-Ciel

	video::SColor fogColor;			// La couleur du brouillard
	float fogStart;					// La position de départ du brouillard par rapport à la caméra
	float fogEnd;					// La position de fin du brouillard par rapport à la caméra

	video::SColorf ambientLight;	// La couleur et la puissance de la lumière ambiente
	video::SColorf sunLightColor;	// La couleur de la lumière directionnelle qui semble provenir du soleil

	video::SColor shadowColor;		// La couleur des ombres

	video::SColor sunColor;			// La couleur du billboard représentant la partie visible du soleil

	io::path skydomeTexturePath;	// L'extension et le suffixe du nom de la texture ne doivent pas être ajoutés
	float skydomeTexturePercentage;
	float skydomeTextureOffset;		// Texture décalée d'une fois vers la gauche : 1.0f <- 0.0f -> -1.0f : Texture décalée d'une fois vers la droite (vue de l'intérieur du skydome)

	float energyFactor;				// Le facteur d'énergie de ce temps sur les panneaux solaires

	float rainFlow;					// La densité de la pluie (le nombre de particules de pluie émises par secondes)

	WeatherID ID;					// L'ID de ce temps

	WeatherInfos(WeatherID weatherID)
	{
		setID(weatherID);
	}

	// Indique les informations sur ce temps d'après son ID
	void setID(WeatherID weatherID)
	{
		ID = weatherID;

		switch (weatherID)
		{
		case WI_sunny:
			fogColor.set(255, 255, 230, 200);
			fogStart = 500.0f;
			fogEnd = 5000.0f;

			ambientLight.set(0.5f, 0.45f, 0.4f);
			sunLightColor.set(1.0f, 0.9f, 0.8f);

			shadowColor.set(150, 0, 0, 0);

			sunColor.set(255, 255, 255, 100);

			skydomeTexturePath = "skydome_0";
			skydomeTexturePercentage = 1.8f;
			skydomeTextureOffset = 0.5f;

			energyFactor = 1.0f;

			rainFlow = 0.0f;
			break;

		case WI_cloudy:
			fogColor.set(255, 200, 200, 200);
			fogStart = 500.0f;
			fogEnd = 5000.0f;

			ambientLight.set(0.4f, 0.4f, 0.4f);
			sunLightColor.set(0.25f, 0.25f, 0.25f);

			shadowColor.set(30, 0, 0, 0);

			sunColor.set(255, 175, 175, 85);

			skydomeTexturePath = "skydome_1";
			skydomeTexturePercentage = 1.8f;
			skydomeTextureOffset = 0.4861f; // 0.5278f : Soleil à droite des nuages

			energyFactor = 0.7f;

			rainFlow = 0.0f;
			break;

		case WI_foggy:
			fogColor.set(255, 150, 150, 150);
			fogStart = -1000.0f;
			fogEnd = 2000.0f;

			ambientLight.set(0.5f, 0.5f, 0.5f);
			sunLightColor.set(0.15f, 0.15f, 0.15f);

			shadowColor.set(10, 0, 0, 0);

			sunColor.set(255, 125, 125, 60);

			skydomeTexturePath = "skydome_2";
			skydomeTexturePercentage = 1.8f;
			skydomeTextureOffset = 0.7806f;

			energyFactor = 0.6f;

			rainFlow = 0.0f;
			break;

		case WI_raining:
			fogColor.set(255, 100, 100, 100);
			fogStart = 0.0f;
			fogEnd = 3000.0f;

			ambientLight.set(0.15f, 0.15f, 0.15f);
			sunLightColor.set(0.1f, 0.15f, 0.2f);

			shadowColor.set(10, 0, 0, 0);

			sunColor.set(255, 75, 75, 35);

			skydomeTexturePath = "skydome_3";
			skydomeTexturePercentage = 1.46f;
			skydomeTextureOffset = 0.4528f;

			energyFactor = 0.4f;

			rainFlow = 1000.0f;
			break;

		case WI_night:
			fogColor.set(255, 40, 50, 70);
			fogStart = 0.0f;
			fogEnd = 3000.0f;

			ambientLight.set(0.1f, 0.15f, 0.2f);
			sunLightColor.set(0.2f, 0.25f, 0.3f);

			shadowColor.set(50, 0, 0, 0);

			sunColor.set(255, 150, 200, 220);

			skydomeTexturePath = "skydome_4";
			skydomeTexturePercentage = 1.74f;
			skydomeTextureOffset = 0.058f;

			energyFactor = 0.2f;

			rainFlow = 0.0f;
			break;

		default:
			LOG_DEBUG("WeatherInfos::setID(" << weatherID << ") : Type de temps inconnu : " << weatherID, ELL_WARNING);

			// Indique tout de même un temps par défaut, pour initialiser cette structure
			setID(WI_sunny);
			return;
		}

		// Applique les paramêtres de configuration s'appliquant aux temps et remet ses valeurs dans les limites
		applyGameConfigAndClamp();
	}

protected:
	// Applique les paramêtres de configuration s'appliquant aux temps (le facteur de visibilité des temps : weatherVisibilityFactor),
	// et vérifie que les valeurs appliquées aux temps sont bien dans les limites d'Irrlicht
	void applyGameConfigAndClamp()
	{
		// Applique les paramêtres de visibilité au temps actuel :
		// Certaines valeurs ne sont pas mises à jour avec le facteur de visibilité car cela n'améliorerait pas la visibilité du jeu, ou cela perturberait la cohérence interne des temps.

		//fogColor.setAlpha(core::clamp<u32>(core::round32(fogColor.getAlpha() * gameConfig.weatherVisibilityFactor), 0, 255));
		//fogColor.setRed(core::clamp<u32>(core::round32(fogColor.getRed() * gameConfig.weatherVisibilityFactor), 0, 255));
		//fogColor.setGreen(core::clamp<u32>(core::round32(fogColor.getGreen() * gameConfig.weatherVisibilityFactor), 0, 255));
		//fogColor.setBlue(core::clamp<u32>(core::round32(fogColor.getBlue() * gameConfig.weatherVisibilityFactor), 0, 255));

		if (fogStart >= 0.0f)
			fogStart *= gameConfig.weatherVisibilityFactor;
		else
			fogStart /= gameConfig.weatherVisibilityFactor;

		if (fogEnd >= 0.0f)
			fogEnd *= gameConfig.weatherVisibilityFactor;
		else
			fogEnd /= gameConfig.weatherVisibilityFactor;

		//ambientLight.a = core::clamp(ambientLight.a * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		ambientLight.r = core::clamp(ambientLight.r * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		ambientLight.g = core::clamp(ambientLight.g * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		ambientLight.b = core::clamp(ambientLight.b * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);

		//sunLightColor.a = core::clamp(sunLightColor.a * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		sunLightColor.r = core::clamp(sunLightColor.r * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		sunLightColor.g = core::clamp(sunLightColor.g * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);
		sunLightColor.b = core::clamp(sunLightColor.b * gameConfig.weatherVisibilityFactor, 0.0f, 1.0f);

		// Pour la couleur des ombres stencil d'Irrlicht, c'est sa composante de couleur alpha qui est surtout importante
		shadowColor.setAlpha(core::clamp<u32>(core::round32(shadowColor.getAlpha() / gameConfig.weatherVisibilityFactor), 0, 255));
		shadowColor.setRed(core::clamp<u32>(core::round32(shadowColor.getRed() / gameConfig.weatherVisibilityFactor), 0, 255));
		shadowColor.setGreen(core::clamp<u32>(core::round32(shadowColor.getGreen() / gameConfig.weatherVisibilityFactor), 0, 255));
		shadowColor.setBlue(core::clamp<u32>(core::round32(shadowColor.getBlue() / gameConfig.weatherVisibilityFactor), 0, 255));

		//sunColor.setAlpha(core::clamp<u32>(core::round32(sunColor.getAlpha() * gameConfig.weatherVisibilityFactor), 0, 255));
		//sunColor.setRed(core::clamp<u32>(core::round32(sunColor.getRed() * gameConfig.weatherVisibilityFactor), 0, 255));
		//sunColor.setGreen(core::clamp<u32>(core::round32(sunColor.getGreen() * gameConfig.weatherVisibilityFactor), 0, 255));
		//sunColor.setBlue(core::clamp<u32>(core::round32(sunColor.getBlue() * gameConfig.weatherVisibilityFactor), 0, 255));

		//rainFlow /= gameConfig.weatherVisibilityFactor;





		// Remet les valeurs du temps dans les limites :

		// Remet le brouillard dans les limites
		fogEnd = core::clamp(fogEnd, 2000.0f, CAMERA_FAR_VALUE);
		fogStart = core::clamp(fogStart, -CAMERA_FAR_VALUE, fogEnd - 100.0f);

		// Remet la densité de la pluie dans les limites
		rainFlow = core::clamp(rainFlow, 0.0f, 3000.0f);
	}
};

#endif
