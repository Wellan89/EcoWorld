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
- Construction d'un b�timent termin�e
- Passage en cam�ra FPS
- Passage en mode Animation de la cam�ra RTS
*/

class IrrKlangManager : public ISoundStopEventReceiver
{
	friend class EcoWorldRenderer;			// Permet � la classe EcoWorldTerrainManager de d�terminer si le son de la pluie est en train d'�tre jou� ou non
	friend class EcoWorldTerrainManager;	// Permet � la classe EcoWorldTerrainManager de cr�er les sons de l'eau d'apr�s les sons d�j� pr�charg�s dans cette classe

public:
	// Enum indiquant quel �v�nement du jeu n�cessitant un son s'est produit
	enum E_GAME_SOUND_EVENT
	{
		EGSE_BATIMENT_CREATE_POSITIVE,		// Cr�ation d'un b�timent accept�e
		EGSE_BATIMENT_CREATE_NEGATIVE,		// Cr�ation d'un b�timent refus�e
		EGSE_BATIMENT_DESTROY_POSITIVE,		// Destruction d'un b�timent accept�e
		EGSE_BATIMENT_DESTROY_NEGATIVE,		// Destruction d'un b�timent refus�e
		EGSE_BATIMENT_DESTROYED,			// B�timent d�truit (� la demande de l'utilisateur)
		EGSE_BATIMENT_DESTROYED_UNWANTED,	// B�timent d�truit (� l'encontre de l'utilisateur : ex : le b�timent a atteint sa fin de vie)
		EGSE_BATIMENT_SELECTED,				// B�timent s�lectionn�
		EGSE_BATIMENT_DESELECTED,			// B�timent d�selectionn�

		EGSE_GUI_MENU_OPEN,					// Son d'ouverture d'un menu de la GUI (permet de forcer ce son)
		EGSE_GUI_MENU_CLOSE,				// Son de fermeture d'un menu de la GUI (permet de forcer ce son)
	};

	// Les types de chaque son et musique pour les classer en cat�gories
	enum E_SOUND_TYPE
	{
		EST_UNKNOW,				// Inconnu : ne pas utiliser

		EST_music,				// Musique (pour le menu principal ou pour le jeu)

		EST_ui_sound,			// Son de l'interface utilisateur ou du jeu
		EST_sound,				// Son pour les bruitages
		EST_ambiantSound,		// Son d'ambiance
		EST_waterSound,			// Son relatif � l'eau
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

	// Initialise IrrKlang, et pr�charge les musiques et les sons
	// Si mainMenuLoading = true, seules les ressources n�c�ssaires au menu principal seront charg�es
	void init(bool mainMenuLoading, CLoadingScreen* loadingScreen = NULL, float loadMin = 0, float loadMax = 100);

	// Joue les sons de la GUI ou du jeu
	void playGUISound(const SEvent& event);			// Suivant un �v�nement de la GUI
	void playGameSound(E_GAME_SOUND_EVENT event);	// Suivant un �v�nement de jeu personnalis�
	void playGameSound3D(E_GAME_SOUND_EVENT event, const core::vector3df& pos);	// Suivant un �v�nement de jeu personnalis� (en 3D)

	// Joue les musiques du menu principal
	void playMainMenuMusics(bool startPaused = false);

	// Joue les musiques du jeu
	void playGameMusics(bool startPaused = false);

	// Joue les sons d'ambiance
	void playAmbiantSounds(bool startPause = false);

	// Active/D�sactive le son d'ambiance de la pluie
	void playRainSound(bool enabled = true);

	// Joue un certain son une seule fois en mode 2D
	void play2DSound(E_SOUND_NAME sound);

	// Joue un certain son une seule fois en mode 3D, � la position sp�cifi�e
	void play3DSound(E_SOUND_NAME sound, const core::vector3df& pos);

	// Recalcule le volume des musiques avec le volume de la configuration du jeu
	void recalculateMusicsVolume();

	// Recalcule le volume des sons avec le volume de la configuration du jeu
	void recalculateSoundsVolume();

	// Appell� quand un son est stopp�
	void OnSoundStopped(ISound* sound, E_STOP_EVENT_CAUSE reason, void* userData);

	// Met � jour irrKlang
	void update(const core::vector3df& listenerPos, const core::vector3df& listenerLookAt, float elapsedTime);

	// Arr�te tous les musiques et sons d'un certain type
	void stopSoundType(E_SOUND_TYPE soundType);

	// Arr�te tous les musiques et sons sauf ceux d'un certain type
	void stopAllSoundsExceptType(E_SOUND_TYPE soundType);

	// Arr�te tous les sons (et pas les musiques !) sauf ceux d'un certain type
	void stopAllSoundsOnlyExceptType(E_SOUND_TYPE soundType);

	// Classe permettant de g�rer les sons d'Irrklang charg�s et/ou en lecture
	class CIrrKlangSoundManipulator
	{
	public:
		E_SOUND_TYPE soundType;		// Le type du son (ou de la musique)
		float defaultVolume;		// Le volume par d�faut du son (sans aucune pr�occupation de la configuration audio, pour �galiser le volume de tous les sons)
		ISoundSource* soundSource;	// La source du son qui sera jou�e (le volume de ce son est r�gl� avec la configuration audio)
		core::list<ISound*> sounds;	// La liste des pointeurs sur ce son actuellement jou� (si vide : ce son n'est pas jou� actuellement)

		// Constructeur par d�faut et destructeur
		CIrrKlangSoundManipulator() : soundType(EST_UNKNOW), defaultVolume(1.0f), soundSource(0) { };
		~CIrrKlangSoundManipulator()
		{
			// D�truit tous les sons restants
			for (core::list<ISound*>::Iterator it = sounds.begin(); it != sounds.end(); ++it)
			{
				(*it)->drop();
				LOG_DEBUG("CIrrKlangSoundManipulator::~CIrrKlangSoundManipulator() : Son detruit automatiquement : soundType = " << soundType, ELL_WARNING);
			}
			sounds.clear();
		};
	};

	// Classe permettant de g�rer l'effet de "fade in/fade out" des sons et musiques
	class CFadingSound
	{
	public:
		CIrrKlangSoundManipulator& fadingSound;	// Le son ou la musique concern� par cet effet de fading

		bool isFadingIn;		// True si le son est en train d'appara�tre ("fade in"), false sinon ("fade out")
		float fadingSpeed;		// La vitesse de "fading" de ce son (en pourcentage de volume par seconde)

		float currentFading;	// L'att�nuation de volume actuelle pour ce son pour l'effet de "fade in/fade out"

		CFadingSound(CIrrKlangSoundManipulator& fadeSound, bool fadeIn, float fadeSpeed)
			: fadingSound(fadeSound), isFadingIn(fadeIn), fadingSpeed(fadeSpeed), currentFading(fadeIn ? 0.0f : 1.0f)
		{ updateFadingEffect(0.0f); }

		// Met � jour l'effet de "fading" du son d'apr�s le temps �coul� et recalcule son volume r�el
		// Si elapsedTime = 0.0f, seul le volume r�el du son sera recalcul�
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

			// Calcule le nouveau volume r�el de ce son
			const float realSoundVolume = currentFading * fadingSound.defaultVolume * (fadingSound.soundType == EST_music ? gameConfig.musicVolume : gameConfig.soundVolume);

			// Modifie le volume par d�faut de ce son
			// D�sactiv� : Les nouveaux sons cr��s avec cette source n'ont pas � avoir un volume diminu� d�s leur cr�ation, puisqu'ils ne subiront pas forc�ment un "fading"
			//if (fadingSound.soundSource)
			//	fadingSound.soundSource->setDefaultVolume(realSoundVolume);

			// Modifie le volume du son actuellement jou�
			const core::list<ISound*>::ConstIterator END = fadingSound.sounds.end();
			for (core::list<ISound*>::ConstIterator it = fadingSound.sounds.begin(); it != END; ++it)
				(*it)->setVolume(realSoundVolume);
		}

		// Retourne si l'effet de "fading" de ce son est termin�
		bool isFadingEnd() const { return (isFadingIn ? currentFading >= 1.0f : currentFading <= 0.0f); }
	};

protected:
	// Le moteur sonore IrrKlang
	ISoundEngine* irrKlang;

#ifdef IRRKLANG_DELAYED_DLL_LOADING
	// Indique si la DLL d'IrrKlang a d�j� �t� charg�e (puisqu'elle est manuellement charg�e en mode diff�r�) :
	// True si les fonctions de la DLL irrKlang.dll ont d�j� �t� charg�es, false sinon
	bool irrKlangDllLoaded;
#endif

	// La liste des sons pr�charg�s
	CIrrKlangSoundManipulator sounds[ESN_COUNT];

	// La liste de toutes les musiques pr�charg�es d'apr�s leur nom, qui seront point�es dans les maps mainMenuMusics et gameMusics
	core::map<core::stringc, CIrrKlangSoundManipulator> loadedMusics;

	// Les listes des musiques du menu principal suivant leur pack, g�r�es dans une map (une liste de toutes les musiques du menu principal, avec pour cl� leur pack de musique)
	// Ces musiques sont recherch�es pendant le chargement du menu principal, dans le FileSystem d'Irrlicht, et contiennent "_MainThemeX_" ou "_MTX_" dans leur nom de fichier o� X est le num�ro du pack de musique o� elles appartiennent
	core::map<u32, core::array<CIrrKlangSoundManipulator*>> mainMenuMusics;

	// Les listes des musiques du jeu suivant leur pack, g�r�es dans une map (une liste de toutes les musiques du jeu, avec pour cl� leur pack de musique)
	// Ces musiques sont recherch�es pendant le chargement du jeu, dans le FileSystem d'Irrlicht, et contiennent "_MusicSetX_" ou "_MSX_" dans leur nom de fichier o� X est le num�ro du pack de musique o� elles appartiennent
	core::map<u32, core::array<CIrrKlangSoundManipulator*>> gameMusics;

	// Les listes des sons/musiques qui ont actuellement des effets de "fade in/fade out" appliqu�s
	core::list<CFadingSound> listFadingSounds;

	// Permet de charger les sons et musiques � partir du file system d'Irrlicht
	CIrrKlangFileFactory* fileFactory;

	// True si tous les sons sont en train d'�tre stopp�s
	bool isStoppingAllSounds;



	// Charge les musiques et les sons n�cessaires uniquement au menu principal
	void loadMainMenuSounds();

	// Charge les musiques et les sons (sauf ceux charg�s pour le menu principal)
	void loadGameSounds(CLoadingScreen* loadingScreen = NULL, float loadMin = 0.0f, float loadMax = 100.0f);

	// Joue une nouvelle musique du menu principal
	void playNewMainMenuMusic(CIrrKlangSoundManipulator* soundManipulator);

	// Joue une nouvelle musique du jeu
	void playNewGameMusic(CIrrKlangSoundManipulator* soundManipulator);

	// Fonctions utilitaires pour la recherche des musiques :

	// Retourne la liste des fichiers pr�sents dans le FileSystem d'IrrLicht contenant une des cha�nes nameToFind1 ou nameToFind2 dans leur nom de fichier
	core::list<io::path> findNamedFiles(const char* nameToFind1, const char* nameToFind2);

	// Charge une musique si elle n'a pas encore �t� charg�e, et l'ajoute � la liste des musiques charg�es
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

	// Arr�te la lecture de toutes les musiques et sons
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
