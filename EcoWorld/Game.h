#ifndef DEF_GAME
#define DEF_GAME

#include <boost/logic/tribool.hpp>	// Pour l'utilisation du type tribool
using namespace boost::logic;

#include "global.h"
#include "GameConfiguration.h"
#include "EcoWorldSystem.h"
#include "GUIManager.h"
#include "BatimentSelector.h"
#include "CGUIFileSelector.h"
#include "CBatimentSceneNode.h"
#include "XEffects/EffectHandler.h"
#ifdef USE_IRRKLANG
#include "IrrKlangManager.h"
#endif
#ifdef USE_RAKNET
#include "RakNetManager.h"
#endif

class EcoWorldRenderer;
class RTSCamera;
class WeatherManager;
class CShaderPreprocessor;
class CScreenQuad;
class CPostProcessManager;

// Structure pour stocker l'état de la souris (à l'extérieur de Game pour la caméra RTS)
struct MouseState
{
	float wheel;
	core::vector2di position;
	core::vector2df positionF;	// Position de la souris normalisée entre 0.0f et 1.0f par rapport à la taille de l'écran
	bool leftButton;
	bool middleButton;
	bool rightButton;

	MouseState() : wheel(0.0f), position(0, 0), positionF(0.0f, 0.0f), leftButton(false), middleButton(false), rightButton(false) { }
};

// Classe principale du jeu, complètement publique et accessible depuis toute classe du programme par le biais de GameState
class Game : public IEventReceiver
{
public:
	// Constructeur (initialise toutes les variables mais ne démarre en aucun cas le jeu : il devra être lancé par un appel à run())
	Game();

	// Lance la boucle principale du jeu
	// Le gameState sera utilisé pour connaître les paramêtres de démarrage du jeu, mais aussi pour indiquer des informations de sortie
	void run();

	// Capte automatiquement les évènements (gère les événements en dernier, passe le relais aux autres fonctions "OnEvent")
    virtual bool OnEvent(const SEvent& event);

	// Capte les évènements passés dans le logger d'Irrlicht pour les envoyer dans le flux de sortie standard cout
	bool OnEventLog(const SEvent& event);

	// Capte les évènements de la GUI (gère les événements en deuxième)
	// sendEventToIrrKlangMgr :	Valeur de retour permettant de déterminer si après cet évènement, cet évènement de la GUI doit être envoyé à l'IrrKlangManager (oui par défaut), ou s'il a été absorbé
    bool OnEventGUI(const SEvent& event
#ifdef USE_IRRKLANG
		, bool& outSendEventToIrrKlangMgr
#endif
		);

	// Capte les évènements du jeu (gère les événements en troisième)
    bool OnEventGame(const SEvent& event);

	// Capte les évènements lors de l'affichage d'un menu (gère les événements en premier)
	bool OnEventMenus(const SEvent& event);

	// Initialise le device et d'autres paramêtres importants (retourne true si l'initialisation a réussi, false sinon)
	bool init();

	// Ajoute toutes les archives du jeu au système de fichier du device
	void addGameArchives();

	// Applique les options de la configuration actuelle du jeu afin de créer les shaders PostProcess et XEffects nécessaires
	void applyGameConfigShadersOptions();

	// Applique les options de la configuration actuelle du jeu au material de remplacement du driver (pour les appliquer à tous les scene nodes) et modifie les paramêtres de création de textures du driver
	void applyGameConfigDriverOptions();

	// Met à jour toute la scène
	// Retourne true si la scène doit être dessinée, false sinon
	bool updateAll();

#ifdef USE_RAKNET
	// Met à jour RakNet et le jeu avec les messages reçus du réseau
	void updateRakNetGameMessages();
#endif

	EcoWorldSystem system;		// Le système de jeu
	EcoWorldRenderer* renderer; // Le scene node qui affichera le système de jeu

	WeatherID currentWeatherID;	// A n'utiliser que mettre à jour le titre du menu principal !

	/* Anciennes déclarations des fonctions placerBatiment :
	// Fonction servant à placer un batiment suivant la position de la souris
	// Si batimentID est différent de BI_aucun, alors la position du batiment et son type seront envoyés au système de jeu
	// Si batimentIndex est spécifié, la position de ce batiment sera prise suivant cet index, et non pas suivant la position actuelle de la souris
	//void placerBatiment(BatimentID batimentID, bool addToSystem, scene::ISceneNode* batiment = 0, core::vector2di batimentIndex = core::vector2di(-1, -1));
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	// Place des batiments suivant un rectangle de sélection (ils seront ajoutés au système de jeu)
	void placerBatimentRectSelection(BatimentID batimentID);
#endif
	*/
	// Le sélecteur de bâtiments, qui gère le placement et la sélection de bâtiments
	BatimentSelector batSelector;

#ifdef _DEBUG
	// Variable permettant de débloquer tous les bâtiments à l'aide d'une combinaison clavier en mode DEBUG
	bool unlockAllBatiments;
#endif

	// Les différentes scènes possibles du jeu
	// Attention : lors de l'ajout d'une nouvelle scène, ne pas oublier de mettre à jour la fonction Game::isInGameScene()
	enum E_CURRENT_SCENE
	{
		ECS_MAIN_MENU_LOADING,				// Chargement du menu principal
		ECS_MAIN_MENU,						// Menu principal
		ECS_OPTIONS_MENU,					// Menu options
		ECS_NEW_GAME_MENU,					// Menu pour créer une nouvelle partie (mode un joueur)
#ifdef USE_RAKNET
		ECS_MULTIPLAYER_MENU,				// Menu pour rejoindre une partie multijoueur
#endif
		ECS_LOADING_GAME,					// Chargement du jeu
		ECS_PLAY,							// Scène du jeu (mode un joueur)
		ECS_GAME_LOST,						// Ecran indiquant au joueur qu'il a perdu
		ECS_GAME_WON,						// Ecran indiquant au joueur qu'il a gagné

		ECS_COUNT
	} currentScene;

	// Remet tout à zéro (les paramêtres permettent de déterminer quelles options spécifiques doivent être remises à zéro)
	// resetGameAndSceneManager :	Remet à zéro le système de jeu et le scene manager (ainsi que le renderer)
	// keepCamera :					Seulement utilisé si resetGameAndSceneManager = true : permet de conserver la caméra RTS du jeu ainsi que son pointeur
	// resetGUI :					Supprime et recrée complètement la GUI (Désactivé : non utilisé)
	// stopIrrKlang :				Arrête tous les sons d'IrrKlang actuellement joués (mais ne les supprime pas)
	void resetGame(bool resetGameAndSceneManager, bool keepCamera, bool stopIrrKlang);

	// Le temps du device (en secondes) à l'heure de la dernière sauvegarde automatique
	u32 lastAutoSaveTime;

	// Crée une sauvegarde automatique du jeu
	void autoSave();

	// Enregistre la partie en cours dans un fichier
	// Retourne true si une erreur s'est produite, false sinon
	bool saveCurrentGame(const io::path& adresse);

	// Enregistre la partie en cours dans un fichier (mode "Efficace")
	// Ne crée pas automatiquement le write file nécessaire, n'affiche pas de message d'erreur
	// writeFile :			Le fichier dans lequel écrire les données
	// multiplayerSave :	Permet de spécifier si l'enregistrement est pour le mode multijoueur : dans ce cas seules les informations importantes seront enregistrées (ex : la GUI ne sera pas enregistrée)
	// Retourne true si une erreur s'est produite, false sinon
	bool saveCurrentGame_Eff(io::IWriteFile* writeFile
#ifdef USE_RAKNET
		, bool multiplayerSave = false
#endif
		);

	// Permet de passer à la scène suivante spécifiée dans newCurrentScene
	void switchToNextScene(E_CURRENT_SCENE newCurrentScene);

	// Dessine l'image de fond lors du chargement du menu principal
	void drawMainMenuLoadingBackground();

	// Crée la caméra RTS (ne devrait être appelé qu'une seule fois lors de la vie de cette classe)
	void createCamera();

	// Crée le menu principal (la GUI du menu principal doit avoir être crée auparavant)
	void createMainMenu();

	// Charge la scène complète du jeu et crée une nouvelle partie (indiquer le nom (avec l'extension) du terrain à charger dans le paramêtre terrainFilename)
	void loadGameData(const io::path& terrainFilename, bool startSoundsPaused = false);

	// Lance la création d'une nouvelle partie en chargeant le terrain spécifié
	// Retourne true si une erreur s'est produite, false sinon
	bool createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const io::path& terrainFilename
#ifdef USE_RAKNET
		, bool multiplayer = false
#endif
		);

	// Charge une partie sauvegardée depuis un fichier
	// Retourne true si une erreur s'est produite, false sinon
	bool loadSavedGame(const io::path& adresse);

	// Charge une partie sauvegardée depuis un fichier (mode "Efficace")
	// Ne crée pas automatiquement le read file nécessaire, n'affiche pas de message d'erreur, ne change pas de scène de jeu
	// -> Gère seulement la barre de chargement et le chargement efficace du fichier fourni
	// readFile :			Le fichier à charger, il sera automatiquement libéré par un appel à readFile->drop() pendant cette fonction
	// outSameGameVersion :	Détermine si la version de la sauvegarde est la même que le version actuelle du jeu
	// outStartPaused :		Détermine si le jeu doit démarrer en mode pause ou non
	// outTerrainFilename :	Détermine quel terrain à charger est demandé par le fichier
	// multiplayerLoad :	Permet de spécifier si le chargement est pour le mode multijoueur : dans ce cas seules les informations importantes seront chargées (ex : la GUI ne sera pas chargée)
	// Retourne true si une erreur s'est produite, false sinon
	// Note : le fichier à charger "readfile" sera automatiquement libéré par un appel à readFile->drop() pendant cette fonction, dès qu'il ne sera plus nécessaire
	bool loadSavedGame_Eff(io::IReadFile* readFile, bool& outSameGameVersion, bool& outStartPaused, core::stringc& outTerrainFilename
#ifdef USE_RAKNET
		, bool multiplayerLoad = false
#endif
		);

#ifdef USE_RAKNET
	// Rejoint une partie multijoueur en réseau avec l'adresse IP spécifiée
	// Retourne true si une erreur s'est produite, false sinon
	bool joinMultiplayerGame(const char* hostIP);
#endif

	// Affiche une boite de dialogue pour choisir un fichier, à sauvegarder ou à ouvrir, suivant la valeur de type
	void showFileDialog(CGUIFileSelector::E_FILESELECTOR_TYPE type);

	// Affiche la boîte de dialogue informant l'utilisateur qu'une erreur s'est produite (mise en pause automatique, traçage de la boîte de dialogue créée, fond modal texturé)
	void showErrorMessageBox(const wchar_t* caption, const wchar_t* text);

	bool hasChangedPauseBeforeShowingFileDialog;		// True si la pause a été changée avant d'afficher la boîte de dialogue pour enregistrer ou charger un fichier
	bool hasChangedPauseBeforeShowingErrorMessageBox;	// True si la pause a été changée avant d'afficher la boîte de dialogue indiquant qu'une erreur s'est produite

	// Change l'état actuel de la pause
	void changePause();

	bool onEchap(bool pressed);	// Fonction appelée lors d'un appui sur la touche Echap
	bool onEnter(bool pressed);	// Fonction appelée lors d'un appui sur la touche Entrée

	void createScreenShot();

	// Variables utilisées pour le calcul du temps écoulé :
	u32 lastDeviceTime, elapsedTimeMs,			// Le temps du timer écoulé depuis la dernière frame (en millisecondes)
		lastRealDeviceTime, realElapsedTimeMs;	// Le temps réel écoulé depuis la dernière frame (en millisecondes)
	float elapsedTime;							// Le temps du timer écoulé depuis la dernière frame (en secondes)

	void calculateElapsedTime();	// Permet de calculer le temps écoulé depuis la dernière frame

	void applyMenuWindowChoice();	// Applique le choix que le joueur a fait dans la fenêtre du menu du jeu qui s'affiche par un appui sur Echap

	void updateGameGUI();			// Actualise la GUI du jeu

	// Permet d'effectuer un rendu à l'écran
	// reinitScene :	Si true, un tout nouveau rendu sera recommencé (commencement d'une nouvelle scène du driver vidéo). Passer à false lorsqu'un rendu de la scène à l'intérieur d'un autre rendu doit être effectué.
	//					A noter que si reinitScene = false, les effets post-rendu ne seront pas appliqués à ce rendu final.
	void render(bool drawSceneManager = true, bool drawGUI = true, bool reinitScene = true);

	// Device, driver, sceneManager, timer et fileSystem
	IrrlichtDevice* device;
	video::IVideoDriver* driver;
	scene::ISceneManager* sceneManager;
	gui::IGUIEnvironment* gui;
	ITimer* deviceTimer;
	io::IFileSystem* fileSystem;

	// Le préprocesseur de shader du jeu, utilisé avant la compilation de chaque shader du jeu
	CShaderPreprocessor* shaderPreprocessor;

	// L'écran de rendu utilisé pour les rendus de PostProcess et de XEffects
	CScreenQuad* screenQuad;

	// Classe principale de PostProcess
	CPostProcessManager* postProcessManager;
	bool postProcessRender;
	u32 realTimeMsCameraStopShaking;	// Le temps réel en ms auquel l'effet de tremblement de la caméra devra être arrêté (pour concorder avec les sons lus par IrrKlang)

	// Classe principale de XEffects
	EffectHandler* xEffects;
	bool xEffectsRender;

	RTSCamera* cameraRTS;	// La camera RTS qui sera utilisée dans la phase de jeu
	bool lockCamera;		// True si la camera RTS doit être bloquée

	bool isPaused;			// True si le jeu est en pause

	bool showFPS;			// True si on affiche les FPS en bas de l'écran (différent des statistiques)

	bool keys[KEY_KEY_CODES_COUNT];	// Les touches du clavier, elles ont la valeur true si elles sont enfoncées

	MouseState mouseState;	// L'état actuel de la souris
	bool isMouseOnGUI;		// True si la souris est sur un élément de la GUI

#ifdef USE_JOYSTICKS
	// Les informations sur les joysticks actuellement connectés à la machine (on stocke ici toutes les informations, mais seul le premier joystick trouvé est utilisé)
	core::array<SJoystickInfo> joysticksInfos;
	core::array<SEvent::SJoystickEvent> joysticksState;

	// Retourne la valeur normalisée de position de l'axe d'un joystick (la valeur retournée est normalement entre -1.0f et 1.0f)
	float getNormalizedJostickAxis(u32 axis, u32 joystick);
	static float getNormalizedJostickAxis(s16 value);
#endif

	// Calcule si la souris est sur un élément de la GUI
	// parent : élément parent à partir duquel vérifier si la souris est sur cet élément ou sur un de ses enfants
	// Retourne true si la souris est sur un élément de la GUI, false sinon
	bool calculerIsMouseOnGUI(gui::IGUIElement* parent = 0);

	// Le GUI Manager
	GUIManager* guiManager;

	// True si les évènements du clavier peuvent êter gérés (représente principalement si la fenêtre de notes de la GUI du jeu a le focus : dans ce cas, elle seule doit gérer les évènements du clavier => canHandleKeysEvent = false)
	bool canHandleKeysEvent;

	// Gestion du menu inférieur de la GUI du jeu :

	// La vitesse de descente ou de montée du menu inférieur de la GUI du jeu (en pourcentage de l'écran par milliseconde réelle (!= des millisecondes de jeu, qui sont dépendantes de la vitesse du timer)) :
	// = 0 : Le menu inférieur reste à sa place actuelle
	// > 0 : Le menu inférieur descend
	// < 0 : Le menu inférieur remonte
	float vitesseDescenteMenuInferieur;

	// Le temps réel à partir duquel la descente du menu inférieur doit commencer
	u32 realTimeMsStartDescenteMenuInferieur;

	// Booléen déterminant si la souris est sur le menu inférieur (permet de savoir si elle vient de le quitter)
	bool isMouseOnMenuInferieur;

	// La position de départ en Y du menu inférieur lorsque sa descente a commencée
	int menuInferieurStartPosY;

	// Détermine si le menu inférieur est en haut :
	// true :			Le menu inférieur est en haut
	// false :			Le menu inférieur est en bas
	// indeterminate :	Impossible de déterminer la position du menu inférieur : il est sûrement en train de se déplacer (vitesseDescenteMenuInferieur != 0.0f)
	tribool isMenuInferieurHaut() const
	{
		if (guiManager && guiManager->guiElements.gameGUI.tabGroupBas)
		{
			const int menuPosY = guiManager->guiElements.gameGUI.tabGroupBas->getRelativePosition().UpperLeftCorner.Y;

			const int minPosY = guiManager->guiElements.gameGUI.tabGroupBas->getParent()->getRelativePosition().LowerRightCorner.Y
				- guiManager->guiElements.gameGUI.tabGroupBas->getTabHeight();
			if (menuPosY >= minPosY)
				return false;
			else
			{
				const int maxPosY = core::floor32(0.8f * (float)driver->getScreenSize().Height);
				if (menuPosY <= maxPosY)
					return true;
			}
		}

		return indeterminate;
	}

	// Gestion du niveau gamma du device :

	// True si on a modifié le niveau gamma du device actuel, false sinon
	bool hasChangedGammaLevel;

	// Le niveau gamma par défaut lors de la création du device
	GammaInfos defaultGammaInfos;

	// Obtient le niveau gamma par défaut du device (doit seulement être appelé juste après la création du device !)
	void getDefaultGammaRamp()
	{
		// Récupère les valeurs par défaut du niveau gamma
		if (device)
			device->getGammaRamp(defaultGammaInfos.red, defaultGammaInfos.green, defaultGammaInfos.blue, defaultGammaInfos.brightness, defaultGammaInfos.contrast);
	}
	// Indique le nouveau niveau gamma du device d'après des informations de gamma
	void setNewGammaRamp(const GammaInfos& newGammaInfos)
	{
		if (device)
		{
			if (hasChangedGammaLevel)
			{
				// Si le niveau gamma du device a changé, on compare les nouvelles valeurs gamma avec les valeurs actuelles obtenues d'après le device
				GammaInfos currentGammaInfos;
				device->getGammaRamp(currentGammaInfos.red, currentGammaInfos.green, currentGammaInfos.blue, currentGammaInfos.brightness, currentGammaInfos.contrast);

				if (currentGammaInfos != newGammaInfos)	// Détermine si le niveau gamma est bien différent avant de le changer
				{
					// Si la valeur gamma demandé est différente, on indique au device les nouvelles valeurs du niveau gamma
					device->setGammaRamp(newGammaInfos.red, newGammaInfos.green, newGammaInfos.blue, newGammaInfos.brightness, newGammaInfos.contrast);

					LOG("Niveau gamma modifié", ELL_INFORMATION);
				}
			}
			// Si le niveau gamma du device n'a pas encore changé, on compare les nouvelles valeurs gamma avec les valeurs par défaut qui ont été obtenues à la création du device
			else if (defaultGammaInfos != newGammaInfos)	// Détermine si le niveau gamma est bien différent avant de le changer
			{
				// Si la valeur gamma demandé est différente, on indique au device les nouvelles valeurs du niveau gamma et on indique que le niveau gamma a changé
				device->setGammaRamp(newGammaInfos.red, newGammaInfos.green, newGammaInfos.blue, newGammaInfos.brightness, newGammaInfos.contrast);
				hasChangedGammaLevel = true;

				LOG("Niveau gamma modifié", ELL_INFORMATION);
			}
		}
	}
	// Réinitialise la valeur gamma du device avec ses valeurs par défaut si elle a été changée (doit être fait au moins à la fermeture du device, car si le niveau gamma n'est pas rétabli, les modifications du niveau gamma seront permanentes après la fermeture du jeu !)
	void resetGammaRamp()
	{
		if (hasChangedGammaLevel && device)
		{
			// Rétablit le niveau gamma par défaut
			device->setGammaRamp(defaultGammaInfos.red, defaultGammaInfos.green, defaultGammaInfos.blue, defaultGammaInfos.brightness, defaultGammaInfos.contrast);
			hasChangedGammaLevel = false;

			LOG("Niveau gamma rétabli", ELL_INFORMATION);
		}
	}



	// Fonctions inline :

	// Retourne true si la partie est dans une scène du jeu, false sinon (si on est dans une scène du menu)
	bool isInGameScene()
	{
		return (currentScene >= ECS_PLAY);
	}



	// Change la couleur apparente d'un bâtiment
	static void changeBatimentColor(scene::ISceneNode* batiment, const video::SColor& color)
	{
		if (!batiment)
			return;

		// Désactive le brouillard pour le batiment
		batiment->setMaterialFlag(video::EMF_FOG_ENABLE, false);

		// Change la couleur de la lumière ambiante sur le bâtiment
		const u32 matCount = batiment->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
			batiment->getMaterial(i).AmbientColor = color;

		// Parcours tous les enfants de ce node
		const core::list<scene::ISceneNode*>& children = batiment->getChildren();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it)
		{
			// Change aussi la couleur apparente des autres enfants de ce bâtiment
			changeBatimentColor((*it), color);
		}
	}
	// Restaure la couleur apparente d'un bâtiment et masque son texte au-dessus de lui
	static void resetBatimentColor(scene::ISceneNode* batiment)
	{
		if (!batiment)
			return;

		// Réactive le brouillard pour le batiment
		batiment->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		// Restaure la couleur de la lumière ambiante sur le bâtiment : ce doit être la même que sa couleur diffuse pour des raisons de cohérence des lumières
		const u32 matCount = batiment->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
			batiment->getMaterial(i).AmbientColor = batiment->getMaterial(i).DiffuseColor;

		// Parcours tous les enfants de ce node
		const core::list<scene::ISceneNode*>& children = batiment->getChildren();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it)
		{
			// Restaure aussi la couleur apparente des autres enfants de ce bâtiment
			resetBatimentColor(*it);
		}
	}



	// Applique un material flag à un scene node et à tous ses enfants
	static void applyMaterialFlag(scene::ISceneNode* node, video::E_MATERIAL_FLAG materialFlag, bool newValue = true)
	{
		if (!node)
			return;

		// Applique le material flag à ce scene node
		node->setMaterialFlag(materialFlag, newValue);

		// Puis à chacun de ses enfants
		const core::list<scene::ISceneNode*>& children = node->getChildren();
		if (children.empty())
			return;
		const core::list<scene::ISceneNode*>::ConstIterator END = children.end();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != END; ++it)
			applyMaterialFlag((*it), materialFlag, newValue);
	}



	// Ajoute à la suite d'un core::stringw un nombre de jours sous un format plus commode à la lecture
	// Exemple : au lieu d'afficher "13787 jours", cette fonction permet d'afficher "38 années 3 mois 17 jours"
	// Cette fonction prend aussi en compte les problèmes de pluriel dans les dates ("1 année" et "1 jour")
	// Si showZeroData = true, un affichage du type "0 années 3 mois 0 jours" sera autorisé, sinon il sera remplacé par "3 mois"
	// Note : si totalJours = 0 et showZeroData = true, alors la chaîne fournie se verra tout de même ajouter "0 jours"
	static void appendDays(core::stringw& str, u32 totalJours, bool showZeroData = false)
	{
		// Calcule les décompositions du temps
		const u32 jours = totalJours % 30;
		const u32 totalMois = totalJours / 30;
		const u32 mois = totalMois % 12;
		const u32 annees = totalMois / 12;

		// Indique quelles valeurs seront affichées
		const bool showAnnees = (annees != 0 || showZeroData);
		const bool showMois = (mois != 0 || showZeroData);
		const bool showJours = (jours != 0 || showZeroData || totalJours == 0);

		// Affiche les années
		if (showAnnees)
		{
			if (annees == 1)
				str.append(L"1 année");
			else
			{
				swprintf_SS(L"%u années", annees);	// %u : Unsigned decimal integer
				str.append(textW_SS);
			}

			// Ajoute une transition (un simple espace) entre années et mois ou entre années et jours
			if (showMois || showJours)
				str.append(L' ');
		}

		// Affiche les mois
		if (showMois)
		{
			swprintf_SS(L"%u mois", mois);			// %u : Unsigned decimal integer
			str.append(textW_SS);

			// Ajoute une transition (un simple espace) entre mois et jours
			if (showJours)
				str.append(L' ');
		}

		// Affiche les jours
		if (showJours)
		{
			if (jours == 1)
				str.append(L"1 jour");
			else
			{
				swprintf_SS(L"%u jours", jours);		// %u : Unsigned decimal integer
				str.append(textW_SS);
			}
		}
	}



	// Ajoute à la suite d'un core::stringw une date sous un format plus commode à la lecture
	// Exemple : au lieu d'afficher "13787 jours", cette fonction permet d'afficher "17 mars 2038"
	// La date correspondant à totalJours = 0 est : "1 janvier 2000"
	static void appendDate(core::stringw& str, u32 totalJours)
	{
		// Calcule les décompositions du temps
		const u32 jours = (totalJours % 30) + 1;
		const u32 totalMois = (totalJours / 30);
		const u32 mois = (totalMois % 12);
		const u32 annees = (totalMois / 12) + 2000;

		// Ajoute les jours
		swprintf_SS(L"%u ", jours);		// %u : Unsigned decimal integer
		str.append(textW_SS);

		// Ajoute les mois
		switch (mois)
		{
		case 0:		str.append(L"janvier");		break;
		case 1:		str.append(L"février");		break;
		case 2:		str.append(L"mars");		break;
		case 3:		str.append(L"avril");		break;
		case 4:		str.append(L"mai");			break;
		case 5:		str.append(L"juin");		break;
		case 6:		str.append(L"juillet");		break;
		case 7:		str.append(L"août");		break;
		case 8:		str.append(L"septembre");	break;
		case 9:		str.append(L"octobre");		break;
		case 10:	str.append(L"novembre");	break;
		case 11:	str.append(L"décembre");	break;
		default:	str.append(L"MOIS");		break;
		}

		// Ajoute les années
		swprintf_SS(L" %u", annees);		// %u : Unsigned decimal integer
		str.append(textW_SS);
	}



	// Retourne un ISceneNode* converti en un CBatimentSceneNode*
	static CBatimentSceneNode* getBatimentNode(scene::ISceneNode* node)
	{
		CBatimentSceneNode* batNode = NULL;
		if (node && node->getType() == ESNT_BATIMENT_NODE)	// Vérifie que le type de ce node est bien un node de bâtiment
			batNode = reinterpret_cast<CBatimentSceneNode*>(node);

		return batNode;
	}
};

#endif
