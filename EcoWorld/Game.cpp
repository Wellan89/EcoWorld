#include "Game.h"
#include "CGUIMenuWindow.h"
#include "CGUINotesWindow.h"
#include "CGUIInformationsWindow.h"
#include "CGUIRessourcesWindow.h"
#include "CGUIMonthTableWindow.h"
#include "CGUIObjectivesWindow.h"
#include "CGUIObjectivesModifier.h"
#include "CGUISortTable.h"
#include "CGUICameraAnimatorWindow.h"
#include "CGUIOptionsMenu.h"
#include "CGUIMessageBox.h"
#include "CLoadingScreen.h"
#include "WeatherManager.h"
#include "EcoWorldRenderer.h"
#include "RealisticWater.h"
#include "RTSCamera.h"
#include "Batiment.h"
#include "CShaderPreprocessor.h"
#include "CScreenQuad.h"
#include "CPostProcessManager.h"

#if defined(_DEBUG) && 0
// Macro de d�bogage (strictement aucun effet en mode Release) ajout�e � (pratiquement) chaque fin de ligne (';') dans les fonctions run() et init() de Game.cpp :
// Elle permet de mesurer les temps de chacune des instructions ex�cut�es et affiche un message d'avertissement lorsqu'une instruction d�passe le temps indiqu� (actuellement : 50 ms)
static u32 rtlv = 0;	// Real Timer Last Value
#define DEBUG_TIMER	if (deviceTimer)\
					{\
						const u32 rt = deviceTimer->getRealTime();\
						if (rtlv != 0 && rt > rtlv + 50)\
						{\
							LOG_DEBUG("Debug Timer : Game.cpp : Function at line " << __LINE__ << " : " << rt - rtlv << " ms needed !", ELL_WARNING);\
						}\
						rtlv = rt;\
					}
#else
#define DEBUG_TIMER
#endif

#define TITRE_FENETRE_JEU		L"EcoWorld"		// Le titre de la fen�tre du jeu
#define USE_AUTOMATIQUE_PAUSE					// Permet de mettre automatiquement le jeu en pause lorsque la fen�tre principale n'est plus active
//#define LOCK_CAMERA_WHEN_MOUSE_ON_GUI			// Permet de bloquer la camera si la souris est sur un �l�ment de la GUI
#define CAN_CHANGE_DRAW_MODE					// Permet de changer l'�tat du mode fil de fer et du mode nuage de points pour tous les scenesNodes
#define CAMERA_CAN_JUMP							// Permet � la camera FPS de sauter
//#define SHOW_MONTH_TABLE_EVERY_MONTH			// Si d�fini le tableau r�capitulatif des fins de mois sera automatiquement affich� tous les mois, sinon il sera affich� automatiquement tous les ans

// Effets post-rendu :
#define REAL_TIME_MS_CAMERA_SHAKE_NORMAL_DESTROY	750		// Le temps r�el en millisecondes de tremblement de la cam�ra lorsqu'un b�timent vient d'�tre d�truit (destruction vonlontaire du joueur)
#define REAL_TIME_MS_CAMERA_SHAKE_OLD_DESTROY		1500	// Le temps r�el en millisecondes de tremblement de la cam�ra lorsqu'un b�timent vient d'�tre d�truit (destruction involontaire du joueur : dur�e de vie termin�e)

Game::Game() : currentScene(ECS_MAIN_MENU_LOADING), showFPS(false), hasChangedPauseBeforeShowingFileDialog(false), hasChangedPauseBeforeShowingErrorMessageBox(false),
#ifdef _DEBUG
 unlockAllBatiments(false),
#endif
 hasChangedGammaLevel(false), isPaused(false), device(0), driver(0), sceneManager(0), gui(0), deviceTimer(0), fileSystem(0), cameraRTS(0),
 postProcessManager(0), postProcessRender(true), realTimeMsCameraStopShaking(0), xEffects(0), xEffectsRender(true), shaderPreprocessor(0), screenQuad(0),
 lastDeviceTime(0), elapsedTimeMs(0), lastRealDeviceTime(0), realElapsedTimeMs(0), elapsedTime(0.0f), lastAutoSaveTime(0), currentWeatherID(WI_sunny),
 isMouseOnGUI(false), lockCamera(false), renderer(0), guiManager(0), vitesseDescenteMenuInferieur(0.0f), realTimeMsStartDescenteMenuInferieur(0), canHandleKeysEvent(false)
{
	// R�initialise l'�tat de toutes les touches du clavier
	for (int i = 0; i < KEY_KEY_CODES_COUNT; ++i)
		keys[i] = false;
}
void Game::run()
{
	// Initialise le device et d'autres param�tres importants
	u8 nbTests = 0;	// Le nombre de tentatives effectu�es pour d�marrer Irrlicht
	if (!init())			// D'abord, avec les options de configuration normales
	{
		++nbTests;
		LOG(endl << "AVERTISSEMENT : La cr�ation du device a �chou� avec les options de configuration normales : Nouvel essai avec les options de configuration pr�c�dentes" << endl, ELL_ERROR);DEBUG_TIMER
		
		// D�sactive la fen�tre de confirmation des param�tres du menu options
		gameState.showOptionsConfirmationWindow = false;

		// Conserve l'�tat actuel de l'audio (car il a �t� modifi� avant le red�marrage de Game, et n'est pas responsable de l'�chec du red�marrage)
		const bool lastAudioEnabled = gameConfig.audioEnabled;
		const float lastMainVolume = gameConfig.mainVolume,
			lastMusicVolume = gameConfig.musicVolume,
			lastSoundVolume = gameConfig.soundVolume;

		gameConfig = lastGameConfig;DEBUG_TIMER

		gameConfig.audioEnabled = lastAudioEnabled;
		gameConfig.mainVolume = lastMainVolume;
		gameConfig.musicVolume = lastMusicVolume;
		gameConfig.soundVolume = lastSoundVolume;

		if (!init())		// Ensuite, avec les anciennes options de configuration
		{
			++nbTests;
			LOG(endl << "ERREUR : La cr�ation du device a �chou� avec les anciennes options de configuration : Nouvel essai avec les options de configuration par d�faut" << endl, ELL_ERROR);DEBUG_TIMER

			// Conserve l'�tat actuel de l'audio (car il a �t� modifi� suivant les options actuelles avant le red�marrage de Game, et n'est pas responsable de l'�chec du red�marrage)
			gameConfig.resetDefaultValues(true);DEBUG_TIMER

			if (!init())	// Et enfin, avec les options de configuration par d�faut
			{
				LOG(endl << "ERREUR FATALE : La cr�ation du device a �chou� avec les options de configuration par d�faut : Fin de l'application !" << endl, ELL_ERROR);DEBUG_TIMER

				// Tout a �chou� : on quitte d�finitivement
				return;
			}
		}
	}

	// Affiche un message � l'utilisateur si les options ont d� �tre r�initialis�es car Irrlicht ne pouvait pas d�marrer
	if (nbTests != 0)
	{
		if (gameState.restart)		// Anciennes options de configuration != Options de configuration par d�faut, lorsque le jeu red�marre apr�s la modification des options du jeu
		{
			if (nbTests == 1)		// Anciennes options de configuration
				showErrorMessageBox(L"Echec du d�marrage", L"Impossible de d�marrer EcoWorld avec les param�tres s�lectionn�s :\r\nEcoWorld utilise actuellement les param�tres pr�c�dents !");
			else if (nbTests == 2)	// Options de configuration par d�faut
				showErrorMessageBox(L"Echec du d�marrage", L"Impossible de d�marrer EcoWorld avec les param�tres s�lectionn�s, ni avec les derniers param�tres utilis�s :\r\nEcoWorld utilise actuellement les param�tres par d�faut !");
		}
		else						// Anciennes options de configuration = Options de configuration par d�faut, lorsque le jeu d�marre pour la premi�re fois
			showErrorMessageBox(L"Echec du d�marrage", L"Impossible de d�marrer EcoWorld avec les param�tres s�lectionn�s :\r\nEcoWorld utilise actuellement les param�tres par d�faut !");

		// D�sactive la fen�tre de confirmation des param�tres du menu options si un message d'erreur a �t� affich� :
		// �vite que les deux modal screen ne se chevauchent, emp�chant ainsi toute int�raction avec l'utilisateur
		gameState.showOptionsConfirmationWindow = false;
	}

	// Evite que le jeu ne red�marre ind�finiment d�s qu'il a red�marr� une fois
	gameState.restart = false;DEBUG_TIMER

#ifdef USE_AUTOMATIQUE_PAUSE
	bool hasChangedPaused = false;
#endif

	// Entre dans la boule principale
	while (device->run())
	{
		// Si on a demand� le red�marrage (dans le menu options par exemple), on ferme le device puis on quitte la boucle principale.
		// On attend que la sortie se fasse avec la v�rification de device->run() pour permettre � la fen�tre d'Irrlicht de g�rer ses messages :
		// l'appel � device->closeDevice() semble envoyer un message de fermeture de la fen�tre principale,
		// si device->run() n'est pas appel�, ce message ne sera pas effac�, et il sera envoy� � la prochaine fen�tre d'Irrlicht cr��e : elle se fermera alors imm�diatement
		if (gameState.restart)
		{
			device->closeDevice();
			continue;	// Annule simplement cette it�ration, mais ne quitte pas la boucle actuelle pour effectuer un prochain appel � device->run()
		}

		// D�termine si la fen�tre de confirmation des param�tres du menu options est affich�e
		// Note : Le pointeur guiManager est toujours valide ici, mais ce n'est pas le cas du pointeur guiManager->guiElements.optionsMenuGUI.optionsMenu qui reste invalide si la GUI n'a pas encore �t� initialis�e
		const bool isOptionsConfirmationWindowShown = (guiManager->guiElements.optionsMenuGUI.optionsMenu ? guiManager->guiElements.optionsMenuGUI.optionsMenu->isConfirmationWindowShown() : false);

		// V�rifie que la fen�tre du jeu est active, sinon on met le jeu en pause
		if (device->isWindowActive()
			|| currentScene == ECS_MAIN_MENU_LOADING || currentScene == ECS_LOADING_GAME		// On n'arr�te jamais le device lorsqu'on est dans un chargement
			|| isOptionsConfirmationWindowShown	// On n'arr�te jamais le device lorsque la fen�tre de confirmation des param�tres du menu options est affich�e :
												// dans ce cas, cela pourrait aboutir � un blocage du jeu en cas de probl�me avec l'activation de la fen�tre d'Irrlicht
#ifdef USE_RAKNET
			|| rkMgr.isNetworkPlaying()		// On n'arr�te jamais le device par une simple d�sactivation de la fen�tre lorsqu'on est en pleine partie multijoueur
#endif
			)
		{
#ifdef USE_AUTOMATIQUE_PAUSE
			if (hasChangedPaused)
			{
				// Si on a chang� la pause automatiquement, on la remet � sa valeur normale
				if (isPaused)
					changePause();DEBUG_TIMER
				hasChangedPaused = false;
			}
#endif

			// Met � jour toute la sc�ne puis affiche un rendu � l'�cran si possible
			if (updateAll())
				render();DEBUG_TIMER
		}
		else
		{
#ifdef USE_AUTOMATIQUE_PAUSE
			// On met le jeu en pause s'il ne l'est pas, et on retient qu'on a chang� la pause automatiquement
			if (!isPaused)
			{
				changePause();DEBUG_TIMER
				render();DEBUG_TIMER

				hasChangedPaused = true;DEBUG_TIMER
			}
#endif

			device->yield();DEBUG_TIMER
		}
	}

#ifdef USE_RAKNET
	// Arr�te l'action en cours de RakNet
	rkMgr.reset();DEBUG_TIMER
#endif

	// R�tablit le niveau gamma par d�faut
	resetGammaRamp();DEBUG_TIMER

	// Bug fix : Masque les deux fen�tres d'ouverture et d'enregistrement de partie pour r�initialiser le dossier de travail du file system d'irrlicht
	// (qui sera utilis� plus tard pour enregistrer le fichier de configuration)
	if (guiManager->guiElements.globalGUI.openFileDialog)
		guiManager->guiElements.globalGUI.openFileDialog->hide();
	if (guiManager->guiElements.globalGUI.saveFileDialog)
		guiManager->guiElements.globalGUI.saveFileDialog->hide();

#ifdef USE_IRRKLANG
	// Indique � l'IrrKlang Manager qu'on va d�truire le device
	if (!gameState.keepIrrKlang)
		ikMgr.stopAllSounds();
#endif

	// Indique au syst�me que le renderer va devenir invalide
	system.setSystemRenderer(NULL);

	// Lib�re la cam�ra RTS
	if (cameraRTS)
		cameraRTS->drop();
	cameraRTS = NULL;

	// D�truit XEffects
	if (xEffects)
		delete xEffects;
	xEffects = NULL;

	// D�truit le PostProcessManager
	if (postProcessManager)
		delete postProcessManager;

	// D�truit l'�cran de rendu
	if (screenQuad)
		delete screenQuad;

	// D�truit le pr�processeur de shader
	if (shaderPreprocessor)
		delete shaderPreprocessor;

	// D�truit le GUI Manager
	if (guiManager)
		delete guiManager;
	guiManager = NULL;

	// D�truit le renderer
	if (renderer)
		delete renderer;
	renderer = NULL;

	// D�truit finalement le device
	device->drop();
	device = NULL;
}
bool Game::init()
{
	// Obtient les param�tres de cr�ation du device
	SIrrlichtCreationParameters params = gameConfig.deviceParams;DEBUG_TIMER
	params.EventReceiver = this;DEBUG_TIMER

	if (!device)	// V�rifie tout de m�me que le device n'existe pas d�j�
		device = createDeviceEx(params);DEBUG_TIMER

	if (!device)
	{
		LOG_DEBUG("Game : La creation du device d'Irrlicht a echouee !", ELL_WARNING);DEBUG_TIMER
		LOG_RELEASE("ERREUR : La cr�ation du device d'Irrlicht a �chou�e !", ELL_WARNING);DEBUG_TIMER
		return false;DEBUG_TIMER
	}
	LOG_DEBUG("Game : Creation du device d'Irrlicht reussie" << endl, ELL_INFORMATION);DEBUG_TIMER

	// Indique le titre de la fen�tre du jeu et si elle est redimensionnable
	device->setWindowCaption(TITRE_FENETRE_JEU);DEBUG_TIMER
	device->setResizable(gameConfig.deviceWindowResizable);DEBUG_TIMER

	// Initialise les pointeurs de base
	driver = device->getVideoDriver();DEBUG_TIMER
	sceneManager = device->getSceneManager();DEBUG_TIMER
	gui = device->getGUIEnvironment();DEBUG_TIMER
	deviceTimer = device->getTimer();DEBUG_TIMER
	fileSystem = device->getFileSystem();DEBUG_TIMER

	// R�cup�re les valeurs par d�faut du niveau gamma
	getDefaultGammaRamp();DEBUG_TIMER

	// Change le niveau Gamma du moniteur si n�cessaire
	setNewGammaRamp(gameConfig.gamma);DEBUG_TIMER

	// Indique le dossier de travail par d�faut du device
	if (gameState.workingDirectory.size())
		if (!fileSystem->changeWorkingDirectoryTo(gameState.workingDirectory))
			LOG("AVERTISSEMENT : Impossible de modifier le dossier de travail actuel du device :" << endl
				<< "     - Nouveau dossier de travail : " << gameState.workingDirectory.c_str() << endl
				<< "     - Dossier de travail actuel : " << fileSystem->getWorkingDirectory().c_str(), ELL_WARNING);DEBUG_TIMER

	// Ajoute toutes les archives du jeu
	addGameArchives();DEBUG_TIMER

	// Applique les options de la configuration actuelle du jeu au driver
	applyGameConfigDriverOptions();DEBUG_TIMER

#ifdef USE_JOYSTICKS
	// Active la gestion des manettes et des joysticks
	device->activateJoysticks(joysticksInfos);DEBUG_TIMER

	// Alloue la m�moire n�cessaire pour les �tats des joysticks
	const u32 joysticksCount = joysticksInfos.size();
	joysticksState.reallocate(joysticksCount);
	const SEvent::SJoystickEvent joystickEmpty = {0, {0}, 0, 0};
	for (u32 i = 0; i < joysticksCount; ++i)
		joysticksState.push_back(joystickEmpty);
#endif

	// Cr�e le pr�processeur de shaders
	if (!shaderPreprocessor)
		shaderPreprocessor = new CShaderPreprocessor(fileSystem, driver->getGPUProgrammingServices());

	// Cr�e l'�cran de rendu pour PostProcess et XEffects (doit toujours �tre cr�� puisque au moins PostProcess est toujours initialis�)
	if (!screenQuad)
		screenQuad = new CScreenQuad(driver);

	// Initialise toujours PostProcess pour qu'il charge ses donn�es depuis les fichiers de configuration et permette ainsi au menu options de mettre � jour sa partie effets post-rendu
	if (!postProcessManager)
		postProcessManager = new CPostProcessManager(device, shaderPreprocessor, screenQuad);

	// Cr�e le renderer qui affichera les objets du syst�me (il sera valide aussi longtemps que cette classe)
	if (!renderer)	// V�rifie tout de m�me que le renderer n'existe pas d�j�
	{
		// Demande au s�lecteur de b�timents d'ajouter les pointeurs sur des scene nodes
		core::list<CBatimentSceneNode**> gameSceneNodesPointers;DEBUG_TIMER
		batSelector.pushBackGamePointers(gameSceneNodesPointers);DEBUG_TIMER

		renderer = new EcoWorldRenderer(gameSceneNodesPointers);DEBUG_TIMER
	}

	// Initialise le GUI Manager (il sera valide aussi longtemps que cette classe)
	if (!guiManager)	// V�rifie tout de m�me que le GUI Manager n'existe pas d�j�
		guiManager = new GUIManager();DEBUG_TIMER

	// Indique le renderer de jeu au syst�me (utilis� comme une interface)
	system.setSystemRenderer(renderer);DEBUG_TIMER

	// Cr�e le skin de la GUI et assigne sa transparence
	guiManager->createGUISkin(gameConfig.guiSkinFile, gameConfig.guiTransparency);DEBUG_TIMER

	// Cr�e les GUIs
	guiManager->createGUIs();DEBUG_TIMER

#ifdef USE_IRRKLANG
	// Initialise IrrKlang, et pr�charge les musiques et les sons si l'audio est activ�
	if (gameConfig.audioEnabled)
		ikMgr.init(true);DEBUG_TIMER
#endif



	// D�termine si on doit charger le menu principal ou si on d�marre directement � la sc�ne du chargement du jeu
	bool loadMainMenu = true;DEBUG_TIMER

	// V�rifie qu'on n'a pas de fichier � ouvrir directement au lancement du jeu (par exemple, par double clic sur une partie sauvegard�e ou sur un terrain dans l'explorateur windows)
	if (gameState.fileToOpen.size() > 0)
	{
		// Supprime les guillements �ventuels de la cha�ne (pr�sents lorsque le chemin d'acc�s comporte des espaces par exemple)
		gameState.fileToOpen.remove('\"');DEBUG_TIMER

		// V�rifie tout d'abord que le fichier sp�cifi� existe
		if (fileSystem->existFile(gameState.fileToOpen))
		{
			const int extPos = gameState.fileToOpen.findLast('.');DEBUG_TIMER
			if (extPos >= 0)
			{
				// Obtient l'extension du fichier � 4 caract�res maximum (on supprime aussi le '.' de l'extension)
				const io::path ext = gameState.fileToOpen.subString(extPos + 1, 4);DEBUG_TIMER

				// V�rifie si le fichier � ouvrir est un terrain (si il se termine par l'extension ".ewt")
				if (ext.equals_ignore_case("ewt"))
				{
					// Si c'est un terrain, on cr�e une nouvelle partie de difficult� normale avec ce terrain
					loadMainMenu = createNewGame(EcoWorldModifiers::ED_normal, gameState.fileToOpen);DEBUG_TIMER	// Indique qu'on a besoin de charger le menu principal si la cr�ation de la nouvelle partie a �chou�
				}
				// Sinon, on consid�re automatiquement que le fichier � charger est une partie sauvegard�e
				else
				{
					// Si c'est une partie, on essaie de la charger directement
					loadMainMenu = loadSavedGame(gameState.fileToOpen);DEBUG_TIMER	// Indique qu'on a besoin de charger le menu principal si le chargement de la partie sauvegard�e a �chou�
				}
			}
		}

		// Oublie enfin le fichier � ouvrir, pour �viter de l'ouvrir � nouveau
		gameState.fileToOpen = "";DEBUG_TIMER
	}

	// Si on n'a pas de fichier � ouvrir, ou si son chargement a �chou� : on cr�e le menu principal
	if (loadMainMenu)
		createMainMenu();DEBUG_TIMER

	return true;DEBUG_TIMER
}
void Game::addGameArchives()
{
	// Ajoute le dossier o� se trouve l'executable
	fileSystem->addFileArchive("./", true, true, io::EFAT_FOLDER);

	// Ajoute toutes les archives que le syst�me de fichier peut trouver
	// et v�rifie ensuite dans chacune des nouvelles archives ajout�es s'il n'y trouve pas d'autres archives
	for (u32 i = 0; i < fileSystem->getFileArchiveCount(); ++i)
	{
		const io::IFileList* const fileList = fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;						// V�rifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);			// Obtient le nom actuel du fichier
			const int extPos = fileName.findLast('.');						// Trouve le dernier '.' du nom du fichier, pour v�rifier son extension
			if (extPos < 0)					continue;						// V�rifie que le nom du fichier contient bien un '.'
			const io::path extension = fileName.subString(extPos + 1, 4);	// Obtient l'extension du fichier � 4 caract�res maximum (on supprime aussi le '.' de l'extension)
			if (extension.size() > 3)		continue;						// Tous les tests d'extension ici sont faits avec 3 caract�res ou moins, si la taille de l'extension est sup�rieure, on sait alors que ce n'est pas une archive chargeable ici

			// V�rifie l'extension du fichier pour d�terminer si c'est une archive :

			if (extension.equals_ignore_case(L"ewd"))
			{
				// Archive EWD : EcoWorld Data (en fait : une archive ZIP avec une extension sp�cialement modifi�e pour EcoWorld)
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_ZIP);

				LOG("Archive added (EWD) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"zip") || extension.equals_ignore_case(L"pk3"))
			{
				// Archive ZIP
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_ZIP);

				LOG("Archive added (ZIP) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"gz") || extension.equals_ignore_case(L"tgz"))
			{
				// Archive GZIP
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_GZIP);

				LOG("Archive added (GZIP) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"tar"))
			{
				// Archive TAR
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_TAR);

				LOG("Archive added (TAR) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"pak"))
			{
				// Archive PAK
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_PAK);

				LOG("Archive added (PAK) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"npk"))
			{
				// Archive NPK
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_NPK);

				LOG("Archive added (NPK) : " << fileName.c_str(), ELL_INFORMATION);
			}
			else if (extension.equals_ignore_case(L"wad"))
			{
				// Archive WAD
				fileSystem->addFileArchive(fileName, true, true, io::EFAT_WAD);

				LOG("Archive added (WAD) : " << fileName.c_str(), ELL_INFORMATION);
			}
		}
	}

	LOG("", ELL_INFORMATION);
}
void Game::applyGameConfigDriverOptions()
{
	if (!driver)
		return;

	// Modifie le mat�riau de remplacement du driver :

	// Active le mat�riau de remplacement pour toutes les passes de rendu et pour certains param�tres des mat�riaux
	video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
	overrideMat.Enabled = true;
	overrideMat.EnablePasses = 127;		// ESNRP_CAMERA | ESNRP_LIGHT | ESNRP_SKY_BOX | ESNRP_SOLID | ESNRP_TRANSPARENT | ESNRP_TRANSPARENT_EFFECT | ESNRP_SHADOW = 127

	// V�rifie si XEffects doit g�rer la lumi�re de la sc�ne, ou si on laisse le scene manager la g�rer
	if (gameConfig.useXEffectsShadows && xEffects)
	{
		// XEffects est activ� :

		// Indique les param�tres du mat�riau de remplacement du driver
		overrideMat.EnableFlags = 18187;	// EMF_WIREFRAME | EMF_POINTCLOUD | EMF_LIGHTING | EMF_BILINEAR_FILTER | EMF_TRILINEAR_FILTER | EMF_ANISOTROPIC_FILTER | EMF_ANTI_ALIASING = 18187

		// D�sactive la lumi�re du sc�ne manager
		overrideMat.Material.Lighting = false;
	}
	else
	{
		// XEffects est d�sactiv� :

		// Indique les param�tres du mat�riau de remplacement du driver
		overrideMat.EnableFlags = 18179;	// EMF_WIREFRAME | EMF_POINTCLOUD | EMF_BILINEAR_FILTER | EMF_TRILINEAR_FILTER | EMF_ANISOTROPIC_FILTER | EMF_ANTI_ALIASING = 18179
	}

	// Indique les param�tres du mat�riau de remplacement
	overrideMat.Material.Wireframe = gameConfig.drawWireframe;
	overrideMat.Material.PointCloud = gameConfig.drawPointCloud;
	for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i)	// Pour l'application correcte des param�tres de rendu des textures, la version d'Irrlicht modifi�e pour EcoWorld est n�cessaire : elle r�sout un bug qui emp�chait son application
	{
		overrideMat.Material.TextureLayer[i].BilinearFilter = gameConfig.bilinearFilterEnabled;
		overrideMat.Material.TextureLayer[i].TrilinearFilter = gameConfig.trilinearFilterEnabled;
		overrideMat.Material.TextureLayer[i].AnisotropicFilter = gameConfig.anisotropicFilter;
	}
	// TODO : Pouvoir aussi activer l'anticr�nelage sp�cifique pour les mat�riaux transparents (EAAM_ALPHA_TO_COVERAGE, quand le type du mat�riau est EMT_TRANSPARENT_ALPHA_CHANNEL_REF)
	overrideMat.Material.AntiAliasing = gameConfig.antialiasingMode;



	// Modifie aussi les param�tres de cr�ation des textures du driver :

	// Indique au driver qu'il doit cr�er des textures de la meilleure qualit� possible si la qualit� des textures est r�gl�e sur Haute
	if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, true);
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
	}
	// Sinon, indique au driver qu'il doit cr�er des textures ayant un temps de rendu le plus petit possible si la qualit� des textures est r�gl�e sur Tr�s Basse
	else if (gameConfig.texturesQuality == GameConfiguration::ETQ_VERY_LOW)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);

		// Le param�tre ETCF_OPTIMIZED_FOR_SPEED provoque des bugs avec OpenGL lors de la cr�ation de textures qui ont une taille de 1537x1537 :
		/*
			- BUG : Terrains : Crash chargement terrains "Beach City" et "Hill City" en OpenGL en qualit� de texture tr�s basse seulement (-> ETCF_OPTIMIZED_FOR_SPEED activ�) ! (Vient de la taille 1537x1537 de la texture !)
				-> OpenGL + ETCF_OPTIMIZED_FOR_SPEED activ� + Texture size = 1537x1537   =>   driver->getTexture Crash !
				=> Voir COpenGLTexture.cpp : Ligne 306
		*/
		// On n'active donc pas l'optimisation de vitesse pour ce driver
		if (driver->getDriverType() != EDT_OPENGL)
			driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, true);

		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
	}
	// Sinon, indique au driver qu'il doit cr�er des textures en mode 16 bits si on est en mode plein �cran avec une profondeur de couleur de 16 bits ou moins
	// (les profondeurs de couleur ne sont valables qu'en plein �cran ; et les textures 32 bits sont inutiles en mode 16 bits)
	else if (gameConfig.deviceParams.Bits <= 16 && gameConfig.deviceParams.Fullscreen)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, true);
	}
}
bool Game::updateAll()
{
	// Calcule le temps �coul� depuis la derni�re frame
	calculateElapsedTime();

#ifdef USE_IRRKLANG
	// Met � jour la position de la cam�ra d'IrrKlang
	if (cameraRTS)
		ikMgr.update(cameraRTS->getAbsolutePosition(), cameraRTS->getTarget(), elapsedTime);
	else
		ikMgr.update(core::vector3df(0.0f), core::vector3df(1.0f), elapsedTime);
#endif

#ifdef USE_RAKNET
	// Met � jour RakNet et le jeu avec les messages re�us du r�seau :
	// On ne met pas � jour RakNet si on attend des informations pour le chargement du jeu :
	// cela signifie que c'est la fonction "Game::joinMultiplayerGame" qui nous a appel�, et qui g�re donc RakNet et ses messages actuellement
	if (rkMgr.getCurrentNetworkState() != RakNetManager::ENGS_WAITING_FOR_GAME_DATA)
		updateRakNetGameMessages();
#endif

	// Actualisation de la sc�ne du jeu d�sactiv�e si le menu �chap est affich� ou si le menu d'enregistrement/chargement de partie est affich�
	if (currentScene == ECS_PLAY && !guiManager->guiElements.gameGUI.menuWindow->isVisible()
		&& !guiManager->guiElements.globalGUI.openFileDialog->isVisible() && !guiManager->guiElements.globalGUI.saveFileDialog->isVisible())
	{
		// Met � jour la fen�tre d'animation de la cam�ra RTS (doit �tre fait � chaque frame de la sc�ne du jeu, m�me lorsque cette fen�tre n'est pas visible)
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->udpate();

		// Evite d'effectuer certaines mises � jour inutiles lorsque le jeu est en pause (mises � jour seulement d�pendantes du timer du jeu)
		if (!isPaused)
		{
#ifdef SHOW_MONTH_TABLE_EVERY_MONTH
			// Retiens le nombre de mois actuel
			const u32 lastMois = system.getTime().getTotalMois();
#else
			// Retiens le nombre d'ann�es actuel
			const u32 lastAnnees = system.getTime().getAnnees();
#endif

			// Actualise le monde, en se souvenant si un b�timent a �t� d�truit ou non
			bool normalBatimentDestroy = false, oldBatimentDestroy = false;
			system.update(elapsedTime, &normalBatimentDestroy, &oldBatimentDestroy);

			// Indique que la cam�ra doit trembler si un b�timent vient d'�tre d�truit (uniquement si les effets post-rendu sont activ�s)
			if (gameConfig.usePostProcessEffects && gameConfig.postProcessShakeCameraOnDestroying && postProcessManager && (normalBatimentDestroy || oldBatimentDestroy))
			{
				// D�termine le temps r�el auquel l'effet de tremblement de la cam�ra devra s'arr�ter
				realTimeMsCameraStopShaking = deviceTimer->getRealTime() + (oldBatimentDestroy ? REAL_TIME_MS_CAMERA_SHAKE_OLD_DESTROY : REAL_TIME_MS_CAMERA_SHAKE_NORMAL_DESTROY);
			}

			// V�rifie si le jeu est termin�
			if (system.isGameFinished())
			{
				// V�rifie si le joueur a perdu
				if (system.isGameLost())
				{
					// Si le joueur a perdu, on change de sc�ne jusqu'� l'�cran lui indiquant qu'il a perdu
					switchToNextScene(ECS_GAME_LOST);
					return false;
				}
				// V�rifie si le joueur a gagn�
				else if (system.isGameWon())
				{
					// Si le joueur a gagn�, on change de sc�ne jusqu'� l'�cran lui indiquant qu'il a gagn�
					switchToNextScene(ECS_GAME_WON);
					return false;
				}
			}

			// V�rifie si le temps pour la sauvegarde automatique est atteint : dans ce cas, on l'effectue
			if (gameConfig.autoSaveFrequency != 0
				&& deviceTimer->getTime() >= lastAutoSaveTime + (gameConfig.autoSaveFrequency * 1000))
				autoSave();

#ifdef SHOW_MONTH_TABLE_EVERY_MONTH	// Affichage tous les mois
			// Si le nombre de mois a chang�, on affiche le tableau r�capitulatif des fins de mois et on lui donne le focus
			if (system.getTime().getTotalMois() != lastMois && guiManager->guiElements.gameGUI.monthTableWindow)
#else								// Affichage tous les ans
			// Si le nombre d'ann�es a chang�, on affiche le tableau r�capitulatif des fins de mois et on lui donne le focus
			if (system.getTime().getAnnees() != lastAnnees && guiManager->guiElements.gameGUI.monthTableWindow)
#endif
			{
				if (!guiManager->guiElements.gameGUI.monthTableWindow->isVisible())
				{
					guiManager->guiElements.gameGUI.monthTableWindow->setVisible(true);
#ifdef USE_IRRKLANG
					// Joue le son d'ouverture d'un menu
					ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_OPEN);
#endif
				}

				// Donne le focus � cette fen�tre
				gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);
			}

			// Actualise le renderer (d�pendant de la pause du jeu car il ne met � jour que le temps du monde, qui est d�pendant de la mise � jour du syst�me)
			renderer->update();
		}

		// N'actualise la GUI que si on n'est pas en mode cam�ra FPS et que la cam�ra RTS n'est pas anim�e par le module d'animation personnalis�e
		if (!cameraRTS->getIsCameraFPSEnabled() && !guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
		{
			// Actualise la GUI du jeu si elle est visible
			if (guiManager->isGUIVisible(GUIManager::EGN_gameGUI))
				updateGameGUI();

			// Actualise le placement et la s�lection des b�timents
			batSelector.update();
		}
	}
	else if (currentScene != ECS_MAIN_MENU_LOADING && currentScene != ECS_LOADING_GAME)
	{
		// Actualise le menu principal et ses sous-menus

		// Met � jour le temps du syst�me de jeu pour le menu principal
		system.updateMainMenu(elapsedTime);

		// Met � jour le titre du menu principal si le temps a chang�
		if (currentScene == ECS_MAIN_MENU && system.getWeatherManager().getCurrentWeatherID() != currentWeatherID)
		{
			guiManager->chooseMainMenuTitleFromWeather(system.getWeatherManager().getCurrentWeatherID(), driver->getScreenSize());
			currentWeatherID = system.getWeatherManager().getCurrentWeatherID();
		}

		// Actualise le renderer avec le temps du syst�me de jeu
		renderer->update();
	}

	return true;
}
void Game::render(bool drawSceneManager, bool drawGUI, bool reinitScene)
{
	// R�initialise la sc�ne du driver : restaure le viewport et commence une nouvelle sc�ne
	if (reinitScene)
	{
		const core::dimension2du& screenSize = driver->getScreenSize();
		driver->setViewPort(core::recti(0, 0, screenSize.Width, screenSize.Height));

		driver->beginScene();
	}

	if (drawSceneManager)
	{
#ifdef _DEBUG
		{
			// Active le mat�riau de remplacement du driver si jamais il a �t� d�sactiv�
			video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
			if (!overrideMat.Enabled)
			{
				LOG_DEBUG("Game::render(...) : Le materiau de remplacement du driver n'est pas active : overrideMat.Enabled = " << overrideMat.Enabled << " !", ELL_WARNING);
				overrideMat.Enabled = true;
			}
		}
#endif

		// Pr�pare les effets post-rendu (uniquement si un nouveau rendu vient de commencer : si reinitScene est � true)
		const irr::core::array<core::stringw>& postProcessEffects = gameConfig.postProcessEffects;
		const u32 postProcessEffectsCount = postProcessEffects.size();
		const bool renderShakeCamera = (gameConfig.postProcessShakeCameraOnDestroying && deviceTimer->getRealTime() < realTimeMsCameraStopShaking);
		const bool renderPostProcess = (gameConfig.usePostProcessEffects && (postProcessEffectsCount || renderShakeCamera || true) && postProcessManager && reinitScene && postProcessRender);
		video::ITexture* renderTarget = NULL;
		if (renderPostProcess)
			renderTarget = postProcessManager->prepare();

		// Pr�pare le rendu de l'eau avec shaders
		RealisticWaterSceneNode* const shaderWaterSceneNode = renderer->getTerrainManager().getShaderWaterSceneNode();
		if (shaderWaterSceneNode)
		{
			// Retiens le dernier �tat du viewport du driver, car il est susceptible d'�tre modifi� par le pr�-rendu de l'eau avec shaders
			const core::recti lastViewPort = game->driver->getViewPort();

			shaderWaterSceneNode->preRender(renderTarget);

			// Si on ne doit pas conserver l'�tat du driver, on veille � ce que son viewport soit bien restaur� 
			if (!reinitScene)
				driver->setViewPort(lastViewPort);
		}

		// V�rifie si XEffects est activ� : dans ce cas, son appel remplace l'appel � sceneManager->drawAll() !
		if (gameConfig.useXEffectsShadows && xEffects && xEffectsRender)
			xEffects->update(renderTarget);	// Sp�cifie aussi la texture RTT dans laquelle effectuer le rendu (n�cessaire si les effets post-rendu sont activ�s)
		else	// Sinon, demande directement au sc�ne manager de r�aliser un rendu
			sceneManager->drawAll();

#if 1
		// Dessine la grille du terrain
		// (Note : On admettra que ce syst�me fonctionne parfaitement, que TAILLE_CARTE soit pair ou non)
		if (currentScene == ECS_PLAY && keys[KEY_KEY_G] && canHandleKeysEvent)		// Un appui sur la touche G en mode d�bogage permet d'afficher la grille du terrain (on v�rifie aussi qu'on peut g�rer les touches du clavier)
		{
			static const int HALF_TAILLE_CARTE = (int)(TAILLE_CARTE * 0.5f);

			SMaterial material;
			material.ZBuffer = video::ECFN_ALWAYS;
			material.ZWriteEnable = false;
			material.Lighting = false;
			driver->setMaterial(material);
			driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

			video::SColor lineColor(255, 0, 0, 255);

			// Horizontales et verticales
			for (int x = 0; x <= TAILLE_CARTE; ++x)	// Conserver le '<=', car il y a TAILLE_CARTE + 1 horizontales � tracer, et autant de verticales
			{
				driver->draw3DLine(
					core::vector3df((x - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, -HALF_TAILLE_CARTE * TAILLE_OBJETS),
					core::vector3df((x - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, HALF_TAILLE_CARTE * TAILLE_OBJETS),
					lineColor);
				driver->draw3DLine(
					core::vector3df(-HALF_TAILLE_CARTE * TAILLE_OBJETS, 0.0f, (x - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
					core::vector3df(HALF_TAILLE_CARTE * TAILLE_OBJETS, 0.0f, (x - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
					lineColor);
			}

			// Diagonales
			for (int x = 0; x < TAILLE_CARTE; ++x)
			{
				for (int y = 0; y < TAILLE_CARTE; ++y)
				{
					// D�fini la couleur suivant la constructibilit� du terrain et si il est situ� en eau profonde
					lineColor.set(255,
						system.carte[x][y].constructible ? 0 : 255,
						(system.carte[x][y].constructible && !system.carte[x][y].deepWater) ? 255 : 0,
						system.carte[x][y].deepWater ? 255 : 0);

					driver->draw3DLine(
						core::vector3df((x - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, (y - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
						core::vector3df((x + 1 - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, (y + 1 - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
						lineColor);
					driver->draw3DLine(
						core::vector3df((x - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, (y + 1 - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
						core::vector3df((x + 1 - HALF_TAILLE_CARTE) * TAILLE_OBJETS, 0.0f, (y - HALF_TAILLE_CARTE) * TAILLE_OBJETS),
						lineColor);
				}
			}
		}
#endif

		// Affiche tous les effets post-rendu
		if (renderPostProcess)
		{
			// Effectue le rendu de profondeur si n�cessaire
			if (gameConfig.postProcessUseDepthRendering)
				postProcessManager->renderDepth(gameConfig.postProcessDefaultDepth);

			// Pr�pare le driver pour les rendus du screen quad que demandera PostProcess
			screenQuad->preRender();

			// Rendu de chacun des effets
			for (u32 i = 0; i < postProcessEffectsCount; ++i)
				postProcessManager->render(postProcessEffects[i]);

			// Dessine l'effet de tremblement de l'�cran si n�cessaire
			if (renderShakeCamera)
				postProcessManager->render(EFFECT_NAME_SHAKE);

			// Effectue un rendu final des effets sur l'�cran
			postProcessManager->update();

			// Termine le rendu du screen quad
			screenQuad->postRender();
		}
	}
	if (drawGUI)
	{
		// Dessine la GUI d'Irrlicht
		gui->drawAll();

#ifdef CAN_BUILD_WITH_SELECTION_RECT
		// Dessine les contours du rectangle de s�lection
		const core::recti& rectSelection = batSelector.getRectSelection();
		if (rectSelection != core::recti(-1, -1, -1, -1))
		{
			const video::SColor rectColor(255, 255, 255, 255);

			core::vector2di tmpPoint(rectSelection.LowerRightCorner.X, rectSelection.UpperLeftCorner.Y);
			driver->draw2DLine(rectSelection.UpperLeftCorner, tmpPoint, rectColor);
			driver->draw2DLine(tmpPoint, rectSelection.LowerRightCorner, rectColor);

			tmpPoint.set(rectSelection.UpperLeftCorner.X, rectSelection.LowerRightCorner.Y);
			driver->draw2DLine(rectSelection.UpperLeftCorner, tmpPoint, rectColor);
			driver->draw2DLine(tmpPoint, rectSelection.LowerRightCorner, rectColor);
		}
#endif

		if (showFPS) // Affiche les infos en bas de l'�cran
		{
			gui::IGUIFont* const font = gui->getSkin()->getFont();
			if (font)
			{
				swprintf_SS(L"%ls     %3d FPS     %0.3f x10^3 triangles this frame     %0.3f x10^3 triangles average (every 1.5s)     %0.3f x10^6 triangles drawn (total)",
					driver->getName(), driver->getFPS(),
					(float)driver->getPrimitiveCountDrawn(0) * 0.001f,		// Triangles dessin�s cette frame
					(float)driver->getPrimitiveCountDrawn(1) * 0.001f,		// Moyenne des triangles dessin�s toutes les 1.5s
					(float)driver->getPrimitiveCountDrawn(2) * 0.000001f);	// Total de triangles dessin�s par ce driver

				// En bas � gauche mais juste au-dessus du menu de construction
				const core::dimension2du& screenSize = driver->getScreenSize();
				if (currentScene == ECS_PLAY && guiManager->guiElements.gameGUI.tabGroupBas && guiManager->isGUIVisible(GUIManager::EGN_gameGUI))
					font->draw(textW_SS,
						core::recti(10, guiManager->guiElements.gameGUI.tabGroupBas->getAbsolutePosition().UpperLeftCorner.Y - 20, screenSize.Width, screenSize.Height),
						video::SColor(255, 255, 255, 255));
				else
					font->draw(textW_SS,
						core::recti(10, screenSize.Height - 30, screenSize.Width, screenSize.Height),
						video::SColor(255, 255, 255, 255));
			}
		}
	}

	if (reinitScene)
		driver->endScene();
}
void Game::resetGame(bool resetGameAndSceneManager, bool keepCamera, bool stopIrrKlang)
{
	if (resetGameAndSceneManager)
	{
#ifdef USE_RAKNET
		// Stoppe l'action en cours de RakNet, sauf s'il est en train de charger les donn�es pour la partie mutlijoueur (dans ce cas, il faut le conserver connect� au serveur durant son chargement)
		if (rkMgr.getCurrentNetworkState() != RakNetManager::ENGS_WAITING_FOR_GAME_DATA)	// Cela signifie qu'on a �t� appel� par joinMultiplayerGame, qui d�sire conserver RakNet en l'�tat
			rkMgr.reset();
#endif

		// Stoppe le timer tant qu'il reste des starts (ils sont compt�s)
		while (!deviceTimer->isStopped()) deviceTimer->stop();

		// Red�marre le timer tant qu'il reste des stops (ils sont compt�s)
		while (deviceTimer->isStopped()) deviceTimer->start();

		// Stoppe le timer par d�faut
		deviceTimer->stop();

		// Remet le temps � z�ro
		deviceTimer->setTime(0);

		// Remet la vitesse normale
		deviceTimer->setSpeed(1.0f);

		// Remet le temps �coul� � z�ro
		lastDeviceTime = 0;
		elapsedTimeMs = 0;
		lastRealDeviceTime = 0;
		realElapsedTimeMs = 0;
		elapsedTime = 0.0f;

		// Remet le syst�me de jeu � z�ro
		system.reset();

		// Arr�te l'animation de la cam�ra RTS et r�initialise l'animation personnalis�e
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			if (guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
				guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

			guiManager->guiElements.gameGUI.cameraAnimatorWindow->reset();
		}

		// Grab la cam�ra RTS pour la conserver si n�cessaire (la cam�ra FPS est automatiquement conserv�e, comme enfant de la cam�ra RTS)
		if (keepCamera && cameraRTS)
			cameraRTS->grab();

		// Efface tous les �l�ments du scene manager
		sceneManager->clear();

		// Attache � nouveau la cam�ra RTS au scene manager puis la droppe une derni�re fois
		if (keepCamera && cameraRTS)
		{
			cameraRTS->setParent(sceneManager->getRootSceneNode());
			cameraRTS->drop();

			// D�bloque la cam�ra RTS si elle �tait bloqu�e
			cameraRTS->setLockCamera(false);

			// Indique au scene manager que la cam�ra RTS est la cam�ra actuelle
			sceneManager->setActiveCamera(cameraRTS);
		}
		else	// Si on ne doit pas les conserver, on r�initialise son pointeur car il est maintenant invalide
			cameraRTS = NULL;

		// R�initialise l'effet de flou de profondeur de PostProcess
		if (postProcessManager)
			postProcessManager->clearDepthPass();
		realTimeMsCameraStopShaking = 0;

		// R�initialise XEffects
		if (xEffects)
			xEffects->reset();

		if (renderer)
		{
			// Remet le renderer � z�ro
			renderer->reset();

#ifdef USE_SPARK
			// Restaure la vitesse du timer de Spark
			renderer->getSparkManager().setTimerSpeed(1.0f);
#endif
		}

		// R�initialise le s�lecteur de b�timents
		batSelector.reset();

		// Remet tous les pointeurs � NULL et d�sactive les �l�ments de la jouabilit� et du syst�me
		lastAutoSaveTime = 0;
		isPaused = false;
		lockCamera = false;

		// Cette valeur n�cessite d'�tre r�initialis�e avant le passage � la sc�ne du jeu car elle n'est mise � jour que dans celle-ci (voir Game::OnEvent())
		isMouseOnGUI = false;

		// Fonctions sp�ciales valides dans toutes les sc�nes du jeu, donc non r�initialis�es :
		//showFPS = false;
		//drawWireframe = false;
		//drawPointCloud = false;
	}

	/*
	if (resetGUI)
	{
		// Remet � z�ro la GUI
		guiManager->resetGUI();

		// Remet les indications sur la GUI � leurs valeurs par d�faut
		isMouseOnGUI = false;
		hasChangedPauseBeforeShowingFileDialog = false;
		hasChangedPauseBeforeShowingErrorMessageBox = false;
		canHandleKeysEvent = false;
		vitesseDescenteMenuInferieur = 0.0f;
		realTimeMsStartDescenteMenuInferieur = 0;
		currentWeatherID = WI_sunny;

		// Recr�e le skin de la GUI
		guiManager->createGUISkin(gameConfig.guiSkinFile, gameConfig.guiTransparency);

		// Recr�e compl�tement la GUI
		guiManager->createGUIs(isPaused, gameConfig, fileSystem, deviceTimer, this, sceneManager, renderer, system);
	}
	*/

#ifdef USE_IRRKLANG
	if (stopIrrKlang)
	{
		// Stoppe tous les sons (mais ne les supprime pas)
		ikMgr.stopAllSounds();
	}
#endif
}
void Game::calculateElapsedTime()
{
	/*
	elapsedTimeMs = 0.0f;

	if (isPaused && !deviceTimer->isStopped()) // Stoppe le timer si on est en pause et qu'il n'a pas �t� stopp�
	{
		deviceTimer->stop();

		LOG_DEBUG("Game::calculerElapsedTime() : Timer non stoppe alors que la pause est activee !", ELL_WARNING);
	}
	if (deviceTimer->isStopped())
		return;
	*/

	// Temps du timer �coul�
	const u32 currentDeviceTime = deviceTimer->getTime();
	elapsedTimeMs = currentDeviceTime - lastDeviceTime;
	lastDeviceTime = currentDeviceTime;
	elapsedTime = ((float)elapsedTimeMs * 0.001f);

	// Temps �coul� r�el
	const u32 currentRealDeviceTime = deviceTimer->getRealTime();
	realElapsedTimeMs = currentRealDeviceTime - lastRealDeviceTime;
	lastRealDeviceTime = currentRealDeviceTime;

	// Remet elapsedTime dans les limites : 0.001f <= elapsedTime <= 5.0f
	//elapsedTime = core::clamp(elapsedTime, 0.001f, 5.0f);
}
bool Game::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		if (event.KeyInput.PressedDown)
			keys[event.KeyInput.Key] = true;
		else
			keys[event.KeyInput.Key] = false;

		keys[KEY_SHIFT] = event.KeyInput.Shift;
		keys[KEY_CONTROL] = event.KeyInput.Control;
	}
	else if (event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		const core::dimension2du& screenSize = driver->getScreenSize();
		mouseState.position = core::vector2di(event.MouseInput.X, event.MouseInput.Y);
		mouseState.positionF = core::vector2df((float)mouseState.position.X / (float)screenSize.Width, (float)mouseState.position.Y / (float)screenSize.Height);
		mouseState.leftButton = event.MouseInput.isLeftPressed();
		mouseState.middleButton = event.MouseInput.isMiddlePressed();
		mouseState.rightButton = event.MouseInput.isRightPressed();

		if (event.MouseInput.Event == EMIE_MOUSE_WHEEL)
			mouseState.wheel += event.MouseInput.Wheel;
		else if (event.MouseInput.Event == EMIE_MOUSE_MOVED)
		{
#ifdef LOCK_CAMERA_WHEN_MOUSE_ON_GUI
			// Recalcule si la souris est sur un �lement de la GUI
			isMouseOnGUI = calculerIsMouseOnGUI();

			bool lockCam = lockCamera;
			lockCam |= isMouseOnGUI;

			// On bloque la camera RTS si la souris est sur un �l�ment de la GUI ou si l'utilisateur a demand� de la bloquer
			if (cameraRTS)
				cameraRTS->setLockCamera(lockCam);
#else
			// Recalcule si la souris est sur un �lement de la GUI (seulement utilis� lorsque la sc�ne actuelle est ECS_PLAY)
			if (currentScene == ECS_PLAY)
				isMouseOnGUI = calculerIsMouseOnGUI();
#endif
		}
	}
#ifdef USE_JOYSTICKS
	else if (event.EventType == EET_JOYSTICK_INPUT_EVENT)
	{
		// Copie l'�tat du joystick
		if (event.JoystickEvent.Joystick < joysticksState.size())
			joysticksState[event.JoystickEvent.Joystick] = event.JoystickEvent;
	}
#endif

	// V�rifie si l'event est pas un texte de log (on affiche les donn�es de log m�me si le device n'a pas encore �t� initialis�, puisqu'il n'est pas n�cessaire)
	if (event.EventType == EET_LOG_TEXT_EVENT)
		return OnEventLog(event);

	if (!device || !guiManager || !renderer)	// Si le device, le gui manager ou le renderer n'ont pas encore �t� initialis�s, on quitte
		return false;

	// V�rifie si on peut g�rer les �v�nements du clavier ou non
	canHandleKeysEvent =
		(guiManager->guiElements.gameGUI.notesWindow ? !guiManager->guiElements.gameGUI.notesWindow->isTextFocused() : true)									// Le texte de notes a-t-il le focus ?
		&& (guiManager->guiElements.gameGUI.cameraAnimatorWindow ? !guiManager->guiElements.gameGUI.cameraAnimatorWindow->hasKeyFocus() : true)					// La fen�tre d'animation de la cam�ra a-t-elle le focus ?
		&& (guiManager->guiElements.newGameMenuGUI.objectivesModifier ? !guiManager->guiElements.newGameMenuGUI.objectivesModifier->isEditBoxFocused() : true)	// L'edit box des objectifs a-t-il le focus ?
		&& (guiManager->guiElements.globalGUI.openFileDialog ? !guiManager->guiElements.globalGUI.openFileDialog->isVisible() : true)							// La bo�te de dialogue d'ouverture de partie a-t-elle le focus ?
		&& (guiManager->guiElements.globalGUI.saveFileDialog ? !guiManager->guiElements.globalGUI.saveFileDialog->isVisible() : true);							// La bo�te de dialogue d'enregistrement de partie a-t-elle le focus ?

	const bool isOpenFileDialogShown = (guiManager->guiElements.globalGUI.openFileDialog ? guiManager->guiElements.globalGUI.openFileDialog->isVisible() : false);
	const bool isSaveFileDialogShown = (guiManager->guiElements.globalGUI.saveFileDialog ? guiManager->guiElements.globalGUI.saveFileDialog->isVisible() : false);

	if ((currentScene == ECS_MAIN_MENU || currentScene == ECS_OPTIONS_MENU || currentScene == ECS_NEW_GAME_MENU
#ifdef USE_RAKNET
		|| currentScene == ECS_MULTIPLAYER_MENU
#endif
		|| currentScene == ECS_GAME_LOST|| currentScene == ECS_GAME_WON)
		&& event.EventType == EET_GUI_EVENT && !isOpenFileDialogShown && !isSaveFileDialogShown
#ifdef ASK_BEFORE_QUIT
		&& !guiManager->guiElements.mainMenuGUI.quitterWindow
#endif
		&& !guiManager->guiElements.globalGUI.errorMessageBox)
	{
#ifdef USE_IRRKLANG
		// Envoie cet �v�nement � l'IrrKlangManager avant qu'il soit �t� tra�t� par la fonction OnEventMenus :
		// En effet, si l'�v�nement est le clic du joueur sur le bouton pour commencer une partie,
		// il faut qu'on entende le son du clic avant le chargement du jeu (command� par OnEventMenus), et non pas apr�s
		ikMgr.playGUISound(event);
#endif
		return OnEventMenus(event);
	}

	// Seul la m�thode OnEventGUI est capable de g�rer les �venements si une fen�tre est affich�e
	if (event.EventType == EET_GUI_EVENT || isOpenFileDialogShown || isSaveFileDialogShown || guiManager->guiElements.globalGUI.errorMessageBox
		|| guiManager->guiElements.gameGUI.menuWindow->isVisible()
#ifdef ASK_BEFORE_QUIT
		|| guiManager->guiElements.mainMenuGUI.quitterWindow
#endif
		)
	{
#ifdef USE_IRRKLANG
		bool sendEventToIrrKlangMgr = true;
		const bool toReturn = OnEventGUI(event, sendEventToIrrKlangMgr);
		if (sendEventToIrrKlangMgr)	// V�rifie qu'on peut toujours envoyer cet �v�nement � l'IrrKlangManager
			ikMgr.playGUISound(event);
		return toReturn;
#else
		return OnEventGUI(event);
#endif
	}

	// Si l'�v�nement a lieu durant la sc�ne du jeu
	if (currentScene == ECS_PLAY)
	{
		// Envoie cet �v�nement au s�lecteur de b�timents
		if (batSelector.OnEvent(event))	// Si l'�v�nement a �t� g�r�, on n'a plus � le faire
			return true;

		// Envoie cet �v�nement au syst�me de jeu
		if (OnEventGame(event))			// Si l'�v�nement a �t� g�r�, on n'a plus � le faire
			return true;
	}

	// On ne g�re ici que les entr�es du clavier
	if (event.EventType != EET_KEY_INPUT_EVENT)
		return false;

	// Ici, on g�re les �v�nements du clavier qui sont valables pendant n'importe quelle sc�ne du jeu

	if (event.KeyInput.Key == KEY_ESCAPE)						// G�re l'appui sur Echap
		return onEchap(event.KeyInput.PressedDown);	// ECHAP
	if (event.KeyInput.Key == KEY_RETURN && canHandleKeysEvent)	// G�re l'appui sur Entr�e (sauf si une EditBox a le focus : dans ce cas, les appuis sur Entr�e seront g�r�es par celle-ci par l'envoi d'�v�nements EGET_EDITBOX_ENTER)
		return onEnter(event.KeyInput.PressedDown);	// ENTREE
	else if ((event.KeyInput.Key == KEY_SNAPSHOT || event.KeyInput.Key == KEY_F12) && !event.KeyInput.PressedDown)
	{
		// Print Screen ou F12 (anciennement : F9) :
		// Cr�e une capture d'�cran

		const bool isTimerStarted = !(deviceTimer->isStopped());
		if (isTimerStarted)
			deviceTimer->stop(); // Stoppe le timer s'il ne l'est pas

		createScreenShot();

		if (isTimerStarted)
			deviceTimer->start(); // Redemarre le timer si on l'a stopp�
	}
	else if ((event.KeyInput.Key == KEY_NUMPAD0 || event.KeyInput.Key == KEY_KEY_0)
		&& event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + 0 :
		// Active ou d�sactive l'affichage des FPS � l'�cran

		showFPS = !showFPS;
	}
	else if (event.KeyInput.Key == KEY_KEY_E && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + E :
		// Active ou d�sactive les effets post-rendu (PostProcess)

		postProcessRender = !postProcessRender;
	}
	else if (event.KeyInput.Key == KEY_KEY_X && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + X :
		// Active ou d�sactive le rendu des ombres (XEffects)

		xEffectsRender = !xEffectsRender;
	}
#ifdef CAN_CHANGE_DRAW_MODE
	else if (event.KeyInput.Key == KEY_KEY_W && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + W :
		// Met tout en mode fil de fer (et enl�ve le mode nuage de points)

		gameConfig.drawWireframe = !(gameConfig.drawWireframe);
		gameConfig.drawPointCloud = false;

		applyGameConfigDriverOptions();
	}
	else if (event.KeyInput.Key == KEY_KEY_P && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + P :
		// Met tout en mode nuage de points (et enl�ve le mode fil de fer)

		gameConfig.drawWireframe = false;
		gameConfig.drawPointCloud = !(gameConfig.drawPointCloud);

		applyGameConfigDriverOptions();
	}
#endif

	// Outils de d�bogage
#ifdef _DEBUG
	else if (event.KeyInput.Key == KEY_KEY_T && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + T :
		// Demande � l'utilisateur de choisir un nouveau temps

		int weatherID = (int)WI_sunny;
		cout << "Choisissez le nouveau temps (entre 0 et " << ((int)WI_COUNT - 1) << ") : ";
		cin >> weatherID;

		weatherID = core::clamp(weatherID, 0, (int)WI_COUNT - 1);

		cout << "    Transition : " << (event.KeyInput.Shift ? "Desactivee" : "Activee") << endl;

		// La touche "Shift" permet de choisir si on utilise une transition ou non
		if (event.KeyInput.Shift)
			system.getWeatherManager().setWeather((WeatherID)weatherID);		// Change le temps sans transition
		else
			system.getWeatherManager().changeWeather((WeatherID)weatherID);		// Change le temps avec une transition
	}
#endif

	return false;
}
bool Game::OnEventLog(const SEvent& event)
{
	if (event.EventType != EET_LOG_TEXT_EVENT)
		return false;
	if (!event.LogEvent.Text)
		return false;

	// Ici, on g�re les �v�nements envoy�s par le logger d'Irrlicht lorsqu'un texte de log demande � �tre affich�

	// Envoie le texte de log dans le logger d'EcoWorld
	// (permet ainsi de rediriger les logs d'Irrlicht dans le fichier de log d'EcoWorld en cas d'absence de console)
	LOG(event.LogEvent.Text, event.LogEvent.Level);

	// Evite qu'Irrlicht envoie � son tour le texte de log dans la console :
	return true;
}
bool Game::OnEventMenus(const SEvent& event)
{
	if (event.EventType != EET_GUI_EVENT || !guiManager)
		return false;

	// Ici, on g�re les �v�nements de la GUI qui ont lieu pendant les sc�nes de menu (menu principal, menu options, menu de cr�ation d'une nouvelle partie, menu de jeu termin�)

	// V�rifie qu'un bouton a bien �t� cliqu�
	if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.newGameBouton)
		{
			// Avance � la sc�ne pour cr�er une nouvelle partie
			switchToNextScene(ECS_NEW_GAME_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.chargerBouton)
		{
			// Affiche la bo�te de dialogue pour charger un fichier,
			// en prenant un modal screen parent pour qu'il puisse avoir acc�s aux �v�nements, mais pas les autres boutons du menu principal
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.optionsBouton)
		{
			// Change de sc�ne pour passer au menu des options
			switchToNextScene(ECS_OPTIONS_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.quitterBouton)
		{
#ifdef ASK_BEFORE_QUIT
			if (!guiManager->guiElements.mainMenuGUI.quitterWindow)
			{
				// Cr�e la fen�tre demandant � l'utilisateur s'il est s�r de vouloir quitter,
				// en prenant l'�l�ment principal du menu comme parent pour qu'il puisse avoir acc�s aux �v�nements
				guiManager->guiElements.mainMenuGUI.quitterWindow = CGUIMessageBox::addMessageBox(gui, L"Quitter", L"Voulez-vous vraiment quitter ?",
					EMBF_YES | EMBF_NO, guiManager->getGUI(GUIManager::EGN_mainMenuGUI), true);
			}
#else
			// Quitte le jeu directement
			device->closeDevice();
#endif
		}
#ifdef USE_RAKNET
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.multiplayerBouton)
		{
			// Avance � la sc�ne pour rejoindre une partie multijoueur
			switchToNextScene(ECS_MULTIPLAYER_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton)
		{
			// Choisi l'adresse IP de l'h�te de la partie multijoueur
			core::stringc hostIP("");

			// V�rifie qu'une partie est bien s�lectionn�e, sinon on emp�che le joueur de continuer
			const int selectedGame = guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->getSelected();
			if (guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable && selectedGame >= 0)
			{
				// Obtient le nom de l'adresse IP de l'h�te d'apr�s le texte de la permi�re cellule de la ligne s�lectionn�e
				hostIP = core::stringc(guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->getCellText(selectedGame, 0));

				// Rejoint la partie multijoueur s�lectionn�e
				// Attention : le param�tre hostIP ne doit pas �tre une r�f�rence pointant sur un �lement de la liste des parties trouv�es de RakNet,
				// car cette liste sera vid�e (par un appel � reset) avant que la connexion r�seau n'ait �t� cr��e entre les deux machines
				joinMultiplayerGame(hostIP.c_str());
			}
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.multiplayerMenuGUI.retourBouton)
		{
#ifdef USE_RAKNET
			// Stoppe l'action en cours de RakNet
			rkMgr.reset();
#endif

			// Retourne � la sc�ne du menu principal
			switchToNextScene(ECS_MAIN_MENU);
		}
#endif
		else if (event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.retourBouton)
		{
			// Retourne � la sc�ne du menu principal
			switchToNextScene(ECS_MAIN_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.commencerBouton)
		{
			// Choisi le nom du terrain
			core::stringw terrainFilename("terrain plat.ewt");

			// V�rifie qu'un terrain est bien s�lectionn�, sinon on emp�che le joueur de continuer
			const int selectedTerrain = guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected();
			if (guiManager->guiElements.newGameMenuGUI.terrainListBox && selectedTerrain >= 0)
			{
				terrainFilename = core::stringw(guiManager->guiElements.newGameMenuGUI.terrainListBox->getListItem(selectedTerrain));
				terrainFilename.append(".ewt");

				// Choisi la difficult� du jeu
				EcoWorldModifiers::E_DIFFICULTY difficulty = EcoWorldModifiers::ED_normal;
				if (guiManager->guiElements.newGameMenuGUI.difficultyComboBox)
				{
					const u32 itemData = guiManager->guiElements.newGameMenuGUI.difficultyComboBox->getItemData(
						guiManager->guiElements.newGameMenuGUI.difficultyComboBox->getSelected());
					difficulty = (EcoWorldModifiers::E_DIFFICULTY)itemData;
				}

				// Cr�e une nouvelle partie avec cette difficult� et avec le terrain s�lectionn�
				createNewGame(difficulty, terrainFilename
#ifdef USE_RAKNET
					, (guiManager->guiElements.newGameMenuGUI.multiplayerEnabledCheckBox ?
						guiManager->guiElements.newGameMenuGUI.multiplayerEnabledCheckBox->isChecked() : false)
#endif
					);
			}
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameEndGUI.continuerPartieBouton)
		{
			// Indique au syst�me de jeu que le joueur d�sire continuer la partie
			system.setContinueGame(true);

			// D�bloque la cam�ra RTS si elle a �t� bloqu�e arbitrairement
			if (cameraRTS)
				cameraRTS->setLockCamera(lockCamera);

			// Red�marre le timer s'il est stopp�
			if (deviceTimer->isStopped())
				deviceTimer->start();

			// Retourne � la sc�ne du jeu
			switchToNextScene(ECS_PLAY);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
		{
			// Recr�e le menu principal
			createMainMenu();
		}
	}
	else if ((event.GUIEvent.EventType == EGET_LISTBOX_CHANGED || event.GUIEvent.EventType == EGET_LISTBOX_SELECTED_AGAIN)
		&& event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.terrainListBox && guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
	{
		// Si le terrain s�lectionn� a chang� dans le menu de cr�ation de nouveau terrain, on actualise son image d'aper�u :

		// Si une texture est d�j� affich�e dans l'aper�u, on la supprime
		if (guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
		{
			video::ITexture* const newGamePreviewImage = guiManager->guiElements.newGameMenuGUI.terrainApercuImage->getImage();
			if (newGamePreviewImage)
			{
				guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);
				driver->removeTexture(newGamePreviewImage);
			}
		}

		// Les informations sur la pr�visualisation de ce terrain
		EcoWorldTerrainManager::TerrainPreviewInfos terrainPreviewInfos;

		// V�rifie qu'un terrain est bien s�lectionn�
		if (guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected() >= 0 && renderer)
		{
			// Choisi le nom du terrain
			core::stringw terrainFilename = guiManager->guiElements.newGameMenuGUI.terrainListBox->getListItem(
					guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected());
			terrainFilename.append(".ewt");

			// Laisse le Terrain Manager essayer de charger les informations de pr�visualisation du terrain
			terrainPreviewInfos = renderer->getTerrainManager().readPreviewInfos(terrainFilename);
		}

		// Indique le texte de pr�visualisation de ce terrain
		if (guiManager->guiElements.newGameMenuGUI.terrainDescriptionTexte)
			guiManager->guiElements.newGameMenuGUI.terrainDescriptionTexte->setText(terrainPreviewInfos.descriptionText.c_str());

		// Si l'image est bien valide, on l'affiche enfin dans la zone de pr�visualisation
		// (si elle n'est pas valide, terrainPreviewInfos.previewImage est NULL et on efface l'image actuelle)
		if (guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
			guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(terrainPreviewInfos.previewImage);
	}

	return false;
}
bool Game::OnEventGUI(const SEvent& event
#ifdef USE_IRRKLANG
					  , bool& sendEventToIrrKlangMgr
#endif
					  )
{
	if (event.EventType != EET_GUI_EVENT || !guiManager)
		return false;

	// Ici, on g�re les �v�nements de la GUI qui ont lieu dans la sc�ne du jeu ainsi que les �v�nements des bo�tes de dialogue d'ouverture et d'enregistrement de fichiers, et la bo�te de dialogue d'erreur
	// Note : on re�oit aussi les �v�nements de la GUI qui ont lieu dans les sc�nes de chargement

	// V�rifie que l'�venement est bien un appui sur un bouton
	if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.objectivesBouton && guiManager->guiElements.gameGUI.objectivesWindow)
		{
			// Affiche/Masque le tableau des objectifs pour la partie actuelle
			guiManager->guiElements.gameGUI.objectivesWindow->setVisible(!guiManager->guiElements.gameGUI.objectivesWindow->isVisible());

			// Si le tableau est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.objectivesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.objectivesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.monthTableBouton && guiManager->guiElements.gameGUI.monthTableWindow)
		{
			// Affiche/Masque le tableau r�capitulatif des fins de mois
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(!guiManager->guiElements.gameGUI.monthTableWindow->isVisible());

			// Si le tableau est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.monthTableWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.ressourcesBouton && guiManager->guiElements.gameGUI.ressourcesWindow)
		{
			// Affiche/Masque le menu Ressources
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(!guiManager->guiElements.gameGUI.ressourcesWindow->isVisible());

			// Si le menu Ressources est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.ressourcesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.ressourcesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.FPSCameraBouton && cameraRTS)
		{
			// Passe en mode Visiteur
			cameraRTS->setIsCameraFPSEnabled(!cameraRTS->getIsCameraFPSEnabled());

			// Annule la s�lection actuelle
			batSelector.annulerSelection();

			// Masque ou active la GUI du jeu
			if (currentScene == ECS_PLAY)
				guiManager->setGUIVisible(GUIManager::EGN_gameGUI, !cameraRTS->getIsCameraFPSEnabled());

#ifdef USE_IRRKLANG
			// TODO : Joue le son de passage en mode Visiteur
			//ikMgr.playGameSound(...);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de passage en mode visiteur + le son de clic sur un bouton)
			//sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.menuBouton && guiManager->guiElements.gameGUI.menuWindow)
		{
			// Met en pause si on n'y est pas d�j�
			guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(!isPaused);
			if (!isPaused)
				changePause();

			// Si la cam�ra FPS � l'int�rieur de la cam�ra RTS est activ�e, on la d�sactive
			if (cameraRTS && cameraRTS->getIsCameraFPSEnabled())
				cameraRTS->setIsCameraFPSEnabled(false);

			// D�s�lectionne le b�timent s�lectionn� et annule la construction/destruction en cours, et on masque aussi la fen�tre d'informations
			batSelector.annulerSelection();
			if (guiManager->guiElements.gameGUI.informationsWindow)
				guiManager->guiElements.gameGUI.informationsWindow->setVisible(false);

			// Affiche la fen�tre du menu du jeu
			guiManager->guiElements.gameGUI.menuWindow->setVisible(true);

			// Donne le focus � la fen�tre du menu du jeu
			gui->setFocus(guiManager->guiElements.gameGUI.menuWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_OPEN);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son d'ouverture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.detruireBouton)
		{
			// Retiens si ce bouton �tait press� (annulerSelection() va rendre ce bouton non press� !)
			const bool wasPressed = guiManager->guiElements.gameGUI.detruireBouton->isPressed();

			// Annule la s�lection actuelle (cr�ation/destruction/s�lection d'un b�timent)
			batSelector.annulerSelection();

			// Rend � nouveau ce bouton press�
			guiManager->guiElements.gameGUI.detruireBouton->setPressed(wasPressed);
		}
		else
		{
			// V�rifie si le bouton sur lequel l'utilsateur a appuy� est un bouton pour construire un batiment
			IGUIButton* boutonBatiment = NULL;
			const core::list<IGUIButton*>::Iterator END = guiManager->guiElements.gameGUI.listeBoutonsBatiments.end();
			for (core::list<IGUIButton*>::Iterator it = guiManager->guiElements.gameGUI.listeBoutonsBatiments.begin(); it != END; ++it)
			{
				IGUIButton* const currentBouton = (*it);
				if (currentBouton == event.GUIEvent.Caller)
				{
					boutonBatiment = currentBouton;
					break;
				}
			}

			if (boutonBatiment)
			{
				// Retiens si ce bouton �tait press� (annulerSelection() va rendre ce bouton non press� !)
				const bool wasPressed = boutonBatiment->isPressed();

				// Annule la s�lection actuelle (cr�ation/destruction/s�lection d'un b�timent)
				batSelector.annulerSelection();

				// Rend � nouveau ce bouton press�
				boutonBatiment->setPressed(wasPressed);

				// Si le bouton est press�, change le batiment actuel, sinon, le d�sactive
				if (wasPressed)
					batSelector.setCurrentBatimentID((BatimentID)(boutonBatiment->getID())); // L'ID du bouton est le m�me que l'ID du batiment qu'il repr�sente
				else
					batSelector.setCurrentBatimentID(BI_aucun);

				return false;
			}
		}
	}
	// Si l'�v�nement est l'arriv�e de la souris sur un �l�ment de la GUI
	else if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.budgetTexte)
		{
			// La souris est sur le texte du budget : on affiche ses informations
			guiManager->guiElements.gameGUI.budgetInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.energieTexte)
		{
			// La souris est sur le texte de l'�nergie : on affiche ses informations
			guiManager->guiElements.gameGUI.energieInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.effetSerreTexte)
		{
			// La souris est sur le texte de l'effet de serre : on affiche ses informations
			guiManager->guiElements.gameGUI.effetSerreInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dechetsTexte)
		{
			// La souris est sur le texte des d�chets : on affiche ses informations
			guiManager->guiElements.gameGUI.dechetsInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.populationTexte)
		{
			// La souris est sur le texte de la population : on affiche ses informations
			guiManager->guiElements.gameGUI.populationInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dateTexte)
		{
			// La souris est sur le texte de la date : on affiche ses informations
			guiManager->guiElements.gameGUI.dateInfosTexte->setVisible(true);
		}
#ifdef MENU_INFERIEUR_REACTIF
		else if (guiManager->guiElements.gameGUI.tabGroupBas)
		{
			// D�termine si la souris vient de passer sur le menu inf�rieur ou sur l'un de ses enfants
			if (calculerIsMouseOnGUI(guiManager->guiElements.gameGUI.tabGroupBas))
			{
				// Si la souris vient de passer est le menu inf�rieur ou sur l'un de ses enfants, on demande sa mont�e quelle que soit sa position actuelle
				vitesseDescenteMenuInferieur = -0.001f;
				realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
				menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
			}
		}
#endif
	}
	// Si l'�v�nement est le d�part de la souris sur un �l�ment de la GUI
	else if (event.GUIEvent.EventType == EGET_ELEMENT_LEFT)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.budgetTexte)
		{
			// La souris a quitt� le texte du budget : on masque ses informations
			guiManager->guiElements.gameGUI.budgetInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.energieTexte)
		{
			// La souris a quitt� le texte de l'�nergie : on masque ses informations
			guiManager->guiElements.gameGUI.energieInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.effetSerreTexte)
		{
			// La souris a quitt� le texte de l'effet de serre : on masque ses informations
			guiManager->guiElements.gameGUI.effetSerreInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dechetsTexte)
		{
			// La souris a quitt� le texte des d�chets : on masque ses informations
			guiManager->guiElements.gameGUI.dechetsInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.populationTexte)
		{
			// La souris a quitt� le texte de la population : on masque ses informations
			guiManager->guiElements.gameGUI.populationInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dateTexte)
		{
			// La souris a quitt� le texte de la date : on masque ses informations
			guiManager->guiElements.gameGUI.dateInfosTexte->setVisible(false);
		}
#ifdef MENU_INFERIEUR_REACTIF
		else if (guiManager->guiElements.gameGUI.tabGroupBas)
		{
			// D�termine si la souris vient de passer sur le menu inf�rieur ou sur l'un de ses enfants
			if (!calculerIsMouseOnGUI(guiManager->guiElements.gameGUI.tabGroupBas))
			{
				// Si la souris vient de quitter le menu inf�rieur et n'est pas non plus sur ses enfants, on demande sa descente quelle que soit la position actuelle de ce menu
				vitesseDescenteMenuInferieur = 0.0001f;
				realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
				menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
			}
		}
#endif
	}
	else if (event.GUIEvent.EventType == EGET_ELEMENT_CLOSED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.objectivesWindow)
		{
			// Le tableau des objectifs a �t� ferm� : on le masque
			guiManager->guiElements.gameGUI.objectivesWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.monthTableWindow)
		{
			// Le tableau r�capitulatif des fins de mois a �t� ferm� : on le masque
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.ressourcesWindow)
		{
			// Le menu Ressources a �t� ferm� : on le masque
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			// Le menu d'animation de la cam�ra a �t� ferm� : on le masque
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.menuWindow)
		{
			// Masque le menu du jeu
			guiManager->guiElements.gameGUI.menuWindow->setVisible(false);

			// Enl�ve la pause
			if (guiManager->guiElements.gameGUI.menuWindow->getHasChangedPause())
				changePause();
			guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(false);

			// Restaure le blocage de la cam�ra RTS, puisqu'elle a �t� bloqu�e lors de l'affichage de ce menu
			cameraRTS->setLockCamera(lockCamera);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a d�j� tra�t� cet event du point de vue sonore (�vite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif

			// Applique le choix demand� par l'utilisateur lors de la fermeture de cette fen�tre
			applyMenuWindowChoice();
		}
	}
#ifndef MENU_INFERIEUR_REACTIF
	else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.tabGroupBas && event.GUIEvent.EventType == EGET_TAB_CHANGED
		&& vitesseDescenteMenuInferieur == 0.0f)
	{
		// L'utilisateur veut descendre ou remonter le menu inf�rieur, on modifie donc sa vitesse

		// Si on a cliqu� sur l'onglet pour r�duire le menu inf�rieur et que ce dernier est en haut, on le r�duit
		const bool activeTabIsReduireTab = (guiManager->guiElements.gameGUI.tabGroupBas->getActiveTab() == guiManager->guiElements.gameGUI.reduireTab->getNumber());
		const tribool menuInferieurHaut = isMenuInferieurHaut();
		if (menuInferieurHaut == true && activeTabIsReduireTab)
		{
			vitesseDescenteMenuInferieur = 0.001f;
			realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
			menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
		}
		else if (menuInferieurHaut == false)	// Si on a cliqu� sur n'importe quel onglet, mais que le menu inf�rieur est en bas, on le remonte
		{
			vitesseDescenteMenuInferieur = -0.001f;
			realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
			menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
		}

		// Active le premier onglet pour �viter que l'onglet de r�duction du menu inf�rieur ne s'active
		if (activeTabIsReduireTab)
			guiManager->guiElements.gameGUI.tabGroupBas->setActiveTab(0);
	}
#endif
	else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.vitesseJeuScrollBar && event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED)
	{
		// L'utilisateur a chang� la vitesse du jeu : on modifie la vitesse du timer du device en cons�quence

		const float newTimerSpeed = guiManager->getGameGUITimerSpeed();

#ifdef USE_RAKNET
		// Indique � RakNet qu'on veut changer la vitesse du jeu, s'il est activ�
		if (rkMgr.isNetworkPlaying())
		{
			RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_GAME_SPEED_CHANGED);
			packet.gameSpeed = newTimerSpeed;
			rkMgr.sendPackets.push_back(packet);
		}
		else
#endif
		{
			deviceTimer->setSpeed(newTimerSpeed);

#ifdef USE_SPARK
			// Indique � Spark que la vitesse du jeu a chang�
			renderer->getSparkManager().setTimerSpeed(newTimerSpeed);
#endif
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.errorMessageBox
		&& (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO || event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL))
	{
		// On indique que le pointeur sur la bo�te de dialogue d'erreur n'est plus valide
		guiManager->guiElements.globalGUI.errorMessageBox = NULL;

		if (hasChangedPauseBeforeShowingErrorMessageBox)
		{
			// Enl�ve la pause si on l'avait activ�e
			changePause();
			hasChangedPauseBeforeShowingErrorMessageBox = false;
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.openFileDialog)
	{
		if (event.GUIEvent.EventType == EGET_FILE_SELECTED || event.GUIEvent.EventType == EGET_FILE_CHOOSE_DIALOG_CANCELLED)
		{
			// Masque la fen�tre de fermeture de fichier
			guiManager->guiElements.globalGUI.openFileDialog->hide();

			if (event.GUIEvent.EventType == EGET_FILE_SELECTED)
			{
				const core::stringw filename = guiManager->guiElements.globalGUI.openFileDialog->getFileNameStr();

				if (filename.size())
				{
					if (hasChangedPauseBeforeShowingFileDialog)
					{
						// Enl�ve la pause si on l'avait activ�e
						changePause();
						hasChangedPauseBeforeShowingFileDialog = false;
					}

					// Charge le jeu
					loadSavedGame(filename);

					// Enl�ve le focus de la GUI
					gui->setFocus(NULL);
				}
			}
			if (hasChangedPauseBeforeShowingFileDialog)
			{
				// Enl�ve la pause si on l'avait activ�e
				changePause();
				hasChangedPauseBeforeShowingFileDialog = false;
			}
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.saveFileDialog)
	{
		if (event.GUIEvent.EventType == EGET_FILE_SELECTED || event.GUIEvent.EventType == EGET_FILE_CHOOSE_DIALOG_CANCELLED)
		{
			// Masque la fen�tre de fermeture de fichier
			guiManager->guiElements.globalGUI.saveFileDialog->hide();

			if (event.GUIEvent.EventType == EGET_FILE_SELECTED)
			{
				core::stringw filename = guiManager->guiElements.globalGUI.saveFileDialog->getFileNameStr();

				if (filename.size())
				{
					if (hasChangedPauseBeforeShowingFileDialog)
					{
						// Enl�ve la pause si on l'avait activ�e
						changePause();
						hasChangedPauseBeforeShowingFileDialog = false;
					}

					// On ne peut changer l'�tat de la pause car il est sauvegard�, donc on stoppe le timer manuellement s'il ne l'est pas
					const bool wasTimerStarted = !deviceTimer->isStopped();
					if (wasTimerStarted)
						deviceTimer->stop();

					// Enregistre le jeu
					saveCurrentGame(filename);

					// Enl�ve le focus de la GUI
					gui->setFocus(NULL);

					// Red�marre le timer s'il n'�tait pas stopp�
					if (wasTimerStarted)
						deviceTimer->start();
				}
			}
			if (hasChangedPauseBeforeShowingFileDialog)
			{
				// Enl�ve la pause si on l'avait activ�e
				changePause();
				hasChangedPauseBeforeShowingFileDialog = false;
			}
		}
	}
#ifdef ASK_BEFORE_QUIT
	else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.quitterWindow
		&& (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO || event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL))
	{
		// Si l'utilisateur a cliqu� sur un bouton (le bouton Oui est inclus) :
		// On oublie le pointeur sur la fen�tre de confirmation
		guiManager->guiElements.mainMenuGUI.quitterWindow = NULL;

		if (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES)	// Si l'utilisateur a cliqu� sur Oui :
			device->closeDevice();								// On quitte le jeu

		return true;
	}
#endif

	return false;
}
bool Game::OnEventGame(const SEvent& event)
{
	if (currentScene != ECS_PLAY)
		return false;

	// Ici, on g�re tous les �v�nements qui ont lieu dans la sc�ne du jeu, autres que ceux de la GUI

	CBatimentSceneNode* const selectedBatiment = batSelector.getSelectedBat();

	// Raccourcis claviers :
	if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		// Sp�cifique au jeu :
		if (event.KeyInput.Key == KEY_SPACE && !event.KeyInput.PressedDown && selectedBatiment && cameraRTS && canHandleKeysEvent)
		{
			// Espace

			// Si un b�timent est s�lectionn� et qu'on a appuy� sur Espace, on pointe la cam�ra sur ce b�timent
			cameraRTS->pointCameraAtNode(selectedBatiment,
				selectedBatiment->getTransformedBoundingBox().getExtent().getLength() * 3.0f);	// Distance au node relative � la taille de sa bounding box
		}
		else if (event.KeyInput.Key == KEY_KEY_N && canHandleKeysEvent)
		{
			// N

			// On peut afficher tous les noms au-dessus des b�timents par un appui sur la touche N
			if (event.KeyInput.PressedDown)
				renderer->setNomsBatimentsVisible(true);
			else
				renderer->setNomsBatimentsVisible(indeterminate);
		}
		else if (event.KeyInput.Key == KEY_KEY_O && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.objectivesWindow
			&& !event.KeyInput.Control)	// Ctrl + O est un autre raccourci clavier
		{
			// O

			// On peut aussi afficher le tableau des objectifs par un appui sur la touche O
			guiManager->guiElements.gameGUI.objectivesWindow->setVisible(!guiManager->guiElements.gameGUI.objectivesWindow->isVisible());

			// Si le tableau est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.objectivesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.objectivesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_T && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.monthTableWindow
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + T est une touche de d�bogage
#endif
			)
		{
			// T

			// On peut aussi afficher le tableau r�capitulatif par un appui sur la touche T
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(!guiManager->guiElements.gameGUI.monthTableWindow->isVisible());

			// Si le tableau est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.monthTableWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_R && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.ressourcesWindow
			&& !(event.KeyInput.Shift && event.KeyInput.Control)	// Ctrl + Maj + R est une touche de la cam�ra (r�initialisation des param�tres de la cam�ra)
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + R est une touche de d�bogage
#endif
			)
		{
			// R

			// On peut aussi afficher le menu Ressources par un appui sur la touche R
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(!guiManager->guiElements.gameGUI.ressourcesWindow->isVisible());

			// Si le menu Ressources est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.ressourcesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.ressourcesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_A && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.cameraAnimatorWindow
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + A est une touche de d�bogage
#endif		
			)
		{
			// A

			// On peut aussi afficher le menu permettant � l'utilisateur de cr�er une animation personnalis�e par un appui sur la touche A
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->setVisible(!guiManager->guiElements.gameGUI.cameraAnimatorWindow->isVisible());

			// Si le menu d'animation est affich�, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.cameraAnimatorWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.cameraAnimatorWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_F5 && !event.KeyInput.PressedDown && guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			// F5

			// Lit directement l'animation personnalis�e de la cam�ra
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->addAnimatorToCamera();
		}
		else if (event.KeyInput.Key == KEY_KEY_F && !event.KeyInput.PressedDown && cameraRTS && canHandleKeysEvent)
		{
			// F

			// On peut aussi passer en mode Visiteur par un appui sur la touche F
			cameraRTS->setIsCameraFPSEnabled(!cameraRTS->getIsCameraFPSEnabled());

			// Annule la s�lection actuelle
			batSelector.annulerSelection();

			// Masque ou active la GUI du jeu
			guiManager->setGUIVisible(GUIManager::EGN_gameGUI, !cameraRTS->getIsCameraFPSEnabled());

#ifdef USE_IRRKLANG
			// TODO : Joue le son de passage en mode Visiteur
			//ikMgr.playGameSound(...);
#endif

			return true;
		}
		else if (event.KeyInput.Key == KEY_KEY_I && !event.KeyInput.PressedDown && guiManager && cameraRTS && !cameraRTS->getIsCameraFPSEnabled() && canHandleKeysEvent)
		{
			// I

			// Un appui sur la touche I masque/active la GUI du jeu
			guiManager->setGUIVisible(GUIManager::EGN_gameGUI, !guiManager->isGUIVisible(GUIManager::EGN_gameGUI));
		}
		else if (event.KeyInput.Key == KEY_KEY_C && !event.KeyInput.PressedDown && guiManager && canHandleKeysEvent)
		{
			// C

			// Un appui sur la touche C r�initialise la GUI du jeu
			guiManager->resetGameGUI();
		}
		else if ((event.KeyInput.Key == KEY_PAUSE || (event.KeyInput.Key == KEY_KEY_P && canHandleKeysEvent && !event.KeyInput.Control))	// Ctrl + P : Mode nuage de points !
			&& !event.KeyInput.PressedDown)
		{
			// Pause ou P

#ifdef USE_RAKNET
			// Indique � RakNet qu'on veut changer la pause du jeu, s'il est activ�
			if (rkMgr.isNetworkPlaying())
			{
				RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_GAME_PAUSE_CHANGED);
				packet.gameIsPaused = !isPaused;
				rkMgr.sendPackets.push_back(packet);
			}
			else
#endif
				// Change l'�tat de la pause
				changePause();
		}
		else if (event.KeyInput.Key == KEY_KEY_L
#ifdef _DEBUG
			&& !event.KeyInput.Control && !event.KeyInput.Shift		// Ctrl + Maj + L et Ctrl + L est sont d'autres raccourcis claviers en mode DEBUG
#else
			&& (!event.KeyInput.Control || !event.KeyInput.Shift)	// Ctrl + Maj + L est un autre raccourci clavier en mode Release
#endif
			&& !event.KeyInput.PressedDown && cameraRTS && canHandleKeysEvent)
		{
			// L

			// Change l'�tat du blocage de la camera RTS pour qu'elle ne bouge plus ou puisse bouger � nouveau
			lockCamera = !lockCamera;

#ifdef LOCK_CAMERA_WHEN_MOUSE_ON_GUI
			// On bloque la camera RTS si la souris est sur un �l�ment de la GUI ou si l'utilisateur a demand� de la bloquer
			cameraRTS->setLockCamera(isMouseOnGUI || lockCamera);
#else
			// On ne bloque la camera que si l'utilisateur a demand� de la bloquer
			cameraRTS->setLockCamera(lockCamera);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_B && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent
			&& guiManager->guiElements.gameGUI.vitesseJeuScrollBar)
		{
			// Ctrl + B

			// Autorise/Interdit le mode Boost :

			// Obtient des valeurs actuelles sur la scroll bar de s�lection de vitesse
			const int currentScrollBarMax = guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getMax();
			const int currentScrollBarPos = (currentScrollBarMax - guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getPos());	// Attention : ne pas oublier que les valeurs sont invers�es

			// D�termine le nouveau maximum de la scroll bar
			const int newScrollBarMax = (currentScrollBarMax == 6 ? 8 : 6);

			// Modifie la valeur maximale de la scroll bar
			guiManager->guiElements.gameGUI.vitesseJeuScrollBar->setMax(newScrollBarMax);

			// Modifie la position actuelle de la scroll bar pour conserver la vitesse actuelle du jeu
			guiManager->guiElements.gameGUI.vitesseJeuScrollBar->setPos(max(newScrollBarMax - currentScrollBarPos, 0));

			// Modifie la vitesse actuelle du timer si elle a �t� modifi�e parce que le maximum a �t� diminu�
			deviceTimer->setSpeed(guiManager->getGameGUITimerSpeed());
		}
		else if (event.KeyInput.Key == KEY_KEY_S && event.KeyInput.Control && canHandleKeysEvent && cameraRTS && !cameraRTS->getIsCameraFPSEnabled())
		{
			// Ctrl + S

			// On affiche la bo�te de dialogue pour sauvegarder un fichier
			if (event.KeyInput.PressedDown)
				showFileDialog(CGUIFileSelector::EFST_SAVE_DIALOG);

			return true;	// Evite que la cam�ra RTS ne re�oive cet event (que la touche soit press�e ou non)
		}
		else if (event.KeyInput.Key == KEY_KEY_O && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent && cameraRTS && !cameraRTS->getIsCameraFPSEnabled())
		{
			// Ctrl + O

			// On affiche la bo�te de dialogue pour ouvrir un fichier
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
		}



		// Non sp�cifique au jeu :
		else if (event.KeyInput.Key == KEY_KEY_L && event.KeyInput.Control && event.KeyInput.Shift && !event.KeyInput.PressedDown && canHandleKeysEvent
			&& guiManager->guiElements.gameGUI.notesWindow)
		{
			// Ctrl + Maj + L

			// Envoie le texte de notes dans le logger, une fois reconvertit en caract�res unicodes
			LOG((core::stringc(guiManager->guiElements.gameGUI.notesWindow->getNotesTexte())).c_str(), ELL_NONE);	// ELL_NONE indique que ce texte de log sera toujours envoy�
		}



		// Outils de d�bogage :
#ifdef _DEBUG
		else if (event.KeyInput.Key == KEY_ADD && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + '+' (pav� num�rique)

			// Ajoute instantan�ment 100 000 � au budget du joueur (ou 10 000 � si Maj est enfonc�)
			system.addBudget(event.KeyInput.Shift ? 10000.0f : 100000.0f);
		}
		else if (event.KeyInput.Key == KEY_SUBTRACT && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + '-' (pav� num�rique)

			// Retire instantan�ment 100 000 � du budget du joueur (ou 10 000 � si Maj est enfonc�)
			system.subtractBudget(event.KeyInput.Shift ? 10000.0f : 100000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_D && event.KeyInput.Control && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + D

			// Ajoute 1 mois au temps syst�me (ou 5 jours si Maj est enfonc�)
			if (!event.KeyInput.PressedDown)
				system.addTime(event.KeyInput.Shift ?
					10.0f :	//  5.0f * 2.0f : 10 jours avec 2 secondes par jour
					60.0f);	// 30.0f * 2.0f : 30 jours avec 2 secondes par jour

			return true;	// Evite que la cam�ra RTS ne re�oive cet event (que la touche soit press�e ou non !)
		}
		else if (event.KeyInput.Key == KEY_KEY_A && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + A

			// Ajoute instantan�ment 10 tonnes de toutes les ressources au stock du joueur (ou 1 tonne si Maj est enfonc�)
			system.addRessources(event.KeyInput.Shift ? 1000.0f : 10000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_Z && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + Z

			// Retire instantan�ment 10 tonnes de toutes les ressources au stock du joueur (ou 1 tonne si Maj est enfonc�)
			system.addRessources(event.KeyInput.Shift ? -1000.0f : -10000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_L && event.KeyInput.Control && !event.KeyInput.Shift && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl + L

			// D�bloque tous les b�timents
			unlockAllBatiments = !unlockAllBatiments;
		}
#endif
	}

	return false;
}
bool Game::onEchap(bool pressed)
{
	// Bouton relach�
	if (!pressed)
	{
		if (currentScene == ECS_PLAY)				// Dans le jeu
		{
			// Si on appuye sur Echap et que la cam�ra RTS est anim�e par sa fen�tre d'animation, on arr�te son animation
			if (guiManager->guiElements.gameGUI.cameraAnimatorWindow && guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
			{
				guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

				// Retourne true pour absorber l'�v�nement, et ainsi �viter que la fen�tre d'animation de la cam�ra re�oive ensuite l'�v�nement et se ferme
				// (l'appui sur Echap permet aussi de fermer certaines fen�tres)
				return true;
			}

			// Si on appuye sur Echap et que la cam�ra FPS dans la cam�ra RTS est activ�e, on repasse en mode cam�ra RTS
			else if (cameraRTS->getIsCameraFPSEnabled())
			{
				cameraRTS->setIsCameraFPSEnabled(false);

				// R�active la GUI du jeu
				if (currentScene == ECS_PLAY)
					guiManager->setGUIVisible(GUIManager::EGN_gameGUI, true);

				// Retourne true pour absorber l'�v�nement, et ainsi �viter qu'une fen�tre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fen�tres)
				return true;
			}

			// Si on appuye sur Echap et qu'on a un batiment � construire/s�lectionn�, on annule sa construction/s�lection
			else if (batSelector.isPlacingOrSelectingBat())
			{
				batSelector.annulerSelection();

				// Retourne true pour absorber l'�v�nement, et ainsi �viter qu'une fen�tre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fen�tres)
				return true;
			}

			// Si on appuye sur Echap et qu'aucune action n'est en cours, on affiche le menu du jeu
			else if (guiManager && guiManager->guiElements.gameGUI.menuWindow && !guiManager->guiElements.gameGUI.menuWindow->isVisible())
			{
				// Met en pause si on n'y est pas d�j�, sauf si on est en mode multijoueur sur RakNet
#ifdef USE_RAKNET
				if (!rkMgr.isNetworkPlaying())
#endif
				{
					guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(!isPaused);
					if (!isPaused)
						changePause();
				}

				// D�s�lectionne le b�timent s�lectionn� et annule la construction/destruction en cours, et on masque aussi la fen�tre d'informations
				batSelector.annulerSelection();
				if (guiManager->guiElements.gameGUI.informationsWindow)
					guiManager->guiElements.gameGUI.informationsWindow->setVisible(false);

				// Bloque la cam�ra RTS pour �viter que le joueur ne la d�place lorsque le menu du jeu est affich�
				cameraRTS->setLockCamera(true);

				// R�active la GUI du jeu si le joueur l'a d�sactiv�e
				guiManager->setGUIVisible(GUIManager::EGN_gameGUI, true);

				// Affiche la fen�tre du menu du jeu
				guiManager->guiElements.gameGUI.menuWindow->setVisible(true);

				// Donne le focus � la fen�tre du menu du jeu
				gui->setFocus(guiManager->guiElements.gameGUI.menuWindow);

#ifdef USE_IRRKLANG
				// Joue le son d'ouverture d'un menu
				ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_OPEN);
#endif

				// Retourne true pour absorber l'�v�nement, et ainsi �viter qu'une fen�tre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fen�tres)
				return true;
			}
		}
		else if (currentScene == ECS_MAIN_MENU)			// Dans le menu principal
		{
			// On simule un appui sur le bouton pour quitter
			if (guiManager->guiElements.mainMenuGUI.quitterBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.mainMenuGUI.quitterBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.mainMenuGUI.quitterBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fen�tre de confirmation des options est affich�e,
			// on simule un appui sur son bouton Non,
			// sinon on simule un appui sur le bouton pour retourner au menu principal.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->simulateNoButtonPress();
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				guiManager->guiElements.optionsMenuGUI.optionsMenu->OnEvent(event);

				guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de cr�ation de nouvelle partie
		{
			// On simule un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.newGameMenuGUI.retourBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.newGameMenuGUI.retourBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.newGameMenuGUI.retourBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'�cran de partie termin�e
		{
			// On simule un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(false);
			}
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On simule un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.multiplayerMenuGUI.retourBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.multiplayerMenuGUI.retourBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.multiplayerMenuGUI.retourBouton->setPressed(false);
			}
		}
#endif
	}
	else	// Bouton enfonc�
	{
		if (currentScene == ECS_MAIN_MENU)				// Dans le menu principal
		{
			// On affiche un appui sur le bouton pour quitter
			if (guiManager->guiElements.mainMenuGUI.quitterBouton)
				guiManager->guiElements.mainMenuGUI.quitterBouton->setPressed(true);
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fen�tre de confirmation des options est affich�e,
			// on affiche un appui sur son bouton Non,
			// sinon on affiche un appui sur le bouton pour retourner au menu principal.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->setNoButtonPressed(true);
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton->setPressed(true);
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de cr�ation de nouvelle partie
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.newGameMenuGUI.retourBouton)
				guiManager->guiElements.newGameMenuGUI.retourBouton->setPressed(true);
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'�cran de partie termin�e
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(true);
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On affiche un appui sur le bouton pour rejoindre la partie actuellement s�lectionn�e
			if (guiManager->guiElements.multiplayerMenuGUI.retourBouton)
				guiManager->guiElements.multiplayerMenuGUI.retourBouton->setPressed(true);
		}
#endif
	}

	return false;
}
bool Game::onEnter(bool pressed)
{
	// Bouton relach�
	if (!pressed)
	{
		if (currentScene == ECS_MAIN_MENU)				// Dans le menu principal
		{
			// On simule un appui sur le bouton pour jouer
			if (guiManager->guiElements.mainMenuGUI.newGameBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.mainMenuGUI.newGameBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.mainMenuGUI.newGameBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fen�tre de confirmation des options est affich�e,
			// on simule un appui sur son bouton Oui,
			// sinon on simule un appui sur le bouton pour appliquer les options.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->simulateYesButtonPress();
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				guiManager->guiElements.optionsMenuGUI.optionsMenu->OnEvent(event);

				guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de cr�ation de nouvelle partie
		{
			// On simule un appui sur le bouton pour commencer le jeu
			if (guiManager->guiElements.newGameMenuGUI.commencerBouton && guiManager->guiElements.newGameMenuGUI.commencerBouton->isEnabled())
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.newGameMenuGUI.commencerBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.newGameMenuGUI.commencerBouton->setPressed(false);
			}
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'�cran de partie termin�e
		{
			// On simule un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(false);
			}
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On simule un appui sur le bouton pour rejoindre la partie actuellement s�lectionn�e
			if (guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton && guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->isEnabled())
			{
				SEvent event;
				event.EventType = EET_GUI_EVENT;
				event.GUIEvent.Caller = guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton;
				event.GUIEvent.Element = NULL;
				event.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				OnEvent(event);

				guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->setPressed(false);
			}
		}
#endif
	}
	else	// Bouton enfonc�
	{
		if (currentScene == ECS_MAIN_MENU)				// Dans le menu principal
		{
			// On affiche un appui sur le bouton pour jouer
			if (guiManager->guiElements.mainMenuGUI.newGameBouton)
				guiManager->guiElements.mainMenuGUI.newGameBouton->setPressed(true);
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fen�tre de confirmation des options est affich�e,
			// on affiche un appui sur son bouton Oui,
			// sinon on affiche un appui sur le bouton pour appliquer les options.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->setYesButtonPressed(true);
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton->setPressed(true);
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de cr�ation de nouvelle partie
		{
			// On affiche un appui sur le bouton pour commencer le jeu
			if (guiManager->guiElements.newGameMenuGUI.commencerBouton && guiManager->guiElements.newGameMenuGUI.commencerBouton->isEnabled())
				guiManager->guiElements.newGameMenuGUI.commencerBouton->setPressed(true);
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'�cran de partie termin�e
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(true);
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On affiche un appui sur le bouton pour rejoindre la partie actuellement s�lectionn�e
			if (guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton && guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->isEnabled())
				guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->setPressed(true);
		}
#endif
	}

	return false;
}
void Game::changePause()
{
	// Change l'�tat de la pause
	isPaused = !isPaused;

	// Fait concorder l'affichage du texte de pause avec la pause actuelle
	if (guiManager->guiElements.globalGUI.pauseTexte)
		guiManager->guiElements.globalGUI.pauseTexte->setVisible(isPaused && currentScene == ECS_PLAY);	// N'affiche le texte de pause que lorsqu'on est dans la sc�ne du jeu

	// Modifie le titre de la fen�tre suivant l'�tat actuel de la pause
	{
		wstring title(TITRE_FENETRE_JEU);
		if (isPaused)
			title.append(L" (en pause)");
		device->setWindowCaption(title.c_str());
	}

	if (isPaused != deviceTimer->isStopped()) // V�rifie que l'�tat du timer concorde avec la pause
	{
		if (isPaused) // Si on est en pause, on stoppe le timer
			deviceTimer->stop();
		else // Si on n'est pas en pause, on red�marre le timer
			deviceTimer->start();
	}
}
void Game::showFileDialog(CGUIFileSelector::E_FILESELECTOR_TYPE type)
{
	// V�rifie que le gui manager et que le type de la bo�te de dialogue est bien valide
	if (!guiManager || (type != CGUIFileSelector::EFST_OPEN_DIALOG && type != CGUIFileSelector::EFST_SAVE_DIALOG))
		return;

	// V�rifie que les bo�tes de dialogues ont bien �t� cr��es
	if (!guiManager->guiElements.globalGUI.openFileDialog || !guiManager->guiElements.globalGUI.saveFileDialog)
		return;

	// V�rifie qu'aucune bo�te de dialogue n'est actuellement affich�e
	if (guiManager->guiElements.globalGUI.openFileDialog->isVisible() || guiManager->guiElements.globalGUI.saveFileDialog->isVisible())
		return;

	if (!isPaused && currentScene == ECS_PLAY)
	{
		// Met le jeu en pause avant d'afficher la bo�te de dialogue si on est dans la sc�ne du jeu
		changePause();
		hasChangedPauseBeforeShowingFileDialog = true;
	}

	if (type == CGUIFileSelector::EFST_OPEN_DIALOG)
		guiManager->guiElements.globalGUI.openFileDialog->showAgain();	// Affiche cette fen�tre � nouveau
	else
		guiManager->guiElements.globalGUI.saveFileDialog->showAgain();	// Affiche cette fen�tre � nouveau
}
void Game::showErrorMessageBox(const wchar_t* caption, const wchar_t* text)
{
	// V�rifie que le pointeur sur la gui est valide et qu'aucune bo�te de dialogue d'erreur n'est affich�e
	if (!gui || !guiManager)
		return;
	if (guiManager->guiElements.globalGUI.errorMessageBox)
		return;

	if (!isPaused && currentScene == ECS_PLAY)
	{
		// Met le jeu en pause avant d'afficher la bo�te de dialogue si on est dans la sc�ne du jeu
		changePause();
		hasChangedPauseBeforeShowingErrorMessageBox = true;
	}

	// Cr�e une bo�te de dialogue d'erreur avec les textes sp�cifi�s, et rend cette bo�te de dialogue modale tout en lui ajoutant un fond gris� derri�re elle
	guiManager->guiElements.globalGUI.errorMessageBox = CGUIMessageBox::addMessageBox(gui, caption, text, EMBF_OK, gui->getRootGUIElement(), true);
}
void Game::createScreenShot()
{
	// Affiche un rendu rapide avant de prendre la capture d'�cran
	//render();	// D�sactiv� : le rendu de la derni�re frame suffit

	long numeroScreenShot = 1;	// Le numero du ScreenShot actuel

	// Cr�e le dossier pour les captures d'�cran
	createDirectory("Screenshots");

	// Cherche le num�ro du ScreenShot suivant pour ne pas r��crire par-dessus un autre ScreenShot
	bool hasFoundNumero = false;
	do
	{
		// Obtient le nom de fichier du ScreenShot actuel
		sprintf_SS("Screenshots/Capture %d.bmp", numeroScreenShot);

		// Regarde si un fichier de ce nom existe d�j�
		bool exists = fileSystem->existFile(text_SS);

		if (!exists)	// Si le fichier n'existe pas, on indique qu'on a trouv� le num�ro du ScreenShot actuel
			hasFoundNumero = true;
		else			// Si le fichier existe d�j�, on incr�mente le num�ro du ScreenShot de 1
			numeroScreenShot++;

		// Petite s�curit� : Ecrit toujours ce screen shot et les suivants dans le screen shot 0 s'il y en a plus de 9999 actuellement
		if (numeroScreenShot > 9999)
		{
			hasFoundNumero = true;
			numeroScreenShot = 0;
		}

	} while (!hasFoundNumero);

	video::IImage* const image = driver->createScreenShot();
	if (image)
	{
		driver->writeImageToFile(image, text_SS);
		image->drop();

		LOG("Capture d'�cran cr��e : " << text_SS, ELL_INFORMATION);
	}
}
void Game::switchToNextScene(E_CURRENT_SCENE newCurrentScene)
{
	// D�sactiv� : Provoque quelques bugs interdisant la remise � z�ro d'une sc�ne (dans la fonction Game::joinMultiplayerGame(hostIP) en particulier :
	// ne permet pas de repasser RakNet en mode recherche de parties par une r�initialisation de la sc�ne)
	//if (currentScene == newCurrentScene)
	//	return;

	// Indique la nouvelle sc�ne actuelle
	currentScene = newCurrentScene;

	// Supprime le focus de la GUI (�vite que l'�lement ayant le focus soit dans une GUI devenue invisible)
	gui->setFocus(NULL);

	// Si une texture est affich�e dans l'aper�u de la liste des terrains, on la supprime
	// (quelle que soit la sc�ne actuelle, elle est inutile : si on passe dans le menu de cr�ation d'une nouvelle partie, elle sera recr��e, sinon elle ne sera pas visible)
	if (guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
	{
		video::ITexture* const newGamePreviewImage = guiManager->guiElements.newGameMenuGUI.terrainApercuImage->getImage();
		if (newGamePreviewImage)
		{
			guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);
			driver->removeTexture(newGamePreviewImage);
		}
	}

	// Annule la s�lection en cours pour r�initialiser la GUI du jeu, au cas o� un bouton de construction serait rest� appuy�
	batSelector.annulerSelection();

	// Indique � la barre de chargement qu'on a termin� le chargement actuel si un chargement est en cours
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->endLoading();

	// Si la cam�ra RTS est anim�e par sa fen�tre d'animation, on arr�te son animation
	if (guiManager->guiElements.gameGUI.cameraAnimatorWindow && guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
		guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

	// Enl�ve la pause si elle est activ�e
	if (isPaused)
		changePause();

	// Red�marre le timer tant qu'il reste des stops (ils sont compt�s) (la fonction changePause ne restaure pas tous les stops !)
	while (deviceTimer->isStopped())	deviceTimer->start();

	switch (currentScene)
	{
	case ECS_MAIN_MENU_LOADING:	// Chargement du menu principal
		// TODO : Cr�er un �cran de chargement pour ce menu (sans barre de chargement)
		break;

	case ECS_MAIN_MENU:	// Menu principal
		// Active la GUI du menu principal
		guiManager->setOnlyGUIVisible(GUIManager::EGN_mainMenuGUI);

		// Actualise le renderer du jeu, pour le mettre � jour avec le temps actuel (ne sera pas fait dans update() si la fen�tre n'est pas encore activ�e car le jeu est en pause)
		renderer->update();
		break;

	case ECS_OPTIONS_MENU:	// Menu options
		// Active la GUI du menu options (le fond du menu principal est cens� �tre d�j� cr��)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_optionsMenuGUI);

		// Active l'onglet du menu options correspondant si n�cessaire
		if (gameState.optionsMenuTab >= 0 && guiManager->guiElements.optionsMenuGUI.optionsMenu)
		{
			guiManager->guiElements.optionsMenuGUI.optionsMenu->setActiveTab(gameState.optionsMenuTab);
			gameState.optionsMenuTab = -1;
		}

		// Affiche la fen�tre de confirmation des nouveaux param�tres du menu options si n�cessaire
		if (gameState.showOptionsConfirmationWindow)
		{
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->showConfirmationWindow();
			gameState.showOptionsConfirmationWindow = false;
		}
		break;

	case ECS_NEW_GAME_MENU:	// Menu pour cr�er une nouvelle partie
		// Met � jour la liste des terrains disponibles
		guiManager->updateListTerrainsNewGameGUI(this);

		// Active la GUI du menu pour cr�er une nouvelle partie (le fond du menu principal est cens� �tre d�j� cr��)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_newGameMenuGUI);

		// Active l'onglet des terrains
		if (guiManager->guiElements.newGameMenuGUI.mainTabGroup && guiManager->guiElements.newGameMenuGUI.terrainTab)
			guiManager->guiElements.newGameMenuGUI.mainTabGroup->setActiveTab(guiManager->guiElements.newGameMenuGUI.terrainTab);

		// Active le bouton "Commencer"
		if (guiManager->guiElements.newGameMenuGUI.commencerBouton)
			guiManager->guiElements.newGameMenuGUI.commencerBouton->setEnabled(true);

		// Donne le focus de la GUI � la liste des terrains
		gui->setFocus(guiManager->guiElements.newGameMenuGUI.terrainListBox);
		break;

#ifdef USE_RAKNET
	case ECS_MULTIPLAYER_MENU:	// Menu pour rejoindre une partie multijoueur
		// D�marre RakNet pour qu'il recherche des parties multijoueurs en r�seau
		if (rkMgr.searchGames())
		{
			// Indique � l'utilisateur qu'une erreur s'est produite
			showErrorMessageBox(L"Avertissement", L"Recherche de parties impossible !");
		}

		// Efface la liste des parties disponibles (elle sera mise � jour r�guli�rement)
		guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->clearRows();

		// Active la GUI du menu pour cr�er une nouvelle partie (le fond du menu principal est cens� �tre d�j� cr��)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_multiplayerMenuGUI);

		// Donne le focus de la GUI � la liste des parties multijoueurs
		gui->setFocus(guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable);
		break;
#endif

	case ECS_LOADING_GAME:	// Chargement du jeu
		// Attention : ici on active seulement l'�cran de chargement, le chargement du jeu se fait dans une des fonctions createNewGame, loadSavedGame... suivant le type de partie � cr�er

		// Indique � la barre de chargement qu'on commence le chargement
		if (guiManager->guiElements.globalGUI.loadingScreen)
			guiManager->guiElements.globalGUI.loadingScreen->beginLoading();

		// Masque toutes les GUIs pour activer la GUI de l'�cran de chargement
		guiManager->hideAllGUIs();
		break;

	case ECS_PLAY:	// Sc�ne du jeu (mode un joueur)
		{
			// Active la GUI du jeu
			guiManager->setOnlyGUIVisible(GUIManager::EGN_gameGUI);



			// Met � jour toutes les fen�tres de la GUI et le renderer au moins une fois, en for�ant leur mise � jour :
			// Evite un bug : Lors du tout premier jour ("jour 0"), si le jeu est mis en pause, l'affichage du tableau des ressources ou des objectifs est bugg� : il n'est mis � jour que lorsqu'au moins un jour syst�me s'est �coul�, or ce n'est pas encore le cas.
			//					Ces fen�tres restent donc non remplies tant qu'au moins 1 jour ne s'est pas �coul�.
			//					Le m�me probl�me se pose ainsi pour le renderer, dont la mise � jour est d�pendante de la pause.

			// Obtient la transparence par d�faut des texte
			const u32 textTransparency = guiManager->getTextTransparency();

			// Actualise le tableau des objectifs
			if (guiManager->guiElements.gameGUI.objectivesWindow)
				guiManager->guiElements.gameGUI.objectivesWindow->update(true);

			// Actualise le tableau r�capitulatif des fins de mois
			if (guiManager->guiElements.gameGUI.monthTableWindow)
				guiManager->guiElements.gameGUI.monthTableWindow->update(true);

			// Actualise le menu Ressources
			if (guiManager->guiElements.gameGUI.ressourcesWindow)
				guiManager->guiElements.gameGUI.ressourcesWindow->update(textTransparency, true);

			// Actualise le renderer du jeu, pour le mettre � jour avec le temps actuel, et met aussi � jour la position du soleil d'apr�s le temps actuel du syst�me de jeu
			renderer->update(true);

#ifdef USE_IRRKLANG
			// Met � jour la position de la cam�ra d'IrrKlang
			if (cameraRTS)
				ikMgr.update(cameraRTS->getAbsolutePosition(), cameraRTS->getTarget(), 0.0f);
			else
				ikMgr.update(core::vector3df(0.0f), core::vector3df(1.0f), 0.0f);
#endif
		}
		break;

	case ECS_GAME_LOST:	// Ecran indiquant au joueur qu'il a perdu
		{
			// Stoppe le timer (pour figer le jeu � l'instant o� le joueur a perdu)
			deviceTimer->stop();

			if (cameraRTS)
			{
				// Repasse en mode cam�ra RTS si la cam�ra FPS est activ�e
				cameraRTS->setIsCameraFPSEnabled(false);

				// Bloque la cam�ra RTS pour emp�cher le joueur de la d�placer/tourner
				cameraRTS->setLockCamera(true);
			}

			// Modifie le texte indiquant au joueur que le jeu est termin� pour lui dire qu'il a perdu
			IGUIStaticText* const endTexte = guiManager->guiElements.gameEndGUI.endTexte;
			if (endTexte)
			{
				endTexte->setText(L"DOMMAGE :\r\nVous n'avez pas rempli vos objectifs pour cette partie !");
				endTexte->setOverrideColor(video::SColor(255, 150, 255, 255));
			}

			// Masque le bouton permettant au joueur de continuer la partie en cours
			if (guiManager->guiElements.gameEndGUI.continuerPartieBouton)
				guiManager->guiElements.gameEndGUI.continuerPartieBouton->setVisible(false);

			// Active la GUI de l'�cran indiquant au joueur que le jeu est termin�
			guiManager->setOnlyGUIVisible(GUIManager::EGN_gameEndGUI);
		}
		break;

	case ECS_GAME_WON:	// Ecran indiquant au joueur qu'il a gagn�
		{
			// Stoppe le timer (pour figer le jeu � l'instant o� le joueur a gagn�)
			deviceTimer->stop();

			if (cameraRTS)
			{
				// Repasse en mode cam�ra RTS si la cam�ra FPS est activ�e
				cameraRTS->setIsCameraFPSEnabled(false);

				// Bloque la cam�ra RTS pour emp�cher le joueur de la d�placer/tourner
				cameraRTS->setLockCamera(true);
			}

			// Modifie le texte indiquant au joueur que le jeu est termin� pour lui dire qu'il a gagn�
			IGUIStaticText* const endTexte = guiManager->guiElements.gameEndGUI.endTexte;
			if (endTexte)
			{
				endTexte->setText(L"FELICITATIONS :\r\nVous avez atteint vos objectifs pour cette partie !");
				endTexte->setOverrideColor(video::SColor(255, 255, 255, 150));
			}

			// Affiche le bouton permettant au joueur de continuer la partie en cours
			if (guiManager->guiElements.gameEndGUI.continuerPartieBouton)
				guiManager->guiElements.gameEndGUI.continuerPartieBouton->setVisible(true);

			// Active la GUI de l'�cran indiquant au joueur que le jeu est termin�
			guiManager->setOnlyGUIVisible(GUIManager::EGN_gameEndGUI);
		}
		break;

	default:
		LOG_DEBUG("Game::switchToNextScene(" << newCurrentScene << ") : Scene inconnue : currentScene = " << currentScene, ELL_WARNING);
		break;
	}
}
void Game::applyMenuWindowChoice()
{
	// Actualise et effectue le choix de la bo�te du dialogue du menu :
	if (guiManager->guiElements.gameGUI.menuWindow && guiManager->guiElements.gameGUI.menuWindow->getUserAction() != CGUIMenuWindow::GMWA_nonChoisi)
	{
		switch (guiManager->guiElements.gameGUI.menuWindow->getUserAction())
		{
		case CGUIMenuWindow::GMWA_creerNouvellePartie:
			// Recr�e le menu principal puis retourne au menu de cr�ation d'une nouvelle partie
			createMainMenu();
			switchToNextScene(ECS_NEW_GAME_MENU);
			break;

		case CGUIMenuWindow::GMWA_recommencerPartie:
			// Cr�e une nouvelle partie avec les param�tres actuels
			createNewGame(system.getModifiers().difficulty, renderer->getTerrainManager().getTerrainInfos().terrainName
#ifdef USE_RAKNET
				, rkMgr.getCurrentNetworkState() == RakNetManager::ENGS_PLAYING_GAME_HOST
#endif
				);
			break;

		case CGUIMenuWindow::GMWA_charger:
			// On affiche la bo�te de dialogue pour charger un fichier
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
			break;

		case CGUIMenuWindow::GMWA_sauvegarder:
			// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible
			if (CAN_WRITE_ON_DISK)
			{
				// On affiche la bo�te de dialogue pour sauvegarder un fichier
				showFileDialog(CGUIFileSelector::EFST_SAVE_DIALOG);
			}
			else
			{
				// Affiche un message d'erreur indiquant qu'on ne peut pas sauvegarder cette partie
				showErrorMessageBox(L"Erreur : Sauvegarde impossible", L"Impossible de sauvegarder cette partie :\r\nLes droits d'�criture sur le disque ont �t� refus�s !");
			}
			break;

		case CGUIMenuWindow::GMWA_retourMenuPrincipal:
			// Recr�e le menu principal
			createMainMenu();
			break;

		case CGUIMenuWindow::GMWA_quitter:
			// On quitte le jeu
			device->closeDevice();
			break;

		case CGUIMenuWindow::GMWA_annuler:
			// On ne fait rien
			break;

			// On ne conna�t pas cette valeur : on affiche un message d'avertissement si on est en mode d�bogage
		default:
			LOG_DEBUG("Game::updateGameGUI() : L'action demandee par l'utilisateur est inconnue : guiManager->guiElements.gameGUI.menuWindow->getUserAction() = "
				<< guiManager->guiElements.gameGUI.menuWindow->getUserAction(), ELL_WARNING);
			break;
		}

		// Indique que le choix de l'utilisateur a �t� trait�
		guiManager->guiElements.gameGUI.menuWindow->setUserAction(CGUIMenuWindow::GMWA_nonChoisi);
		return;
	}
}
void Game::updateGameGUI()
{
	if (currentScene != ECS_PLAY)
		return;

	// Obtient la transparence par d�faut des textes
	const u32 textTransparency = guiManager->getTextTransparency();

	// Met � jour les couleurs des textes et des informations
	const EcoWorldInfos& worldInfos = system.getInfos();
	{
		// Permet de savoir si on est en train de construire ou de d�truire un b�timent, pour savoir si on peut modifier la couleur du texte du budget ou non
		const bool isCreatingOrDestroyingBatiment = batSelector.isPlacingBat();

		const video::SColor defaultColor(textTransparency, 255, 255, 255);
		const video::SColor redColor(textTransparency, 255, 50, 50);

		// Budget : par valeur (on ne le modifie seulement si on n'est pas en train de cr�er ou d�truire un b�timent)
		if (guiManager->guiElements.gameGUI.budgetTexte && !isCreatingOrDestroyingBatiment)
		{
			if (worldInfos.budget < 0.0f)
			{
#if 1	// Texte du budget clignotant blanc/rouge lorsque le budget est n�gatif
				if (!isPaused)	// Evite l'animation du texte du budget si on est en pause : on l'affiche alors toujours rouge
				{
					// Calcule la fr�quence des clignotements (en ms) en fonction du budget (ici toujours n�gatif) :
					// frequence = f(budget) = budget / 100 + 1150
					const u32 frequence = (u32)(core::round_(0.01f * worldInfos.budget + 1150));
					const bool red = (((deviceTimer->getRealTime() / frequence) % 2) == 1);
					guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(red ? redColor : defaultColor);
				}
				else
#endif	// Sinon : Texte du budget toujours rouge lorsque le budget est n�gatif
					guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(redColor);
			}
			else
				guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(defaultColor);
		}

		// Budget (information) : par �volution
		if (guiManager->guiElements.gameGUI.budgetInfosTexte)
		{
			if (worldInfos.budgetEvolutionM < 0.0f)
				guiManager->guiElements.gameGUI.budgetInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.budgetInfosTexte->setOverrideColor(defaultColor);
		}

		// Energie : par valeur (on le modifie seulement si on n'est pas en train de cr�er un b�timent)
		if (guiManager->guiElements.gameGUI.energieTexte && (batSelector.getCurrentCreatingBat() == BI_aucun))
		{
			if (worldInfos.energie < 0.0f)
				guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.energieTexte->setOverrideColor(defaultColor);
		}

		// Energie (informations) : par valeur
		if (guiManager->guiElements.gameGUI.energieInfosTexte)
		{
			if (worldInfos.energie < 0.0f)
				guiManager->guiElements.gameGUI.energieInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.energieInfosTexte->setOverrideColor(defaultColor);
		}

		// Effet de serre : par taxes
		if (guiManager->guiElements.gameGUI.effetSerreTexte)
		{
			if (worldInfos.taxes[EcoWorldInfos::TI_effetSerre] > 0.0f)
				guiManager->guiElements.gameGUI.effetSerreTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.effetSerreTexte->setOverrideColor(defaultColor);
		}

		// Effet de serre (informations) : par �volution
		if (guiManager->guiElements.gameGUI.effetSerreInfosTexte)
		{
			if (worldInfos.effetSerreEvolutionM > 0.0f)
				guiManager->guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(defaultColor);
		}

		// D�chets : par taxes
		if (guiManager->guiElements.gameGUI.dechetsTexte)
		{
			if (worldInfos.taxes[EcoWorldInfos::TI_dechets] > 0.0f)
				guiManager->guiElements.gameGUI.dechetsTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.dechetsTexte->setOverrideColor(defaultColor);
		}

		// D�chets (informations) : par �volution
		if (guiManager->guiElements.gameGUI.dechetsInfosTexte)
		{
			if (worldInfos.dechetsEvolutionM > 0.0f)
				guiManager->guiElements.gameGUI.dechetsInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.dechetsInfosTexte->setOverrideColor(defaultColor);
		}

		// Population (texte et informations) : par valeur
		if (worldInfos.popRealSatisfaction < HABITANTS_MIN_SATISFACTION)
		{
			if (guiManager->guiElements.gameGUI.populationTexte)
				guiManager->guiElements.gameGUI.populationTexte->setOverrideColor(redColor);
			if (guiManager->guiElements.gameGUI.populationInfosTexte)
				guiManager->guiElements.gameGUI.populationInfosTexte->setOverrideColor(redColor);
		}
		else
		{
			if (guiManager->guiElements.gameGUI.populationTexte)
				guiManager->guiElements.gameGUI.populationTexte->setOverrideColor(defaultColor);
			if (guiManager->guiElements.gameGUI.populationInfosTexte)
				guiManager->guiElements.gameGUI.populationInfosTexte->setOverrideColor(defaultColor);
		}
	}

	// Remet les textes � jour
	if (guiManager->guiElements.gameGUI.budgetTexte)
	{
		swprintf_SS(L"Budget : %.2f �", worldInfos.budget);
		guiManager->guiElements.gameGUI.budgetTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.energieTexte)
	{
		swprintf_SS(L"Energie : %+.0f W", worldInfos.energie);
		guiManager->guiElements.gameGUI.energieTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.effetSerreTexte)
	{
		swprintf_SS(L"Effet de serre : %.0f kg", worldInfos.effetSerre);
		guiManager->guiElements.gameGUI.effetSerreTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.dechetsTexte)
	{
		swprintf_SS(L"D�chets : %.0f kg", worldInfos.dechets);
		guiManager->guiElements.gameGUI.dechetsTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.populationTexte)
	{
		if (worldInfos.population == 1)
			guiManager->guiElements.gameGUI.populationTexte->setText(L"1 Habitant");
		else
		{
			swprintf_SS(L"%u Habitants", worldInfos.population);
			guiManager->guiElements.gameGUI.populationTexte->setText(textW_SS);
		}
	}
	if (guiManager->guiElements.gameGUI.dateTexte)
	{
#if 1
		// Affiche le temps �coul� sous forme de date
		core::stringw str;
		appendDate(str, (u32)(system.getTime().getTotalJours()));
		guiManager->guiElements.gameGUI.dateTexte->setText(str.c_str());
#else
		// Date affich�e sans correction des pluriels
		swprintf_SS(L"%d ann�es  %d mois  %d jours",
			system.getTime().getAnnees(), system.getTime().getMois(), system.getTime().getJours());
		guiManager->guiElements.gameGUI.dateTexte->setText(text);
#endif
	}

	if (guiManager->guiElements.gameGUI.budgetInfosTexte && guiManager->guiElements.gameGUI.budgetInfosTexte->isVisible())
	{
		swprintf_SS(L"Evolution (M) : %+.2f �\r\nEvolution (A) : %+.2f �",
			worldInfos.budgetEvolutionM, worldInfos.budgetEvolutionA);
		guiManager->guiElements.gameGUI.budgetInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.energieInfosTexte && guiManager->guiElements.gameGUI.energieInfosTexte->isVisible())
	{
		swprintf_SS(L"Energie disponible : %.0f %%",	// '%%' : Symbole %
			worldInfos.pourcentageEnergieDisponible * 100.0f);
		guiManager->guiElements.gameGUI.energieInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.effetSerreInfosTexte && guiManager->guiElements.gameGUI.effetSerreInfosTexte->isVisible())
	{
		swprintf_SS(L"Evolution (M) : %+.0f kg\r\nTaxe (M) : %+.2f �",
			worldInfos.effetSerreEvolutionM, -(worldInfos.taxes[EcoWorldInfos::TI_effetSerre]));
		guiManager->guiElements.gameGUI.effetSerreInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.dechetsInfosTexte && guiManager->guiElements.gameGUI.dechetsInfosTexte->isVisible())
	{
		swprintf_SS(L"Evolution (M) : %+.0f kg\r\nTaxe (M) : %+.2f �",
			worldInfos.dechetsEvolutionM, -(worldInfos.taxes[EcoWorldInfos::TI_dechets]));
		guiManager->guiElements.gameGUI.dechetsInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.populationInfosTexte && guiManager->guiElements.gameGUI.populationInfosTexte->isVisible())
	{
		swprintf_SS(L"Satisfaction : %.0f %%\r\nSatisfaction r�elle : %.0f %%",			// '%%' : Symbole %
			fabs(worldInfos.popSatisfaction * 100.0f),		// "fabs" est utilis� pour toujours enlever le signe "-", m�me lorsque le r�sultat est nul
			fabs(worldInfos.popRealSatisfaction * 100.0f));	// La satisfaction r�elle est aussi d�pendante du pourcentage d'�nergie disponible pour cette maison (facteur entre *0.5f � 0% d'�nergie � *1.0f � 100% d'�nergie)
		guiManager->guiElements.gameGUI.populationInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.dateInfosTexte && guiManager->guiElements.gameGUI.dateInfosTexte->isVisible())
	{
		core::stringw str;
		appendDays(str, system.getTime().getTotalJours(), true);	// Ajoute le temps �coul� � la description

		// Ajoute la vitesse actuelle du jeu et le temps d'un jour
		const float currentTimerSpeed = deviceTimer->getSpeed();
		const float currentDayTime = (DAY_TIME / currentTimerSpeed);
		if (currentDayTime < 0.2f)
			swprintf_SS(L"\r\nVitesse du jeu : %.1f\r\n1 jour <=> %.2f sec",	// Le temps d'un jour est tr�s petit : on augmente la pr�cision de son affichage
				currentTimerSpeed, currentDayTime);
		else
			swprintf_SS(L"\r\nVitesse du jeu : %.1f\r\n1 jour <=> %.1f sec",
				currentTimerSpeed, currentDayTime);

		str.append(textW_SS);
		guiManager->guiElements.gameGUI.dateInfosTexte->setText(str.c_str());
	}

	// Actualise le tableau des objectifs, s'il est visible
	if (guiManager->guiElements.gameGUI.objectivesWindow)
		if (guiManager->guiElements.gameGUI.objectivesWindow->isVisible())
			guiManager->guiElements.gameGUI.objectivesWindow->update();

	// Actualise le tableau r�capitulatif des fins de mois, s'il est visible
	if (guiManager->guiElements.gameGUI.monthTableWindow)
		if (guiManager->guiElements.gameGUI.monthTableWindow->isVisible())
			guiManager->guiElements.gameGUI.monthTableWindow->update();

	// Actualise le menu Ressources, s'il est visible
	if (guiManager->guiElements.gameGUI.ressourcesWindow)
		if (guiManager->guiElements.gameGUI.ressourcesWindow->isVisible())
			guiManager->guiElements.gameGUI.ressourcesWindow->update(textTransparency);

	// Descends ou remonte le menu inf�rieur
	if (vitesseDescenteMenuInferieur != 0.0f && guiManager->guiElements.gameGUI.tabGroupBas)
	{
		// Calcule le temps �coul� depuis le d�but de la descente de ce menu
		const u32 realTimeMs = deviceTimer->getRealTime();
		const u32 realElapsedTimeMsDescenteMenu = (realTimeMs > realTimeMsStartDescenteMenuInferieur ? realTimeMs - realTimeMsStartDescenteMenuInferieur : (u32)0);

		// Calcule la nouvelle position en Y du menu inf�rieur
		const u32 screenSizeHeight = driver->getScreenSize().Height;
		core::recti tabGroupBasPos = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition();
		tabGroupBasPos.UpperLeftCorner.Y = menuInferieurStartPosY + core::round32(vitesseDescenteMenuInferieur * (float)(screenSizeHeight * realElapsedTimeMsDescenteMenu));
		guiManager->guiElements.gameGUI.tabGroupBas->setRelativePosition(tabGroupBasPos);

		// V�rifie si le menu est arriv� en haut ou en bas
		const tribool menuInferieurHaut = isMenuInferieurHaut();
		if (vitesseDescenteMenuInferieur > 0.0f && menuInferieurHaut == false)
		{
			// Menu en bas :
			vitesseDescenteMenuInferieur = 0.0f;

			// minPosY
			tabGroupBasPos.UpperLeftCorner.Y =  guiManager->guiElements.gameGUI.tabGroupBas->getParent()->getRelativePosition().LowerRightCorner.Y - guiManager->guiElements.gameGUI.tabGroupBas->getTabHeight();
			guiManager->guiElements.gameGUI.tabGroupBas->setRelativePosition(tabGroupBasPos);

#ifndef MENU_INFERIEUR_REACTIF
			guiManager->guiElements.gameGUI.reduireTab->setText(L"+");
#endif
		}
		else if (vitesseDescenteMenuInferieur < 0.0f && menuInferieurHaut == true)
		{
			// Menu en haut :
			vitesseDescenteMenuInferieur = 0.0f;

			// maxPosY
			tabGroupBasPos.UpperLeftCorner.Y = core::floor32(0.8f * (float)(screenSizeHeight));
			guiManager->guiElements.gameGUI.tabGroupBas->setRelativePosition(tabGroupBasPos);

#ifndef MENU_INFERIEUR_REACTIF
			guiManager->guiElements.gameGUI.reduireTab->setText(L"-");
#endif
		}
	}
}
bool Game::calculerIsMouseOnGUI(gui::IGUIElement* parent)
{
#if 1
	if (!parent)
		parent = gui->getRootGUIElement();

	// Obtient l'�l�ment le plus "au-dessus" qui contient la position de la souris
	gui::IGUIElement* const element = parent->getElementFromPoint(mouseState.position);
	if (element)
	{
		// On v�rifie que l'�l�ment n'est l'�lement de base de la GUI, car cet �l�ment contient tout l'�cran :
		// la souris sera donc obligatoirement dedans, et mouseOnGUI sera toujours �gal � true
		if (element == gui->getRootGUIElement())
			return false;

		// On v�rifie que l'�l�ment n'est pas un �l�ment parent d'une GUI, car ce sont des �l�ments qui entourent tout l'�cran :
		// la souris sera donc obligatoirement dedans, et mouseOnGUI sera toujours �gal � true
		for (int i = 0; i < GUIManager::EGN_COUNT; ++i)
			if (element == guiManager->getGUI((GUIManager::E_GUI_NAME)i))
				return false;

		// Si l'�l�ment point� n'est aucun de ces �l�ments, c'est que la souris est bien sur un �l�ment de la gui
		return true;
	}

	return false;
#else
	// Ancienne Version moins optimis�e : Prototype de la fonction :

	// Calcule si la souris est sur un �l�ment de la GUI
	// maxLoop : nombre maximal d'appels r�currents que peut faire cette fonction (-1 = pas de limites)
	// parent : �l�ment parent � partir duquel v�rifier si la souris est sur cet �l�ment ou sur un de ses enfants
	// currentLoop : num�ro de boucle actuel, ne pas utiliser
	// Retourne true si la souris est sur un �l�ment de la GUI, false sinon
	// bool calculerIsMouseOnGUI(int maxLoop = 5, gui::IGUIElement* parent = 0, int currentLoop = 0);

	if ((maxLoop >= 0 && currentLoop >= maxLoop) || currentLoop < 0)
		return false;

	if (!parent)
		parent = gui->getRootGUIElement();

	// Parcourt tous les enfants de l'�l�ment racine de la GUI (non inclus car ce dernier contient tout l'�cran, donc la souris sera forc�ment dedans)
	const core::list<gui::IGUIElement*> children = parent->getChildren();
	core::list<gui::IGUIElement*>::ConstIterator it = children.begin();
	for (; it != children.end(); ++it)
	{
		// On v�rifie que l'�l�ment n'est pas un �l�ment parent d'une GUI, car ce sont des �l�ments qui entourent tout l'�cran, la souris sera donc forc�ment dedans
		bool valide = true;
		for (int i = 0; i < GUIManager::EGN_COUNT && valide; ++i)
			if ((*it) == guiManager->getGUI(i))
				valide = false;

		if ((*it)->isVisible())
		{
			// On regarde si l'�l�ment est visible et si la position de la souris est dedans
			if ((*it)->isPointInside(mouseState.position) && valide)
			{
				// Si oui, on indique que la souris est sur un �l�ment de la GUI et on quitte
				// (cela ne sert plus � rien de continuer � parcourir les tableaux)
				return true;
			}
			else
			{
				// Sinon, on v�rifie ses enfants :
				// Si la souris est sur un �l�ment de la GUI, on quitte
				if (calculerIsMouseOnGUI(maxLoop, (*it), currentLoop + 1))
					return true;
			}
		}
	}

	return false;
#endif
}
void Game::autoSave()
{
	// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible !
	if (!CAN_WRITE_ON_DISK)
	{
		LOG("AVERTISSEMENT : Impossible de cr�er une sauvegarde automatique : Les droits d'�criture sur le disque ont �t� refus�s !", ELL_WARNING);
		return;
	}

	// On ne peut changer l'�tat de la pause car il est sauvegard�, donc on stoppe le timer manuellement s'il ne l'est pas
	const bool wasTimerStarted = !deviceTimer->isStopped();
	if (wasTimerStarted)
		deviceTimer->stop();

	// Obtient la date r�elle actuelle gr�ce au timer du device
	const ITimer::RealTimeDate timeDate = deviceTimer->getRealTimeAndDate();

	// Cr�e le dossier pour les sauvegardes automatiques
	createDirectory("Saves");	// Ce dossier a normalement d�j� �t� cr�� par l'appel � createDirectories dans le main, mais v�rifie qu'il est bien cr��, au cas o� il aurait �t� supprim� manuellement par le joueur entre-temps
	createDirectory("Saves/AutoSaves");

	// Cr�e le nom du fichier avec la date actuelle et le nom du terrain actuel
	// Format :	"Saves/AutoSaves/24-08-11 21h57 (1422634) terrain plat.ewg"
	//			Pour une partie automatiquement enregistr�e le "24-08-11" � "21h57", dont le temps de jeu est "1422634" ms (= 23 min 42 s 634 ms), sur le terrain "terrain plat"
	//			L'indication du temps de jeu sert surtout � �viter que des sauvegardes automatiques ne s'ajoutent au bout du m�me fichier, sous pr�texte qu'ils ont le m�me nom
	//			(peut arriver si la vitesse du jeu est si rapide que deux sauvegardes sont enregistr�es dans la m�me minute).
	io::path terrainName;
	core::cutFilenameExtension(terrainName, renderer->getTerrainManager().getTerrainInfos().terrainName);
	terrainName = core::deletePathFromFilename(terrainName);

	sprintf_SS("Saves/AutoSaves/%02d-%02d-%02d %02dh%02d (%d) %s.ewg",
		timeDate.Day, timeDate.Month, (abs(timeDate.Year) % 2000),
		timeDate.Hour, timeDate.Minute,
		core::round32(system.getTime().getTotalTime() * 1000.0f),
		terrainName);

	// Enregistre le jeu � cette adresse
	saveCurrentGame(text_SS);

	// Indique l'heure du timer � laquelle l'enregistrement automatique a �t� effectu�
	lastAutoSaveTime = deviceTimer->getTime();

	// Red�marre le timer s'il n'�tait pas stopp�
	if (wasTimerStarted)
		deviceTimer->start();
}
bool Game::saveCurrentGame(const io::path& adresse)
{
	// V�rifie qu'on peut �crire sur le disque dur de l'ordinateur cible !
	if (!CAN_WRITE_ON_DISK)
	{
		LOG("ERREUR : Impossible d'enregistrer la partie actuelle (" << adresse.c_str() << ") : Les droits d'�criture sur le disque ont �t� refus�s !", ELL_ERROR);
		return true;
	}

	// Cr�e les donn�es pour le fichier
	io::IWriteFile* writeFile = fileSystem->createAndWriteFile(adresse);

	// Enregistre la partie en mode "Efficace"
	if (saveCurrentGame_Eff(writeFile))
	{
		if (writeFile)
			writeFile->drop();

		// Indique � l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors de l'enregistrement de la partie !");
		return true;
	}

	if (writeFile)
		writeFile->drop();

	LOG(endl << "Jeu enregistr� : " << adresse.c_str() << endl, ELL_INFORMATION);

	return false;
}
bool Game::saveCurrentGame_Eff(io::IWriteFile* writeFile
#ifdef USE_RAKNET
							   , bool multiplayerSave
#endif
							   )
{
	if (!writeFile)
		return true;

	// Cr�e les donn�es pour le fichier
	io::IXMLWriter* writer = fileSystem->createXMLWriter(writeFile);
	if (!writer)
		return true;

	io::IAttributes* out = fileSystem->createEmptyAttributes(driver);
	if (!out)
	{
		writer->drop();
		return true;
	}

	// Ecrit le header XML
	writer->writeXMLHeader();

	// Ecrit quelques donn�es sur Game
	out->addString("EcoWorldVersion", ECOWORLD_VERSION);
	io::path terrainName = "terrain plat.ewt";
	if (renderer)
	{
		// Obtient le nom du terrain actuellement charg� dans le renderer :

		const core::stringc& rendererTerrainName = renderer->getTerrainManager().getTerrainInfos().terrainName;

		// Supprime le chemin d'acc�s au terrain (ex : "E:\EcoWorld/data/Terrains/Terrain.ewt") pour ne conserver que son nom (ex : "Terrain.ewt") :
		const int lastSlash = rendererTerrainName.findLastChar("/\\", 2);	// Recherche le dernier '/' ou '\'
		if (lastSlash >= 0)	// Si un slash a bien �t� trouv�
		{
			// On ne r�cup�re que la sous-cha�ne de ce terrain � partir de ce caract�re (exclu) :
			terrainName = rendererTerrainName.subString(lastSlash + 1, terrainName.size());
		}
		else
			terrainName = rendererTerrainName;

		// Rend la casse du terrain en minuscule car le syst�me de fichier d'Irrlicht est en minuscules (non n�cessaire en pratique, mais permet de conserver une certaine coh�rence)
		terrainName.make_lower();
	}
	out->addString("TerrainName", terrainName.c_str());
	out->addBool("IsPaused", isPaused);
	out->addFloat("TimerSpeed", deviceTimer->getSpeed());
#ifdef USE_RAKNET
	if (!multiplayerSave)	// On n'enregistre pas les informations de la cam�ra en mode multijoueur
#endif
		out->addBool("LockCamera", lockCamera);

	out->write(writer, false, L"Game");
	out->clear();

	// Ecrit les donn�es de la camera RTS (si une camera FPS est utilis�e, les donn�es de la camera ne seront pas enregistr�es)
	if (
#ifdef USE_RAKNET
		!multiplayerSave &&	// On n'enregistre pas les informations de la cam�ra en mode multijoueur
#endif
		cameraRTS)
	{
		cameraRTS->serializeAttributes(out);

		out->write(writer, false, L"Camera");
		out->clear();
	}

	// Ecrit les donn�es de l'animation de la cam�ra et du GUI Manager
	if (
#ifdef USE_RAKNET
		!multiplayerSave &&	// On n'enregistre pas les informations de l'animation de la cam�ra et de la GUI en mode multijoueur
#endif
		guiManager)
	{
		// Ecrit les donn�es de l'animation de la cam�ra
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->save(out, writer);

		// Ecrit les donn�es du GUI Manager
		guiManager->save(out, writer, (isMenuInferieurHaut() != false));
	}

	// Ecrit les donn�es du renderer
	// TODO : Si n�cessaire
	//if (renderer)
	//	renderer->save(out, writer);

	// Enregistre le syst�me
	system.save(out, writer);

	// Supprime les donn�es pour le fichier
	out->drop();
	writer->drop();

	return false;
}
bool Game::createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const io::path& terrainFilename
#ifdef USE_RAKNET
						 , bool multiplayer
#endif
						 )
{
	// Passe � la sc�ne de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Indique la valeur maximale de la barre de chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->setMaxPercent(100.0f);

	// Charge les donn�es du jeu
	loadGameData(terrainFilename);

	// Indique au syst�me qu'on veut cr�er une nouvelle partie avec la difficult� sp�cifi�e et les objectifs personnalis�s par l'utilisateur
	system.createNewGame(difficulty,
		(guiManager->guiElements.newGameMenuGUI.objectivesModifier ?
			guiManager->guiElements.newGameMenuGUI.objectivesModifier->getObjectives() : EcoWorldSystem::getNewGameDefaultObjectives()));

#ifdef USE_RAKNET
	// Si le mode multijoueur est activ�e, on initialise RakNet
	if (multiplayer)
	{
		if (rkMgr.createGame())
		{
			// Indique � l'utilisateur qu'une erreur s'est produite (avant le changement de sc�ne, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
			showErrorMessageBox(L"Erreur", L"Impossible d'activer le mode multijoueur !");

			// Retourne � la sc�ne du menu de cr�ation de partie
			createMainMenu();
			switchToNextScene(ECS_NEW_GAME_MENU);

			return true;
		}
	}
#endif

	// Passe � la sc�ne du jeu
	switchToNextScene(ECS_PLAY);

	return false;
}
bool Game::loadSavedGame(const io::path& adresse)
{
	// Cr�e les donn�es pour le fichier
	io::IReadFile* const readFile = fileSystem->createAndOpenFile(adresse);
	if (!readFile)
	{
		// Indique � l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie !");
		return true;
	}



	// Passe � la sc�ne de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Charge le jeu avec la m�thode "Efficace"
	bool sameGameVersion = false, startPaused = false;
	core::stringc terrainFilename;
	if (loadSavedGame_Eff(readFile, sameGameVersion, startPaused, terrainFilename))
	{
		// Indique � l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie !");

		// Retourne au menu principal
		createMainMenu();

		// D�sactiv� : On ne lib�re pas le readfile car il a automatiquement �t� lib�r� dans la fonction loadSavedGame_Eff
		//readFile->drop();
		return true;
	}

	// D�sactiv� : On ne lib�re pas le readfile car il a automatiquement �t� lib�r� dans la fonction loadSavedGame_Eff
	//readFile->drop();

	// Passe � la sc�ne du jeu
	switchToNextScene(ECS_PLAY);

	// Restaure l'�tat de la pause
	if (startPaused != isPaused)
		changePause();

#ifdef USE_IRRKLANG
	// Red�marre les sons d'IrrKlang si le mode audio est activ�
	if (gameConfig.audioEnabled)
		ikMgr.setAllSoundsPaused(false);
#endif

	// Avertit l'utilisateur si la version du jeu charg�e est diff�rente de la version actuelle du jeu
	if (!sameGameVersion)
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLa version de cette sauvegarde est diff�rente de la version actuelle du jeu.\r\nDes erreurs pourraient se produire en cours de jeu !");
	// Sinon, avertit l'utilisateur si le terrain sp�cifi� n'a pas �t� trouv�
	else if (!fileSystem->existFile(terrainFilename))
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLe terrain utilis� dans cette sauvegarde n'a pas �t� trouv�.\r\nDes erreurs pourraient se produire en cours de jeu !");

	LOG(endl << "Jeu charg� : " << adresse.c_str() << endl, ELL_INFORMATION);

	return false;
}
bool Game::loadSavedGame_Eff(io::IReadFile* readFile, bool& outSameGameVersion, bool& outStartPaused, core::stringc& outTerrainFilename
#ifdef USE_RAKNET
							 , bool multiplayerLoad
#endif
							 )
{
	// Par d�faut, on consid�re que la version de la sauvegarde est diff�rente
	outSameGameVersion = false;
	outStartPaused = false;
	outTerrainFilename = "";

	// Indique la valeur maximale de la barre de chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->setMaxPercent(200.0f);	// 0.0f � 100.0f : Chargement normal du jeu ; 100.0f � 200.0f : Chargement de la partie sauvegard�e

	// Cr�e les donn�es pour le fichier
	io::IXMLReader* const reader = fileSystem->createXMLReader(readFile);
	if (readFile)
		readFile->drop();	// Le fichier de lecture n'est plus n�cessaire : on le lib�re automatiquement
	if (!reader)
		return true;

	io::IAttributes* const in = fileSystem->createEmptyAttributes(driver);
	if (!in)
	{
		reader->drop();
		return true;
	}

	// Charge quelques donn�es sur Game (avant le chargement complet du jeu pour que la fonction "loadGameData" charge le bon terrain)
	float timerSpeed = 1.0f;
	lockCamera = false;
	//reader->resetPosition();
	if (in->read(reader, false, L"Game"))
	{
		if (in->existsAttribute("EcoWorldVersion"))	outSameGameVersion = (in->getAttributeAsString("EcoWorldVersion").equals_ignore_case(ECOWORLD_VERSION));
		if (in->existsAttribute("TerrainName"))		outTerrainFilename = in->getAttributeAsString("TerrainName");
		if (in->existsAttribute("TimerSpeed"))		timerSpeed = in->getAttributeAsFloat("TimerSpeed");
		if (in->existsAttribute("IsPaused"))		outStartPaused = in->getAttributeAsBool("IsPaused");

		// On garde cette donn�e pour la fin de la fonction :
		if (
#ifdef USE_RAKNET
			!multiplayerLoad &&	// On ne charge pas les informations de la cam�ra en mode multijoueur
#endif
			in->existsAttribute("LockCamera"))		lockCamera = in->getAttributeAsBool("LockCamera");

		in->clear();
	}

	// Charge les donn�es du jeu avec le terrain sp�cifi�, en ne d�marrant les sons qu'en mode pause
	loadGameData(outTerrainFilename, true);

	// Charge les donn�es de la camera RTS
	if (
#ifdef USE_RAKNET
		!multiplayerLoad &&	// On ne charge pas les informations de la cam�ra en mode multijoueur
#endif
		cameraRTS)
	{
		reader->resetPosition();
		if (in->read(reader, false, L"Camera"))
		{
			cameraRTS->deserializeAttributes(in);
			in->clear();
		}

		// Change le blocage de la cam�ra
		cameraRTS->setLockCamera(lockCamera);

		in->clear();
	}

	// Charge les donn�es de l'animation de la cam�ra et du GUI Manager
	// TODO : G�rer la barre de chargement si n�cessaire
	isMouseOnGUI = false;
	vitesseDescenteMenuInferieur = 0.0f;
#ifdef USE_RAKNET
	if (!multiplayerLoad)	// On ne charge pas les informations de l'animation de la cam�ra et de la GUI en mode multijoueur
#endif
	{
		// Charge les donn�es de l'animation de la cam�ra
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->load(in, reader);

		// Charge les donn�es du GUI Manager
		guiManager->load(in, reader);
	}

	// Charge les donn�es du renderer
	// TODO : Si n�cessaire + G�rer la barre de chargement si n�cessaire
	//if (renderer)
	//	renderer->load(in, reader);

	// Charge le syst�me de jeu (il commandera directement au renderer la cr�ation des b�timents)
	system.load(in, reader, guiManager->guiElements.globalGUI.loadingScreen, 100.0f, 200.0f);

	// Indique � la barre de chargement qu'on a termin� le chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->endLoading();

	// Supprime les donn�es pour le fichier
	in->drop();
	reader->drop();



	// Modifie la vitesse du timer du jeu
	deviceTimer->setSpeed(timerSpeed);

	// Indique la nouvelle vitesse du jeu � la GUI
	guiManager->setGameGUITimerSpeed(timerSpeed);

#ifdef USE_SPARK
	// Indique � Spark que la vitesse du jeu a chang�
	renderer->getSparkManager().setTimerSpeed(timerSpeed);
#endif

	return false;
}
void Game::createCamera()
{
	// Cr�e la cam�ra RTS (pour la premi�re et unique fois)
	if (!cameraRTS)
	{
		// Cr�e la cam�ra FPS de la cam�ra RTS
#ifdef CAMERA_CAN_JUMP
		SKeyMap keyMap[10];
#else
		SKeyMap keyMap[8];
#endif
		keyMap[0].Action = EKA_MOVE_FORWARD;
		keyMap[0].KeyCode = KEY_UP;
		keyMap[1].Action = EKA_MOVE_FORWARD;
		keyMap[1].KeyCode = KEY_KEY_Z;
		keyMap[2].Action = EKA_MOVE_BACKWARD;
		keyMap[2].KeyCode = KEY_DOWN;
		keyMap[3].Action = EKA_MOVE_BACKWARD;
		keyMap[3].KeyCode = KEY_KEY_S;
		keyMap[4].Action = EKA_STRAFE_LEFT;
		keyMap[4].KeyCode = KEY_LEFT;
		keyMap[5].Action = EKA_STRAFE_LEFT;
		keyMap[5].KeyCode = KEY_KEY_Q;
		keyMap[6].Action = EKA_STRAFE_RIGHT;
		keyMap[6].KeyCode = KEY_RIGHT;
		keyMap[7].Action = EKA_STRAFE_RIGHT;
		keyMap[7].KeyCode = KEY_KEY_D;
#ifdef CAMERA_CAN_JUMP
		keyMap[8].Action = EKA_JUMP_UP;
		keyMap[8].KeyCode = KEY_KEY_J;
		keyMap[9].Action = EKA_JUMP_UP;
		keyMap[9].KeyCode = KEY_KEY_E;
#endif

		// La camera FPS
		scene::ICameraSceneNode* const cameraFPS = sceneManager->addCameraSceneNodeFPS(0, 100.0f, 0.005f * TAILLE_OBJETS, -1, keyMap,
#ifdef CAMERA_CAN_JUMP
			10,
#ifdef USE_COLLISIONS
			true,
#else
			false,
#endif
			0.05f * TAILLE_OBJETS, false);
#else
			8,
#ifdef USE_COLLISIONS
			true,
#else
			false,
#endif
			0.0f, false);
#endif

		cameraFPS->setNearValue(CAMERA_NEAR_VALUE);
		cameraFPS->setFarValue(CAMERA_FAR_VALUE);

		// La cam�ra RTS
		cameraRTS = new RTSCamera(sceneManager->getRootSceneNode(), -1, -500.0f, 100.0f, 350.0f, 500.0f, cameraFPS);

		cameraRTS->setTerrainLimits(core::rectf(-TERRAIN_LIMITS, -TERRAIN_LIMITS, TERRAIN_LIMITS, TERRAIN_LIMITS));

		cameraRTS->setNearValue(CAMERA_NEAR_VALUE);
		cameraRTS->setFarValue(CAMERA_FAR_VALUE);
	}
	else
	{
		// Supprime les animators de la cam�ra RTS (ils devront �tre recr��s si n�cessaire)
		cameraRTS->removeAnimators();

		// Indique au scene manager que la cam�ra RTS est la cam�ra actuelle
		cameraRTS->setIsCameraFPSEnabled(false);
	}
}
void Game::createMainMenu()
{
	// Passe � la sc�ne de chargement du menu principal
	switchToNextScene(ECS_MAIN_MENU_LOADING);

	// Efface tous les �l�ments du syst�me de jeu et du sceneManager (en conservant la cam�ra RTS si elle existe), et stoppe IrrKlang si autoris�
	resetGame(true, true, !gameState.keepIrrKlang);



	// TEST :
#if defined(_DEBUG) && 0
#ifdef CAN_USE_ROMAIN_DARCEL_RESSOURCES
	if (gameConfig.mainIconsSet == GameConfiguration::EMIS_NEXT_GEN)
	{
		gui::IGUIImage* img = gui->addImage(driver->getTexture("MainMenu Background.jpg"), core::vector2di(0, 0));
		img->getParent()->sendToBack(img);
	}
#endif
#endif



	// Cr�e la cam�ra RTS si elle n'a pas encore �t� cr��e, et la r�initialise
	createCamera();

	// Indique la positions et la direction de la cam�ra RTS (relativement � TAILLE_OBJETS) (le +128.0f en Y est d� au d�calage des terrains en Y de +128.0f)
	cameraRTS->setPosition(core::vector3df(0.0f, 128.0f + 5.0f * TAILLE_OBJETS, 0.0f));	// 100.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTarget(core::vector3df(0.0f, 128.0f, 0.0f));

	// On ne met pas de limites pour la cam�ra, elle n'est pas cens� bouger de la trajectoire que lui fait suivre son animator
	cameraRTS->setZoomLock(false, 0.0f, 550.0f);
	cameraRTS->setRotateXLock(false, 0.0f, 0.0f);
	cameraRTS->setRotateYLock(false, 0.0f, 90.0f);

	// On bloque la camera par d�faut (elle doit �tre bloqu�e pour que les animators puissent fonctionner)
	lockCamera = true;
	cameraRTS->setLockCamera(true);



	// Ajoute l'animator pour faire tourner la cam�ra
	scene::ISceneNodeAnimator* const anim = sceneManager->createFlyCircleAnimator(
		cameraRTS->getPosition(), 500.0f, -0.00001f, core::vector3df(0.0f, 1.0f, 0.0f), 0.25f);
	cameraRTS->addAnimator(anim);
	anim->drop();



	// Compile les effets n�cessaires de PostProcess
	if (gameConfig.usePostProcessEffects && postProcessManager)
	{
		// Compile les shaders particuliers du jeu : shader de rendu final sur l'�cran, shader de tremblement de la cam�ra et shader de profondeur de la sc�ne
		postProcessManager->compileParticularShaders(gameConfig.postProcessShakeCameraOnDestroying, gameConfig.postProcessUseDepthRendering);

		// Compile les effets suppl�mentaires de la configuration du jeu
		postProcessManager->compileEffects(gameConfig.postProcessEffects);
	}

	// Initialise XEffects si ses ombres sont activ�es
	if (gameConfig.useXEffectsShadows && !xEffects)
		xEffects = new EffectHandler(device, gameConfig.xEffectsScreenRTTSize,
			gameConfig.xEffectsUseVSMShadows,
			gameConfig.xEffectsUseRoundSpotlights,
			gameConfig.xEffectsUse32BitDepthBuffers,
			gameConfig.xEffectsFilterType,
			shaderPreprocessor, screenQuad);

	// Re-v�rifie les options de la configuration actuelle du jeu au driver (elles sont d�pendantes de l'utilisation de XEffects)
	applyGameConfigDriverOptions();



	// Si un temps forc� est sp�cifi� dans GameState, on l'applique obligatoirement (peut aussi �tre utilis� � l'appel de cette fonction pour forcer un certain temps)
	if (gameState.lastWeather != -1)
	{
		system.getWeatherManager().setWeather((WeatherID)(gameState.lastWeather));
		gameState.lastWeather = -1;
	}
	else
	{
		system.getWeatherManager().setWeather((WeatherID)(rand() % WI_COUNT));

		LOG_DEBUG("Temps du menu principal : " << system.getWeatherManager().getCurrentWeatherID(), ELL_INFORMATION);
	}

	// Met � jour le titre du menu principal
	if (guiManager)
	{
		// Pr�charge tous les titres du menu principal susceptibles d'�tre affich�s avec cette r�solution, par un changement de temps
		guiManager->loadMainMenuTitles(driver->getScreenSize());

		// Choisis le titre actuel du menu principal
		guiManager->chooseMainMenuTitleFromWeather(system.getWeatherManager().getCurrentWeatherID(), driver->getScreenSize());
		currentWeatherID = system.getWeatherManager().getCurrentWeatherID();
	}

	if (renderer)
	{
		// Initialise le monde grace au renderer
		renderer->initWorld(true);

		// Charge le terrain en d�sactivant la cr�ation des triangles selectors du terrain et du sol invisible et en d�sactivant la conservation des informations sur le terrain en m�moire
		renderer->loadNewTerrain("Terrain Plat.ewt", gameConfig.audioEnabled, false, false);
	}



#ifdef USE_IRRKLANG
	// Joue la musique du menu principal si on peut modifier IrrKlang et qu'on ne doit pas charger directement de partie ou de terrain (on ne d�marre pas la musique du menu principal lors d'un double-clic dans l'explorateur windows)
	if (!gameState.keepIrrKlang)
		ikMgr.playMainMenuMusics(!gameConfig.audioEnabled);
#endif



	// On doit red�marrer dans le menu options, on change donc de sc�ne une fois le menu principal initialis�
	if (gameState.restartToOptionsMenu)
	{
		gameState.restartToOptionsMenu = false;

		// Indique le temps du device avec l'ancien temps
		if (gameState.lastDeviceTime != 0)
		{
			deviceTimer->setTime(gameState.lastDeviceTime);
			lastDeviceTime = gameState.lastDeviceTime;	// R�initialise aussi le temps de derni�re mise � jour du temps �coul�, pour �viter que le prochain temps �coul� ne vale gameState.lastDeviceTime
		}
		gameState.lastDeviceTime = 0;

		// Indique aussi qu'on peut � nouveau modifier IrrKlang
		gameState.keepIrrKlang = false;

		// Change de sc�ne jusqu'au menu options
		switchToNextScene(ECS_OPTIONS_MENU);
	}
	else	// Change de sc�ne jusqu'au menu principal
		switchToNextScene(ECS_MAIN_MENU);
}
void Game::loadGameData(const io::path& terrainFilename, bool startSoundsPaused)
{
	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(0.0f))	return;

	// Indique qu'on ne peut pas conserver IrrKlang pendant le chargement du jeu :
	gameState.keepIrrKlang = false;

	// Efface tous les �l�ments du syst�me de jeu et du sceneManager (en conservant la cam�ra RTS)
	// Note : On ne stoppe pas IrrKlang (sauf si le mode audio est d�sactiv�) pour permettre � la musique de continuer pendant le chargement du jeu
	resetGame(true, true, !gameConfig.audioEnabled);

#ifdef USE_IRRKLANG
	// Si le mode audio est activ�, on stoppe tous les sons (et pas les musiques) d'IrrKlang qui sont en train d'�tre jou�s,
	// sauf les sons de la GUI pour pouvoir entendre le son du clic sur le bouton
	if (gameConfig.audioEnabled)
		ikMgr.stopAllSoundsOnlyExceptType(IrrKlangManager::EST_ui_sound);
#endif



	// Cr�e la cam�ra RTS si elle n'a pas encore �t� cr��e (par exemple, parce que le menu principal n'a pas �t� cr�� : lors d'un double-clic sur une partie ou un terrain dans l'explorateur windows), et la r�initialise
	createCamera();

	// Replace la camera � l'origine (relativement � TAILLE_OBJETS) (le +128.0f en Y est d� au d�calage des terrains en Y de +128.0f)
	cameraRTS->setPosition(core::vector3df(0.0f, 128.0f + 20.0f * TAILLE_OBJETS, -20.0f * TAILLE_OBJETS));	// (0.0f, 400.0f, -400.0f) pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTarget(core::vector3df(0.0f, 128.0f + 2.0f * TAILLE_OBJETS, 0.0f));						// (0.0f, 40.0f, 0.0f) pour TAILLE_OBJETS = 20.0f

	// Indique les valeurs de d�placement de la cam�ra, rendus relatifs � TAILLE_OBJETS :
	cameraRTS->setZoomStep(5.0f * TAILLE_OBJETS);			// 100.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setZoomSpeed(17.5f * TAILLE_OBJETS);			// 350.0f pour TAILLE_OBJETS = 20.0f
	//cameraRTS->setTranslateSpeed(25.0f * TAILLE_OBJETS);	// 500.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTranslateSpeed(40.0f * TAILLE_OBJETS);	// 800.0f pour TAILLE_OBJETS = 20.0f

	// La vitesse de rotation de la cam�ra ne doit pas �tre d�pendante de TAILLE_OBJETS
	cameraRTS->setRotationSpeed(-500.0f);

	// Zoom : 50.0f -> 150.0f -> 250.0f ... -> 550.0f -> 650.0f -> 750.0f -> ... -> 1050.0f pour TAILLE_OBJETS = 20.0f :
	cameraRTS->setZoomLock(true, 2.5f * TAILLE_OBJETS, 52.5f * TAILLE_OBJETS);

	// Limite les mouvements de la cam�ra RTS
	cameraRTS->setRotateXLock(false, 0.0f, 0.0f);
	cameraRTS->setRotateYLock(true, 0.0f, 90.0f);



	// Indique la vitesse actuelle du jeu � la GUI
	guiManager->setGameGUITimerSpeed(deviceTimer->getSpeed());

	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(5.0f))	return;



#ifdef USE_IRRKLANG
	// Charge IrrKlang au complet (doit �tre fait avant le chargement du terrain, car ce dernier utilise des sons durant son chargement)
	if (gameConfig.audioEnabled)
		ikMgr.init(false, guiManager->guiElements.globalGUI.loadingScreen, 5, 25);
#endif



	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(25.0f))	return;



	// Initialise le renderer du jeu
	if (renderer)
	{
		// Charge le terrain
		renderer->loadNewTerrain(
			fileSystem->existFile(terrainFilename) ? terrainFilename.c_str() : "terrain plat.ewt",		// V�rifie que le terrain existe, sinon on en choisi un par d�faut
			gameConfig.audioEnabled, true, true, guiManager->guiElements.globalGUI.loadingScreen, 25, 50);

		if (guiManager->guiElements.globalGUI.loadingScreen)
			if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(50.0f))	return;

		// Initialise compl�tement le monde avec le renderer (charge les b�timents du jeu)
		renderer->initWorld(false, guiManager->guiElements.globalGUI.loadingScreen, 50, 95);
	}



	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(95.0f))	return;



#ifdef USE_COLLISIONS
	// Ajoute le collision response animator � la cam�ra
	if (renderer->getCameraTriangleSelector())
	{
		scene::ISceneNodeAnimatorCollisionResponse* const collider = sceneManager->createCollisionResponseAnimator(renderer->getCameraTriangleSelector(), cameraRTS->getFPSCamera(),
			core::vector3df(0.4f * TAILLE_OBJETS, 0.8f * TAILLE_OBJETS, 0.4f * TAILLE_OBJETS),	// Ancien : (20.0f, 40.0f, 20.0f) pour TAILLE_OBJETS = 40.0f
			core::vector3df(0.0f, -0.3f * TAILLE_OBJETS, 0.0f),									// Ancien : -10.0f pour TAILLE_OBJETS = 20.0f
			core::vector3df(0.0f, 0.0f, 0.0f));
		cameraRTS->setCollisionAnimator(collider);
		collider->drop();
	}
#endif



#ifdef USE_IRRKLANG
	// Arr�te tous les sons d'IrrKlang (dont la musique du menu principal qui est encore en train d'�tre jou�e)
	ikMgr.stopAllSounds();

	// D�marre les musiques et sons
	if (gameConfig.audioEnabled)
	{
		// Joue la musique du jeu
		ikMgr.playGameMusics(startSoundsPaused);

		// Joue les sons d'ambiance
		ikMgr.playAmbiantSounds(startSoundsPaused);
	}
#endif

	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(100.0f);
}
#ifdef USE_RAKNET
bool Game::joinMultiplayerGame(const char* hostIP)
{
	if (!hostIP)
		return true;

	// Demande � RakNet les informations sur la partie multijoueur pour le chargement
	if (rkMgr.queryGameData(hostIP))
	{
		// Indique � l'utilisateur qu'une erreur s'est produite (avant le changement de sc�ne, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible de se connecter � cet h�te !");

		// Retourne � la sc�ne du menu pour rejoindre une partie multijoueur
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}

	// Cr�e une message box de fond pour faire patienter le joueur et lui permettre d'annuler cette recherche
	gui::IGUIWindow* const waitMessageBox = CGUIMessageBox::addMessageBox(gui,
		L"Rejoindre une partie multijoueur", L"Connexion en cours. Veuillez patienter...", gui::EMBF_CANCEL, gui->getRootGUIElement(), true);
	if (waitMessageBox)
	{
		// Grab cette bo�te de dialogue : on aura encore besoin d'y acc�der m�me si sa fonction remove() a �t� appel�e (dans le cas d'un appui sur le bouton Annuler)
		waitMessageBox->grab();

		// Masque le bouton pour fermer cette bo�te de dialogue (seul le bouton annuler est visible)
		waitMessageBox->getCloseButton()->setVisible(false);
	}

	// Les informations sur cette partie multijoueur (sous forme de partie sauvegard�e)
	char* gameSaveData = NULL;
	u32 gameSaveDataSize = 0;

	// Initialisation des variables dans cet ordre : device->run() met � jour le timer d'Irrlicht et deviceTimer->getRealTime() obtient le temps r�el
	bool keepSearchingGameData = device->run();
	const u32 timeoutRealTime = deviceTimer->getRealTime() + 60000;	// S�curit� : 60 secondes max d'attente pour obtenir les informations sur la partie

	// Boucle d'attente des informations sur la partie � rejoindre :
	while (keepSearchingGameData && deviceTimer->getRealTime() < timeoutRealTime)
	{
		// Met � jour RakNet
		rkMgr.update();

		// V�rifie si la r�ponse du serveur n'est pas arriv�e
		if (rkMgr.receivedPackets.size())
		{
			// Parcours la liste des messages de RakNet
			const core::list<RakNetManager::RakNetGamePacket>::Iterator END = rkMgr.receivedPackets.end();
			for (core::list<RakNetManager::RakNetGamePacket>::Iterator it = rkMgr.receivedPackets.begin(); it != END; ++it)
			{
				const RakNetManager::RakNetGamePacket& gamePacket = (*it);
				if (gamePacket.packetType == RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_RECEIVED)
				{
					// La r�ception des donn�es du jeu a r�ussie : on continue le chargement
					keepSearchingGameData = false;

					// Retiens les informations sur la partie multijoueur de ce message
					gameSaveData = gamePacket.gameSavedData.data;
					gameSaveDataSize = gamePacket.gameSavedData.dataSize;

					// Supprime ce message de la liste des messages du jeu puis quitte cette boucle
					rkMgr.receivedPackets.erase(it);
					break;
				}
				else if (gamePacket.packetType == RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_FAILED)
				{
					// La r�ception des donn�es du jeu a �chou�e : on annule le chargement
					keepSearchingGameData = false;

					// Supprime ce message de la liste des messages du jeu puis quitte cette boucle
					rkMgr.receivedPackets.erase(it);
					break;
				}
			}

			// Met � jour le device (pour mettre � jour le timer entre-autres, ainsi que la fen�tre du jeu)
			bool continueSearch = device->run();

			// V�rifie que la message box n'a pas �t� ferm�e (dans ce cas, remove() a �t� appel� : elle n'a d�sormais plus de parent)
			if (waitMessageBox)
				continueSearch &= (waitMessageBox->getParent() != NULL);	// Arr�te la recherche si elle n'a plus de parent

			if (continueSearch)	// V�rifie que la recherche n'a pas �t� annul�e
			{
				// Effectue un rendu rapide de la sc�ne
				if (updateAll())
					render();
			}
			else
			{
				// La fen�tre du jeu a �t� ferm�e : on quitte le chargement
				keepSearchingGameData = false;
				if (gameSaveData)
				{
					delete[] gameSaveData;	// N'oublie pas de d�truire gameData s'il est valide car il a �t� cr�� avec new[]
					gameSaveData = NULL;
					gameSaveDataSize = 0;
				}
			}
		}
	}

	// Supprime la message box de fond
	if (waitMessageBox)
	{
		waitMessageBox->remove();
		waitMessageBox->drop();	// Supprime d�finitivement cette message box, pour annuler le grab pr�c�dent
	}

	// Quitte si on n'a pas pu recevoir les informations sur la partie multijoueur � charger
	if (!gameSaveData || !gameSaveDataSize)
	{
		// Indique � l'utilisateur qu'une erreur s'est produite (avant le changement de sc�ne, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible d'obtenir les informations sur cette partie multijoueur !");

		// Retourne � la sc�ne du menu pour rejoindre une partie multijoueur
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}



	// Cr�e le fichier de lecture de la m�moire pour pouvoir charger les informations re�ues de l'h�te comme un fichier
	io::IReadFile* readFile = fileSystem->createMemoryReadFile(gameSaveData, gameSaveDataSize, "Multiplayer_ServeurSavedGame_Load");

	// Passe � la sc�ne de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Charge le jeu avec la m�thode "Efficace" et en mode multijoueur
	bool sameGameVersion = false, startPaused = false;
	core::stringc terrainFilename;
	if (loadSavedGame_Eff(readFile, sameGameVersion, startPaused, terrainFilename, true))
	{
		// Indique � l'utilisateur qu'une erreur s'est produite (avant le changement de sc�ne, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie envoy�e !");

		// Retourne � la sc�ne du menu pour rejoindre une partie multijoueur
		createMainMenu();
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		// Lib�re les donn�es sur cette partie multijoueur, car elles ont �t� allou�es avec new[]
		//readFile->drop();	// D�sactiv� : D�j� effectu� automatiquement dans loadSavedGame_Eff
		delete[] gameSaveData;
		return true;
	}

	// Lib�re les donn�es sur cette partie multijoueur, car elles ont �t� allou�es avec new[]
	//readFile->drop();	// D�sactiv� : D�j� effectu� automatiquement dans loadSavedGame_Eff
	delete[] gameSaveData;



	// Connecte RakNet � cet h�te (il n'est pas n�cessaire de pr�ciser l'adresse IP de l'h�te, car on y est d�j� connect� avec l'appel � RakNetManager::queryGameData(hostIP))
	if (rkMgr.joinGame())
	{
		// Indique � l'utilisateur qu'une erreur s'est produite (avant le changement de sc�ne, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible de se connecter � cet h�te !");

		// Retourne � la sc�ne du menu pour rejoindre une partie multijoueur
		createMainMenu();
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}



	// Passe � la sc�ne du jeu
	switchToNextScene(ECS_PLAY);

	// Restaure l'�tat de la pause
	if (startPaused != isPaused)
		changePause();

#ifdef USE_IRRKLANG
	// Red�marre les sons d'IrrKlang si le mode audio est activ�
	if (gameConfig.audioEnabled)
		ikMgr.setAllSoundsPaused(false);
#endif

	// Avertit l'utilisateur si la version du jeu de l'h�te est diff�rente de la version actuelle du jeu
	if (!sameGameVersion)
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLa version du jeu de l'h�te est diff�rente de votre version du jeu.\r\nDes erreurs pourraient se produire en cours de jeu !");
	// Sinon, avertit l'utilisateur si le terrain sp�cifi� n'a pas �t� trouv�
	else if (!fileSystem->existFile(terrainFilename))
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLe terrain utilis� par l'h�te n'a pas �t� trouv�.\r\nDes erreurs pourraient se produire en cours de jeu !");

	LOG("Partie multijoueur charg�e avec succ�s. Adresse de l'h�te : " << hostIP, ELL_INFORMATION);

	return false;
}
void Game::updateRakNetGameMessages()
{
	// Met tout d'abord � jour RakNet
	rkMgr.update();

	// V�rifie que des messages ont �t� re�us, sinon on quitte
	if (!(rkMgr.receivedPackets.size()))
		return;

	// Parcours la liste des messages de RakNet
	const core::list<RakNetManager::RakNetGamePacket>::Iterator END = rkMgr.receivedPackets.end();
	for (core::list<RakNetManager::RakNetGamePacket>::Iterator it = rkMgr.receivedPackets.begin(); it != END; ++it)
	{
		const RakNetManager::RakNetGamePacket& packet = (*it);

		switch (packet.packetType)
		{
			// Jeu :
		case RakNetManager::EGPT_GAME_PAUSE_CHANGED:
			if (isPaused != packet.gameIsPaused)
				changePause();
			break;
		case RakNetManager::EGPT_GAME_SPEED_CHANGED:
			deviceTimer->setSpeed(packet.gameSpeed);						// Indique au timer la nouvelle vitesse du jeu
			guiManager->setGameGUITimerSpeed(packet.gameSpeed);				// Modifie la position du curseur de la GUI
#ifdef USE_SPARK
			renderer->getSparkManager().setTimerSpeed(packet.gameSpeed);	// Indique � Spark que la vitesse du jeu a chang�
#endif
			break;
		case RakNetManager::EGPT_WEATHER_CHOOSEN_NEW:
			system.getWeatherManager().setNewWeather((WeatherID)packet.newChoosenWeatherInfos.weatherID, packet.newChoosenWeatherInfos.transitionBeginTime);
			break;
		case RakNetManager::EGPT_BATIMENT_CONSTRUCTED:
			system.addBatiment((BatimentID)(packet.gameConstructInfos.batimentID),
				core::vector2di(packet.gameConstructInfos.indexX, packet.gameConstructInfos.indexY), packet.gameConstructInfos.rotation,
				NULL, NULL, NULL, true, packet.gameConstructInfos.dureeVie);
#ifdef USE_IRRKLANG
			// Joue le son de la cr�ation d'un b�timent accept�e
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
#endif
			break;
		case RakNetManager::EGPT_BATIMENT_DESTROYED:
			system.destroyBatiment(core::vector2di(packet.gameDestroyInfos.indexX, packet.gameDestroyInfos.indexY), NULL, true);
#ifdef USE_IRRKLANG
			// Joue le son de la destruction d'un b�timent accept�e
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_POSITIVE);
#endif
			break;
		case RakNetManager::EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED:
			{
				// V�rifie que l'index fourni est bien dans les limites du terrain
				if (packet.gameBatPPChangeInfos.indexX < 0 || packet.gameBatPPChangeInfos.indexY < 0
					|| packet.gameBatPPChangeInfos.indexX >= TAILLE_CARTE || packet.gameBatPPChangeInfos.indexY >= TAILLE_CARTE)
				{
					LOG_DEBUG("Game::updateRakNetGameMessages() : La modification du pourcentage de production du batiment est forcee, mais n'a pas pu etre effectue car la position specifiee se situe en-dehors des limites du terrain :" << endl
						<< "    indexX = " << packet.gameBatPPChangeInfos.indexX << "   ;   indexY = " << packet.gameBatPPChangeInfos.indexY, ELL_WARNING);
					LOG_RELEASE("AVERTISSEMENT : Modification forc�e du pourcentage de production : Position en-dehors des limites du terrain !", ELL_WARNING);
				}
				else
				{
					Batiment* const bat = system.carte[packet.gameBatPPChangeInfos.indexX][packet.gameBatPPChangeInfos.indexY].batiment;
					if (!bat)
					{
						// V�rifie qu'il existe bien un batiment � cet emplacement, sinon on ne peut le d�truire, et on retourne une erreur
						LOG_DEBUG("Game::updateRakNetGameMessages() : La modification du pourcentage de production du batiment est forcee, mais aucun batiment n'occupe cette position :" << endl
							<< "    indexX = " << packet.gameBatPPChangeInfos.indexX << "   ;   indexY = " << packet.gameBatPPChangeInfos.indexY
							<< "    system.carte[indexX][indexY].batiment = " << bat, ELL_WARNING);
						LOG_RELEASE("AVERTISSEMENT : Modification forc�e du pourcentage de production : Aucun b�timent n'occupe cette position !", ELL_WARNING);
					}
					else
						bat->getInfos().pourcentageProduction = packet.gameBatPPChangeInfos.newProductionPercentage;
				}
			}
			break;

			// Chargement de parties multijoueurs :
		case RakNetManager::EGPT_MULTIPLAYER_GAMES_LIST_CHANGED:
			// Met � jour le GUI Manager avec les parties en r�seau trouv�es
			guiManager->updateMultiplayerGamesTableNewGameGUI(rkMgr.multiplayerGames);
			break;

			// Paquets qui ne doivent normalement pas �tre g�r�s ici :
		case RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_RECEIVED:
			// Message non utilis� ici : on le lib�re de la m�moire
			if (packet.gameSavedData.data)
				delete[] packet.gameSavedData.data;
			LOG_DEBUG("Game::updateRakNetGameMessages() : Paquet EGPT_MULTIPLAYER_GAME_DATA_RECEIVED recu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;
		case RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_FAILED:
			// Message d'�chec vide : on ne fait rien
			LOG_DEBUG("Game::updateRakNetGameMessages() : Paquet EGPT_MULTIPLAYER_GAME_DATA_FAILED recu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;

			// Paquet inconnu :
		default:
			LOG_DEBUG("Game::updateRakNetGameMessages() : Le type d'un packet de jeu est inconnu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;
		}
	}

	// Supprime les messages de la liste des messages � tra�ter
	rkMgr.receivedPackets.clear();
}
#endif
#ifdef USE_JOYSTICKS
float Game::getNormalizedJostickAxis(u32 axis, u32 joystick)
{
	if (joystick > joysticksState.size() || axis > SEvent::SJoystickEvent::NUMBER_OF_AXES)
		return 0.0f;

	return getNormalizedJostickAxis(joysticksState[joystick].Axis[axis]);
}
float Game::getNormalizedJostickAxis(s16 value)
{
	// Les valeurs des axes retourn�es par Irrlicht sont les valeurs "brutes" sorties du Joystick :
	// On �vite une "zone morte" entre -1000 et 1000 pour filtrer les valeurs proches de 0
	if (value >= -1000 && value <= 1000)
		return 0.0f;



	// Normalise la valeur de l'axe retourn�e : �vite les valeurs au-dessus de 1.0f et en dessous de -1.0f :

	// La valeur est comprise entre -32768 et 32767 : on la remet dans les limites de -32767 et 32767 pour pouvoir la normaliser par une simple division
	value = core::clamp<s16>(value, -32767, 32767);

	return ((float)value / 32767.0f);
}
#endif
