#ifndef DEF_IRRKLANG_MANAGER
#define DEF_IRRKLANG_MANAGER

#include "global.h"

#ifdef USE_IRRKLANG

#include <irrklang.h>
#include "GameConfiguration.h"

using namespace irrklang;

class EcoWorldRenderer;
class EcoWorldTerrainManager;
class CIrrKlangFileFactory;
class CLoadingScreen;

/*
TODO : Sons manquants :
- Construction d'un bâtiment terminée
- Passage en caméra FPS
- Passage en mode Animation de la caméra RTS
*/

class IrrKlangManager : public ISoundStopEventReceiver
{
	friend class EcoWorldRenderer;			// Permet à la classe EcoWorldTerrainManager de déterminer si le son de la pluie est en train d'être joué ou non
	friend class EcoWorldTerrainManager;	// Permet à la classe EcoWorldTerrainManager de créer les sons de l'eau d'après les sons déjà préchargés dans cette classe

public:
	// Enum indiquant quel évènement du jeu nécessitant un son s'est produit
	enum E_GAME_SOUND_EVENT
	{
		EGSE_BATIMENT_CREATE_POSITIVE,		// Création d'un bâtiment acceptée
		EGSE_BATIMENT_CREATE_NEGATIVE,		// Création d'un bâtiment refusée
		EGSE_BATIMENT_DESTROY_POSITIVE,		// Destruction d'un bâtiment acceptée
		EGSE_BATIMENT_DESTROY_NEGATIVE,		// Destruction d'un bâtiment refusée
		EGSE_BATIMENT_DESTROYED,			// Bâtiment détruit (à la demande de l'utilisateur)
		EGSE_BATIMENT_DESTROYED_UNWANTED,	// Bâtiment détruit (à l'encontre de l'utilisateur : ex : le bâtiment a atteint sa fin de vie)
		EGSE_BATIMENT_SELECTED,				// Bâtiment sélectionné
		EGSE_BATIMENT_DESELECTED,			// Bâtiment déselectionné

		EGSE_GUI_MENU_OPEN,					// Son d'ouverture d'un menu de la GUI (permet de forcer ce son)
		EGSE_GUI_MENU_CLOSE,				// Son de fermeture d'un menu de la GUI (permet de forcer ce son)
	};

	// Les types de chaque son et musique pour les classer en catégories
	enum E_SOUND_TYPE
	{
		EST_UNKNOW,				// Inconnu : ne pas utiliser

		EST_music,				// Musique (pour le menu principal ou pour le jeu)

		EST_ui_sound,			// Son de l'interface utilisateur ou du jeu
		EST_sound,				// Son pour les bruitages
		EST_ambiantSound,		// Son d'ambiance
		EST_waterSound,			// Son relatif à l'eau
	};

	// Les noms de chaque son
	enum E_SOUND_NAME
	{
		ESN_ui_button_click,				// se_button_small_click.wav
		ESN_ui_tab_change,					// se_tab_click.wav
		ESN_ui_menu_open,					// se_open_menu.wav
		ESN_ui_menu_close,					// se_menu_close (b).wav
		ESN_ui_batiment_create_positive,	// se_place_building_positive.wav
		ESN_ui_batiment_create_negative,	// se_place_building_negative.wav
		ESN_ui_batiment_destroy_positive,	// se_place_building_positive (e).wav
		ESN_ui_batiment_destroyed,			// building_small_destroy.wav
		ESN_ui_batiment_destroyed_large,	// building_large_destroy.wav
		ESN_ui_batiment_selected,			// se_building_medium_select.wav
		ESN_ui_batiment_deselected,			// se_building_medium_deselect.wav

		ESN_ambiant_atmosphere,				// atmo_generic.wav
		ESN_ambiant_water,					// water_close_atmo.wav
		ESN_ambiant_rain,					// water_waterfall_medium.wav

		ESN_water_1,						// water_open_sea1.wav
		ESN_water_2,						// water_open_sea2.wav
		ESN_water_3,						// water_open_sea3.wav
		ESN_water_4,						// water_open_sea4.wav
		ESN_water_5,						// water_open_sea5.wav
		ESN_water_6,						// water_open_sea6.wav

		ESN_COUNT
	};

	// Constructeur et destructeur
	IrrKlangManager();
	~IrrKlangManager();

	// Initialise IrrKlang, et précharge les musiques et les sons
	// Si mainMenuLoading = true, seules les ressources nécéssaires au menu principal seront chargées
	void init(bool mainMenuLoading, CLoadingScreen* loadingScreen = NULL, float loadMin = 0, float loadMax = 100);

	// Joue les sons de la GUI ou du jeu
	void playGUISound(const SEvent& event);			// Suivant un évènement de la GUI
	void playGameSound(E_GAME_SOUND_EVENT event);	// Suivant un évènement de jeu personnalisé
	void playGameSound3D(E_GAME_SOUND_EVENT event, const core::vector3df& pos);	// Suivant un évènement de jeu personnalisé (en 3D)

	// Joue les musiques du menu principal
	void playMainMenuMusics(bool startPaused = false);

	// Joue les musiques du jeu
	void playGameMusics(bool startPaused = false);

	// Joue les sons d'ambiance
	void playAmbiantSounds(bool startPause = false);

	// Active/Désactive le son d'ambiance de la pluie
	void playRainSound(bool enabled = true);

	// Joue un certain son une seule fois en mode 2D
	void play2DSound(E_SOUND_NAME sound);

	// Joue un certain son une seule fois en mode 3D, à la position spécifiée
	void play3DSound(E_SOUND_NAME sound, const core::vector3df& pos);

	// Recalcule le volume des musiques avec le volume de la configuration du jeu
	void recalculateMusicsVolume();

	// Recalcule le volume des sons avec le volume de la configuration du jeu
	void recalculateSoundsVolume();

	// Appellé quand un son est stoppé
	void OnSoundStopped(ISound* sound, E_STOP_EVENT_CAUSE reason, void* userData);

	// Met à jour irrKlang
	void update(const core::vector3df& listenerPos, const core::vector3df& listenerLookAt, float elapsedTime);

	// Arrête tous les musiques et sons d'un certain type
	void stopSoundType(E_SOUND_TYPE soundType);

	// Arrête tous les musiques et sons sauf ceux d'un certain type
	void stopAllSoundsExceptType(E_SOUND_TYPE soundType);

	// Arrête tous les sons (et pas les musiques !) sauf ceux d'un certain type
	void stopAllSoundsOnlyExceptType(E_SOUND_TYPE soundType);

	// Classe permettant de gérer les sons d'Irrklang chargés et/ou en lecture
	class CIrrKlangSoundManipulator
	{
	public:
		E_SOUND_TYPE soundType;		// Le type du son (ou de la musique)
		float defaultVolume;		// Le volume par défaut du son (sans aucune préoccupation de la configuration audio, pour égaliser le volume de tous les sons)
		ISoundSource* soundSource;	// La source du son qui sera jouée (le volume de ce son est réglé avec la configuration audio)
		core::list<ISound*> sounds;	// La liste des pointeurs sur ce son actuellement joué (si vide : ce son n'est pas joué actuellement)

		// Constructeur par défaut et destructeur
		CIrrKlangSoundManipulator() : soundType(EST_UNKNOW), defaultVolume(1.0f), soundSource(0) { };
		~CIrrKlangSoundManipulator()
		{
			// Détruit tous les sons restants
			for (core::list<ISound*>::Iterator it = sounds.begin(); it != sounds.end(); ++it)
			{
				(*it)->drop();
				LOG_DEBUG("CIrrKlangSoundManipulator::~CIrrKlangSoundManipulator() : Son detruit automatiquement : soundType = " << soundType, ELL_WARNING);
			}
			sounds.clear();
		};
	};

	// Classe permettant de gérer l'effet de "fade in/fade out" des sons et musiques
	class CFadingSound
	{
	public:
		CIrrKlangSoundManipulator& fadingSound;	// Le son ou la musique concerné par cet effet de fading

		bool isFadingIn;		// True si le son est en train d'apparaître ("fade in"), false sinon ("fade out")
		float fadingSpeed;		// La vitesse de "fading" de ce son (en pourcentage de volume par seconde)

		float currentFading;	// L'atténuation de volume actuelle pour ce son pour l'effet de "fade in/fade out"

		CFadingSound(CIrrKlangSoundManipulator& fadeSound, bool fadeIn, float fadeSpeed)
			: fadingSound(fadeSound), isFadingIn(fadeIn), fadingSpeed(fadeSpeed), currentFading(fadeIn ? 0.0f : 1.0f)
		{ updateFadingEffect(0.0f); }

		// Met à jour l'effet de "fading" du son d'après le temps écoulé et recalcule son volume réel
		// Si elapsedTime = 0.0f, seul le volume réel du son sera recalculé
		void updateFadingEffect(float elapsedTime)
		{
			// Calcule l'effet de "fading" du son
			if (isFadingIn)
				currentFading += elapsedTime * fadingSpeed;
			else
				currentFading -= elapsedTime * fadingSpeed;

			// Replace l'effet de "fading" entre 0 et 1
			if (currentFading > 1.0f)
				currentFading = 1.0f;
			if (currentFading < 0.0f)
				currentFading = 0.0f;

			// Calcule le nouveau volume réel de ce son
			const float realSoundVolume = currentFading * fadingSound.defaultVolume * (fadingSound.soundType == EST_music ? gameConfig.musicVolume : gameConfig.soundVolume);

			// Modifie le volume par défaut de ce son
			// Désactivé : Les nouveaux sons créés avec cette source n'ont pas à avoir un volume diminué dès leur création, puisqu'ils ne subiront pas forcément un "fading"
			//if (fadingSound.soundSource)
			//	fadingSound.soundSource->setDefaultVolume(realSoundVolume);

			// Modifie le volume du son actuellement joué
			const core::list<ISound*>::ConstIterator END = fadingSound.sounds.end();
			for (core::list<ISound*>::ConstIterator it = fadingSound.sounds.begin(); it != END; ++it)
				(*it)->setVolume(realSoundVolume);
		}

		// Retourne si l'effet de "fading" de ce son est terminé
		bool isFadingEnd() const { return (isFadingIn ? currentFading >= 1.0f : currentFading <= 0.0f); }
	};

protected:
	// Le moteur sonore IrrKlang
	ISoundEngine* irrKlang;

#ifdef IRRKLANG_DELAYED_DLL_LOADING
	// Indique si la DLL d'IrrKlang a déjà été chargée (puisqu'elle est manuellement chargée en mode différé) :
	// True si les fonctions de la DLL irrKlang.dll ont déjà été chargées, false sinon
	bool irrKlangDllLoaded;
#endif

	// La liste des sons préchargés
	CIrrKlangSoundManipulator sounds[ESN_COUNT];

	// La liste de toutes les musiques préchargées d'après leur nom, qui seront pointées dans les maps mainMenuMusics et gameMusics
	core::map<core::stringc, CIrrKlangSoundManipulator> loadedMusics;

	// Les listes des musiques du menu principal suivant leur pack, gérées dans une map (une liste de toutes les musiques du menu principal, avec pour clé leur pack de musique)
	// Ces musiques sont recherchées pendant le chargement du menu principal, dans le FileSystem d'Irrlicht, et contiennent "_MainThemeX_" ou "_MTX_" dans leur nom de fichier où X est le numéro du pack de musique où elles appartiennent
	core::map<u32, core::array<CIrrKlangSoundManipulator*>> mainMenuMusics;

	// Les listes des musiques du jeu suivant leur pack, gérées dans une map (une liste de toutes les musiques du jeu, avec pour clé leur pack de musique)
	// Ces musiques sont recherchées pendant le chargement du jeu, dans le FileSystem d'Irrlicht, et contiennent "_MusicSetX_" ou "_MSX_" dans leur nom de fichier où X est le numéro du pack de musique où elles appartiennent
	core::map<u32, core::array<CIrrKlangSoundManipulator*>> gameMusics;

	// Les listes des sons/musiques qui ont actuellement des effets de "fade in/fade out" appliqués
	core::list<CFadingSound> listFadingSounds;

	// Permet de charger les sons et musiques à partir du file system d'Irrlicht
	CIrrKlangFileFactory* fileFactory;

	// True si tous les sons sont en train d'être stoppés
	bool isStoppingAllSounds;



	// Charge les musiques et les sons nécessaires uniquement au menu principal
	void loadMainMenuSounds();

	// Charge les musiques et les sons (sauf ceux chargés pour le menu principal)
	void loadGameSounds(CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Joue une nouvelle musique du menu principal
	void playNewMainMenuMusic(CIrrKlangSoundManipulator* soundManipulator);

	// Joue une nouvelle musique du jeu
	void playNewGameMusic(CIrrKlangSoundManipulator* soundManipulator);

	// Fonctions utilitaires pour la recherche des musiques :

	// Retourne la liste des fichiers présents dans le FileSystem d'IrrLicht contenant une des chaînes nameToFind1 ou nameToFind2 dans leur nom de fichier
	core::list<io::path> findNamedFiles(const char* nameToFind1, const char* nameToFind2);

	// Charge une musique si elle n'a pas encore été chargée, et l'ajoute à la liste des musiques chargées
	CIrrKlangSoundManipulator* loadMusic(const io::path& fileName);

public:
	// Recalcule le volume principal du moteur de sons avec le volume de la configuration du jeu
	void recalculateMainVolume()
	{
		if (irrKlang)
			irrKlang->setSoundVolume(gameConfig.mainVolume);
	}

	// Modifie la pause de tous les sons et musiques
	void setAllSoundsPaused(bool paused) { if (irrKlang) irrKlang->setAllSoundsPaused(paused); }

	// Arrête la lecture de toutes les musiques et sons
	void stopAllSounds()
	{
		if (irrKlang)
		{
			isStoppingAllSounds = true;
			irrKlang->stopAllSounds();
			isStoppingAllSounds = false;
		}
	}
};

#endif
#endif
