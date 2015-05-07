#include "global.h"

#ifdef USE_IRRKLANG

#include "IrrKlangManager.h"
#include "IrrKlangFileFactory.h"
#include "Game.h"
#include "GUIManager.h"
#include "CGUIMenuWindow.h"
#include "CGUIRessourcesWindow.h"
#include "CGUIMonthTableWindow.h"
#include "CLoadingScreen.h"

#ifdef IRRKLANG_DELAYED_DLL_LOADING
#define WIN32_LEAN_AND_MEAN	// R�duit la taille du header windows
#include <windows.h>		// N�cessaire pour l'inclusion de delayimp.h
#include <delayimp.h>		// Pour l'utilisation de __HrLoadAllImportsForDll
#endif

IrrKlangManager::IrrKlangManager() : irrKlang(0), fileFactory(0),
#ifdef IRRKLANG_DELAYED_DLL_LOADING
	irrKlangDllLoaded(false),
#endif
	isStoppingAllSounds(false)
{
}
IrrKlangManager::~IrrKlangManager()
{
	isStoppingAllSounds = true;

	if (fileFactory)
		fileFactory->drop();

	if (irrKlang)
		irrKlang->drop();
}
core::list<io::path> IrrKlangManager::findNamedFiles(const char* nameToFind1, const char* nameToFind2)
{
	// Obtient le file system d'Irrlicht
	io::IFileSystem* const fileSystem = game->fileSystem;

	// La liste des fichiers trouv�s
	core::list<io::path> foundFiles;

	// Parcours le syst�me de fichiers d'Irrlicht et cherche les fichiers contenant nameToFind dans leur nom
	const u32 archiveCount = fileSystem->getFileArchiveCount();
	for (u32 i = 0; i < archiveCount; ++i)
	{
		const io::IFileList* const fileList = fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;				// V�rifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);	// Obtient le nom actuel du fichier

			// V�rifie que le nom du fichier contient bien une des cha�nes de caract�re recherch�es
			if (fileName.find(nameToFind1) >= 0 || fileName.find(nameToFind2) >= 0)
			{
				// Si c'est le cas, on l'ajoute � la liste des fichiers trouv�s
				foundFiles.push_back(fileName);
			}
		}
	}

	// Retourne finalement la liste des fichiers trouv�s
	return foundFiles;
}
void IrrKlangManager::init(bool mainMenuLoading, CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	// V�rifie que le mode audio est activ�
	if (!gameConfig.audioEnabled)
		return;

#ifdef IRRKLANG_DELAYED_DLL_LOADING
	// Essaie de charger toutes les fonctions de la DLL irrKlang.dll
	if (!irrKlangDllLoaded)
	{
		// Charge toutes les fonctions de la DLL irrKlang.dll : voir http://msdn.microsoft.com/fr-fr/library/8yfshtha.aspx

		// __HrLoadAllImportsForDll peut lancer une exception : on la r�cup�re ici
		// Attention : C'est en fait une exception SEH qui est envoy�e par cette fonction (et non pas une exception standard C++) :
		// pour qu'elle soit r�cup�r�e par le bloc catch, le compilateur doit avoir l'option /EHa activ�e (attention : l'activation de cette option d�sactive dertaines optimisations du compilateur sur les blocs catch : voir http://msdn.microsoft.com/en-us/library/1deeycx5.aspx)
		// Pour plus d'informations, voir : http://members.gamedev.net/sicrane/articles/exception.html
		// TODO : Explorer une solution interm�diaire permettant de convertir les exceptions SEH en exceptions C++ : voir http://msdn.microsoft.com/en-us/library/5z4bw5h5.aspx
		//try
		//{
			irrKlangDllLoaded = SUCCEEDED(__HrLoadAllImportsForDll("irrKlang.dll"));
		//}
		//catch (...)
		//{
		//	irrKlangDllLoaded = false;
		//}

		// Avertit l'utilisateur si le chargement de la DLL a �chou�
		if (irrKlangDllLoaded)
		{
			// R�ussite du chargement de la DLL :
			LOG("IkMgr: Loading irrKlang.dll : Succeed !", ELL_INFORMATION);
		}
		else
		{
			// Echec du chargement de la DLL :
			LOG("IkMgr: Loading irrKlang.dll : Failed !", ELL_ERROR);
		}
	}

	// On n'a pas pu charger les fonctions de la DLL irrKlang.dll : on annule la cr�ation d'IrrKlang
	if (!irrKlangDllLoaded)
		return;
#endif

	// Cr�e IrrKlang
	if (!irrKlang)
	{
		irrKlang = createIrrKlangDevice(gameConfig.irrKlangDriver, gameConfig.irrKlangOptions,
			(gameConfig.irrKlangDeviceID.size() ? gameConfig.irrKlangDeviceID.c_str() : NULL));

		if (!irrKlang)
		{
			LOG_DEBUG("IkMgr: Le chargement d'IrrKlang a echoue", ELL_ERROR);
			LOG_RELEASE("AVERTISSEMENT : Le chargement d'IrrKlang a �chou�. Les musiques et sons ne seront pas jou�s.", ELL_WARNING);
			return;
		}
		LOG_DEBUG("IkMgr: Chargement d'IrrKlang reussi", ELL_INFORMATION);

		if (irrKlang)
		{
			// Cr�e le file factory pour IrrKlang
			if (!fileFactory)
				fileFactory = new CIrrKlangFileFactory();

			irrKlang->addFileFactory(fileFactory);

			// Modifie l'attenuation des sons avec les unit�s du jeu (on consid�re ici TAILLE_OBJETS / 2 = 1 m)
			// TODO : Utiliser les unit�s r�elles ! => Ne fonctionne pas actuellement, par manque d'unit� �talon
			//irrKlang->setDopplerEffectParameters(1.0f, TAILLE_OBJETS * 0.5f);
			//irrKlang->setDefault3DSoundMinDistance(10.0f * TAILLE_OBJETS);	// 200.0f pour TAILLE_OBJETS = 20.0f
			//irrKlang->setRolloffFactor(2.0f / TAILLE_OBJETS);	// 0.1f pour TAILLE_OBJETS = 20.0f
		}
	}
	if (!irrKlang)
		return;



	// Charge les musiques et les sons du menu principal
	loadMainMenuSounds();

	// Si on doit charger tout le jeu, on charge les autres musiques et sons
	if (!mainMenuLoading)
		loadGameSounds(loadingScreen, loadMin, loadMax);

	// R�gle le volume principal
	recalculateMainVolume();

	// R�gle le volume des musiques
	recalculateMusicsVolume();

	// R�gle le volume des sons
	recalculateSoundsVolume();
}
IrrKlangManager::CIrrKlangSoundManipulator* IrrKlangManager::loadMusic(const io::path& fileName)
{
	if (!irrKlang)
		return NULL;

	// V�rifie que la musique n'est pas d�j� charg�e
	core::map<core::stringc, CIrrKlangSoundManipulator>::Node* mapNode = loadedMusics.find(fileName);
	if (mapNode)
		return &(mapNode->getValue());

	// Sinon, demande � IrrKlang de charger la musique demand�e
	irrklang::ISoundSource* musicSource = irrKlang->addSoundSourceFromFile(fileName.c_str(), irrklang::ESM_STREAMING, true);
	if (!musicSource)
		return NULL;

	// Ajoute cette musique � la liste des musiques pr�charg�es, puis la retourne
	CIrrKlangSoundManipulator music;
	music.soundSource = musicSource;
	music.soundType = EST_music;
	loadedMusics.insert(fileName, music);
	mapNode = loadedMusics.find(fileName);
	return &(mapNode->getValue());
}
void IrrKlangManager::loadMainMenuSounds()
{
	if (!irrKlang)
		return;



	// Musiques :

	// V�rifie que les musiques du menu principal de ce pack n'ont pas encore �t� charg�es
	if (!mainMenuMusics.find(gameConfig.gameMusicsSet))
	{
		// Recherche et charge les musiques du menu principal avec le pack de musiques actuel :

		// Les musiques recherch�es doivent contenir "_MainThemeX_" ou "_MTX_" dans leur nom de fichier (sans �tre sensible � la casse), X �tant le num�ro du pack de musiques actuel
		sprintf_SS("_mt%u_", gameConfig.gameMusicsSet);
		const core::stringc strMT(text_SS);
		sprintf_SS("_maintheme%u_", gameConfig.gameMusicsSet);

		// Obtient la liste des fichiers trouv�s
		core::list<io::path> foundFiles = findNamedFiles(strMT.c_str(), text_SS);

		// Charge chacune des musiques trouv�es
		core::array<CIrrKlangSoundManipulator*> musics(foundFiles.size());
		const core::list<io::path>::ConstIterator END = foundFiles.end();
		for (core::list<io::path>::ConstIterator it = foundFiles.begin(); it != END; ++it)
		{
			CIrrKlangSoundManipulator* const music = loadMusic((*it));
			if (music)	musics.push_back(music);
		}

		// Ajoute les musiques trouv�es � la liste des musiques du menu principal
		mainMenuMusics.insert(gameConfig.gameMusicsSet, musics);
	}



	// Sons :
	if (!sounds[ESN_ui_button_click].soundSource)
	{
		sounds[ESN_ui_button_click].soundSource = irrKlang->addSoundSourceFromFile("se_button_small_click.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_button_click].soundType = EST_ui_sound;
	}
	if (!sounds[ESN_ui_tab_change].soundSource)
	{
		sounds[ESN_ui_tab_change].soundSource = irrKlang->addSoundSourceFromFile("se_tab_click.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_tab_change].soundType = EST_ui_sound;
	}
	if (!sounds[ESN_ambiant_rain].soundSource)
	{
		sounds[ESN_ambiant_rain].soundSource = irrKlang->addSoundSourceFromFile("water_waterfall_medium.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ambiant_rain].soundType = EST_ambiantSound;
		sounds[ESN_ambiant_rain].defaultVolume = 0.2f;
	}
}
void IrrKlangManager::loadGameSounds(CLoadingScreen* loadingScreen, float loadMin, float loadMax)
{
	if (!irrKlang)
		return;
	if (loadingScreen)
		if (loadingScreen->setPercentAndDraw(loadMin))	return;

	// Doit repr�senter de sons (uniquement !) qui seront charg�s dans cette fonction (c'est donc un nombre entier, mais repr�sent� sous forme de float pour optimisation !)
	// -> A incr�menter � chaque ajout de nouveau sons � charger dans cette fonction
#define NB_SOUNDS	 17.0f



	// Musiques :

	// V�rifie que les musiques du jeu de ce pack n'ont pas encore �t� charg�es
	if (!gameMusics.find(gameConfig.gameMusicsSet))
	{
		// Recherche et charge les musiques du menu jeu avec le pack de musiques actuel :

		// Les musiques recherch�es doivent contenir "_MusicSetX_" ou "_MSX_" dans leur nom de fichier (sans �tre sensible � la casse), X �tant le num�ro du pack de musiques actuel
		sprintf_SS("_ms%u_", gameConfig.gameMusicsSet);
		const core::stringc strMS(text_SS);
		sprintf_SS("_musicset%u_", gameConfig.gameMusicsSet);

		// Obtient la liste des fichiers trouv�s
		core::list<io::path> foundFiles = findNamedFiles(strMS.c_str(), text_SS);

		// Charge chacune des musiques trouv�es :

		// Variables pour la barre de chargement
		const u32 foundFilesSize = foundFiles.size();
		const float loadStep = (loadMax - loadMin) * 0.2f / (float)foundFilesSize;	// 20% du chargement accord�
		float musiqueActuelle = 1.0f;

		core::array<CIrrKlangSoundManipulator*> musics(foundFilesSize);
		const core::list<io::path>::ConstIterator END = foundFiles.end();
		for (core::list<io::path>::ConstIterator it = foundFiles.begin(); it != END; ++it)
		{
			CIrrKlangSoundManipulator* const music = loadMusic((*it));
			if (music)	musics.push_back(music);

			if (loadingScreen)
				if (loadingScreen->setPercentAndDraw(loadMin + loadStep * musiqueActuelle))	return;
			++musiqueActuelle;
		}

		// Ajoute les musiques trouv�es � la liste des musiques du jeu
		gameMusics.insert(gameConfig.gameMusicsSet, musics);

		if (loadingScreen)
		{
			loadMin += (loadMax - loadMin) * 0.2f;	// Change le nouveau minimum de la barre de chargement pour simplifier les calculs lors du chargement des sons
			if (loadingScreen->setPercentAndDraw(loadMin))	return;
		}
	}



	// Sons :

	// Variables pour la barre de chargement
	const float step = (loadMax - loadMin) / NB_SOUNDS;
	float nbActuel = 1.0f;

	if (!sounds[ESN_ui_menu_open].soundSource)
	{
		sounds[ESN_ui_menu_open].soundSource = irrKlang->addSoundSourceFromFile("se_open_menu.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_menu_open].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_menu_close].soundSource)
	{
		sounds[ESN_ui_menu_close].soundSource = irrKlang->addSoundSourceFromFile("se_menu_close (b).wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_menu_close].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_create_positive].soundSource)
	{
		sounds[ESN_ui_batiment_create_positive].soundSource = irrKlang->addSoundSourceFromFile("se_place_building_positive.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_create_positive].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_create_negative].soundSource)
	{
		sounds[ESN_ui_batiment_create_negative].soundSource = irrKlang->addSoundSourceFromFile("se_place_building_negative.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_create_negative].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_destroy_positive].soundSource)
	{
		sounds[ESN_ui_batiment_destroy_positive].soundSource = irrKlang->addSoundSourceFromFile("se_place_building_positive (e).wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_destroy_positive].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_destroyed].soundSource)
	{
		sounds[ESN_ui_batiment_destroyed].soundSource = irrKlang->addSoundSourceFromFile("building_small_destroy.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_destroyed].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_destroyed_large].soundSource)
	{
		sounds[ESN_ui_batiment_destroyed_large].soundSource = irrKlang->addSoundSourceFromFile("building_large_destroy.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_destroyed_large].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_selected].soundSource)
	{
		sounds[ESN_ui_batiment_selected].soundSource = irrKlang->addSoundSourceFromFile("se_building_medium_select.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_selected].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ui_batiment_deselected].soundSource)
	{
		sounds[ESN_ui_batiment_deselected].soundSource = irrKlang->addSoundSourceFromFile("se_building_medium_deselect.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ui_batiment_deselected].soundType = EST_ui_sound;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ambiant_atmosphere].soundSource)
	{
		sounds[ESN_ambiant_atmosphere].soundSource = irrKlang->addSoundSourceFromFile("atmo_generic.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ambiant_atmosphere].soundType = EST_ambiantSound;
		//sounds[ESN_ambiant_atmosphere].defaultVolume = 0.31f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_ambiant_water].soundSource)
	{
		sounds[ESN_ambiant_water].soundSource = irrKlang->addSoundSourceFromFile("water_close_atmo.wav", ESM_NO_STREAMING, true);
		sounds[ESN_ambiant_water].soundType = EST_ambiantSound;
		//sounds[ESN_ambiant_water].defaultVolume = 0.26f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_1].soundSource)
	{
		sounds[ESN_water_1].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea1.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_1].soundType = EST_waterSound;
		sounds[ESN_water_1].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_2].soundSource)
	{
		sounds[ESN_water_2].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea2.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_2].soundType = EST_waterSound;
		sounds[ESN_water_2].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_3].soundSource)
	{
		sounds[ESN_water_3].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea3.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_3].soundType = EST_waterSound;
		sounds[ESN_water_3].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_4].soundSource)
	{
		sounds[ESN_water_4].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea4.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_4].soundType = EST_waterSound;
		sounds[ESN_water_4].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_5].soundSource)
	{
		sounds[ESN_water_5].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea5.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_5].soundType = EST_waterSound;
		sounds[ESN_water_5].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
	if (!sounds[ESN_water_6].soundSource)
	{
		sounds[ESN_water_6].soundSource = irrKlang->addSoundSourceFromFile("water_open_sea6.wav", ESM_NO_STREAMING, true);
		sounds[ESN_water_6].soundType = EST_waterSound;
		sounds[ESN_water_6].defaultVolume = 0.32f;
	}	if (loadingScreen) { if (loadingScreen->setPercentAndDraw(loadMin + step * nbActuel))	return; } ++nbActuel;
}
void IrrKlangManager::playGUISound(const SEvent& event)
{
	if (!irrKlang || event.EventType != EET_GUI_EVENT || !gameConfig.audioEnabled)
		return;

	// Sons globaux de la GUI :
	switch (event.GUIEvent.EventType)
	{
	case EGET_BUTTON_CLICKED:	// Appui sur un bouton
		play2DSound(ESN_ui_button_click);	break;
	case EGET_TAB_CHANGED:		// Changement d'onglet
		play2DSound(ESN_ui_tab_change);		break;

	default: break;
	}
}
void IrrKlangManager::playGameSound(E_GAME_SOUND_EVENT event)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	switch (event)
	{
	case EGSE_BATIMENT_CREATE_POSITIVE:		// Cr�ation d'un b�timent accept�e
		play2DSound(ESN_ui_batiment_create_positive);	break;
	case EGSE_BATIMENT_DESTROY_POSITIVE:		// Destruction d'un b�timent accept�e
		play2DSound(ESN_ui_batiment_destroy_positive);	break;
	case EGSE_BATIMENT_CREATE_NEGATIVE:		// Cr�ation d'un b�timent refus�e
	case EGSE_BATIMENT_DESTROY_NEGATIVE:		// Destruction d'un b�timent refus�e
		play2DSound(ESN_ui_batiment_create_negative);	break;
	case EGSE_BATIMENT_DESTROYED:				// Destruction volontaire d'un b�timent
		play2DSound(ESN_ui_batiment_destroyed);			break;
	case EGSE_BATIMENT_DESTROYED_UNWANTED:	// Destruction ind�sir�e d'un b�timent
		play2DSound(ESN_ui_batiment_destroyed_large);	break;
	case EGSE_BATIMENT_SELECTED:			// S�lection d'un b�timent
		play2DSound(ESN_ui_batiment_selected);			break;
	case EGSE_BATIMENT_DESELECTED:			// D�selection d'un b�timent
		play2DSound(ESN_ui_batiment_deselected);		break;

	case EGSE_GUI_MENU_OPEN:				// Son d'ouverture d'un menu
		play2DSound(ESN_ui_menu_open);					break;
	case EGSE_GUI_MENU_CLOSE:				// Son de fermeture d'un menu
		play2DSound(ESN_ui_menu_close);					break;

	default:
		LOG_DEBUG("IkMgr::playGameSound(" << event << ") : Evenement de jeu inconnu : event = " << event, ELL_WARNING);
		break;
	}
}
void IrrKlangManager::playGameSound3D(E_GAME_SOUND_EVENT event, const core::vector3df& pos)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	switch (event)
	{
	case EGSE_BATIMENT_CREATE_POSITIVE:		// Cr�ation d'un b�timent accept�e
		play3DSound(ESN_ui_batiment_create_positive, pos);	break;
	case EGSE_BATIMENT_DESTROY_POSITIVE:		// Destruction d'un b�timent accept�e
		play3DSound(ESN_ui_batiment_destroy_positive, pos);	break;
	case EGSE_BATIMENT_CREATE_NEGATIVE:		// Cr�ation d'un b�timent refus�e
	case EGSE_BATIMENT_DESTROY_NEGATIVE:	// Destruction d'un b�timent refus�e
		play3DSound(ESN_ui_batiment_create_negative, pos);	break;
	case EGSE_BATIMENT_DESTROYED:				// Destruction volontaire d'un b�timent
		play3DSound(ESN_ui_batiment_destroyed, pos);			break;
	case EGSE_BATIMENT_DESTROYED_UNWANTED:	// Destruction ind�sir�e d'un b�timent
		play3DSound(ESN_ui_batiment_destroyed_large, pos);	break;
	case EGSE_BATIMENT_SELECTED:			// S�lection d'un b�timent
		play3DSound(ESN_ui_batiment_selected, pos);			break;
	case EGSE_BATIMENT_DESELECTED:			// D�selection d'un b�timent
		play3DSound(ESN_ui_batiment_deselected, pos);		break;

	default:
		LOG_DEBUG("IkMgr::playGameSound3D(" << event << ") : Evenement de jeu inconnu : event = " << event, ELL_WARNING);
		break;
	}
}
void IrrKlangManager::playMainMenuMusics(bool startPaused)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	// Obtient le tableau du pack de musiques actuelle
	const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = mainMenuMusics.find(gameConfig.gameMusicsSet);
	if (!mapNode)
	{
		// Pour conserver une coh�rence avec la m�thode IrrKlangManager::playGameMusics en cas d'�chec :
		// Arr�te la lecture de toutes les musiques
		stopSoundType(EST_music);
		return;
	}

	const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
	const u32 musicsSize = musics.size();
	if (!musicsSize)
	{
		// Pour conserver une coh�rence avec la m�thode IrrKlangManager::playGameMusics en cas d'�chec :
		// Arr�te la lecture de toutes les musiques
		stopSoundType(EST_music);
		return;
	}



	// V�rifie si des musiques du menu principal sont d�j� en train d'�tre jou�es : si c'est le cas, on enl�ve simplement leur pause puis on quitte
	bool mainMenuMusicFound = false;
	if (musicsSize)	// V�rification n�cessaire pour �tre certain que musics[0] existe, pour la cr�ation de END
	{
		const core::list<ISound*>::ConstIterator END = musics[0]->sounds.end();
		for (u32 i = 0; i < musicsSize; ++i)
		{
			for (core::list<ISound*>::ConstIterator it = musics[i]->sounds.begin(); it != END; ++it)
			{
				if ((*it)->getIsPaused() != startPaused)
					(*it)->setIsPaused(startPaused);
				mainMenuMusicFound = true;
			}
		}
	}
	if (mainMenuMusicFound)
		return;



	// Arr�te d'abord la lecture de toutes les musiques
	stopSoundType(EST_music);

	// Choisis une musique du menu principal au hasard (entre 0 et musicsSize)
	const int musiqueNb = (musicsSize > 1 ? (rand() % musicsSize) : 0);

	// Joue la musique choisie
	CIrrKlangSoundManipulator* music = musics[musiqueNb];
	if (music->soundSource)
	{
		ISound* const sound = irrKlang->play2D(music->soundSource, false, startPaused, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, music);
			music->sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << music->soundType, ELL_INFORMATION);
		}
	}
}
void IrrKlangManager::playGameMusics(bool startPaused)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	// Arr�te d'abord la lecture de toutes les musiques
	stopSoundType(EST_music);

	// Obtient le tableau du pack de musiques actuelle
	const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = gameMusics.find(gameConfig.gameMusicsSet);
	if (!mapNode)
		return;

	const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
	const u32 musicsSize = musics.size();
	if (!musicsSize)
		return;

	// Choisis une musique du jeu au hasard (entre 0 et musicsSize)
	const int musiqueNb = (musicsSize > 1 ? (rand() % musicsSize) : 0);

	// Joue la musique choisie
	CIrrKlangSoundManipulator* const music = musics[musiqueNb];
	if (music->soundSource)
	{
		ISound* const sound = irrKlang->play2D(music->soundSource, false, startPaused, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, music);
			music->sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << music->soundType, ELL_INFORMATION);
		}
	}
}
void IrrKlangManager::playAmbiantSounds(bool startPaused)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	// V�rifie que ce son d'ambiance n'est pas d�j� jou�
	if (sounds[ESN_ambiant_atmosphere].sounds.size() > 0)
		return;

	// Joue le sons d'ambiance en boucle en 3D
	if (sounds[ESN_ambiant_atmosphere].soundSource)
	{
		ISound* const sound = irrKlang->play3D(sounds[ESN_ambiant_atmosphere].soundSource,
			vec3df(0.0f, 55.0f * TAILLE_OBJETS, 0.0f),	// 1100.0f pour TAILLE_OBJETS = 20.0f
			true, startPaused, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, &(sounds[ESN_ambiant_atmosphere]));
			sound->setMinDistance(10.0f * TAILLE_OBJETS);	// 200.0f pour TAILLE_OBJETS = 20.0f
			sound->setMaxDistance(30.0f * TAILLE_OBJETS);	// 600.0f pour TAILLE_OBJETS = 20.0f

			sounds[ESN_ambiant_atmosphere].sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << sounds[ESN_ambiant_atmosphere].soundType, ELL_INFORMATION);
		}
	}
}
void IrrKlangManager::playRainSound(bool enabled)
{
	if (!irrKlang || !gameConfig.audioEnabled)
		return;

	// Joue le son d'ambiance de la pluie en boucle en 3D
	if (enabled && sounds[ESN_ambiant_rain].soundSource)
	{
		// R�initialise le volume par d�faut de ce son pour qu'il ne soit pas jou� un instant avec une trop grande intensit�
		const float lastDefaultVolume = sounds[ESN_ambiant_rain].soundSource->getDefaultVolume();
		sounds[ESN_ambiant_rain].soundSource->setDefaultVolume(0.0f);

		ISound* const sound = irrKlang->play2D(sounds[ESN_ambiant_rain].soundSource, true, false, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, &(sounds[ESN_ambiant_rain]));

			sounds[ESN_ambiant_rain].sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << sounds[ESN_ambiant_rain].soundType, ELL_INFORMATION);
		}

		// Restaure le volume par d�faut de ce son
		sounds[ESN_ambiant_rain].soundSource->setDefaultVolume(lastDefaultVolume);
	}

	// Cr�e un "fade in/fade out" pour le son d'ambiance de la pluie, qui durera exactement le temps d'une transition pour le weather manager
	listFadingSounds.push_back(CFadingSound(sounds[ESN_ambiant_rain], enabled, 1.0f / WEATHER_TRANSITION_TIME));
}
void IrrKlangManager::play2DSound(E_SOUND_NAME sound)
{
	if (!irrKlang || !gameConfig.audioEnabled || sound >= ESN_COUNT)
		return;
	if (!sounds[sound].soundSource)
		return;

	// Joue le son simplement, sans garder sa trace
	irrKlang->play2D(sounds[sound].soundSource);
}
void IrrKlangManager::play3DSound(E_SOUND_NAME sound, const core::vector3df& pos)
{
	if (!irrKlang || !gameConfig.audioEnabled || sound >= ESN_COUNT)
		return;
	if (!sounds[sound].soundSource)
		return;

	// Joue le son simplement, sans garder sa trace
	irrKlang->play3D(sounds[sound].soundSource, pos);
}
void IrrKlangManager::stopSoundType(E_SOUND_TYPE soundType)
{
	if (!irrKlang)
		return;

	if (soundType == EST_music)
	{
		// Parcours toutes les musiques du pack de musiques actuel pour les arr�ter :

		for (int musicType = 0; musicType < 2; ++musicType)
		{
			// Obtient le tableau du pack de musiques actuelle
			const core::map<u32, core::array<CIrrKlangSoundManipulator*>>& musicsMap = (musicType == 0 ? mainMenuMusics : gameMusics);
			const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = musicsMap.find(gameConfig.gameMusicsSet);
			if (!mapNode)
				continue;
			const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
			const u32 musicsSize = musics.size();
			if (!musicsSize)	// V�rification n�cessaire pour �tre certain que musics[0] existe, pour la cr�ation de END
				continue;

			const core::list<ISound*>::ConstIterator END = musics[0]->sounds.end();
			for (u32 i = 0; i < musicsSize; ++i)
				for (core::list<ISound*>::ConstIterator it = musics[i]->sounds.begin(); it != END; ++it)
					(*it)->stop();
		}
	}
	else
	{
		// Parcours tous les sons pour trouver ceux qui ont ce type et les arr�te :
		const core::list<ISound*>::ConstIterator END = sounds[0].sounds.end();
		for (int i = 0; i < ESN_COUNT; ++i)
			if (sounds[i].soundType == soundType)
				for (core::list<ISound*>::ConstIterator it = sounds[i].sounds.begin(); it != END; ++it)
					(*it)->stop();
	}
}
void IrrKlangManager::stopAllSoundsExceptType(E_SOUND_TYPE soundType)
{
	if (!irrKlang)
		return;

	// Si le type de son � ne pas arr�ter n'est pas une musique, on arr�te toutes les musiques
	if (soundType != EST_music)
	{
		// Parcours toutes les musiques du pack de musiques actuel pour les arr�ter :

		for (int musicType = 0; musicType < 2; ++musicType)
		{
			// Obtient le tableau du pack de musiques actuelle
			const core::map<u32, core::array<CIrrKlangSoundManipulator*>>& musicsMap = (musicType == 0 ? mainMenuMusics : gameMusics);
			const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = musicsMap.find(gameConfig.gameMusicsSet);
			if (!mapNode)
				continue;
			const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
			const u32 musicsSize = musics.size();
			if (!musicsSize)	// V�rification n�cessaire pour �tre certain que musics[0] existe, pour la cr�ation de END
				continue;

			const core::list<ISound*>::ConstIterator END = musics[0]->sounds.end();
			for (u32 i = 0; i < musicsSize; ++i)
				for (core::list<ISound*>::ConstIterator it = musics[i]->sounds.begin(); it != END; ++it)
					(*it)->stop();
		}
	}

	// Arr�te tous les sons qui n'ont pas ce type
	stopAllSoundsOnlyExceptType(soundType);
}
void IrrKlangManager::stopAllSoundsOnlyExceptType(E_SOUND_TYPE soundType)
{
	if (!irrKlang)
		return;

	// Parcours tous les sons pour trouver ceux qui n'ont pas ce type et les arr�te :
	const core::list<ISound*>::ConstIterator END = sounds[0].sounds.end();
	for (int i = 0; i < ESN_COUNT; ++i)
		if (sounds[i].soundType != soundType)
			for (core::list<ISound*>::ConstIterator it = sounds[i].sounds.begin(); it != END; ++it)
				(*it)->stop();
}
void IrrKlangManager::recalculateMusicsVolume()
{
	if (!irrKlang)
		return;

	// Parcours toutes les musiques du pack de musiques actuel pour r�gler leur volume
	for (int musicType = 0; musicType < 2; ++musicType)
	{
		// Obtient le tableau du pack de musiques actuelle
		const core::map<u32, core::array<CIrrKlangSoundManipulator*>>& musicsMap = (musicType == 0 ? mainMenuMusics : gameMusics);
		const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = musicsMap.find(gameConfig.gameMusicsSet);
		if (!mapNode)
			continue;
		const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
		const u32 musicsSize = musics.size();
		if (!musicsSize)	// V�rification n�cessaire pour �tre certain que musics[0] existe, pour la cr�ation de END
			continue;

		const core::list<ISound*>::ConstIterator END = musics[0]->sounds.end();
		for (u32 i = 0; i < musicsSize; ++i)
		{
			// Calcule le nouveau volume de la musique suivant son volume par d�faut
			const float newVolume = musics[i]->defaultVolume * gameConfig.musicVolume;

			// Modifie le volume par d�faut de cette musique
			if (musics[i]->soundSource)
				musics[i]->soundSource->setDefaultVolume(newVolume);

			// Si on a un pointeur sur cette musique, on modifie aussi son volume actuel
			for (core::list<ISound*>::Iterator it = musics[i]->sounds.begin(); it != END; ++it)
				(*it)->setVolume(newVolume);
		}
	}

	// Recalcule les volumes des musiques affect�es par l'effet de "fading"
	const core::list<CFadingSound>::Iterator END_fading = listFadingSounds.end();
	for (core::list<CFadingSound>::Iterator it = listFadingSounds.begin(); it != END_fading; ++it)
		(*it).updateFadingEffect(0.0f);
}
void IrrKlangManager::recalculateSoundsVolume()
{
	if (!irrKlang)
		return;

	// R�gle le volume des sons
	for (int i = 0; i < ESN_COUNT; ++i)
	{
		// V�rifie ce son est bien du type demand�
		//if (sounds[i].soundType != EST_waterSound)
		{
			// Calcule le nouveau volume du son suivant son volume par d�faut
			const float newVolume = sounds[i].defaultVolume * gameConfig.soundVolume;

			// Modifie le volume par d�faut de ce son
			if (sounds[i].soundSource)
				sounds[i].soundSource->setDefaultVolume(newVolume);

			// Si on a un pointeur sur ce son, on modifie aussi son volume actuel
			for (core::list<ISound*>::Iterator it = sounds[i].sounds.begin(); it != sounds[i].sounds.end(); ++it)
				(*it)->setVolume(newVolume);
		}
	}

	// Recalcule les volumes des sons affect�s par l'effet de "fading"
	for (core::list<CFadingSound>::Iterator it = listFadingSounds.begin(); it != listFadingSounds.end(); ++it)
		(*it).updateFadingEffect(0.0f);
}
void IrrKlangManager::playNewMainMenuMusic(CIrrKlangSoundManipulator* soundManipulator)
{
	// V�rifie que le son qu'on nous a envoy� est bien une musique du menu principal
	if (!irrKlang  || !gameConfig.audioEnabled || !soundManipulator)
		return;
	if (soundManipulator->soundType != EST_music)
		return;

	// Obtient le tableau du pack de musiques actuelle
	const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = mainMenuMusics.find(gameConfig.gameMusicsSet);
	if (!mapNode)
		return;

	const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
	const u32 musicsSize = musics.size();
	if (!musicsSize)
		return;

	// Obtient le num�ro de la musique en cours si c'est une musique du jeu
	u32 currentMusicNb;
	bool foundMusic = false;
	for (u32 i = 0; i < musicsSize; ++i)
	{
		if (musics[i] == soundManipulator)
		{
			currentMusicNb = i;
			foundMusic = true;
			break;
		}
	}

	// Si on n'a pu trouver la musique du jeu actuelle, on annule
	if (!foundMusic)
		return;

	// Choisis une musique du menu principal au hasard (entre 0 et musicsSize), tout en v�rifiant que ce n'est pas celle qui vient d'�tre jou�e
	int musiqueNb;
	if (musicsSize > 1)
	{
		do
		{
			musiqueNb = (rand() % musicsSize);
		} while (musiqueNb == currentMusicNb);
	}
	else
		musiqueNb = currentMusicNb;

	// Joue la musique choisie
	CIrrKlangSoundManipulator* const music = musics[musiqueNb];
	if (music->soundSource)
	{
		ISound* const sound = irrKlang->play2D(music->soundSource, false, false, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, music);
			music->sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << music->soundType, ELL_INFORMATION);
		}
	}
}
void IrrKlangManager::playNewGameMusic(CIrrKlangSoundManipulator* soundManipulator)
{
	// V�rifie que le son qu'on nous a envoy� est bien une musique du jeu
	if (!irrKlang  || !gameConfig.audioEnabled || !soundManipulator)
		return;
	if (soundManipulator->soundType != EST_music)
		return;

	// Obtient le tableau du pack de musiques actuelle
	const core::map<u32, core::array<CIrrKlangSoundManipulator*>>::Node* const mapNode = gameMusics.find(gameConfig.gameMusicsSet);
	if (!mapNode)
		return;

	const core::array<CIrrKlangSoundManipulator*> musics = mapNode->getValue();
	const u32 musicsSize = musics.size();
	if (!musicsSize)
		return;

	// Obtient le num�ro de la musique en cours si c'est une musique du jeu
	u32 currentMusicNb;
	bool foundMusic = false;
	for (u32 i = 0; i < musicsSize; ++i)
	{
		if (musics[i] == soundManipulator)
		{
			currentMusicNb = i;
			foundMusic = true;
			break;
		}
	}

	// Si on n'a pu trouver la musique du jeu actuelle, on annule
	if (!foundMusic)
		return;

	// Choisis une musique du jeu au hasard (entre 0 et musicsSize), tout en v�rifiant que ce n'est pas celle qui vient d'�tre jou�e
	int musiqueNb;
	if (musicsSize > 1)
	{
		do
		{
			musiqueNb = (rand() % musicsSize);
		} while (musiqueNb == currentMusicNb);
	}
	else
		musiqueNb = currentMusicNb;

	// Joue la musique choisie
	CIrrKlangSoundManipulator* const music = musics[musiqueNb];
	if (music->soundSource)
	{
		ISound* const sound = irrKlang->play2D(music->soundSource, false, false, true);
		if (sound)
		{
			sound->setSoundStopEventReceiver(this, music);
			music->sounds.push_back(sound);

			LOG_DEBUG("IkMgr: ISound created : soundType = " << music->soundType, ELL_INFORMATION);
		}
	}
}
void IrrKlangManager::OnSoundStopped(ISound* const sound, E_STOP_EVENT_CAUSE reason, void* userData)
{
	// V�rifie qu'on a bien un pointeur vers IrrKlang et ce son
	if (!irrKlang || !sound)
		return;

	// Les donn�es utilisateurs contiennent normalement le pointeur sur l'interface permettant de manier le son jou�
#ifdef _DEBUG
	int soundType = -1;
#endif
	if (userData)
	{
		CIrrKlangSoundManipulator* const soundManipulator = reinterpret_cast<CIrrKlangSoundManipulator*>(userData);

		if (soundManipulator)
		{
			// V�rifie que le son s'est bien termin� correctement jusqu'� la fin de sa lecture (et non pas parce qu'on l'a arr�t� volontairement pour couper irrKlang)
			if (reason == ESEC_SOUND_FINISHED_PLAYING && soundManipulator->soundType == EST_music
				&& !isStoppingAllSounds)	// Bug d'IrrKlang : lors de l'appel � irrKlang->stopAllSounds(), les sons sont stopp�s avec la raison ESEC_SOUND_FINISHED_PLAYING !
			{
				// Si c'est une musique, on en joue une nouvelle :

				// D�termine si on doit jouer une musique du jeu ou une musique du menu principal
				if (game->isInGameScene())
					playNewGameMusic(soundManipulator);			// Joue une nouvelle musique du jeu
				else
					playNewMainMenuMusic(soundManipulator);		// Joue une nouvelle musique du menu principal
			}

			// Efface le pointeur sur ce son
			for (core::list<ISound*>::Iterator it = soundManipulator->sounds.begin(); it != soundManipulator->sounds.end(); ++it)
			{
				if ((*it) == sound)
				{
					soundManipulator->sounds.erase(it);
					break;
				}
			}

#ifdef _DEBUG
			soundType = soundManipulator->soundType;
#endif
		}
	}

	sound->drop();

	LOG_DEBUG("IkMgr: ISound dropped : soundType = " << soundType, ELL_INFORMATION);
}
void IrrKlangManager::update(const core::vector3df& listenerPos, const core::vector3df& listenerLookAt, float elapsedTime)
{
	if (!gameConfig.audioEnabled || !irrKlang)
		return;

	// Met � jour les effets de "fade in/fade out" des sons ou des musiques
	if (elapsedTime != 0.0f)
	{
		const core::list<CFadingSound>::Iterator END_fading = listFadingSounds.end();
		core::list<CFadingSound>::Iterator it = listFadingSounds.begin();
		while (it != END_fading)
		{
			(*it).updateFadingEffect(elapsedTime);

			// Supprime l'effet de fading s'il est termin�
			if ((*it).isFadingEnd())
				listFadingSounds.erase(it);	// La fonction "erase" incr�mente automatiquement l'it�rateur
			else
				++it;
		}
	}

	// Indique � IrrKlang la position de l'�couteur et sa direction
	irrKlang->setListenerPosition(listenerPos, listenerLookAt);

	// Modifie la position du son d'ambiance g�n�ral (repr�sentant le vent) pour qu'il ait la m�me position en X et en Z que le listener.
	// Ainsi, son volume 3D ne sera d�termin� que par l'altitude du listener.
	const core::list<ISound*>::ConstIterator END = sounds[ESN_ambiant_atmosphere].sounds.end();
	for (core::list<ISound*>::ConstIterator it = sounds[ESN_ambiant_atmosphere].sounds.begin(); it != END; ++it)
		(*it)->setPosition(vec3df(listenerPos.X, 55.0f * TAILLE_OBJETS, listenerPos.Z));	// PosY : 1100.0f pour TAILLE_OBJETS = 20.0f
}

#endif
