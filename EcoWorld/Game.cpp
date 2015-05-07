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
// Macro de débogage (strictement aucun effet en mode Release) ajoutée à (pratiquement) chaque fin de ligne (';') dans les fonctions run() et init() de Game.cpp :
// Elle permet de mesurer les temps de chacune des instructions exécutées et affiche un message d'avertissement lorsqu'une instruction dépasse le temps indiqué (actuellement : 50 ms)
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

#define TITRE_FENETRE_JEU		L"EcoWorld"		// Le titre de la fenêtre du jeu
#define USE_AUTOMATIQUE_PAUSE					// Permet de mettre automatiquement le jeu en pause lorsque la fenêtre principale n'est plus active
//#define LOCK_CAMERA_WHEN_MOUSE_ON_GUI			// Permet de bloquer la camera si la souris est sur un élément de la GUI
#define CAN_CHANGE_DRAW_MODE					// Permet de changer l'état du mode fil de fer et du mode nuage de points pour tous les scenesNodes
#define CAMERA_CAN_JUMP							// Permet à la camera FPS de sauter
//#define SHOW_MONTH_TABLE_EVERY_MONTH			// Si défini le tableau récapitulatif des fins de mois sera automatiquement affiché tous les mois, sinon il sera affiché automatiquement tous les ans

// Effets post-rendu :
#define REAL_TIME_MS_CAMERA_SHAKE_NORMAL_DESTROY	750		// Le temps réel en millisecondes de tremblement de la caméra lorsqu'un bâtiment vient d'être détruit (destruction vonlontaire du joueur)
#define REAL_TIME_MS_CAMERA_SHAKE_OLD_DESTROY		1500	// Le temps réel en millisecondes de tremblement de la caméra lorsqu'un bâtiment vient d'être détruit (destruction involontaire du joueur : durée de vie terminée)

Game::Game() : currentScene(ECS_MAIN_MENU_LOADING), showFPS(false), hasChangedPauseBeforeShowingFileDialog(false), hasChangedPauseBeforeShowingErrorMessageBox(false),
#ifdef _DEBUG
 unlockAllBatiments(false),
#endif
 hasChangedGammaLevel(false), isPaused(false), device(0), driver(0), sceneManager(0), gui(0), deviceTimer(0), fileSystem(0), cameraRTS(0),
 postProcessManager(0), postProcessRender(true), realTimeMsCameraStopShaking(0), xEffects(0), xEffectsRender(true), shaderPreprocessor(0), screenQuad(0),
 lastDeviceTime(0), elapsedTimeMs(0), lastRealDeviceTime(0), realElapsedTimeMs(0), elapsedTime(0.0f), lastAutoSaveTime(0), currentWeatherID(WI_sunny),
 isMouseOnGUI(false), lockCamera(false), renderer(0), guiManager(0), vitesseDescenteMenuInferieur(0.0f), realTimeMsStartDescenteMenuInferieur(0), canHandleKeysEvent(false)
{
	// Réinitialise l'état de toutes les touches du clavier
	for (int i = 0; i < KEY_KEY_CODES_COUNT; ++i)
		keys[i] = false;
}
void Game::run()
{
	// Initialise le device et d'autres paramêtres importants
	u8 nbTests = 0;	// Le nombre de tentatives effectuées pour démarrer Irrlicht
	if (!init())			// D'abord, avec les options de configuration normales
	{
		++nbTests;
		LOG(endl << "AVERTISSEMENT : La création du device a échoué avec les options de configuration normales : Nouvel essai avec les options de configuration précédentes" << endl, ELL_ERROR);DEBUG_TIMER
		
		// Désactive la fenêtre de confirmation des paramêtres du menu options
		gameState.showOptionsConfirmationWindow = false;

		// Conserve l'état actuel de l'audio (car il a été modifié avant le redémarrage de Game, et n'est pas responsable de l'échec du redémarrage)
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
			LOG(endl << "ERREUR : La création du device a échoué avec les anciennes options de configuration : Nouvel essai avec les options de configuration par défaut" << endl, ELL_ERROR);DEBUG_TIMER

			// Conserve l'état actuel de l'audio (car il a été modifié suivant les options actuelles avant le redémarrage de Game, et n'est pas responsable de l'échec du redémarrage)
			gameConfig.resetDefaultValues(true);DEBUG_TIMER

			if (!init())	// Et enfin, avec les options de configuration par défaut
			{
				LOG(endl << "ERREUR FATALE : La création du device a échoué avec les options de configuration par défaut : Fin de l'application !" << endl, ELL_ERROR);DEBUG_TIMER

				// Tout a échoué : on quitte définitivement
				return;
			}
		}
	}

	// Affiche un message à l'utilisateur si les options ont dû être réinitialisées car Irrlicht ne pouvait pas démarrer
	if (nbTests != 0)
	{
		if (gameState.restart)		// Anciennes options de configuration != Options de configuration par défaut, lorsque le jeu redémarre après la modification des options du jeu
		{
			if (nbTests == 1)		// Anciennes options de configuration
				showErrorMessageBox(L"Echec du démarrage", L"Impossible de démarrer EcoWorld avec les paramêtres sélectionnés :\r\nEcoWorld utilise actuellement les paramêtres précédents !");
			else if (nbTests == 2)	// Options de configuration par défaut
				showErrorMessageBox(L"Echec du démarrage", L"Impossible de démarrer EcoWorld avec les paramêtres sélectionnés, ni avec les derniers paramêtres utilisés :\r\nEcoWorld utilise actuellement les paramêtres par défaut !");
		}
		else						// Anciennes options de configuration = Options de configuration par défaut, lorsque le jeu démarre pour la première fois
			showErrorMessageBox(L"Echec du démarrage", L"Impossible de démarrer EcoWorld avec les paramêtres sélectionnés :\r\nEcoWorld utilise actuellement les paramêtres par défaut !");

		// Désactive la fenêtre de confirmation des paramêtres du menu options si un message d'erreur a été affiché :
		// évite que les deux modal screen ne se chevauchent, empêchant ainsi toute intéraction avec l'utilisateur
		gameState.showOptionsConfirmationWindow = false;
	}

	// Evite que le jeu ne redémarre indéfiniment dès qu'il a redémarré une fois
	gameState.restart = false;DEBUG_TIMER

#ifdef USE_AUTOMATIQUE_PAUSE
	bool hasChangedPaused = false;
#endif

	// Entre dans la boule principale
	while (device->run())
	{
		// Si on a demandé le redémarrage (dans le menu options par exemple), on ferme le device puis on quitte la boucle principale.
		// On attend que la sortie se fasse avec la vérification de device->run() pour permettre à la fenêtre d'Irrlicht de gérer ses messages :
		// l'appel à device->closeDevice() semble envoyer un message de fermeture de la fenêtre principale,
		// si device->run() n'est pas appelé, ce message ne sera pas effacé, et il sera envoyé à la prochaine fenêtre d'Irrlicht créée : elle se fermera alors immédiatement
		if (gameState.restart)
		{
			device->closeDevice();
			continue;	// Annule simplement cette itération, mais ne quitte pas la boucle actuelle pour effectuer un prochain appel à device->run()
		}

		// Détermine si la fenêtre de confirmation des paramêtres du menu options est affichée
		// Note : Le pointeur guiManager est toujours valide ici, mais ce n'est pas le cas du pointeur guiManager->guiElements.optionsMenuGUI.optionsMenu qui reste invalide si la GUI n'a pas encore été initialisée
		const bool isOptionsConfirmationWindowShown = (guiManager->guiElements.optionsMenuGUI.optionsMenu ? guiManager->guiElements.optionsMenuGUI.optionsMenu->isConfirmationWindowShown() : false);

		// Vérifie que la fenêtre du jeu est active, sinon on met le jeu en pause
		if (device->isWindowActive()
			|| currentScene == ECS_MAIN_MENU_LOADING || currentScene == ECS_LOADING_GAME		// On n'arrête jamais le device lorsqu'on est dans un chargement
			|| isOptionsConfirmationWindowShown	// On n'arrête jamais le device lorsque la fenêtre de confirmation des paramêtres du menu options est affichée :
												// dans ce cas, cela pourrait aboutir à un blocage du jeu en cas de problème avec l'activation de la fenêtre d'Irrlicht
#ifdef USE_RAKNET
			|| rkMgr.isNetworkPlaying()		// On n'arrête jamais le device par une simple désactivation de la fenêtre lorsqu'on est en pleine partie multijoueur
#endif
			)
		{
#ifdef USE_AUTOMATIQUE_PAUSE
			if (hasChangedPaused)
			{
				// Si on a changé la pause automatiquement, on la remet à sa valeur normale
				if (isPaused)
					changePause();DEBUG_TIMER
				hasChangedPaused = false;
			}
#endif

			// Met à jour toute la scène puis affiche un rendu à l'écran si possible
			if (updateAll())
				render();DEBUG_TIMER
		}
		else
		{
#ifdef USE_AUTOMATIQUE_PAUSE
			// On met le jeu en pause s'il ne l'est pas, et on retient qu'on a changé la pause automatiquement
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
	// Arrête l'action en cours de RakNet
	rkMgr.reset();DEBUG_TIMER
#endif

	// Rétablit le niveau gamma par défaut
	resetGammaRamp();DEBUG_TIMER

	// Bug fix : Masque les deux fenêtres d'ouverture et d'enregistrement de partie pour réinitialiser le dossier de travail du file system d'irrlicht
	// (qui sera utilisé plus tard pour enregistrer le fichier de configuration)
	if (guiManager->guiElements.globalGUI.openFileDialog)
		guiManager->guiElements.globalGUI.openFileDialog->hide();
	if (guiManager->guiElements.globalGUI.saveFileDialog)
		guiManager->guiElements.globalGUI.saveFileDialog->hide();

#ifdef USE_IRRKLANG
	// Indique à l'IrrKlang Manager qu'on va détruire le device
	if (!gameState.keepIrrKlang)
		ikMgr.stopAllSounds();
#endif

	// Indique au système que le renderer va devenir invalide
	system.setSystemRenderer(NULL);

	// Libère la caméra RTS
	if (cameraRTS)
		cameraRTS->drop();
	cameraRTS = NULL;

	// Détruit XEffects
	if (xEffects)
		delete xEffects;
	xEffects = NULL;

	// Détruit le PostProcessManager
	if (postProcessManager)
		delete postProcessManager;

	// Détruit l'écran de rendu
	if (screenQuad)
		delete screenQuad;

	// Détruit le préprocesseur de shader
	if (shaderPreprocessor)
		delete shaderPreprocessor;

	// Détruit le GUI Manager
	if (guiManager)
		delete guiManager;
	guiManager = NULL;

	// Détruit le renderer
	if (renderer)
		delete renderer;
	renderer = NULL;

	// Détruit finalement le device
	device->drop();
	device = NULL;
}
bool Game::init()
{
	// Obtient les paramêtres de création du device
	SIrrlichtCreationParameters params = gameConfig.deviceParams;DEBUG_TIMER
	params.EventReceiver = this;DEBUG_TIMER

	if (!device)	// Vérifie tout de même que le device n'existe pas déjà
		device = createDeviceEx(params);DEBUG_TIMER

	if (!device)
	{
		LOG_DEBUG("Game : La creation du device d'Irrlicht a echouee !", ELL_WARNING);DEBUG_TIMER
		LOG_RELEASE("ERREUR : La création du device d'Irrlicht a échouée !", ELL_WARNING);DEBUG_TIMER
		return false;DEBUG_TIMER
	}
	LOG_DEBUG("Game : Creation du device d'Irrlicht reussie" << endl, ELL_INFORMATION);DEBUG_TIMER

	// Indique le titre de la fenêtre du jeu et si elle est redimensionnable
	device->setWindowCaption(TITRE_FENETRE_JEU);DEBUG_TIMER
	device->setResizable(gameConfig.deviceWindowResizable);DEBUG_TIMER

	// Initialise les pointeurs de base
	driver = device->getVideoDriver();DEBUG_TIMER
	sceneManager = device->getSceneManager();DEBUG_TIMER
	gui = device->getGUIEnvironment();DEBUG_TIMER
	deviceTimer = device->getTimer();DEBUG_TIMER
	fileSystem = device->getFileSystem();DEBUG_TIMER

	// Récupère les valeurs par défaut du niveau gamma
	getDefaultGammaRamp();DEBUG_TIMER

	// Change le niveau Gamma du moniteur si nécessaire
	setNewGammaRamp(gameConfig.gamma);DEBUG_TIMER

	// Indique le dossier de travail par défaut du device
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

	// Alloue la mémoire nécessaire pour les états des joysticks
	const u32 joysticksCount = joysticksInfos.size();
	joysticksState.reallocate(joysticksCount);
	const SEvent::SJoystickEvent joystickEmpty = {0, {0}, 0, 0};
	for (u32 i = 0; i < joysticksCount; ++i)
		joysticksState.push_back(joystickEmpty);
#endif

	// Crée le préprocesseur de shaders
	if (!shaderPreprocessor)
		shaderPreprocessor = new CShaderPreprocessor(fileSystem, driver->getGPUProgrammingServices());

	// Crée l'écran de rendu pour PostProcess et XEffects (doit toujours être créé puisque au moins PostProcess est toujours initialisé)
	if (!screenQuad)
		screenQuad = new CScreenQuad(driver);

	// Initialise toujours PostProcess pour qu'il charge ses données depuis les fichiers de configuration et permette ainsi au menu options de mettre à jour sa partie effets post-rendu
	if (!postProcessManager)
		postProcessManager = new CPostProcessManager(device, shaderPreprocessor, screenQuad);

	// Crée le renderer qui affichera les objets du système (il sera valide aussi longtemps que cette classe)
	if (!renderer)	// Vérifie tout de même que le renderer n'existe pas déjà
	{
		// Demande au sélecteur de bâtiments d'ajouter les pointeurs sur des scene nodes
		core::list<CBatimentSceneNode**> gameSceneNodesPointers;DEBUG_TIMER
		batSelector.pushBackGamePointers(gameSceneNodesPointers);DEBUG_TIMER

		renderer = new EcoWorldRenderer(gameSceneNodesPointers);DEBUG_TIMER
	}

	// Initialise le GUI Manager (il sera valide aussi longtemps que cette classe)
	if (!guiManager)	// Vérifie tout de même que le GUI Manager n'existe pas déjà
		guiManager = new GUIManager();DEBUG_TIMER

	// Indique le renderer de jeu au système (utilisé comme une interface)
	system.setSystemRenderer(renderer);DEBUG_TIMER

	// Crée le skin de la GUI et assigne sa transparence
	guiManager->createGUISkin(gameConfig.guiSkinFile, gameConfig.guiTransparency);DEBUG_TIMER

	// Crée les GUIs
	guiManager->createGUIs();DEBUG_TIMER

#ifdef USE_IRRKLANG
	// Initialise IrrKlang, et précharge les musiques et les sons si l'audio est activé
	if (gameConfig.audioEnabled)
		ikMgr.init(true);DEBUG_TIMER
#endif



	// Détermine si on doit charger le menu principal ou si on démarre directement à la scène du chargement du jeu
	bool loadMainMenu = true;DEBUG_TIMER

	// Vérifie qu'on n'a pas de fichier à ouvrir directement au lancement du jeu (par exemple, par double clic sur une partie sauvegardée ou sur un terrain dans l'explorateur windows)
	if (gameState.fileToOpen.size() > 0)
	{
		// Supprime les guillements éventuels de la chaîne (présents lorsque le chemin d'accès comporte des espaces par exemple)
		gameState.fileToOpen.remove('\"');DEBUG_TIMER

		// Vérifie tout d'abord que le fichier spécifié existe
		if (fileSystem->existFile(gameState.fileToOpen))
		{
			const int extPos = gameState.fileToOpen.findLast('.');DEBUG_TIMER
			if (extPos >= 0)
			{
				// Obtient l'extension du fichier à 4 caractères maximum (on supprime aussi le '.' de l'extension)
				const io::path ext = gameState.fileToOpen.subString(extPos + 1, 4);DEBUG_TIMER

				// Vérifie si le fichier à ouvrir est un terrain (si il se termine par l'extension ".ewt")
				if (ext.equals_ignore_case("ewt"))
				{
					// Si c'est un terrain, on crée une nouvelle partie de difficulté normale avec ce terrain
					loadMainMenu = createNewGame(EcoWorldModifiers::ED_normal, gameState.fileToOpen);DEBUG_TIMER	// Indique qu'on a besoin de charger le menu principal si la création de la nouvelle partie a échoué
				}
				// Sinon, on considère automatiquement que le fichier à charger est une partie sauvegardée
				else
				{
					// Si c'est une partie, on essaie de la charger directement
					loadMainMenu = loadSavedGame(gameState.fileToOpen);DEBUG_TIMER	// Indique qu'on a besoin de charger le menu principal si le chargement de la partie sauvegardée a échoué
				}
			}
		}

		// Oublie enfin le fichier à ouvrir, pour éviter de l'ouvrir à nouveau
		gameState.fileToOpen = "";DEBUG_TIMER
	}

	// Si on n'a pas de fichier à ouvrir, ou si son chargement a échoué : on crée le menu principal
	if (loadMainMenu)
		createMainMenu();DEBUG_TIMER

	return true;DEBUG_TIMER
}
void Game::addGameArchives()
{
	// Ajoute le dossier où se trouve l'executable
	fileSystem->addFileArchive("./", true, true, io::EFAT_FOLDER);

	// Ajoute toutes les archives que le système de fichier peut trouver
	// et vérifie ensuite dans chacune des nouvelles archives ajoutées s'il n'y trouve pas d'autres archives
	for (u32 i = 0; i < fileSystem->getFileArchiveCount(); ++i)
	{
		const io::IFileList* const fileList = fileSystem->getFileArchive(i)->getFileList();
		const u32 fileCount = fileList->getFileCount();
		for (u32 j = 0; j < fileCount; ++j)
		{
			if (fileList->isDirectory(j))	continue;						// Vérifie que le fichier actuel n'est pas un dossier

			const io::path& fileName = fileList->getFileName(j);			// Obtient le nom actuel du fichier
			const int extPos = fileName.findLast('.');						// Trouve le dernier '.' du nom du fichier, pour vérifier son extension
			if (extPos < 0)					continue;						// Vérifie que le nom du fichier contient bien un '.'
			const io::path extension = fileName.subString(extPos + 1, 4);	// Obtient l'extension du fichier à 4 caractères maximum (on supprime aussi le '.' de l'extension)
			if (extension.size() > 3)		continue;						// Tous les tests d'extension ici sont faits avec 3 caractères ou moins, si la taille de l'extension est supérieure, on sait alors que ce n'est pas une archive chargeable ici

			// Vérifie l'extension du fichier pour déterminer si c'est une archive :

			if (extension.equals_ignore_case(L"ewd"))
			{
				// Archive EWD : EcoWorld Data (en fait : une archive ZIP avec une extension spécialement modifiée pour EcoWorld)
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

	// Modifie le matériau de remplacement du driver :

	// Active le matériau de remplacement pour toutes les passes de rendu et pour certains paramêtres des matériaux
	video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
	overrideMat.Enabled = true;
	overrideMat.EnablePasses = 127;		// ESNRP_CAMERA | ESNRP_LIGHT | ESNRP_SKY_BOX | ESNRP_SOLID | ESNRP_TRANSPARENT | ESNRP_TRANSPARENT_EFFECT | ESNRP_SHADOW = 127

	// Vérifie si XEffects doit gérer la lumière de la scène, ou si on laisse le scene manager la gérer
	if (gameConfig.useXEffectsShadows && xEffects)
	{
		// XEffects est activé :

		// Indique les paramêtres du matériau de remplacement du driver
		overrideMat.EnableFlags = 18187;	// EMF_WIREFRAME | EMF_POINTCLOUD | EMF_LIGHTING | EMF_BILINEAR_FILTER | EMF_TRILINEAR_FILTER | EMF_ANISOTROPIC_FILTER | EMF_ANTI_ALIASING = 18187

		// Désactive la lumière du scène manager
		overrideMat.Material.Lighting = false;
	}
	else
	{
		// XEffects est désactivé :

		// Indique les paramêtres du matériau de remplacement du driver
		overrideMat.EnableFlags = 18179;	// EMF_WIREFRAME | EMF_POINTCLOUD | EMF_BILINEAR_FILTER | EMF_TRILINEAR_FILTER | EMF_ANISOTROPIC_FILTER | EMF_ANTI_ALIASING = 18179
	}

	// Indique les paramêtres du matériau de remplacement
	overrideMat.Material.Wireframe = gameConfig.drawWireframe;
	overrideMat.Material.PointCloud = gameConfig.drawPointCloud;
	for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i)	// Pour l'application correcte des paramêtres de rendu des textures, la version d'Irrlicht modifiée pour EcoWorld est nécessaire : elle résout un bug qui empêchait son application
	{
		overrideMat.Material.TextureLayer[i].BilinearFilter = gameConfig.bilinearFilterEnabled;
		overrideMat.Material.TextureLayer[i].TrilinearFilter = gameConfig.trilinearFilterEnabled;
		overrideMat.Material.TextureLayer[i].AnisotropicFilter = gameConfig.anisotropicFilter;
	}
	// TODO : Pouvoir aussi activer l'anticrénelage spécifique pour les matériaux transparents (EAAM_ALPHA_TO_COVERAGE, quand le type du matériau est EMT_TRANSPARENT_ALPHA_CHANNEL_REF)
	overrideMat.Material.AntiAliasing = gameConfig.antialiasingMode;



	// Modifie aussi les paramêtres de création des textures du driver :

	// Indique au driver qu'il doit créer des textures de la meilleure qualité possible si la qualité des textures est réglée sur Haute
	if (gameConfig.texturesQuality == GameConfiguration::ETQ_HIGH)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, true);
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
	}
	// Sinon, indique au driver qu'il doit créer des textures ayant un temps de rendu le plus petit possible si la qualité des textures est réglée sur Très Basse
	else if (gameConfig.texturesQuality == GameConfiguration::ETQ_VERY_LOW)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);

		// Le paramêtre ETCF_OPTIMIZED_FOR_SPEED provoque des bugs avec OpenGL lors de la création de textures qui ont une taille de 1537x1537 :
		/*
			- BUG : Terrains : Crash chargement terrains "Beach City" et "Hill City" en OpenGL en qualité de texture très basse seulement (-> ETCF_OPTIMIZED_FOR_SPEED activé) ! (Vient de la taille 1537x1537 de la texture !)
				-> OpenGL + ETCF_OPTIMIZED_FOR_SPEED activé + Texture size = 1537x1537   =>   driver->getTexture Crash !
				=> Voir COpenGLTexture.cpp : Ligne 306
		*/
		// On n'active donc pas l'optimisation de vitesse pour ce driver
		if (driver->getDriverType() != EDT_OPENGL)
			driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, true);

		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
	}
	// Sinon, indique au driver qu'il doit créer des textures en mode 16 bits si on est en mode plein écran avec une profondeur de couleur de 16 bits ou moins
	// (les profondeurs de couleur ne sont valables qu'en plein écran ; et les textures 32 bits sont inutiles en mode 16 bits)
	else if (gameConfig.deviceParams.Bits <= 16 && gameConfig.deviceParams.Fullscreen)
	{
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_QUALITY, false);
		driver->setTextureCreationFlag(video::ETCF_OPTIMIZED_FOR_SPEED, false);
		driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, true);
	}
}
bool Game::updateAll()
{
	// Calcule le temps écoulé depuis la dernière frame
	calculateElapsedTime();

#ifdef USE_IRRKLANG
	// Met à jour la position de la caméra d'IrrKlang
	if (cameraRTS)
		ikMgr.update(cameraRTS->getAbsolutePosition(), cameraRTS->getTarget(), elapsedTime);
	else
		ikMgr.update(core::vector3df(0.0f), core::vector3df(1.0f), elapsedTime);
#endif

#ifdef USE_RAKNET
	// Met à jour RakNet et le jeu avec les messages reçus du réseau :
	// On ne met pas à jour RakNet si on attend des informations pour le chargement du jeu :
	// cela signifie que c'est la fonction "Game::joinMultiplayerGame" qui nous a appelé, et qui gère donc RakNet et ses messages actuellement
	if (rkMgr.getCurrentNetworkState() != RakNetManager::ENGS_WAITING_FOR_GAME_DATA)
		updateRakNetGameMessages();
#endif

	// Actualisation de la scène du jeu désactivée si le menu échap est affiché ou si le menu d'enregistrement/chargement de partie est affiché
	if (currentScene == ECS_PLAY && !guiManager->guiElements.gameGUI.menuWindow->isVisible()
		&& !guiManager->guiElements.globalGUI.openFileDialog->isVisible() && !guiManager->guiElements.globalGUI.saveFileDialog->isVisible())
	{
		// Met à jour la fenêtre d'animation de la caméra RTS (doit être fait à chaque frame de la scène du jeu, même lorsque cette fenêtre n'est pas visible)
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->udpate();

		// Evite d'effectuer certaines mises à jour inutiles lorsque le jeu est en pause (mises à jour seulement dépendantes du timer du jeu)
		if (!isPaused)
		{
#ifdef SHOW_MONTH_TABLE_EVERY_MONTH
			// Retiens le nombre de mois actuel
			const u32 lastMois = system.getTime().getTotalMois();
#else
			// Retiens le nombre d'années actuel
			const u32 lastAnnees = system.getTime().getAnnees();
#endif

			// Actualise le monde, en se souvenant si un bâtiment a été détruit ou non
			bool normalBatimentDestroy = false, oldBatimentDestroy = false;
			system.update(elapsedTime, &normalBatimentDestroy, &oldBatimentDestroy);

			// Indique que la caméra doit trembler si un bâtiment vient d'être détruit (uniquement si les effets post-rendu sont activés)
			if (gameConfig.usePostProcessEffects && gameConfig.postProcessShakeCameraOnDestroying && postProcessManager && (normalBatimentDestroy || oldBatimentDestroy))
			{
				// Détermine le temps réel auquel l'effet de tremblement de la caméra devra s'arrêter
				realTimeMsCameraStopShaking = deviceTimer->getRealTime() + (oldBatimentDestroy ? REAL_TIME_MS_CAMERA_SHAKE_OLD_DESTROY : REAL_TIME_MS_CAMERA_SHAKE_NORMAL_DESTROY);
			}

			// Vérifie si le jeu est terminé
			if (system.isGameFinished())
			{
				// Vérifie si le joueur a perdu
				if (system.isGameLost())
				{
					// Si le joueur a perdu, on change de scène jusqu'à l'écran lui indiquant qu'il a perdu
					switchToNextScene(ECS_GAME_LOST);
					return false;
				}
				// Vérifie si le joueur a gagné
				else if (system.isGameWon())
				{
					// Si le joueur a gagné, on change de scène jusqu'à l'écran lui indiquant qu'il a gagné
					switchToNextScene(ECS_GAME_WON);
					return false;
				}
			}

			// Vérifie si le temps pour la sauvegarde automatique est atteint : dans ce cas, on l'effectue
			if (gameConfig.autoSaveFrequency != 0
				&& deviceTimer->getTime() >= lastAutoSaveTime + (gameConfig.autoSaveFrequency * 1000))
				autoSave();

#ifdef SHOW_MONTH_TABLE_EVERY_MONTH	// Affichage tous les mois
			// Si le nombre de mois a changé, on affiche le tableau récapitulatif des fins de mois et on lui donne le focus
			if (system.getTime().getTotalMois() != lastMois && guiManager->guiElements.gameGUI.monthTableWindow)
#else								// Affichage tous les ans
			// Si le nombre d'années a changé, on affiche le tableau récapitulatif des fins de mois et on lui donne le focus
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

				// Donne le focus à cette fenêtre
				gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);
			}

			// Actualise le renderer (dépendant de la pause du jeu car il ne met à jour que le temps du monde, qui est dépendant de la mise à jour du système)
			renderer->update();
		}

		// N'actualise la GUI que si on n'est pas en mode caméra FPS et que la caméra RTS n'est pas animée par le module d'animation personnalisée
		if (!cameraRTS->getIsCameraFPSEnabled() && !guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
		{
			// Actualise la GUI du jeu si elle est visible
			if (guiManager->isGUIVisible(GUIManager::EGN_gameGUI))
				updateGameGUI();

			// Actualise le placement et la sélection des bâtiments
			batSelector.update();
		}
	}
	else if (currentScene != ECS_MAIN_MENU_LOADING && currentScene != ECS_LOADING_GAME)
	{
		// Actualise le menu principal et ses sous-menus

		// Met à jour le temps du système de jeu pour le menu principal
		system.updateMainMenu(elapsedTime);

		// Met à jour le titre du menu principal si le temps a changé
		if (currentScene == ECS_MAIN_MENU && system.getWeatherManager().getCurrentWeatherID() != currentWeatherID)
		{
			guiManager->chooseMainMenuTitleFromWeather(system.getWeatherManager().getCurrentWeatherID(), driver->getScreenSize());
			currentWeatherID = system.getWeatherManager().getCurrentWeatherID();
		}

		// Actualise le renderer avec le temps du système de jeu
		renderer->update();
	}

	return true;
}
void Game::render(bool drawSceneManager, bool drawGUI, bool reinitScene)
{
	// Réinitialise la scène du driver : restaure le viewport et commence une nouvelle scène
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
			// Active le matériau de remplacement du driver si jamais il a été désactivé
			video::SOverrideMaterial& overrideMat = driver->getOverrideMaterial();
			if (!overrideMat.Enabled)
			{
				LOG_DEBUG("Game::render(...) : Le materiau de remplacement du driver n'est pas active : overrideMat.Enabled = " << overrideMat.Enabled << " !", ELL_WARNING);
				overrideMat.Enabled = true;
			}
		}
#endif

		// Prépare les effets post-rendu (uniquement si un nouveau rendu vient de commencer : si reinitScene est à true)
		const irr::core::array<core::stringw>& postProcessEffects = gameConfig.postProcessEffects;
		const u32 postProcessEffectsCount = postProcessEffects.size();
		const bool renderShakeCamera = (gameConfig.postProcessShakeCameraOnDestroying && deviceTimer->getRealTime() < realTimeMsCameraStopShaking);
		const bool renderPostProcess = (gameConfig.usePostProcessEffects && (postProcessEffectsCount || renderShakeCamera || true) && postProcessManager && reinitScene && postProcessRender);
		video::ITexture* renderTarget = NULL;
		if (renderPostProcess)
			renderTarget = postProcessManager->prepare();

		// Prépare le rendu de l'eau avec shaders
		RealisticWaterSceneNode* const shaderWaterSceneNode = renderer->getTerrainManager().getShaderWaterSceneNode();
		if (shaderWaterSceneNode)
		{
			// Retiens le dernier état du viewport du driver, car il est susceptible d'être modifié par le pré-rendu de l'eau avec shaders
			const core::recti lastViewPort = game->driver->getViewPort();

			shaderWaterSceneNode->preRender(renderTarget);

			// Si on ne doit pas conserver l'état du driver, on veille à ce que son viewport soit bien restauré 
			if (!reinitScene)
				driver->setViewPort(lastViewPort);
		}

		// Vérifie si XEffects est activé : dans ce cas, son appel remplace l'appel à sceneManager->drawAll() !
		if (gameConfig.useXEffectsShadows && xEffects && xEffectsRender)
			xEffects->update(renderTarget);	// Spécifie aussi la texture RTT dans laquelle effectuer le rendu (nécessaire si les effets post-rendu sont activés)
		else	// Sinon, demande directement au scène manager de réaliser un rendu
			sceneManager->drawAll();

#if 1
		// Dessine la grille du terrain
		// (Note : On admettra que ce système fonctionne parfaitement, que TAILLE_CARTE soit pair ou non)
		if (currentScene == ECS_PLAY && keys[KEY_KEY_G] && canHandleKeysEvent)		// Un appui sur la touche G en mode débogage permet d'afficher la grille du terrain (on vérifie aussi qu'on peut gérer les touches du clavier)
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
			for (int x = 0; x <= TAILLE_CARTE; ++x)	// Conserver le '<=', car il y a TAILLE_CARTE + 1 horizontales à tracer, et autant de verticales
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
					// Défini la couleur suivant la constructibilité du terrain et si il est situé en eau profonde
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
			// Effectue le rendu de profondeur si nécessaire
			if (gameConfig.postProcessUseDepthRendering)
				postProcessManager->renderDepth(gameConfig.postProcessDefaultDepth);

			// Prépare le driver pour les rendus du screen quad que demandera PostProcess
			screenQuad->preRender();

			// Rendu de chacun des effets
			for (u32 i = 0; i < postProcessEffectsCount; ++i)
				postProcessManager->render(postProcessEffects[i]);

			// Dessine l'effet de tremblement de l'écran si nécessaire
			if (renderShakeCamera)
				postProcessManager->render(EFFECT_NAME_SHAKE);

			// Effectue un rendu final des effets sur l'écran
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
		// Dessine les contours du rectangle de sélection
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

		if (showFPS) // Affiche les infos en bas de l'écran
		{
			gui::IGUIFont* const font = gui->getSkin()->getFont();
			if (font)
			{
				swprintf_SS(L"%ls     %3d FPS     %0.3f x10^3 triangles this frame     %0.3f x10^3 triangles average (every 1.5s)     %0.3f x10^6 triangles drawn (total)",
					driver->getName(), driver->getFPS(),
					(float)driver->getPrimitiveCountDrawn(0) * 0.001f,		// Triangles dessinés cette frame
					(float)driver->getPrimitiveCountDrawn(1) * 0.001f,		// Moyenne des triangles dessinés toutes les 1.5s
					(float)driver->getPrimitiveCountDrawn(2) * 0.000001f);	// Total de triangles dessinés par ce driver

				// En bas à gauche mais juste au-dessus du menu de construction
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
		// Stoppe l'action en cours de RakNet, sauf s'il est en train de charger les données pour la partie mutlijoueur (dans ce cas, il faut le conserver connecté au serveur durant son chargement)
		if (rkMgr.getCurrentNetworkState() != RakNetManager::ENGS_WAITING_FOR_GAME_DATA)	// Cela signifie qu'on a été appelé par joinMultiplayerGame, qui désire conserver RakNet en l'état
			rkMgr.reset();
#endif

		// Stoppe le timer tant qu'il reste des starts (ils sont comptés)
		while (!deviceTimer->isStopped()) deviceTimer->stop();

		// Redémarre le timer tant qu'il reste des stops (ils sont comptés)
		while (deviceTimer->isStopped()) deviceTimer->start();

		// Stoppe le timer par défaut
		deviceTimer->stop();

		// Remet le temps à zéro
		deviceTimer->setTime(0);

		// Remet la vitesse normale
		deviceTimer->setSpeed(1.0f);

		// Remet le temps écoulé à zéro
		lastDeviceTime = 0;
		elapsedTimeMs = 0;
		lastRealDeviceTime = 0;
		realElapsedTimeMs = 0;
		elapsedTime = 0.0f;

		// Remet le système de jeu à zéro
		system.reset();

		// Arrête l'animation de la caméra RTS et réinitialise l'animation personnalisée
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			if (guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
				guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

			guiManager->guiElements.gameGUI.cameraAnimatorWindow->reset();
		}

		// Grab la caméra RTS pour la conserver si nécessaire (la caméra FPS est automatiquement conservée, comme enfant de la caméra RTS)
		if (keepCamera && cameraRTS)
			cameraRTS->grab();

		// Efface tous les éléments du scene manager
		sceneManager->clear();

		// Attache à nouveau la caméra RTS au scene manager puis la droppe une dernière fois
		if (keepCamera && cameraRTS)
		{
			cameraRTS->setParent(sceneManager->getRootSceneNode());
			cameraRTS->drop();

			// Débloque la caméra RTS si elle était bloquée
			cameraRTS->setLockCamera(false);

			// Indique au scene manager que la caméra RTS est la caméra actuelle
			sceneManager->setActiveCamera(cameraRTS);
		}
		else	// Si on ne doit pas les conserver, on réinitialise son pointeur car il est maintenant invalide
			cameraRTS = NULL;

		// Réinitialise l'effet de flou de profondeur de PostProcess
		if (postProcessManager)
			postProcessManager->clearDepthPass();
		realTimeMsCameraStopShaking = 0;

		// Réinitialise XEffects
		if (xEffects)
			xEffects->reset();

		if (renderer)
		{
			// Remet le renderer à zéro
			renderer->reset();

#ifdef USE_SPARK
			// Restaure la vitesse du timer de Spark
			renderer->getSparkManager().setTimerSpeed(1.0f);
#endif
		}

		// Réinitialise le sélecteur de bâtiments
		batSelector.reset();

		// Remet tous les pointeurs à NULL et désactive les éléments de la jouabilité et du système
		lastAutoSaveTime = 0;
		isPaused = false;
		lockCamera = false;

		// Cette valeur nécessite d'être réinitialisée avant le passage à la scène du jeu car elle n'est mise à jour que dans celle-ci (voir Game::OnEvent())
		isMouseOnGUI = false;

		// Fonctions spéciales valides dans toutes les scènes du jeu, donc non réinitialisées :
		//showFPS = false;
		//drawWireframe = false;
		//drawPointCloud = false;
	}

	/*
	if (resetGUI)
	{
		// Remet à zéro la GUI
		guiManager->resetGUI();

		// Remet les indications sur la GUI à leurs valeurs par défaut
		isMouseOnGUI = false;
		hasChangedPauseBeforeShowingFileDialog = false;
		hasChangedPauseBeforeShowingErrorMessageBox = false;
		canHandleKeysEvent = false;
		vitesseDescenteMenuInferieur = 0.0f;
		realTimeMsStartDescenteMenuInferieur = 0;
		currentWeatherID = WI_sunny;

		// Recrée le skin de la GUI
		guiManager->createGUISkin(gameConfig.guiSkinFile, gameConfig.guiTransparency);

		// Recrée complètement la GUI
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

	if (isPaused && !deviceTimer->isStopped()) // Stoppe le timer si on est en pause et qu'il n'a pas été stoppé
	{
		deviceTimer->stop();

		LOG_DEBUG("Game::calculerElapsedTime() : Timer non stoppe alors que la pause est activee !", ELL_WARNING);
	}
	if (deviceTimer->isStopped())
		return;
	*/

	// Temps du timer écoulé
	const u32 currentDeviceTime = deviceTimer->getTime();
	elapsedTimeMs = currentDeviceTime - lastDeviceTime;
	lastDeviceTime = currentDeviceTime;
	elapsedTime = ((float)elapsedTimeMs * 0.001f);

	// Temps écoulé réel
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
			// Recalcule si la souris est sur un élement de la GUI
			isMouseOnGUI = calculerIsMouseOnGUI();

			bool lockCam = lockCamera;
			lockCam |= isMouseOnGUI;

			// On bloque la camera RTS si la souris est sur un élément de la GUI ou si l'utilisateur a demandé de la bloquer
			if (cameraRTS)
				cameraRTS->setLockCamera(lockCam);
#else
			// Recalcule si la souris est sur un élement de la GUI (seulement utilisé lorsque la scène actuelle est ECS_PLAY)
			if (currentScene == ECS_PLAY)
				isMouseOnGUI = calculerIsMouseOnGUI();
#endif
		}
	}
#ifdef USE_JOYSTICKS
	else if (event.EventType == EET_JOYSTICK_INPUT_EVENT)
	{
		// Copie l'état du joystick
		if (event.JoystickEvent.Joystick < joysticksState.size())
			joysticksState[event.JoystickEvent.Joystick] = event.JoystickEvent;
	}
#endif

	// Vérifie si l'event est pas un texte de log (on affiche les données de log même si le device n'a pas encore été initialisé, puisqu'il n'est pas nécessaire)
	if (event.EventType == EET_LOG_TEXT_EVENT)
		return OnEventLog(event);

	if (!device || !guiManager || !renderer)	// Si le device, le gui manager ou le renderer n'ont pas encore été initialisés, on quitte
		return false;

	// Vérifie si on peut gérer les évènements du clavier ou non
	canHandleKeysEvent =
		(guiManager->guiElements.gameGUI.notesWindow ? !guiManager->guiElements.gameGUI.notesWindow->isTextFocused() : true)									// Le texte de notes a-t-il le focus ?
		&& (guiManager->guiElements.gameGUI.cameraAnimatorWindow ? !guiManager->guiElements.gameGUI.cameraAnimatorWindow->hasKeyFocus() : true)					// La fenêtre d'animation de la caméra a-t-elle le focus ?
		&& (guiManager->guiElements.newGameMenuGUI.objectivesModifier ? !guiManager->guiElements.newGameMenuGUI.objectivesModifier->isEditBoxFocused() : true)	// L'edit box des objectifs a-t-il le focus ?
		&& (guiManager->guiElements.globalGUI.openFileDialog ? !guiManager->guiElements.globalGUI.openFileDialog->isVisible() : true)							// La boîte de dialogue d'ouverture de partie a-t-elle le focus ?
		&& (guiManager->guiElements.globalGUI.saveFileDialog ? !guiManager->guiElements.globalGUI.saveFileDialog->isVisible() : true);							// La boîte de dialogue d'enregistrement de partie a-t-elle le focus ?

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
		// Envoie cet évènement à l'IrrKlangManager avant qu'il soit été traîté par la fonction OnEventMenus :
		// En effet, si l'évènement est le clic du joueur sur le bouton pour commencer une partie,
		// il faut qu'on entende le son du clic avant le chargement du jeu (commandé par OnEventMenus), et non pas après
		ikMgr.playGUISound(event);
#endif
		return OnEventMenus(event);
	}

	// Seul la méthode OnEventGUI est capable de gérer les évenements si une fenêtre est affichée
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
		if (sendEventToIrrKlangMgr)	// Vérifie qu'on peut toujours envoyer cet évènement à l'IrrKlangManager
			ikMgr.playGUISound(event);
		return toReturn;
#else
		return OnEventGUI(event);
#endif
	}

	// Si l'évènement a lieu durant la scène du jeu
	if (currentScene == ECS_PLAY)
	{
		// Envoie cet évènement au sélecteur de bâtiments
		if (batSelector.OnEvent(event))	// Si l'évènement a été géré, on n'a plus à le faire
			return true;

		// Envoie cet évènement au système de jeu
		if (OnEventGame(event))			// Si l'évènement a été géré, on n'a plus à le faire
			return true;
	}

	// On ne gère ici que les entrées du clavier
	if (event.EventType != EET_KEY_INPUT_EVENT)
		return false;

	// Ici, on gère les évènements du clavier qui sont valables pendant n'importe quelle scène du jeu

	if (event.KeyInput.Key == KEY_ESCAPE)						// Gère l'appui sur Echap
		return onEchap(event.KeyInput.PressedDown);	// ECHAP
	if (event.KeyInput.Key == KEY_RETURN && canHandleKeysEvent)	// Gère l'appui sur Entrée (sauf si une EditBox a le focus : dans ce cas, les appuis sur Entrée seront gérées par celle-ci par l'envoi d'évènements EGET_EDITBOX_ENTER)
		return onEnter(event.KeyInput.PressedDown);	// ENTREE
	else if ((event.KeyInput.Key == KEY_SNAPSHOT || event.KeyInput.Key == KEY_F12) && !event.KeyInput.PressedDown)
	{
		// Print Screen ou F12 (anciennement : F9) :
		// Crée une capture d'écran

		const bool isTimerStarted = !(deviceTimer->isStopped());
		if (isTimerStarted)
			deviceTimer->stop(); // Stoppe le timer s'il ne l'est pas

		createScreenShot();

		if (isTimerStarted)
			deviceTimer->start(); // Redemarre le timer si on l'a stoppé
	}
	else if ((event.KeyInput.Key == KEY_NUMPAD0 || event.KeyInput.Key == KEY_KEY_0)
		&& event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + 0 :
		// Active ou désactive l'affichage des FPS à l'écran

		showFPS = !showFPS;
	}
	else if (event.KeyInput.Key == KEY_KEY_E && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + E :
		// Active ou désactive les effets post-rendu (PostProcess)

		postProcessRender = !postProcessRender;
	}
	else if (event.KeyInput.Key == KEY_KEY_X && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + X :
		// Active ou désactive le rendu des ombres (XEffects)

		xEffectsRender = !xEffectsRender;
	}
#ifdef CAN_CHANGE_DRAW_MODE
	else if (event.KeyInput.Key == KEY_KEY_W && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + W :
		// Met tout en mode fil de fer (et enlève le mode nuage de points)

		gameConfig.drawWireframe = !(gameConfig.drawWireframe);
		gameConfig.drawPointCloud = false;

		applyGameConfigDriverOptions();
	}
	else if (event.KeyInput.Key == KEY_KEY_P && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + P :
		// Met tout en mode nuage de points (et enlève le mode fil de fer)

		gameConfig.drawWireframe = false;
		gameConfig.drawPointCloud = !(gameConfig.drawPointCloud);

		applyGameConfigDriverOptions();
	}
#endif

	// Outils de débogage
#ifdef _DEBUG
	else if (event.KeyInput.Key == KEY_KEY_T && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
	{
		// Ctrl + T :
		// Demande à l'utilisateur de choisir un nouveau temps

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

	// Ici, on gère les évènements envoyés par le logger d'Irrlicht lorsqu'un texte de log demande à être affiché

	// Envoie le texte de log dans le logger d'EcoWorld
	// (permet ainsi de rediriger les logs d'Irrlicht dans le fichier de log d'EcoWorld en cas d'absence de console)
	LOG(event.LogEvent.Text, event.LogEvent.Level);

	// Evite qu'Irrlicht envoie à son tour le texte de log dans la console :
	return true;
}
bool Game::OnEventMenus(const SEvent& event)
{
	if (event.EventType != EET_GUI_EVENT || !guiManager)
		return false;

	// Ici, on gère les évènements de la GUI qui ont lieu pendant les scènes de menu (menu principal, menu options, menu de création d'une nouvelle partie, menu de jeu terminé)

	// Vérifie qu'un bouton a bien été cliqué
	if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.newGameBouton)
		{
			// Avance à la scène pour créer une nouvelle partie
			switchToNextScene(ECS_NEW_GAME_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.chargerBouton)
		{
			// Affiche la boîte de dialogue pour charger un fichier,
			// en prenant un modal screen parent pour qu'il puisse avoir accès aux évènements, mais pas les autres boutons du menu principal
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.optionsBouton)
		{
			// Change de scène pour passer au menu des options
			switchToNextScene(ECS_OPTIONS_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.quitterBouton)
		{
#ifdef ASK_BEFORE_QUIT
			if (!guiManager->guiElements.mainMenuGUI.quitterWindow)
			{
				// Crée la fenêtre demandant à l'utilisateur s'il est sûr de vouloir quitter,
				// en prenant l'élément principal du menu comme parent pour qu'il puisse avoir accès aux évènements
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
			// Avance à la scène pour rejoindre une partie multijoueur
			switchToNextScene(ECS_MULTIPLAYER_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton)
		{
			// Choisi l'adresse IP de l'hôte de la partie multijoueur
			core::stringc hostIP("");

			// Vérifie qu'une partie est bien sélectionnée, sinon on empêche le joueur de continuer
			const int selectedGame = guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->getSelected();
			if (guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable && selectedGame >= 0)
			{
				// Obtient le nom de l'adresse IP de l'hôte d'après le texte de la permière cellule de la ligne sélectionnée
				hostIP = core::stringc(guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->getCellText(selectedGame, 0));

				// Rejoint la partie multijoueur sélectionnée
				// Attention : le paramêtre hostIP ne doit pas être une référence pointant sur un élement de la liste des parties trouvées de RakNet,
				// car cette liste sera vidée (par un appel à reset) avant que la connexion réseau n'ait été créée entre les deux machines
				joinMultiplayerGame(hostIP.c_str());
			}
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.multiplayerMenuGUI.retourBouton)
		{
#ifdef USE_RAKNET
			// Stoppe l'action en cours de RakNet
			rkMgr.reset();
#endif

			// Retourne à la scène du menu principal
			switchToNextScene(ECS_MAIN_MENU);
		}
#endif
		else if (event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.retourBouton)
		{
			// Retourne à la scène du menu principal
			switchToNextScene(ECS_MAIN_MENU);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.commencerBouton)
		{
			// Choisi le nom du terrain
			core::stringw terrainFilename("terrain plat.ewt");

			// Vérifie qu'un terrain est bien sélectionné, sinon on empêche le joueur de continuer
			const int selectedTerrain = guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected();
			if (guiManager->guiElements.newGameMenuGUI.terrainListBox && selectedTerrain >= 0)
			{
				terrainFilename = core::stringw(guiManager->guiElements.newGameMenuGUI.terrainListBox->getListItem(selectedTerrain));
				terrainFilename.append(".ewt");

				// Choisi la difficulté du jeu
				EcoWorldModifiers::E_DIFFICULTY difficulty = EcoWorldModifiers::ED_normal;
				if (guiManager->guiElements.newGameMenuGUI.difficultyComboBox)
				{
					const u32 itemData = guiManager->guiElements.newGameMenuGUI.difficultyComboBox->getItemData(
						guiManager->guiElements.newGameMenuGUI.difficultyComboBox->getSelected());
					difficulty = (EcoWorldModifiers::E_DIFFICULTY)itemData;
				}

				// Crée une nouvelle partie avec cette difficulté et avec le terrain sélectionné
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
			// Indique au système de jeu que le joueur désire continuer la partie
			system.setContinueGame(true);

			// Débloque la caméra RTS si elle a été bloquée arbitrairement
			if (cameraRTS)
				cameraRTS->setLockCamera(lockCamera);

			// Redémarre le timer s'il est stoppé
			if (deviceTimer->isStopped())
				deviceTimer->start();

			// Retourne à la scène du jeu
			switchToNextScene(ECS_PLAY);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
		{
			// Recrée le menu principal
			createMainMenu();
		}
	}
	else if ((event.GUIEvent.EventType == EGET_LISTBOX_CHANGED || event.GUIEvent.EventType == EGET_LISTBOX_SELECTED_AGAIN)
		&& event.GUIEvent.Caller == guiManager->guiElements.newGameMenuGUI.terrainListBox && guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
	{
		// Si le terrain sélectionné a changé dans le menu de création de nouveau terrain, on actualise son image d'aperçu :

		// Si une texture est déjà affichée dans l'aperçu, on la supprime
		if (guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
		{
			video::ITexture* const newGamePreviewImage = guiManager->guiElements.newGameMenuGUI.terrainApercuImage->getImage();
			if (newGamePreviewImage)
			{
				guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);
				driver->removeTexture(newGamePreviewImage);
			}
		}

		// Les informations sur la prévisualisation de ce terrain
		EcoWorldTerrainManager::TerrainPreviewInfos terrainPreviewInfos;

		// Vérifie qu'un terrain est bien sélectionné
		if (guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected() >= 0 && renderer)
		{
			// Choisi le nom du terrain
			core::stringw terrainFilename = guiManager->guiElements.newGameMenuGUI.terrainListBox->getListItem(
					guiManager->guiElements.newGameMenuGUI.terrainListBox->getSelected());
			terrainFilename.append(".ewt");

			// Laisse le Terrain Manager essayer de charger les informations de prévisualisation du terrain
			terrainPreviewInfos = renderer->getTerrainManager().readPreviewInfos(terrainFilename);
		}

		// Indique le texte de prévisualisation de ce terrain
		if (guiManager->guiElements.newGameMenuGUI.terrainDescriptionTexte)
			guiManager->guiElements.newGameMenuGUI.terrainDescriptionTexte->setText(terrainPreviewInfos.descriptionText.c_str());

		// Si l'image est bien valide, on l'affiche enfin dans la zone de prévisualisation
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

	// Ici, on gère les évènements de la GUI qui ont lieu dans la scène du jeu ainsi que les évènements des boîtes de dialogue d'ouverture et d'enregistrement de fichiers, et la boîte de dialogue d'erreur
	// Note : on reçoit aussi les évènements de la GUI qui ont lieu dans les scènes de chargement

	// Vérifie que l'évenement est bien un appui sur un bouton
	if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.objectivesBouton && guiManager->guiElements.gameGUI.objectivesWindow)
		{
			// Affiche/Masque le tableau des objectifs pour la partie actuelle
			guiManager->guiElements.gameGUI.objectivesWindow->setVisible(!guiManager->guiElements.gameGUI.objectivesWindow->isVisible());

			// Si le tableau est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.objectivesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.objectivesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.monthTableBouton && guiManager->guiElements.gameGUI.monthTableWindow)
		{
			// Affiche/Masque le tableau récapitulatif des fins de mois
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(!guiManager->guiElements.gameGUI.monthTableWindow->isVisible());

			// Si le tableau est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.monthTableWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.ressourcesBouton && guiManager->guiElements.gameGUI.ressourcesWindow)
		{
			// Affiche/Masque le menu Ressources
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(!guiManager->guiElements.gameGUI.ressourcesWindow->isVisible());

			// Si le menu Ressources est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.ressourcesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.ressourcesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son d'ouverture/fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.FPSCameraBouton && cameraRTS)
		{
			// Passe en mode Visiteur
			cameraRTS->setIsCameraFPSEnabled(!cameraRTS->getIsCameraFPSEnabled());

			// Annule la sélection actuelle
			batSelector.annulerSelection();

			// Masque ou active la GUI du jeu
			if (currentScene == ECS_PLAY)
				guiManager->setGUIVisible(GUIManager::EGN_gameGUI, !cameraRTS->getIsCameraFPSEnabled());

#ifdef USE_IRRKLANG
			// TODO : Joue le son de passage en mode Visiteur
			//ikMgr.playGameSound(...);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de passage en mode visiteur + le son de clic sur un bouton)
			//sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.menuBouton && guiManager->guiElements.gameGUI.menuWindow)
		{
			// Met en pause si on n'y est pas déjà
			guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(!isPaused);
			if (!isPaused)
				changePause();

			// Si la caméra FPS à l'intérieur de la caméra RTS est activée, on la désactive
			if (cameraRTS && cameraRTS->getIsCameraFPSEnabled())
				cameraRTS->setIsCameraFPSEnabled(false);

			// Désélectionne le bâtiment sélectionné et annule la construction/destruction en cours, et on masque aussi la fenêtre d'informations
			batSelector.annulerSelection();
			if (guiManager->guiElements.gameGUI.informationsWindow)
				guiManager->guiElements.gameGUI.informationsWindow->setVisible(false);

			// Affiche la fenêtre du menu du jeu
			guiManager->guiElements.gameGUI.menuWindow->setVisible(true);

			// Donne le focus à la fenêtre du menu du jeu
			gui->setFocus(guiManager->guiElements.gameGUI.menuWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_OPEN);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son d'ouverture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.detruireBouton)
		{
			// Retiens si ce bouton était pressé (annulerSelection() va rendre ce bouton non pressé !)
			const bool wasPressed = guiManager->guiElements.gameGUI.detruireBouton->isPressed();

			// Annule la sélection actuelle (création/destruction/sélection d'un bâtiment)
			batSelector.annulerSelection();

			// Rend à nouveau ce bouton pressé
			guiManager->guiElements.gameGUI.detruireBouton->setPressed(wasPressed);
		}
		else
		{
			// Vérifie si le bouton sur lequel l'utilsateur a appuyé est un bouton pour construire un batiment
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
				// Retiens si ce bouton était pressé (annulerSelection() va rendre ce bouton non pressé !)
				const bool wasPressed = boutonBatiment->isPressed();

				// Annule la sélection actuelle (création/destruction/sélection d'un bâtiment)
				batSelector.annulerSelection();

				// Rend à nouveau ce bouton pressé
				boutonBatiment->setPressed(wasPressed);

				// Si le bouton est pressé, change le batiment actuel, sinon, le désactive
				if (wasPressed)
					batSelector.setCurrentBatimentID((BatimentID)(boutonBatiment->getID())); // L'ID du bouton est le même que l'ID du batiment qu'il représente
				else
					batSelector.setCurrentBatimentID(BI_aucun);

				return false;
			}
		}
	}
	// Si l'évènement est l'arrivée de la souris sur un élément de la GUI
	else if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.budgetTexte)
		{
			// La souris est sur le texte du budget : on affiche ses informations
			guiManager->guiElements.gameGUI.budgetInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.energieTexte)
		{
			// La souris est sur le texte de l'énergie : on affiche ses informations
			guiManager->guiElements.gameGUI.energieInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.effetSerreTexte)
		{
			// La souris est sur le texte de l'effet de serre : on affiche ses informations
			guiManager->guiElements.gameGUI.effetSerreInfosTexte->setVisible(true);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dechetsTexte)
		{
			// La souris est sur le texte des déchets : on affiche ses informations
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
			// Détermine si la souris vient de passer sur le menu inférieur ou sur l'un de ses enfants
			if (calculerIsMouseOnGUI(guiManager->guiElements.gameGUI.tabGroupBas))
			{
				// Si la souris vient de passer est le menu inférieur ou sur l'un de ses enfants, on demande sa montée quelle que soit sa position actuelle
				vitesseDescenteMenuInferieur = -0.001f;
				realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
				menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
			}
		}
#endif
	}
	// Si l'évènement est le départ de la souris sur un élément de la GUI
	else if (event.GUIEvent.EventType == EGET_ELEMENT_LEFT)
	{
		if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.budgetTexte)
		{
			// La souris a quitté le texte du budget : on masque ses informations
			guiManager->guiElements.gameGUI.budgetInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.energieTexte)
		{
			// La souris a quitté le texte de l'énergie : on masque ses informations
			guiManager->guiElements.gameGUI.energieInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.effetSerreTexte)
		{
			// La souris a quitté le texte de l'effet de serre : on masque ses informations
			guiManager->guiElements.gameGUI.effetSerreInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dechetsTexte)
		{
			// La souris a quitté le texte des déchets : on masque ses informations
			guiManager->guiElements.gameGUI.dechetsInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.populationTexte)
		{
			// La souris a quitté le texte de la population : on masque ses informations
			guiManager->guiElements.gameGUI.populationInfosTexte->setVisible(false);
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.dateTexte)
		{
			// La souris a quitté le texte de la date : on masque ses informations
			guiManager->guiElements.gameGUI.dateInfosTexte->setVisible(false);
		}
#ifdef MENU_INFERIEUR_REACTIF
		else if (guiManager->guiElements.gameGUI.tabGroupBas)
		{
			// Détermine si la souris vient de passer sur le menu inférieur ou sur l'un de ses enfants
			if (!calculerIsMouseOnGUI(guiManager->guiElements.gameGUI.tabGroupBas))
			{
				// Si la souris vient de quitter le menu inférieur et n'est pas non plus sur ses enfants, on demande sa descente quelle que soit la position actuelle de ce menu
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
			// Le tableau des objectifs a été fermé : on le masque
			guiManager->guiElements.gameGUI.objectivesWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.monthTableWindow)
		{
			// Le tableau récapitulatif des fins de mois a été fermé : on le masque
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.ressourcesWindow)
		{
			// Le menu Ressources a été fermé : on le masque
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			// Le menu d'animation de la caméra a été fermé : on le masque
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->setVisible(false);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif
		}
		else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.menuWindow)
		{
			// Masque le menu du jeu
			guiManager->guiElements.gameGUI.menuWindow->setVisible(false);

			// Enlève la pause
			if (guiManager->guiElements.gameGUI.menuWindow->getHasChangedPause())
				changePause();
			guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(false);

			// Restaure le blocage de la caméra RTS, puisqu'elle a été bloquée lors de l'affichage de ce menu
			cameraRTS->setLockCamera(lockCamera);

#ifdef USE_IRRKLANG
			// Joue le son de fermeture d'un menu
			ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_CLOSE);

			// Indique qu'on a déjà traîté cet event du point de vue sonore (évite ainsi d'avoir le son de fermeture du menu + le son de clic sur un bouton)
			sendEventToIrrKlangMgr = false;
#endif

			// Applique le choix demandé par l'utilisateur lors de la fermeture de cette fenêtre
			applyMenuWindowChoice();
		}
	}
#ifndef MENU_INFERIEUR_REACTIF
	else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.tabGroupBas && event.GUIEvent.EventType == EGET_TAB_CHANGED
		&& vitesseDescenteMenuInferieur == 0.0f)
	{
		// L'utilisateur veut descendre ou remonter le menu inférieur, on modifie donc sa vitesse

		// Si on a cliqué sur l'onglet pour réduire le menu inférieur et que ce dernier est en haut, on le réduit
		const bool activeTabIsReduireTab = (guiManager->guiElements.gameGUI.tabGroupBas->getActiveTab() == guiManager->guiElements.gameGUI.reduireTab->getNumber());
		const tribool menuInferieurHaut = isMenuInferieurHaut();
		if (menuInferieurHaut == true && activeTabIsReduireTab)
		{
			vitesseDescenteMenuInferieur = 0.001f;
			realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
			menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
		}
		else if (menuInferieurHaut == false)	// Si on a cliqué sur n'importe quel onglet, mais que le menu inférieur est en bas, on le remonte
		{
			vitesseDescenteMenuInferieur = -0.001f;
			realTimeMsStartDescenteMenuInferieur = deviceTimer->getRealTime();
			menuInferieurStartPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;
		}

		// Active le premier onglet pour éviter que l'onglet de réduction du menu inférieur ne s'active
		if (activeTabIsReduireTab)
			guiManager->guiElements.gameGUI.tabGroupBas->setActiveTab(0);
	}
#endif
	else if (event.GUIEvent.Caller == guiManager->guiElements.gameGUI.vitesseJeuScrollBar && event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED)
	{
		// L'utilisateur a changé la vitesse du jeu : on modifie la vitesse du timer du device en conséquence

		const float newTimerSpeed = guiManager->getGameGUITimerSpeed();

#ifdef USE_RAKNET
		// Indique à RakNet qu'on veut changer la vitesse du jeu, s'il est activé
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
			// Indique à Spark que la vitesse du jeu a changé
			renderer->getSparkManager().setTimerSpeed(newTimerSpeed);
#endif
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.errorMessageBox
		&& (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO || event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL))
	{
		// On indique que le pointeur sur la boîte de dialogue d'erreur n'est plus valide
		guiManager->guiElements.globalGUI.errorMessageBox = NULL;

		if (hasChangedPauseBeforeShowingErrorMessageBox)
		{
			// Enlève la pause si on l'avait activée
			changePause();
			hasChangedPauseBeforeShowingErrorMessageBox = false;
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.openFileDialog)
	{
		if (event.GUIEvent.EventType == EGET_FILE_SELECTED || event.GUIEvent.EventType == EGET_FILE_CHOOSE_DIALOG_CANCELLED)
		{
			// Masque la fenêtre de fermeture de fichier
			guiManager->guiElements.globalGUI.openFileDialog->hide();

			if (event.GUIEvent.EventType == EGET_FILE_SELECTED)
			{
				const core::stringw filename = guiManager->guiElements.globalGUI.openFileDialog->getFileNameStr();

				if (filename.size())
				{
					if (hasChangedPauseBeforeShowingFileDialog)
					{
						// Enlève la pause si on l'avait activée
						changePause();
						hasChangedPauseBeforeShowingFileDialog = false;
					}

					// Charge le jeu
					loadSavedGame(filename);

					// Enlève le focus de la GUI
					gui->setFocus(NULL);
				}
			}
			if (hasChangedPauseBeforeShowingFileDialog)
			{
				// Enlève la pause si on l'avait activée
				changePause();
				hasChangedPauseBeforeShowingFileDialog = false;
			}
		}
	}
	else if (event.GUIEvent.Caller == guiManager->guiElements.globalGUI.saveFileDialog)
	{
		if (event.GUIEvent.EventType == EGET_FILE_SELECTED || event.GUIEvent.EventType == EGET_FILE_CHOOSE_DIALOG_CANCELLED)
		{
			// Masque la fenêtre de fermeture de fichier
			guiManager->guiElements.globalGUI.saveFileDialog->hide();

			if (event.GUIEvent.EventType == EGET_FILE_SELECTED)
			{
				core::stringw filename = guiManager->guiElements.globalGUI.saveFileDialog->getFileNameStr();

				if (filename.size())
				{
					if (hasChangedPauseBeforeShowingFileDialog)
					{
						// Enlève la pause si on l'avait activée
						changePause();
						hasChangedPauseBeforeShowingFileDialog = false;
					}

					// On ne peut changer l'état de la pause car il est sauvegardé, donc on stoppe le timer manuellement s'il ne l'est pas
					const bool wasTimerStarted = !deviceTimer->isStopped();
					if (wasTimerStarted)
						deviceTimer->stop();

					// Enregistre le jeu
					saveCurrentGame(filename);

					// Enlève le focus de la GUI
					gui->setFocus(NULL);

					// Redémarre le timer s'il n'était pas stoppé
					if (wasTimerStarted)
						deviceTimer->start();
				}
			}
			if (hasChangedPauseBeforeShowingFileDialog)
			{
				// Enlève la pause si on l'avait activée
				changePause();
				hasChangedPauseBeforeShowingFileDialog = false;
			}
		}
	}
#ifdef ASK_BEFORE_QUIT
	else if (event.GUIEvent.Caller == guiManager->guiElements.mainMenuGUI.quitterWindow
		&& (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES || event.GUIEvent.EventType == EGET_MESSAGEBOX_NO || event.GUIEvent.EventType == EGET_MESSAGEBOX_OK || event.GUIEvent.EventType == EGET_MESSAGEBOX_CANCEL))
	{
		// Si l'utilisateur a cliqué sur un bouton (le bouton Oui est inclus) :
		// On oublie le pointeur sur la fenêtre de confirmation
		guiManager->guiElements.mainMenuGUI.quitterWindow = NULL;

		if (event.GUIEvent.EventType == EGET_MESSAGEBOX_YES)	// Si l'utilisateur a cliqué sur Oui :
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

	// Ici, on gère tous les évènements qui ont lieu dans la scène du jeu, autres que ceux de la GUI

	CBatimentSceneNode* const selectedBatiment = batSelector.getSelectedBat();

	// Raccourcis claviers :
	if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		// Spécifique au jeu :
		if (event.KeyInput.Key == KEY_SPACE && !event.KeyInput.PressedDown && selectedBatiment && cameraRTS && canHandleKeysEvent)
		{
			// Espace

			// Si un bâtiment est sélectionné et qu'on a appuyé sur Espace, on pointe la caméra sur ce bâtiment
			cameraRTS->pointCameraAtNode(selectedBatiment,
				selectedBatiment->getTransformedBoundingBox().getExtent().getLength() * 3.0f);	// Distance au node relative à la taille de sa bounding box
		}
		else if (event.KeyInput.Key == KEY_KEY_N && canHandleKeysEvent)
		{
			// N

			// On peut afficher tous les noms au-dessus des bâtiments par un appui sur la touche N
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

			// Si le tableau est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.objectivesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.objectivesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_T && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.monthTableWindow
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + T est une touche de débogage
#endif
			)
		{
			// T

			// On peut aussi afficher le tableau récapitulatif par un appui sur la touche T
			guiManager->guiElements.gameGUI.monthTableWindow->setVisible(!guiManager->guiElements.gameGUI.monthTableWindow->isVisible());

			// Si le tableau est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.monthTableWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.monthTableWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_R && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.ressourcesWindow
			&& !(event.KeyInput.Shift && event.KeyInput.Control)	// Ctrl + Maj + R est une touche de la caméra (réinitialisation des paramêtres de la caméra)
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + R est une touche de débogage
#endif
			)
		{
			// R

			// On peut aussi afficher le menu Ressources par un appui sur la touche R
			guiManager->guiElements.gameGUI.ressourcesWindow->setVisible(!guiManager->guiElements.gameGUI.ressourcesWindow->isVisible());

			// Si le menu Ressources est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.ressourcesWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.ressourcesWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_A && !event.KeyInput.PressedDown && canHandleKeysEvent && guiManager->guiElements.gameGUI.cameraAnimatorWindow
#ifdef _DEBUG
			&& !event.KeyInput.Control	// Ctrl + A est une touche de débogage
#endif		
			)
		{
			// A

			// On peut aussi afficher le menu permettant à l'utilisateur de créer une animation personnalisée par un appui sur la touche A
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->setVisible(!guiManager->guiElements.gameGUI.cameraAnimatorWindow->isVisible());

			// Si le menu d'animation est affiché, on lui donne le focus
			gui->setFocus(guiManager->guiElements.gameGUI.cameraAnimatorWindow);

#ifdef USE_IRRKLANG
			// Joue le son d'ouverture/fermeture d'un menu
			ikMgr.playGameSound(guiManager->guiElements.gameGUI.cameraAnimatorWindow->isVisible() ? IrrKlangManager::EGSE_GUI_MENU_OPEN : IrrKlangManager::EGSE_GUI_MENU_CLOSE);
#endif
		}
		else if (event.KeyInput.Key == KEY_F5 && !event.KeyInput.PressedDown && guiManager->guiElements.gameGUI.cameraAnimatorWindow)
		{
			// F5

			// Lit directement l'animation personnalisée de la caméra
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->addAnimatorToCamera();
		}
		else if (event.KeyInput.Key == KEY_KEY_F && !event.KeyInput.PressedDown && cameraRTS && canHandleKeysEvent)
		{
			// F

			// On peut aussi passer en mode Visiteur par un appui sur la touche F
			cameraRTS->setIsCameraFPSEnabled(!cameraRTS->getIsCameraFPSEnabled());

			// Annule la sélection actuelle
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

			// Un appui sur la touche C réinitialise la GUI du jeu
			guiManager->resetGameGUI();
		}
		else if ((event.KeyInput.Key == KEY_PAUSE || (event.KeyInput.Key == KEY_KEY_P && canHandleKeysEvent && !event.KeyInput.Control))	// Ctrl + P : Mode nuage de points !
			&& !event.KeyInput.PressedDown)
		{
			// Pause ou P

#ifdef USE_RAKNET
			// Indique à RakNet qu'on veut changer la pause du jeu, s'il est activé
			if (rkMgr.isNetworkPlaying())
			{
				RakNetManager::RakNetGamePacket packet(RakNetManager::EGPT_GAME_PAUSE_CHANGED);
				packet.gameIsPaused = !isPaused;
				rkMgr.sendPackets.push_back(packet);
			}
			else
#endif
				// Change l'état de la pause
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

			// Change l'êtat du blocage de la camera RTS pour qu'elle ne bouge plus ou puisse bouger à nouveau
			lockCamera = !lockCamera;

#ifdef LOCK_CAMERA_WHEN_MOUSE_ON_GUI
			// On bloque la camera RTS si la souris est sur un élément de la GUI ou si l'utilisateur a demandé de la bloquer
			cameraRTS->setLockCamera(isMouseOnGUI || lockCamera);
#else
			// On ne bloque la camera que si l'utilisateur a demandé de la bloquer
			cameraRTS->setLockCamera(lockCamera);
#endif
		}
		else if (event.KeyInput.Key == KEY_KEY_B && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent
			&& guiManager->guiElements.gameGUI.vitesseJeuScrollBar)
		{
			// Ctrl + B

			// Autorise/Interdit le mode Boost :

			// Obtient des valeurs actuelles sur la scroll bar de sélection de vitesse
			const int currentScrollBarMax = guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getMax();
			const int currentScrollBarPos = (currentScrollBarMax - guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getPos());	// Attention : ne pas oublier que les valeurs sont inversées

			// Détermine le nouveau maximum de la scroll bar
			const int newScrollBarMax = (currentScrollBarMax == 6 ? 8 : 6);

			// Modifie la valeur maximale de la scroll bar
			guiManager->guiElements.gameGUI.vitesseJeuScrollBar->setMax(newScrollBarMax);

			// Modifie la position actuelle de la scroll bar pour conserver la vitesse actuelle du jeu
			guiManager->guiElements.gameGUI.vitesseJeuScrollBar->setPos(max(newScrollBarMax - currentScrollBarPos, 0));

			// Modifie la vitesse actuelle du timer si elle a été modifiée parce que le maximum a été diminué
			deviceTimer->setSpeed(guiManager->getGameGUITimerSpeed());
		}
		else if (event.KeyInput.Key == KEY_KEY_S && event.KeyInput.Control && canHandleKeysEvent && cameraRTS && !cameraRTS->getIsCameraFPSEnabled())
		{
			// Ctrl + S

			// On affiche la boîte de dialogue pour sauvegarder un fichier
			if (event.KeyInput.PressedDown)
				showFileDialog(CGUIFileSelector::EFST_SAVE_DIALOG);

			return true;	// Evite que la caméra RTS ne reçoive cet event (que la touche soit pressée ou non)
		}
		else if (event.KeyInput.Key == KEY_KEY_O && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent && cameraRTS && !cameraRTS->getIsCameraFPSEnabled())
		{
			// Ctrl + O

			// On affiche la boîte de dialogue pour ouvrir un fichier
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
		}



		// Non spécifique au jeu :
		else if (event.KeyInput.Key == KEY_KEY_L && event.KeyInput.Control && event.KeyInput.Shift && !event.KeyInput.PressedDown && canHandleKeysEvent
			&& guiManager->guiElements.gameGUI.notesWindow)
		{
			// Ctrl + Maj + L

			// Envoie le texte de notes dans le logger, une fois reconvertit en caractères unicodes
			LOG((core::stringc(guiManager->guiElements.gameGUI.notesWindow->getNotesTexte())).c_str(), ELL_NONE);	// ELL_NONE indique que ce texte de log sera toujours envoyé
		}



		// Outils de débogage :
#ifdef _DEBUG
		else if (event.KeyInput.Key == KEY_ADD && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + '+' (pavé numérique)

			// Ajoute instantanément 100 000 € au budget du joueur (ou 10 000 € si Maj est enfoncé)
			system.addBudget(event.KeyInput.Shift ? 10000.0f : 100000.0f);
		}
		else if (event.KeyInput.Key == KEY_SUBTRACT && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + '-' (pavé numérique)

			// Retire instantanément 100 000 € du budget du joueur (ou 10 000 € si Maj est enfoncé)
			system.subtractBudget(event.KeyInput.Shift ? 10000.0f : 100000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_D && event.KeyInput.Control && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + D

			// Ajoute 1 mois au temps système (ou 5 jours si Maj est enfoncé)
			if (!event.KeyInput.PressedDown)
				system.addTime(event.KeyInput.Shift ?
					10.0f :	//  5.0f * 2.0f : 10 jours avec 2 secondes par jour
					60.0f);	// 30.0f * 2.0f : 30 jours avec 2 secondes par jour

			return true;	// Evite que la caméra RTS ne reçoive cet event (que la touche soit pressée ou non !)
		}
		else if (event.KeyInput.Key == KEY_KEY_A && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + A

			// Ajoute instantanément 10 tonnes de toutes les ressources au stock du joueur (ou 1 tonne si Maj est enfoncé)
			system.addRessources(event.KeyInput.Shift ? 1000.0f : 10000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_Z && event.KeyInput.Control && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl [+ Maj] + Z

			// Retire instantanément 10 tonnes de toutes les ressources au stock du joueur (ou 1 tonne si Maj est enfoncé)
			system.addRessources(event.KeyInput.Shift ? -1000.0f : -10000.0f);
		}
		else if (event.KeyInput.Key == KEY_KEY_L && event.KeyInput.Control && !event.KeyInput.Shift && !event.KeyInput.PressedDown && canHandleKeysEvent)
		{
			// Ctrl + L

			// Débloque tous les bâtiments
			unlockAllBatiments = !unlockAllBatiments;
		}
#endif
	}

	return false;
}
bool Game::onEchap(bool pressed)
{
	// Bouton relaché
	if (!pressed)
	{
		if (currentScene == ECS_PLAY)				// Dans le jeu
		{
			// Si on appuye sur Echap et que la caméra RTS est animée par sa fenêtre d'animation, on arrête son animation
			if (guiManager->guiElements.gameGUI.cameraAnimatorWindow && guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
			{
				guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

				// Retourne true pour absorber l'évènement, et ainsi éviter que la fenêtre d'animation de la caméra reçoive ensuite l'évènement et se ferme
				// (l'appui sur Echap permet aussi de fermer certaines fenêtres)
				return true;
			}

			// Si on appuye sur Echap et que la caméra FPS dans la caméra RTS est activée, on repasse en mode caméra RTS
			else if (cameraRTS->getIsCameraFPSEnabled())
			{
				cameraRTS->setIsCameraFPSEnabled(false);

				// Réactive la GUI du jeu
				if (currentScene == ECS_PLAY)
					guiManager->setGUIVisible(GUIManager::EGN_gameGUI, true);

				// Retourne true pour absorber l'évènement, et ainsi éviter qu'une fenêtre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fenêtres)
				return true;
			}

			// Si on appuye sur Echap et qu'on a un batiment à construire/sélectionné, on annule sa construction/sélection
			else if (batSelector.isPlacingOrSelectingBat())
			{
				batSelector.annulerSelection();

				// Retourne true pour absorber l'évènement, et ainsi éviter qu'une fenêtre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fenêtres)
				return true;
			}

			// Si on appuye sur Echap et qu'aucune action n'est en cours, on affiche le menu du jeu
			else if (guiManager && guiManager->guiElements.gameGUI.menuWindow && !guiManager->guiElements.gameGUI.menuWindow->isVisible())
			{
				// Met en pause si on n'y est pas déjà, sauf si on est en mode multijoueur sur RakNet
#ifdef USE_RAKNET
				if (!rkMgr.isNetworkPlaying())
#endif
				{
					guiManager->guiElements.gameGUI.menuWindow->setHasChangedPause(!isPaused);
					if (!isPaused)
						changePause();
				}

				// Désélectionne le bâtiment sélectionné et annule la construction/destruction en cours, et on masque aussi la fenêtre d'informations
				batSelector.annulerSelection();
				if (guiManager->guiElements.gameGUI.informationsWindow)
					guiManager->guiElements.gameGUI.informationsWindow->setVisible(false);

				// Bloque la caméra RTS pour éviter que le joueur ne la déplace lorsque le menu du jeu est affiché
				cameraRTS->setLockCamera(true);

				// Réactive la GUI du jeu si le joueur l'a désactivée
				guiManager->setGUIVisible(GUIManager::EGN_gameGUI, true);

				// Affiche la fenêtre du menu du jeu
				guiManager->guiElements.gameGUI.menuWindow->setVisible(true);

				// Donne le focus à la fenêtre du menu du jeu
				gui->setFocus(guiManager->guiElements.gameGUI.menuWindow);

#ifdef USE_IRRKLANG
				// Joue le son d'ouverture d'un menu
				ikMgr.playGameSound(IrrKlangManager::EGSE_GUI_MENU_OPEN);
#endif

				// Retourne true pour absorber l'évènement, et ainsi éviter qu'une fenêtre du jeu ne se ferme (l'appui sur Echap permet aussi de fermer certaines fenêtres)
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
			// Si la fenêtre de confirmation des options est affichée,
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
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de création de nouvelle partie
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
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'écran de partie terminée
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
	else	// Bouton enfoncé
	{
		if (currentScene == ECS_MAIN_MENU)				// Dans le menu principal
		{
			// On affiche un appui sur le bouton pour quitter
			if (guiManager->guiElements.mainMenuGUI.quitterBouton)
				guiManager->guiElements.mainMenuGUI.quitterBouton->setPressed(true);
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fenêtre de confirmation des options est affichée,
			// on affiche un appui sur son bouton Non,
			// sinon on affiche un appui sur le bouton pour retourner au menu principal.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->setNoButtonPressed(true);
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->retourBouton->setPressed(true);
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de création de nouvelle partie
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.newGameMenuGUI.retourBouton)
				guiManager->guiElements.newGameMenuGUI.retourBouton->setPressed(true);
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'écran de partie terminée
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(true);
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On affiche un appui sur le bouton pour rejoindre la partie actuellement sélectionnée
			if (guiManager->guiElements.multiplayerMenuGUI.retourBouton)
				guiManager->guiElements.multiplayerMenuGUI.retourBouton->setPressed(true);
		}
#endif
	}

	return false;
}
bool Game::onEnter(bool pressed)
{
	// Bouton relaché
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
			// Si la fenêtre de confirmation des options est affichée,
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
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de création de nouvelle partie
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
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'écran de partie terminée
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
			// On simule un appui sur le bouton pour rejoindre la partie actuellement sélectionnée
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
	else	// Bouton enfoncé
	{
		if (currentScene == ECS_MAIN_MENU)				// Dans le menu principal
		{
			// On affiche un appui sur le bouton pour jouer
			if (guiManager->guiElements.mainMenuGUI.newGameBouton)
				guiManager->guiElements.mainMenuGUI.newGameBouton->setPressed(true);
		}
		else if (currentScene == ECS_OPTIONS_MENU)		// Dans le menu options
		{
			// Si la fenêtre de confirmation des options est affichée,
			// on affiche un appui sur son bouton Oui,
			// sinon on affiche un appui sur le bouton pour appliquer les options.
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->confirmationWindow->setYesButtonPressed(true);
			else if (guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->appliquerBouton->setPressed(true);
		}
		else if (currentScene == ECS_NEW_GAME_MENU)		// Dans le menu de création de nouvelle partie
		{
			// On affiche un appui sur le bouton pour commencer le jeu
			if (guiManager->guiElements.newGameMenuGUI.commencerBouton && guiManager->guiElements.newGameMenuGUI.commencerBouton->isEnabled())
				guiManager->guiElements.newGameMenuGUI.commencerBouton->setPressed(true);
		}
		else if (currentScene == ECS_GAME_LOST || currentScene == ECS_GAME_WON)		// Dans l'écran de partie terminée
		{
			// On affiche un appui sur le bouton pour retourner au menu principal
			if (guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton)
				guiManager->guiElements.gameEndGUI.retourMenuPrincipalBouton->setPressed(true);
		}
#ifdef USE_RAKNET
		else if (currentScene == ECS_MULTIPLAYER_MENU)	// Dans le menu pour rejoindre une partie multijoueur
		{
			// On affiche un appui sur le bouton pour rejoindre la partie actuellement sélectionnée
			if (guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton && guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->isEnabled())
				guiManager->guiElements.multiplayerMenuGUI.rejoindreBouton->setPressed(true);
		}
#endif
	}

	return false;
}
void Game::changePause()
{
	// Change l'état de la pause
	isPaused = !isPaused;

	// Fait concorder l'affichage du texte de pause avec la pause actuelle
	if (guiManager->guiElements.globalGUI.pauseTexte)
		guiManager->guiElements.globalGUI.pauseTexte->setVisible(isPaused && currentScene == ECS_PLAY);	// N'affiche le texte de pause que lorsqu'on est dans la scène du jeu

	// Modifie le titre de la fenêtre suivant l'état actuel de la pause
	{
		wstring title(TITRE_FENETRE_JEU);
		if (isPaused)
			title.append(L" (en pause)");
		device->setWindowCaption(title.c_str());
	}

	if (isPaused != deviceTimer->isStopped()) // Vérifie que l'êtat du timer concorde avec la pause
	{
		if (isPaused) // Si on est en pause, on stoppe le timer
			deviceTimer->stop();
		else // Si on n'est pas en pause, on redémarre le timer
			deviceTimer->start();
	}
}
void Game::showFileDialog(CGUIFileSelector::E_FILESELECTOR_TYPE type)
{
	// Vérifie que le gui manager et que le type de la boîte de dialogue est bien valide
	if (!guiManager || (type != CGUIFileSelector::EFST_OPEN_DIALOG && type != CGUIFileSelector::EFST_SAVE_DIALOG))
		return;

	// Vérifie que les boîtes de dialogues ont bien été créées
	if (!guiManager->guiElements.globalGUI.openFileDialog || !guiManager->guiElements.globalGUI.saveFileDialog)
		return;

	// Vérifie qu'aucune boîte de dialogue n'est actuellement affichée
	if (guiManager->guiElements.globalGUI.openFileDialog->isVisible() || guiManager->guiElements.globalGUI.saveFileDialog->isVisible())
		return;

	if (!isPaused && currentScene == ECS_PLAY)
	{
		// Met le jeu en pause avant d'afficher la boîte de dialogue si on est dans la scène du jeu
		changePause();
		hasChangedPauseBeforeShowingFileDialog = true;
	}

	if (type == CGUIFileSelector::EFST_OPEN_DIALOG)
		guiManager->guiElements.globalGUI.openFileDialog->showAgain();	// Affiche cette fenêtre à nouveau
	else
		guiManager->guiElements.globalGUI.saveFileDialog->showAgain();	// Affiche cette fenêtre à nouveau
}
void Game::showErrorMessageBox(const wchar_t* caption, const wchar_t* text)
{
	// Vérifie que le pointeur sur la gui est valide et qu'aucune boîte de dialogue d'erreur n'est affichée
	if (!gui || !guiManager)
		return;
	if (guiManager->guiElements.globalGUI.errorMessageBox)
		return;

	if (!isPaused && currentScene == ECS_PLAY)
	{
		// Met le jeu en pause avant d'afficher la boîte de dialogue si on est dans la scène du jeu
		changePause();
		hasChangedPauseBeforeShowingErrorMessageBox = true;
	}

	// Crée une boîte de dialogue d'erreur avec les textes spécifiés, et rend cette boîte de dialogue modale tout en lui ajoutant un fond grisé derrière elle
	guiManager->guiElements.globalGUI.errorMessageBox = CGUIMessageBox::addMessageBox(gui, caption, text, EMBF_OK, gui->getRootGUIElement(), true);
}
void Game::createScreenShot()
{
	// Affiche un rendu rapide avant de prendre la capture d'écran
	//render();	// Désactivé : le rendu de la dernière frame suffit

	long numeroScreenShot = 1;	// Le numero du ScreenShot actuel

	// Crée le dossier pour les captures d'écran
	createDirectory("Screenshots");

	// Cherche le numéro du ScreenShot suivant pour ne pas réécrire par-dessus un autre ScreenShot
	bool hasFoundNumero = false;
	do
	{
		// Obtient le nom de fichier du ScreenShot actuel
		sprintf_SS("Screenshots/Capture %d.bmp", numeroScreenShot);

		// Regarde si un fichier de ce nom existe déjà
		bool exists = fileSystem->existFile(text_SS);

		if (!exists)	// Si le fichier n'existe pas, on indique qu'on a trouvé le numéro du ScreenShot actuel
			hasFoundNumero = true;
		else			// Si le fichier existe déjà, on incrémente le numéro du ScreenShot de 1
			numeroScreenShot++;

		// Petite sécurité : Ecrit toujours ce screen shot et les suivants dans le screen shot 0 s'il y en a plus de 9999 actuellement
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

		LOG("Capture d'écran créée : " << text_SS, ELL_INFORMATION);
	}
}
void Game::switchToNextScene(E_CURRENT_SCENE newCurrentScene)
{
	// Désactivé : Provoque quelques bugs interdisant la remise à zéro d'une scène (dans la fonction Game::joinMultiplayerGame(hostIP) en particulier :
	// ne permet pas de repasser RakNet en mode recherche de parties par une réinitialisation de la scène)
	//if (currentScene == newCurrentScene)
	//	return;

	// Indique la nouvelle scène actuelle
	currentScene = newCurrentScene;

	// Supprime le focus de la GUI (évite que l'élement ayant le focus soit dans une GUI devenue invisible)
	gui->setFocus(NULL);

	// Si une texture est affichée dans l'aperçu de la liste des terrains, on la supprime
	// (quelle que soit la scène actuelle, elle est inutile : si on passe dans le menu de création d'une nouvelle partie, elle sera recréée, sinon elle ne sera pas visible)
	if (guiManager->guiElements.newGameMenuGUI.terrainApercuImage)
	{
		video::ITexture* const newGamePreviewImage = guiManager->guiElements.newGameMenuGUI.terrainApercuImage->getImage();
		if (newGamePreviewImage)
		{
			guiManager->guiElements.newGameMenuGUI.terrainApercuImage->setImage(NULL);
			driver->removeTexture(newGamePreviewImage);
		}
	}

	// Annule la sélection en cours pour réinitialiser la GUI du jeu, au cas où un bouton de construction serait resté appuyé
	batSelector.annulerSelection();

	// Indique à la barre de chargement qu'on a terminé le chargement actuel si un chargement est en cours
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->endLoading();

	// Si la caméra RTS est animée par sa fenêtre d'animation, on arrête son animation
	if (guiManager->guiElements.gameGUI.cameraAnimatorWindow && guiManager->guiElements.gameGUI.cameraAnimatorWindow->isCameraAnimationEnabled())
		guiManager->guiElements.gameGUI.cameraAnimatorWindow->removeAnimatorFromCamera();

	// Enlève la pause si elle est activée
	if (isPaused)
		changePause();

	// Redémarre le timer tant qu'il reste des stops (ils sont comptés) (la fonction changePause ne restaure pas tous les stops !)
	while (deviceTimer->isStopped())	deviceTimer->start();

	switch (currentScene)
	{
	case ECS_MAIN_MENU_LOADING:	// Chargement du menu principal
		// TODO : Créer un écran de chargement pour ce menu (sans barre de chargement)
		break;

	case ECS_MAIN_MENU:	// Menu principal
		// Active la GUI du menu principal
		guiManager->setOnlyGUIVisible(GUIManager::EGN_mainMenuGUI);

		// Actualise le renderer du jeu, pour le mettre à jour avec le temps actuel (ne sera pas fait dans update() si la fenêtre n'est pas encore activée car le jeu est en pause)
		renderer->update();
		break;

	case ECS_OPTIONS_MENU:	// Menu options
		// Active la GUI du menu options (le fond du menu principal est censé être déjà créé)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_optionsMenuGUI);

		// Active l'onglet du menu options correspondant si nécessaire
		if (gameState.optionsMenuTab >= 0 && guiManager->guiElements.optionsMenuGUI.optionsMenu)
		{
			guiManager->guiElements.optionsMenuGUI.optionsMenu->setActiveTab(gameState.optionsMenuTab);
			gameState.optionsMenuTab = -1;
		}

		// Affiche la fenêtre de confirmation des nouveaux paramêtres du menu options si nécessaire
		if (gameState.showOptionsConfirmationWindow)
		{
			if (guiManager->guiElements.optionsMenuGUI.optionsMenu)
				guiManager->guiElements.optionsMenuGUI.optionsMenu->showConfirmationWindow();
			gameState.showOptionsConfirmationWindow = false;
		}
		break;

	case ECS_NEW_GAME_MENU:	// Menu pour créer une nouvelle partie
		// Met à jour la liste des terrains disponibles
		guiManager->updateListTerrainsNewGameGUI(this);

		// Active la GUI du menu pour créer une nouvelle partie (le fond du menu principal est censé être déjà créé)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_newGameMenuGUI);

		// Active l'onglet des terrains
		if (guiManager->guiElements.newGameMenuGUI.mainTabGroup && guiManager->guiElements.newGameMenuGUI.terrainTab)
			guiManager->guiElements.newGameMenuGUI.mainTabGroup->setActiveTab(guiManager->guiElements.newGameMenuGUI.terrainTab);

		// Active le bouton "Commencer"
		if (guiManager->guiElements.newGameMenuGUI.commencerBouton)
			guiManager->guiElements.newGameMenuGUI.commencerBouton->setEnabled(true);

		// Donne le focus de la GUI à la liste des terrains
		gui->setFocus(guiManager->guiElements.newGameMenuGUI.terrainListBox);
		break;

#ifdef USE_RAKNET
	case ECS_MULTIPLAYER_MENU:	// Menu pour rejoindre une partie multijoueur
		// Démarre RakNet pour qu'il recherche des parties multijoueurs en réseau
		if (rkMgr.searchGames())
		{
			// Indique à l'utilisateur qu'une erreur s'est produite
			showErrorMessageBox(L"Avertissement", L"Recherche de parties impossible !");
		}

		// Efface la liste des parties disponibles (elle sera mise à jour régulièrement)
		guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable->clearRows();

		// Active la GUI du menu pour créer une nouvelle partie (le fond du menu principal est censé être déjà créé)
		guiManager->setOnlyGUIVisible(GUIManager::EGN_multiplayerMenuGUI);

		// Donne le focus de la GUI à la liste des parties multijoueurs
		gui->setFocus(guiManager->guiElements.multiplayerMenuGUI.multiplayerGamesTable);
		break;
#endif

	case ECS_LOADING_GAME:	// Chargement du jeu
		// Attention : ici on active seulement l'écran de chargement, le chargement du jeu se fait dans une des fonctions createNewGame, loadSavedGame... suivant le type de partie à créer

		// Indique à la barre de chargement qu'on commence le chargement
		if (guiManager->guiElements.globalGUI.loadingScreen)
			guiManager->guiElements.globalGUI.loadingScreen->beginLoading();

		// Masque toutes les GUIs pour activer la GUI de l'écran de chargement
		guiManager->hideAllGUIs();
		break;

	case ECS_PLAY:	// Scène du jeu (mode un joueur)
		{
			// Active la GUI du jeu
			guiManager->setOnlyGUIVisible(GUIManager::EGN_gameGUI);



			// Met à jour toutes les fenêtres de la GUI et le renderer au moins une fois, en forçant leur mise à jour :
			// Evite un bug : Lors du tout premier jour ("jour 0"), si le jeu est mis en pause, l'affichage du tableau des ressources ou des objectifs est buggé : il n'est mis à jour que lorsqu'au moins un jour système s'est écoulé, or ce n'est pas encore le cas.
			//					Ces fenêtres restent donc non remplies tant qu'au moins 1 jour ne s'est pas écoulé.
			//					Le même problème se pose ainsi pour le renderer, dont la mise à jour est dépendante de la pause.

			// Obtient la transparence par défaut des texte
			const u32 textTransparency = guiManager->getTextTransparency();

			// Actualise le tableau des objectifs
			if (guiManager->guiElements.gameGUI.objectivesWindow)
				guiManager->guiElements.gameGUI.objectivesWindow->update(true);

			// Actualise le tableau récapitulatif des fins de mois
			if (guiManager->guiElements.gameGUI.monthTableWindow)
				guiManager->guiElements.gameGUI.monthTableWindow->update(true);

			// Actualise le menu Ressources
			if (guiManager->guiElements.gameGUI.ressourcesWindow)
				guiManager->guiElements.gameGUI.ressourcesWindow->update(textTransparency, true);

			// Actualise le renderer du jeu, pour le mettre à jour avec le temps actuel, et met aussi à jour la position du soleil d'après le temps actuel du système de jeu
			renderer->update(true);

#ifdef USE_IRRKLANG
			// Met à jour la position de la caméra d'IrrKlang
			if (cameraRTS)
				ikMgr.update(cameraRTS->getAbsolutePosition(), cameraRTS->getTarget(), 0.0f);
			else
				ikMgr.update(core::vector3df(0.0f), core::vector3df(1.0f), 0.0f);
#endif
		}
		break;

	case ECS_GAME_LOST:	// Ecran indiquant au joueur qu'il a perdu
		{
			// Stoppe le timer (pour figer le jeu à l'instant où le joueur a perdu)
			deviceTimer->stop();

			if (cameraRTS)
			{
				// Repasse en mode caméra RTS si la caméra FPS est activée
				cameraRTS->setIsCameraFPSEnabled(false);

				// Bloque la caméra RTS pour empêcher le joueur de la déplacer/tourner
				cameraRTS->setLockCamera(true);
			}

			// Modifie le texte indiquant au joueur que le jeu est terminé pour lui dire qu'il a perdu
			IGUIStaticText* const endTexte = guiManager->guiElements.gameEndGUI.endTexte;
			if (endTexte)
			{
				endTexte->setText(L"DOMMAGE :\r\nVous n'avez pas rempli vos objectifs pour cette partie !");
				endTexte->setOverrideColor(video::SColor(255, 150, 255, 255));
			}

			// Masque le bouton permettant au joueur de continuer la partie en cours
			if (guiManager->guiElements.gameEndGUI.continuerPartieBouton)
				guiManager->guiElements.gameEndGUI.continuerPartieBouton->setVisible(false);

			// Active la GUI de l'écran indiquant au joueur que le jeu est terminé
			guiManager->setOnlyGUIVisible(GUIManager::EGN_gameEndGUI);
		}
		break;

	case ECS_GAME_WON:	// Ecran indiquant au joueur qu'il a gagné
		{
			// Stoppe le timer (pour figer le jeu à l'instant où le joueur a gagné)
			deviceTimer->stop();

			if (cameraRTS)
			{
				// Repasse en mode caméra RTS si la caméra FPS est activée
				cameraRTS->setIsCameraFPSEnabled(false);

				// Bloque la caméra RTS pour empêcher le joueur de la déplacer/tourner
				cameraRTS->setLockCamera(true);
			}

			// Modifie le texte indiquant au joueur que le jeu est terminé pour lui dire qu'il a gagné
			IGUIStaticText* const endTexte = guiManager->guiElements.gameEndGUI.endTexte;
			if (endTexte)
			{
				endTexte->setText(L"FELICITATIONS :\r\nVous avez atteint vos objectifs pour cette partie !");
				endTexte->setOverrideColor(video::SColor(255, 255, 255, 150));
			}

			// Affiche le bouton permettant au joueur de continuer la partie en cours
			if (guiManager->guiElements.gameEndGUI.continuerPartieBouton)
				guiManager->guiElements.gameEndGUI.continuerPartieBouton->setVisible(true);

			// Active la GUI de l'écran indiquant au joueur que le jeu est terminé
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
	// Actualise et effectue le choix de la boîte du dialogue du menu :
	if (guiManager->guiElements.gameGUI.menuWindow && guiManager->guiElements.gameGUI.menuWindow->getUserAction() != CGUIMenuWindow::GMWA_nonChoisi)
	{
		switch (guiManager->guiElements.gameGUI.menuWindow->getUserAction())
		{
		case CGUIMenuWindow::GMWA_creerNouvellePartie:
			// Recrée le menu principal puis retourne au menu de création d'une nouvelle partie
			createMainMenu();
			switchToNextScene(ECS_NEW_GAME_MENU);
			break;

		case CGUIMenuWindow::GMWA_recommencerPartie:
			// Crée une nouvelle partie avec les paramêtres actuels
			createNewGame(system.getModifiers().difficulty, renderer->getTerrainManager().getTerrainInfos().terrainName
#ifdef USE_RAKNET
				, rkMgr.getCurrentNetworkState() == RakNetManager::ENGS_PLAYING_GAME_HOST
#endif
				);
			break;

		case CGUIMenuWindow::GMWA_charger:
			// On affiche la boîte de dialogue pour charger un fichier
			showFileDialog(CGUIFileSelector::EFST_OPEN_DIALOG);
			break;

		case CGUIMenuWindow::GMWA_sauvegarder:
			// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible
			if (CAN_WRITE_ON_DISK)
			{
				// On affiche la boîte de dialogue pour sauvegarder un fichier
				showFileDialog(CGUIFileSelector::EFST_SAVE_DIALOG);
			}
			else
			{
				// Affiche un message d'erreur indiquant qu'on ne peut pas sauvegarder cette partie
				showErrorMessageBox(L"Erreur : Sauvegarde impossible", L"Impossible de sauvegarder cette partie :\r\nLes droits d'écriture sur le disque ont été refusés !");
			}
			break;

		case CGUIMenuWindow::GMWA_retourMenuPrincipal:
			// Recrée le menu principal
			createMainMenu();
			break;

		case CGUIMenuWindow::GMWA_quitter:
			// On quitte le jeu
			device->closeDevice();
			break;

		case CGUIMenuWindow::GMWA_annuler:
			// On ne fait rien
			break;

			// On ne connaît pas cette valeur : on affiche un message d'avertissement si on est en mode débogage
		default:
			LOG_DEBUG("Game::updateGameGUI() : L'action demandee par l'utilisateur est inconnue : guiManager->guiElements.gameGUI.menuWindow->getUserAction() = "
				<< guiManager->guiElements.gameGUI.menuWindow->getUserAction(), ELL_WARNING);
			break;
		}

		// Indique que le choix de l'utilisateur a été traité
		guiManager->guiElements.gameGUI.menuWindow->setUserAction(CGUIMenuWindow::GMWA_nonChoisi);
		return;
	}
}
void Game::updateGameGUI()
{
	if (currentScene != ECS_PLAY)
		return;

	// Obtient la transparence par défaut des textes
	const u32 textTransparency = guiManager->getTextTransparency();

	// Met à jour les couleurs des textes et des informations
	const EcoWorldInfos& worldInfos = system.getInfos();
	{
		// Permet de savoir si on est en train de construire ou de détruire un bâtiment, pour savoir si on peut modifier la couleur du texte du budget ou non
		const bool isCreatingOrDestroyingBatiment = batSelector.isPlacingBat();

		const video::SColor defaultColor(textTransparency, 255, 255, 255);
		const video::SColor redColor(textTransparency, 255, 50, 50);

		// Budget : par valeur (on ne le modifie seulement si on n'est pas en train de créer ou détruire un bâtiment)
		if (guiManager->guiElements.gameGUI.budgetTexte && !isCreatingOrDestroyingBatiment)
		{
			if (worldInfos.budget < 0.0f)
			{
#if 1	// Texte du budget clignotant blanc/rouge lorsque le budget est négatif
				if (!isPaused)	// Evite l'animation du texte du budget si on est en pause : on l'affiche alors toujours rouge
				{
					// Calcule la fréquence des clignotements (en ms) en fonction du budget (ici toujours négatif) :
					// frequence = f(budget) = budget / 100 + 1150
					const u32 frequence = (u32)(core::round_(0.01f * worldInfos.budget + 1150));
					const bool red = (((deviceTimer->getRealTime() / frequence) % 2) == 1);
					guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(red ? redColor : defaultColor);
				}
				else
#endif	// Sinon : Texte du budget toujours rouge lorsque le budget est négatif
					guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(redColor);
			}
			else
				guiManager->guiElements.gameGUI.budgetTexte->setOverrideColor(defaultColor);
		}

		// Budget (information) : par évolution
		if (guiManager->guiElements.gameGUI.budgetInfosTexte)
		{
			if (worldInfos.budgetEvolutionM < 0.0f)
				guiManager->guiElements.gameGUI.budgetInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.budgetInfosTexte->setOverrideColor(defaultColor);
		}

		// Energie : par valeur (on le modifie seulement si on n'est pas en train de créer un bâtiment)
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

		// Effet de serre (informations) : par évolution
		if (guiManager->guiElements.gameGUI.effetSerreInfosTexte)
		{
			if (worldInfos.effetSerreEvolutionM > 0.0f)
				guiManager->guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.effetSerreInfosTexte->setOverrideColor(defaultColor);
		}

		// Déchets : par taxes
		if (guiManager->guiElements.gameGUI.dechetsTexte)
		{
			if (worldInfos.taxes[EcoWorldInfos::TI_dechets] > 0.0f)
				guiManager->guiElements.gameGUI.dechetsTexte->setOverrideColor(redColor);
			else
				guiManager->guiElements.gameGUI.dechetsTexte->setOverrideColor(defaultColor);
		}

		// Déchets (informations) : par évolution
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

	// Remet les textes à jour
	if (guiManager->guiElements.gameGUI.budgetTexte)
	{
		swprintf_SS(L"Budget : %.2f €", worldInfos.budget);
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
		swprintf_SS(L"Déchets : %.0f kg", worldInfos.dechets);
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
		// Affiche le temps écoulé sous forme de date
		core::stringw str;
		appendDate(str, (u32)(system.getTime().getTotalJours()));
		guiManager->guiElements.gameGUI.dateTexte->setText(str.c_str());
#else
		// Date affichée sans correction des pluriels
		swprintf_SS(L"%d années  %d mois  %d jours",
			system.getTime().getAnnees(), system.getTime().getMois(), system.getTime().getJours());
		guiManager->guiElements.gameGUI.dateTexte->setText(text);
#endif
	}

	if (guiManager->guiElements.gameGUI.budgetInfosTexte && guiManager->guiElements.gameGUI.budgetInfosTexte->isVisible())
	{
		swprintf_SS(L"Evolution (M) : %+.2f €\r\nEvolution (A) : %+.2f €",
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
		swprintf_SS(L"Evolution (M) : %+.0f kg\r\nTaxe (M) : %+.2f €",
			worldInfos.effetSerreEvolutionM, -(worldInfos.taxes[EcoWorldInfos::TI_effetSerre]));
		guiManager->guiElements.gameGUI.effetSerreInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.dechetsInfosTexte && guiManager->guiElements.gameGUI.dechetsInfosTexte->isVisible())
	{
		swprintf_SS(L"Evolution (M) : %+.0f kg\r\nTaxe (M) : %+.2f €",
			worldInfos.dechetsEvolutionM, -(worldInfos.taxes[EcoWorldInfos::TI_dechets]));
		guiManager->guiElements.gameGUI.dechetsInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.populationInfosTexte && guiManager->guiElements.gameGUI.populationInfosTexte->isVisible())
	{
		swprintf_SS(L"Satisfaction : %.0f %%\r\nSatisfaction réelle : %.0f %%",			// '%%' : Symbole %
			fabs(worldInfos.popSatisfaction * 100.0f),		// "fabs" est utilisé pour toujours enlever le signe "-", même lorsque le résultat est nul
			fabs(worldInfos.popRealSatisfaction * 100.0f));	// La satisfaction réelle est aussi dépendante du pourcentage d'énergie disponible pour cette maison (facteur entre *0.5f à 0% d'énergie à *1.0f à 100% d'énergie)
		guiManager->guiElements.gameGUI.populationInfosTexte->setText(textW_SS);
	}
	if (guiManager->guiElements.gameGUI.dateInfosTexte && guiManager->guiElements.gameGUI.dateInfosTexte->isVisible())
	{
		core::stringw str;
		appendDays(str, system.getTime().getTotalJours(), true);	// Ajoute le temps écoulé à la description

		// Ajoute la vitesse actuelle du jeu et le temps d'un jour
		const float currentTimerSpeed = deviceTimer->getSpeed();
		const float currentDayTime = (DAY_TIME / currentTimerSpeed);
		if (currentDayTime < 0.2f)
			swprintf_SS(L"\r\nVitesse du jeu : %.1f\r\n1 jour <=> %.2f sec",	// Le temps d'un jour est très petit : on augmente la précision de son affichage
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

	// Actualise le tableau récapitulatif des fins de mois, s'il est visible
	if (guiManager->guiElements.gameGUI.monthTableWindow)
		if (guiManager->guiElements.gameGUI.monthTableWindow->isVisible())
			guiManager->guiElements.gameGUI.monthTableWindow->update();

	// Actualise le menu Ressources, s'il est visible
	if (guiManager->guiElements.gameGUI.ressourcesWindow)
		if (guiManager->guiElements.gameGUI.ressourcesWindow->isVisible())
			guiManager->guiElements.gameGUI.ressourcesWindow->update(textTransparency);

	// Descends ou remonte le menu inférieur
	if (vitesseDescenteMenuInferieur != 0.0f && guiManager->guiElements.gameGUI.tabGroupBas)
	{
		// Calcule le temps écoulé depuis le début de la descente de ce menu
		const u32 realTimeMs = deviceTimer->getRealTime();
		const u32 realElapsedTimeMsDescenteMenu = (realTimeMs > realTimeMsStartDescenteMenuInferieur ? realTimeMs - realTimeMsStartDescenteMenuInferieur : (u32)0);

		// Calcule la nouvelle position en Y du menu inférieur
		const u32 screenSizeHeight = driver->getScreenSize().Height;
		core::recti tabGroupBasPos = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition();
		tabGroupBasPos.UpperLeftCorner.Y = menuInferieurStartPosY + core::round32(vitesseDescenteMenuInferieur * (float)(screenSizeHeight * realElapsedTimeMsDescenteMenu));
		guiManager->guiElements.gameGUI.tabGroupBas->setRelativePosition(tabGroupBasPos);

		// Vérifie si le menu est arrivé en haut ou en bas
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

	// Obtient l'élément le plus "au-dessus" qui contient la position de la souris
	gui::IGUIElement* const element = parent->getElementFromPoint(mouseState.position);
	if (element)
	{
		// On vérifie que l'élément n'est l'élement de base de la GUI, car cet élément contient tout l'écran :
		// la souris sera donc obligatoirement dedans, et mouseOnGUI sera toujours égal à true
		if (element == gui->getRootGUIElement())
			return false;

		// On vérifie que l'élément n'est pas un élément parent d'une GUI, car ce sont des éléments qui entourent tout l'écran :
		// la souris sera donc obligatoirement dedans, et mouseOnGUI sera toujours égal à true
		for (int i = 0; i < GUIManager::EGN_COUNT; ++i)
			if (element == guiManager->getGUI((GUIManager::E_GUI_NAME)i))
				return false;

		// Si l'élément pointé n'est aucun de ces éléments, c'est que la souris est bien sur un élément de la gui
		return true;
	}

	return false;
#else
	// Ancienne Version moins optimisée : Prototype de la fonction :

	// Calcule si la souris est sur un élément de la GUI
	// maxLoop : nombre maximal d'appels récurrents que peut faire cette fonction (-1 = pas de limites)
	// parent : élément parent à partir duquel vérifier si la souris est sur cet élément ou sur un de ses enfants
	// currentLoop : numéro de boucle actuel, ne pas utiliser
	// Retourne true si la souris est sur un élément de la GUI, false sinon
	// bool calculerIsMouseOnGUI(int maxLoop = 5, gui::IGUIElement* parent = 0, int currentLoop = 0);

	if ((maxLoop >= 0 && currentLoop >= maxLoop) || currentLoop < 0)
		return false;

	if (!parent)
		parent = gui->getRootGUIElement();

	// Parcourt tous les enfants de l'élément racine de la GUI (non inclus car ce dernier contient tout l'écran, donc la souris sera forcément dedans)
	const core::list<gui::IGUIElement*> children = parent->getChildren();
	core::list<gui::IGUIElement*>::ConstIterator it = children.begin();
	for (; it != children.end(); ++it)
	{
		// On vérifie que l'élément n'est pas un élément parent d'une GUI, car ce sont des éléments qui entourent tout l'écran, la souris sera donc forcément dedans
		bool valide = true;
		for (int i = 0; i < GUIManager::EGN_COUNT && valide; ++i)
			if ((*it) == guiManager->getGUI(i))
				valide = false;

		if ((*it)->isVisible())
		{
			// On regarde si l'élément est visible et si la position de la souris est dedans
			if ((*it)->isPointInside(mouseState.position) && valide)
			{
				// Si oui, on indique que la souris est sur un élément de la GUI et on quitte
				// (cela ne sert plus à rien de continuer à parcourir les tableaux)
				return true;
			}
			else
			{
				// Sinon, on vérifie ses enfants :
				// Si la souris est sur un élément de la GUI, on quitte
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
	// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible !
	if (!CAN_WRITE_ON_DISK)
	{
		LOG("AVERTISSEMENT : Impossible de créer une sauvegarde automatique : Les droits d'écriture sur le disque ont été refusés !", ELL_WARNING);
		return;
	}

	// On ne peut changer l'état de la pause car il est sauvegardé, donc on stoppe le timer manuellement s'il ne l'est pas
	const bool wasTimerStarted = !deviceTimer->isStopped();
	if (wasTimerStarted)
		deviceTimer->stop();

	// Obtient la date réelle actuelle grâce au timer du device
	const ITimer::RealTimeDate timeDate = deviceTimer->getRealTimeAndDate();

	// Crée le dossier pour les sauvegardes automatiques
	createDirectory("Saves");	// Ce dossier a normalement déjà été créé par l'appel à createDirectories dans le main, mais vérifie qu'il est bien créé, au cas où il aurait été supprimé manuellement par le joueur entre-temps
	createDirectory("Saves/AutoSaves");

	// Crée le nom du fichier avec la date actuelle et le nom du terrain actuel
	// Format :	"Saves/AutoSaves/24-08-11 21h57 (1422634) terrain plat.ewg"
	//			Pour une partie automatiquement enregistrée le "24-08-11" à "21h57", dont le temps de jeu est "1422634" ms (= 23 min 42 s 634 ms), sur le terrain "terrain plat"
	//			L'indication du temps de jeu sert surtout à éviter que des sauvegardes automatiques ne s'ajoutent au bout du même fichier, sous prétexte qu'ils ont le même nom
	//			(peut arriver si la vitesse du jeu est si rapide que deux sauvegardes sont enregistrées dans la même minute).
	io::path terrainName;
	core::cutFilenameExtension(terrainName, renderer->getTerrainManager().getTerrainInfos().terrainName);
	terrainName = core::deletePathFromFilename(terrainName);

	sprintf_SS("Saves/AutoSaves/%02d-%02d-%02d %02dh%02d (%d) %s.ewg",
		timeDate.Day, timeDate.Month, (abs(timeDate.Year) % 2000),
		timeDate.Hour, timeDate.Minute,
		core::round32(system.getTime().getTotalTime() * 1000.0f),
		terrainName);

	// Enregistre le jeu à cette adresse
	saveCurrentGame(text_SS);

	// Indique l'heure du timer à laquelle l'enregistrement automatique a été effectué
	lastAutoSaveTime = deviceTimer->getTime();

	// Redémarre le timer s'il n'était pas stoppé
	if (wasTimerStarted)
		deviceTimer->start();
}
bool Game::saveCurrentGame(const io::path& adresse)
{
	// Vérifie qu'on peut écrire sur le disque dur de l'ordinateur cible !
	if (!CAN_WRITE_ON_DISK)
	{
		LOG("ERREUR : Impossible d'enregistrer la partie actuelle (" << adresse.c_str() << ") : Les droits d'écriture sur le disque ont été refusés !", ELL_ERROR);
		return true;
	}

	// Crée les données pour le fichier
	io::IWriteFile* writeFile = fileSystem->createAndWriteFile(adresse);

	// Enregistre la partie en mode "Efficace"
	if (saveCurrentGame_Eff(writeFile))
	{
		if (writeFile)
			writeFile->drop();

		// Indique à l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors de l'enregistrement de la partie !");
		return true;
	}

	if (writeFile)
		writeFile->drop();

	LOG(endl << "Jeu enregistré : " << adresse.c_str() << endl, ELL_INFORMATION);

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

	// Crée les données pour le fichier
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

	// Ecrit quelques données sur Game
	out->addString("EcoWorldVersion", ECOWORLD_VERSION);
	io::path terrainName = "terrain plat.ewt";
	if (renderer)
	{
		// Obtient le nom du terrain actuellement chargé dans le renderer :

		const core::stringc& rendererTerrainName = renderer->getTerrainManager().getTerrainInfos().terrainName;

		// Supprime le chemin d'accès au terrain (ex : "E:\EcoWorld/data/Terrains/Terrain.ewt") pour ne conserver que son nom (ex : "Terrain.ewt") :
		const int lastSlash = rendererTerrainName.findLastChar("/\\", 2);	// Recherche le dernier '/' ou '\'
		if (lastSlash >= 0)	// Si un slash a bien été trouvé
		{
			// On ne récupère que la sous-chaîne de ce terrain à partir de ce caractère (exclu) :
			terrainName = rendererTerrainName.subString(lastSlash + 1, terrainName.size());
		}
		else
			terrainName = rendererTerrainName;

		// Rend la casse du terrain en minuscule car le système de fichier d'Irrlicht est en minuscules (non nécessaire en pratique, mais permet de conserver une certaine cohérence)
		terrainName.make_lower();
	}
	out->addString("TerrainName", terrainName.c_str());
	out->addBool("IsPaused", isPaused);
	out->addFloat("TimerSpeed", deviceTimer->getSpeed());
#ifdef USE_RAKNET
	if (!multiplayerSave)	// On n'enregistre pas les informations de la caméra en mode multijoueur
#endif
		out->addBool("LockCamera", lockCamera);

	out->write(writer, false, L"Game");
	out->clear();

	// Ecrit les données de la camera RTS (si une camera FPS est utilisée, les données de la camera ne seront pas enregistrées)
	if (
#ifdef USE_RAKNET
		!multiplayerSave &&	// On n'enregistre pas les informations de la caméra en mode multijoueur
#endif
		cameraRTS)
	{
		cameraRTS->serializeAttributes(out);

		out->write(writer, false, L"Camera");
		out->clear();
	}

	// Ecrit les données de l'animation de la caméra et du GUI Manager
	if (
#ifdef USE_RAKNET
		!multiplayerSave &&	// On n'enregistre pas les informations de l'animation de la caméra et de la GUI en mode multijoueur
#endif
		guiManager)
	{
		// Ecrit les données de l'animation de la caméra
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->save(out, writer);

		// Ecrit les données du GUI Manager
		guiManager->save(out, writer, (isMenuInferieurHaut() != false));
	}

	// Ecrit les données du renderer
	// TODO : Si nécessaire
	//if (renderer)
	//	renderer->save(out, writer);

	// Enregistre le système
	system.save(out, writer);

	// Supprime les données pour le fichier
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
	// Passe à la scène de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Indique la valeur maximale de la barre de chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->setMaxPercent(100.0f);

	// Charge les données du jeu
	loadGameData(terrainFilename);

	// Indique au système qu'on veut créer une nouvelle partie avec la difficulté spécifiée et les objectifs personnalisés par l'utilisateur
	system.createNewGame(difficulty,
		(guiManager->guiElements.newGameMenuGUI.objectivesModifier ?
			guiManager->guiElements.newGameMenuGUI.objectivesModifier->getObjectives() : EcoWorldSystem::getNewGameDefaultObjectives()));

#ifdef USE_RAKNET
	// Si le mode multijoueur est activée, on initialise RakNet
	if (multiplayer)
	{
		if (rkMgr.createGame())
		{
			// Indique à l'utilisateur qu'une erreur s'est produite (avant le changement de scène, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
			showErrorMessageBox(L"Erreur", L"Impossible d'activer le mode multijoueur !");

			// Retourne à la scène du menu de création de partie
			createMainMenu();
			switchToNextScene(ECS_NEW_GAME_MENU);

			return true;
		}
	}
#endif

	// Passe à la scène du jeu
	switchToNextScene(ECS_PLAY);

	return false;
}
bool Game::loadSavedGame(const io::path& adresse)
{
	// Crée les données pour le fichier
	io::IReadFile* const readFile = fileSystem->createAndOpenFile(adresse);
	if (!readFile)
	{
		// Indique à l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie !");
		return true;
	}



	// Passe à la scène de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Charge le jeu avec la méthode "Efficace"
	bool sameGameVersion = false, startPaused = false;
	core::stringc terrainFilename;
	if (loadSavedGame_Eff(readFile, sameGameVersion, startPaused, terrainFilename))
	{
		// Indique à l'utilisateur qu'une erreur s'est produite
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie !");

		// Retourne au menu principal
		createMainMenu();

		// Désactivé : On ne libère pas le readfile car il a automatiquement été libéré dans la fonction loadSavedGame_Eff
		//readFile->drop();
		return true;
	}

	// Désactivé : On ne libère pas le readfile car il a automatiquement été libéré dans la fonction loadSavedGame_Eff
	//readFile->drop();

	// Passe à la scène du jeu
	switchToNextScene(ECS_PLAY);

	// Restaure l'état de la pause
	if (startPaused != isPaused)
		changePause();

#ifdef USE_IRRKLANG
	// Redémarre les sons d'IrrKlang si le mode audio est activé
	if (gameConfig.audioEnabled)
		ikMgr.setAllSoundsPaused(false);
#endif

	// Avertit l'utilisateur si la version du jeu chargée est différente de la version actuelle du jeu
	if (!sameGameVersion)
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLa version de cette sauvegarde est différente de la version actuelle du jeu.\r\nDes erreurs pourraient se produire en cours de jeu !");
	// Sinon, avertit l'utilisateur si le terrain spécifié n'a pas été trouvé
	else if (!fileSystem->existFile(terrainFilename))
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLe terrain utilisé dans cette sauvegarde n'a pas été trouvé.\r\nDes erreurs pourraient se produire en cours de jeu !");

	LOG(endl << "Jeu chargé : " << adresse.c_str() << endl, ELL_INFORMATION);

	return false;
}
bool Game::loadSavedGame_Eff(io::IReadFile* readFile, bool& outSameGameVersion, bool& outStartPaused, core::stringc& outTerrainFilename
#ifdef USE_RAKNET
							 , bool multiplayerLoad
#endif
							 )
{
	// Par défaut, on considère que la version de la sauvegarde est différente
	outSameGameVersion = false;
	outStartPaused = false;
	outTerrainFilename = "";

	// Indique la valeur maximale de la barre de chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->setMaxPercent(200.0f);	// 0.0f à 100.0f : Chargement normal du jeu ; 100.0f à 200.0f : Chargement de la partie sauvegardée

	// Crée les données pour le fichier
	io::IXMLReader* const reader = fileSystem->createXMLReader(readFile);
	if (readFile)
		readFile->drop();	// Le fichier de lecture n'est plus nécessaire : on le libère automatiquement
	if (!reader)
		return true;

	io::IAttributes* const in = fileSystem->createEmptyAttributes(driver);
	if (!in)
	{
		reader->drop();
		return true;
	}

	// Charge quelques données sur Game (avant le chargement complet du jeu pour que la fonction "loadGameData" charge le bon terrain)
	float timerSpeed = 1.0f;
	lockCamera = false;
	//reader->resetPosition();
	if (in->read(reader, false, L"Game"))
	{
		if (in->existsAttribute("EcoWorldVersion"))	outSameGameVersion = (in->getAttributeAsString("EcoWorldVersion").equals_ignore_case(ECOWORLD_VERSION));
		if (in->existsAttribute("TerrainName"))		outTerrainFilename = in->getAttributeAsString("TerrainName");
		if (in->existsAttribute("TimerSpeed"))		timerSpeed = in->getAttributeAsFloat("TimerSpeed");
		if (in->existsAttribute("IsPaused"))		outStartPaused = in->getAttributeAsBool("IsPaused");

		// On garde cette donnée pour la fin de la fonction :
		if (
#ifdef USE_RAKNET
			!multiplayerLoad &&	// On ne charge pas les informations de la caméra en mode multijoueur
#endif
			in->existsAttribute("LockCamera"))		lockCamera = in->getAttributeAsBool("LockCamera");

		in->clear();
	}

	// Charge les données du jeu avec le terrain spécifié, en ne démarrant les sons qu'en mode pause
	loadGameData(outTerrainFilename, true);

	// Charge les données de la camera RTS
	if (
#ifdef USE_RAKNET
		!multiplayerLoad &&	// On ne charge pas les informations de la caméra en mode multijoueur
#endif
		cameraRTS)
	{
		reader->resetPosition();
		if (in->read(reader, false, L"Camera"))
		{
			cameraRTS->deserializeAttributes(in);
			in->clear();
		}

		// Change le blocage de la caméra
		cameraRTS->setLockCamera(lockCamera);

		in->clear();
	}

	// Charge les données de l'animation de la caméra et du GUI Manager
	// TODO : Gérer la barre de chargement si nécessaire
	isMouseOnGUI = false;
	vitesseDescenteMenuInferieur = 0.0f;
#ifdef USE_RAKNET
	if (!multiplayerLoad)	// On ne charge pas les informations de l'animation de la caméra et de la GUI en mode multijoueur
#endif
	{
		// Charge les données de l'animation de la caméra
		if (guiManager->guiElements.gameGUI.cameraAnimatorWindow)
			guiManager->guiElements.gameGUI.cameraAnimatorWindow->load(in, reader);

		// Charge les données du GUI Manager
		guiManager->load(in, reader);
	}

	// Charge les données du renderer
	// TODO : Si nécessaire + Gérer la barre de chargement si nécessaire
	//if (renderer)
	//	renderer->load(in, reader);

	// Charge le système de jeu (il commandera directement au renderer la création des bâtiments)
	system.load(in, reader, guiManager->guiElements.globalGUI.loadingScreen, 100.0f, 200.0f);

	// Indique à la barre de chargement qu'on a terminé le chargement
	if (guiManager->guiElements.globalGUI.loadingScreen)
		guiManager->guiElements.globalGUI.loadingScreen->endLoading();

	// Supprime les données pour le fichier
	in->drop();
	reader->drop();



	// Modifie la vitesse du timer du jeu
	deviceTimer->setSpeed(timerSpeed);

	// Indique la nouvelle vitesse du jeu à la GUI
	guiManager->setGameGUITimerSpeed(timerSpeed);

#ifdef USE_SPARK
	// Indique à Spark que la vitesse du jeu a changé
	renderer->getSparkManager().setTimerSpeed(timerSpeed);
#endif

	return false;
}
void Game::createCamera()
{
	// Crée la caméra RTS (pour la première et unique fois)
	if (!cameraRTS)
	{
		// Crée la caméra FPS de la caméra RTS
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

		// La caméra RTS
		cameraRTS = new RTSCamera(sceneManager->getRootSceneNode(), -1, -500.0f, 100.0f, 350.0f, 500.0f, cameraFPS);

		cameraRTS->setTerrainLimits(core::rectf(-TERRAIN_LIMITS, -TERRAIN_LIMITS, TERRAIN_LIMITS, TERRAIN_LIMITS));

		cameraRTS->setNearValue(CAMERA_NEAR_VALUE);
		cameraRTS->setFarValue(CAMERA_FAR_VALUE);
	}
	else
	{
		// Supprime les animators de la caméra RTS (ils devront être recréés si nécessaire)
		cameraRTS->removeAnimators();

		// Indique au scene manager que la caméra RTS est la caméra actuelle
		cameraRTS->setIsCameraFPSEnabled(false);
	}
}
void Game::createMainMenu()
{
	// Passe à la scène de chargement du menu principal
	switchToNextScene(ECS_MAIN_MENU_LOADING);

	// Efface tous les éléments du système de jeu et du sceneManager (en conservant la caméra RTS si elle existe), et stoppe IrrKlang si autorisé
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



	// Crée la caméra RTS si elle n'a pas encore été créée, et la réinitialise
	createCamera();

	// Indique la positions et la direction de la caméra RTS (relativement à TAILLE_OBJETS) (le +128.0f en Y est dû au décalage des terrains en Y de +128.0f)
	cameraRTS->setPosition(core::vector3df(0.0f, 128.0f + 5.0f * TAILLE_OBJETS, 0.0f));	// 100.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTarget(core::vector3df(0.0f, 128.0f, 0.0f));

	// On ne met pas de limites pour la caméra, elle n'est pas censé bouger de la trajectoire que lui fait suivre son animator
	cameraRTS->setZoomLock(false, 0.0f, 550.0f);
	cameraRTS->setRotateXLock(false, 0.0f, 0.0f);
	cameraRTS->setRotateYLock(false, 0.0f, 90.0f);

	// On bloque la camera par défaut (elle doit être bloquée pour que les animators puissent fonctionner)
	lockCamera = true;
	cameraRTS->setLockCamera(true);



	// Ajoute l'animator pour faire tourner la caméra
	scene::ISceneNodeAnimator* const anim = sceneManager->createFlyCircleAnimator(
		cameraRTS->getPosition(), 500.0f, -0.00001f, core::vector3df(0.0f, 1.0f, 0.0f), 0.25f);
	cameraRTS->addAnimator(anim);
	anim->drop();



	// Compile les effets nécessaires de PostProcess
	if (gameConfig.usePostProcessEffects && postProcessManager)
	{
		// Compile les shaders particuliers du jeu : shader de rendu final sur l'écran, shader de tremblement de la caméra et shader de profondeur de la scène
		postProcessManager->compileParticularShaders(gameConfig.postProcessShakeCameraOnDestroying, gameConfig.postProcessUseDepthRendering);

		// Compile les effets supplémentaires de la configuration du jeu
		postProcessManager->compileEffects(gameConfig.postProcessEffects);
	}

	// Initialise XEffects si ses ombres sont activées
	if (gameConfig.useXEffectsShadows && !xEffects)
		xEffects = new EffectHandler(device, gameConfig.xEffectsScreenRTTSize,
			gameConfig.xEffectsUseVSMShadows,
			gameConfig.xEffectsUseRoundSpotlights,
			gameConfig.xEffectsUse32BitDepthBuffers,
			gameConfig.xEffectsFilterType,
			shaderPreprocessor, screenQuad);

	// Re-vérifie les options de la configuration actuelle du jeu au driver (elles sont dépendantes de l'utilisation de XEffects)
	applyGameConfigDriverOptions();



	// Si un temps forcé est spécifié dans GameState, on l'applique obligatoirement (peut aussi être utilisé à l'appel de cette fonction pour forcer un certain temps)
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

	// Met à jour le titre du menu principal
	if (guiManager)
	{
		// Précharge tous les titres du menu principal susceptibles d'être affichés avec cette résolution, par un changement de temps
		guiManager->loadMainMenuTitles(driver->getScreenSize());

		// Choisis le titre actuel du menu principal
		guiManager->chooseMainMenuTitleFromWeather(system.getWeatherManager().getCurrentWeatherID(), driver->getScreenSize());
		currentWeatherID = system.getWeatherManager().getCurrentWeatherID();
	}

	if (renderer)
	{
		// Initialise le monde grace au renderer
		renderer->initWorld(true);

		// Charge le terrain en désactivant la création des triangles selectors du terrain et du sol invisible et en désactivant la conservation des informations sur le terrain en mémoire
		renderer->loadNewTerrain("Terrain Plat.ewt", gameConfig.audioEnabled, false, false);
	}



#ifdef USE_IRRKLANG
	// Joue la musique du menu principal si on peut modifier IrrKlang et qu'on ne doit pas charger directement de partie ou de terrain (on ne démarre pas la musique du menu principal lors d'un double-clic dans l'explorateur windows)
	if (!gameState.keepIrrKlang)
		ikMgr.playMainMenuMusics(!gameConfig.audioEnabled);
#endif



	// On doit redémarrer dans le menu options, on change donc de scène une fois le menu principal initialisé
	if (gameState.restartToOptionsMenu)
	{
		gameState.restartToOptionsMenu = false;

		// Indique le temps du device avec l'ancien temps
		if (gameState.lastDeviceTime != 0)
		{
			deviceTimer->setTime(gameState.lastDeviceTime);
			lastDeviceTime = gameState.lastDeviceTime;	// Réinitialise aussi le temps de dernière mise à jour du temps écoulé, pour éviter que le prochain temps écoulé ne vale gameState.lastDeviceTime
		}
		gameState.lastDeviceTime = 0;

		// Indique aussi qu'on peut à nouveau modifier IrrKlang
		gameState.keepIrrKlang = false;

		// Change de scène jusqu'au menu options
		switchToNextScene(ECS_OPTIONS_MENU);
	}
	else	// Change de scène jusqu'au menu principal
		switchToNextScene(ECS_MAIN_MENU);
}
void Game::loadGameData(const io::path& terrainFilename, bool startSoundsPaused)
{
	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(0.0f))	return;

	// Indique qu'on ne peut pas conserver IrrKlang pendant le chargement du jeu :
	gameState.keepIrrKlang = false;

	// Efface tous les éléments du système de jeu et du sceneManager (en conservant la caméra RTS)
	// Note : On ne stoppe pas IrrKlang (sauf si le mode audio est désactivé) pour permettre à la musique de continuer pendant le chargement du jeu
	resetGame(true, true, !gameConfig.audioEnabled);

#ifdef USE_IRRKLANG
	// Si le mode audio est activé, on stoppe tous les sons (et pas les musiques) d'IrrKlang qui sont en train d'être joués,
	// sauf les sons de la GUI pour pouvoir entendre le son du clic sur le bouton
	if (gameConfig.audioEnabled)
		ikMgr.stopAllSoundsOnlyExceptType(IrrKlangManager::EST_ui_sound);
#endif



	// Crée la caméra RTS si elle n'a pas encore été créée (par exemple, parce que le menu principal n'a pas été créé : lors d'un double-clic sur une partie ou un terrain dans l'explorateur windows), et la réinitialise
	createCamera();

	// Replace la camera à l'origine (relativement à TAILLE_OBJETS) (le +128.0f en Y est dû au décalage des terrains en Y de +128.0f)
	cameraRTS->setPosition(core::vector3df(0.0f, 128.0f + 20.0f * TAILLE_OBJETS, -20.0f * TAILLE_OBJETS));	// (0.0f, 400.0f, -400.0f) pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTarget(core::vector3df(0.0f, 128.0f + 2.0f * TAILLE_OBJETS, 0.0f));						// (0.0f, 40.0f, 0.0f) pour TAILLE_OBJETS = 20.0f

	// Indique les valeurs de déplacement de la caméra, rendus relatifs à TAILLE_OBJETS :
	cameraRTS->setZoomStep(5.0f * TAILLE_OBJETS);			// 100.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setZoomSpeed(17.5f * TAILLE_OBJETS);			// 350.0f pour TAILLE_OBJETS = 20.0f
	//cameraRTS->setTranslateSpeed(25.0f * TAILLE_OBJETS);	// 500.0f pour TAILLE_OBJETS = 20.0f
	cameraRTS->setTranslateSpeed(40.0f * TAILLE_OBJETS);	// 800.0f pour TAILLE_OBJETS = 20.0f

	// La vitesse de rotation de la caméra ne doit pas être dépendante de TAILLE_OBJETS
	cameraRTS->setRotationSpeed(-500.0f);

	// Zoom : 50.0f -> 150.0f -> 250.0f ... -> 550.0f -> 650.0f -> 750.0f -> ... -> 1050.0f pour TAILLE_OBJETS = 20.0f :
	cameraRTS->setZoomLock(true, 2.5f * TAILLE_OBJETS, 52.5f * TAILLE_OBJETS);

	// Limite les mouvements de la caméra RTS
	cameraRTS->setRotateXLock(false, 0.0f, 0.0f);
	cameraRTS->setRotateYLock(true, 0.0f, 90.0f);



	// Indique la vitesse actuelle du jeu à la GUI
	guiManager->setGameGUITimerSpeed(deviceTimer->getSpeed());

	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(5.0f))	return;



#ifdef USE_IRRKLANG
	// Charge IrrKlang au complet (doit être fait avant le chargement du terrain, car ce dernier utilise des sons durant son chargement)
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
			fileSystem->existFile(terrainFilename) ? terrainFilename.c_str() : "terrain plat.ewt",		// Vérifie que le terrain existe, sinon on en choisi un par défaut
			gameConfig.audioEnabled, true, true, guiManager->guiElements.globalGUI.loadingScreen, 25, 50);

		if (guiManager->guiElements.globalGUI.loadingScreen)
			if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(50.0f))	return;

		// Initialise complètement le monde avec le renderer (charge les bâtiments du jeu)
		renderer->initWorld(false, guiManager->guiElements.globalGUI.loadingScreen, 50, 95);
	}



	if (guiManager->guiElements.globalGUI.loadingScreen)
		if (guiManager->guiElements.globalGUI.loadingScreen->setPercentAndDraw(95.0f))	return;



#ifdef USE_COLLISIONS
	// Ajoute le collision response animator à la caméra
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
	// Arrête tous les sons d'IrrKlang (dont la musique du menu principal qui est encore en train d'être jouée)
	ikMgr.stopAllSounds();

	// Démarre les musiques et sons
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

	// Demande à RakNet les informations sur la partie multijoueur pour le chargement
	if (rkMgr.queryGameData(hostIP))
	{
		// Indique à l'utilisateur qu'une erreur s'est produite (avant le changement de scène, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible de se connecter à cet hôte !");

		// Retourne à la scène du menu pour rejoindre une partie multijoueur
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}

	// Crée une message box de fond pour faire patienter le joueur et lui permettre d'annuler cette recherche
	gui::IGUIWindow* const waitMessageBox = CGUIMessageBox::addMessageBox(gui,
		L"Rejoindre une partie multijoueur", L"Connexion en cours. Veuillez patienter...", gui::EMBF_CANCEL, gui->getRootGUIElement(), true);
	if (waitMessageBox)
	{
		// Grab cette boîte de dialogue : on aura encore besoin d'y accéder même si sa fonction remove() a été appelée (dans le cas d'un appui sur le bouton Annuler)
		waitMessageBox->grab();

		// Masque le bouton pour fermer cette boîte de dialogue (seul le bouton annuler est visible)
		waitMessageBox->getCloseButton()->setVisible(false);
	}

	// Les informations sur cette partie multijoueur (sous forme de partie sauvegardée)
	char* gameSaveData = NULL;
	u32 gameSaveDataSize = 0;

	// Initialisation des variables dans cet ordre : device->run() met à jour le timer d'Irrlicht et deviceTimer->getRealTime() obtient le temps réel
	bool keepSearchingGameData = device->run();
	const u32 timeoutRealTime = deviceTimer->getRealTime() + 60000;	// Sécurité : 60 secondes max d'attente pour obtenir les informations sur la partie

	// Boucle d'attente des informations sur la partie à rejoindre :
	while (keepSearchingGameData && deviceTimer->getRealTime() < timeoutRealTime)
	{
		// Met à jour RakNet
		rkMgr.update();

		// Vérifie si la réponse du serveur n'est pas arrivée
		if (rkMgr.receivedPackets.size())
		{
			// Parcours la liste des messages de RakNet
			const core::list<RakNetManager::RakNetGamePacket>::Iterator END = rkMgr.receivedPackets.end();
			for (core::list<RakNetManager::RakNetGamePacket>::Iterator it = rkMgr.receivedPackets.begin(); it != END; ++it)
			{
				const RakNetManager::RakNetGamePacket& gamePacket = (*it);
				if (gamePacket.packetType == RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_RECEIVED)
				{
					// La réception des données du jeu a réussie : on continue le chargement
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
					// La réception des données du jeu a échouée : on annule le chargement
					keepSearchingGameData = false;

					// Supprime ce message de la liste des messages du jeu puis quitte cette boucle
					rkMgr.receivedPackets.erase(it);
					break;
				}
			}

			// Met à jour le device (pour mettre à jour le timer entre-autres, ainsi que la fenêtre du jeu)
			bool continueSearch = device->run();

			// Vérifie que la message box n'a pas été fermée (dans ce cas, remove() a été appelé : elle n'a désormais plus de parent)
			if (waitMessageBox)
				continueSearch &= (waitMessageBox->getParent() != NULL);	// Arrête la recherche si elle n'a plus de parent

			if (continueSearch)	// Vérifie que la recherche n'a pas été annulée
			{
				// Effectue un rendu rapide de la scène
				if (updateAll())
					render();
			}
			else
			{
				// La fenêtre du jeu a été fermée : on quitte le chargement
				keepSearchingGameData = false;
				if (gameSaveData)
				{
					delete[] gameSaveData;	// N'oublie pas de détruire gameData s'il est valide car il a été créé avec new[]
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
		waitMessageBox->drop();	// Supprime définitivement cette message box, pour annuler le grab précédent
	}

	// Quitte si on n'a pas pu recevoir les informations sur la partie multijoueur à charger
	if (!gameSaveData || !gameSaveDataSize)
	{
		// Indique à l'utilisateur qu'une erreur s'est produite (avant le changement de scène, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible d'obtenir les informations sur cette partie multijoueur !");

		// Retourne à la scène du menu pour rejoindre une partie multijoueur
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}



	// Crée le fichier de lecture de la mémoire pour pouvoir charger les informations reçues de l'hôte comme un fichier
	io::IReadFile* readFile = fileSystem->createMemoryReadFile(gameSaveData, gameSaveDataSize, "Multiplayer_ServeurSavedGame_Load");

	// Passe à la scène de chargement du jeu
	switchToNextScene(ECS_LOADING_GAME);

	// Charge le jeu avec la méthode "Efficace" et en mode multijoueur
	bool sameGameVersion = false, startPaused = false;
	core::stringc terrainFilename;
	if (loadSavedGame_Eff(readFile, sameGameVersion, startPaused, terrainFilename, true))
	{
		// Indique à l'utilisateur qu'une erreur s'est produite (avant le changement de scène, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Une erreur s'est produite lors du chargement de la partie envoyée !");

		// Retourne à la scène du menu pour rejoindre une partie multijoueur
		createMainMenu();
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		// Libère les données sur cette partie multijoueur, car elles ont été allouées avec new[]
		//readFile->drop();	// Désactivé : Déjà effectué automatiquement dans loadSavedGame_Eff
		delete[] gameSaveData;
		return true;
	}

	// Libère les données sur cette partie multijoueur, car elles ont été allouées avec new[]
	//readFile->drop();	// Désactivé : Déjà effectué automatiquement dans loadSavedGame_Eff
	delete[] gameSaveData;



	// Connecte RakNet à cet hôte (il n'est pas nécessaire de préciser l'adresse IP de l'hôte, car on y est déjà connecté avec l'appel à RakNetManager::queryGameData(hostIP))
	if (rkMgr.joinGame())
	{
		// Indique à l'utilisateur qu'une erreur s'est produite (avant le changement de scène, car ce dernier peut lui aussi envoyer des messages d'erreur, qui remplaceraient alors celui-ci !)
		showErrorMessageBox(L"Erreur", L"Impossible de se connecter à cet hôte !");

		// Retourne à la scène du menu pour rejoindre une partie multijoueur
		createMainMenu();
		switchToNextScene(ECS_MULTIPLAYER_MENU);

		return true;
	}



	// Passe à la scène du jeu
	switchToNextScene(ECS_PLAY);

	// Restaure l'état de la pause
	if (startPaused != isPaused)
		changePause();

#ifdef USE_IRRKLANG
	// Redémarre les sons d'IrrKlang si le mode audio est activé
	if (gameConfig.audioEnabled)
		ikMgr.setAllSoundsPaused(false);
#endif

	// Avertit l'utilisateur si la version du jeu de l'hôte est différente de la version actuelle du jeu
	if (!sameGameVersion)
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLa version du jeu de l'hôte est différente de votre version du jeu.\r\nDes erreurs pourraient se produire en cours de jeu !");
	// Sinon, avertit l'utilisateur si le terrain spécifié n'a pas été trouvé
	else if (!fileSystem->existFile(terrainFilename))
		showErrorMessageBox(L"Avertissement", L"Attention :\r\nLe terrain utilisé par l'hôte n'a pas été trouvé.\r\nDes erreurs pourraient se produire en cours de jeu !");

	LOG("Partie multijoueur chargée avec succès. Adresse de l'hôte : " << hostIP, ELL_INFORMATION);

	return false;
}
void Game::updateRakNetGameMessages()
{
	// Met tout d'abord à jour RakNet
	rkMgr.update();

	// Vérifie que des messages ont été reçus, sinon on quitte
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
			renderer->getSparkManager().setTimerSpeed(packet.gameSpeed);	// Indique à Spark que la vitesse du jeu a changé
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
			// Joue le son de la création d'un bâtiment acceptée
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_CREATE_POSITIVE);
#endif
			break;
		case RakNetManager::EGPT_BATIMENT_DESTROYED:
			system.destroyBatiment(core::vector2di(packet.gameDestroyInfos.indexX, packet.gameDestroyInfos.indexY), NULL, true);
#ifdef USE_IRRKLANG
			// Joue le son de la destruction d'un bâtiment acceptée
			ikMgr.playGameSound(IrrKlangManager::EGSE_BATIMENT_DESTROY_POSITIVE);
#endif
			break;
		case RakNetManager::EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED:
			{
				// Vérifie que l'index fourni est bien dans les limites du terrain
				if (packet.gameBatPPChangeInfos.indexX < 0 || packet.gameBatPPChangeInfos.indexY < 0
					|| packet.gameBatPPChangeInfos.indexX >= TAILLE_CARTE || packet.gameBatPPChangeInfos.indexY >= TAILLE_CARTE)
				{
					LOG_DEBUG("Game::updateRakNetGameMessages() : La modification du pourcentage de production du batiment est forcee, mais n'a pas pu etre effectue car la position specifiee se situe en-dehors des limites du terrain :" << endl
						<< "    indexX = " << packet.gameBatPPChangeInfos.indexX << "   ;   indexY = " << packet.gameBatPPChangeInfos.indexY, ELL_WARNING);
					LOG_RELEASE("AVERTISSEMENT : Modification forcée du pourcentage de production : Position en-dehors des limites du terrain !", ELL_WARNING);
				}
				else
				{
					Batiment* const bat = system.carte[packet.gameBatPPChangeInfos.indexX][packet.gameBatPPChangeInfos.indexY].batiment;
					if (!bat)
					{
						// Vérifie qu'il existe bien un batiment à cet emplacement, sinon on ne peut le détruire, et on retourne une erreur
						LOG_DEBUG("Game::updateRakNetGameMessages() : La modification du pourcentage de production du batiment est forcee, mais aucun batiment n'occupe cette position :" << endl
							<< "    indexX = " << packet.gameBatPPChangeInfos.indexX << "   ;   indexY = " << packet.gameBatPPChangeInfos.indexY
							<< "    system.carte[indexX][indexY].batiment = " << bat, ELL_WARNING);
						LOG_RELEASE("AVERTISSEMENT : Modification forcée du pourcentage de production : Aucun bâtiment n'occupe cette position !", ELL_WARNING);
					}
					else
						bat->getInfos().pourcentageProduction = packet.gameBatPPChangeInfos.newProductionPercentage;
				}
			}
			break;

			// Chargement de parties multijoueurs :
		case RakNetManager::EGPT_MULTIPLAYER_GAMES_LIST_CHANGED:
			// Met à jour le GUI Manager avec les parties en réseau trouvées
			guiManager->updateMultiplayerGamesTableNewGameGUI(rkMgr.multiplayerGames);
			break;

			// Paquets qui ne doivent normalement pas être gérés ici :
		case RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_RECEIVED:
			// Message non utilisé ici : on le libère de la mémoire
			if (packet.gameSavedData.data)
				delete[] packet.gameSavedData.data;
			LOG_DEBUG("Game::updateRakNetGameMessages() : Paquet EGPT_MULTIPLAYER_GAME_DATA_RECEIVED recu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;
		case RakNetManager::EGPT_MULTIPLAYER_GAME_DATA_FAILED:
			// Message d'échec vide : on ne fait rien
			LOG_DEBUG("Game::updateRakNetGameMessages() : Paquet EGPT_MULTIPLAYER_GAME_DATA_FAILED recu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;

			// Paquet inconnu :
		default:
			LOG_DEBUG("Game::updateRakNetGameMessages() : Le type d'un packet de jeu est inconnu : packet.packetType = " << packet.packetType, ELL_WARNING);
			break;
		}
	}

	// Supprime les messages de la liste des messages à traîter
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
	// Les valeurs des axes retournées par Irrlicht sont les valeurs "brutes" sorties du Joystick :
	// On évite une "zone morte" entre -1000 et 1000 pour filtrer les valeurs proches de 0
	if (value >= -1000 && value <= 1000)
		return 0.0f;



	// Normalise la valeur de l'axe retournée : évite les valeurs au-dessus de 1.0f et en dessous de -1.0f :

	// La valeur est comprise entre -32768 et 32767 : on la remet dans les limites de -32767 et 32767 pour pouvoir la normaliser par une simple division
	value = core::clamp<s16>(value, -32767, 32767);

	return ((float)value / 32767.0f);
}
#endif
