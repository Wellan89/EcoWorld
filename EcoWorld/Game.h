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

// Structure pour stocker l'�tat de la souris (� l'ext�rieur de Game pour la cam�ra RTS)
struct MouseState
{
	float wheel;
	core::vector2di position;
	core::vector2df positionF;	// Position de la souris normalis�e entre 0.0f et 1.0f par rapport � la taille de l'�cran
	bool leftButton;
	bool middleButton;
	bool rightButton;

	MouseState() : wheel(0.0f), position(0, 0), positionF(0.0f, 0.0f), leftButton(false), middleButton(false), rightButton(false) { }
};

// Classe principale du jeu, compl�tement publique et accessible depuis toute classe du programme par le biais de GameState
class Game : public IEventReceiver
{
public:
	// Constructeur (initialise toutes les variables mais ne d�marre en aucun cas le jeu : il devra �tre lanc� par un appel � run())
	Game();

	// Lance la boucle principale du jeu
	// Le gameState sera utilis� pour conna�tre les param�tres de d�marrage du jeu, mais aussi pour indiquer des informations de sortie
	void run();

	// Capte automatiquement les �v�nements (g�re les �v�nements en dernier, passe le relais aux autres fonctions "OnEvent")
    virtual bool OnEvent(const SEvent& event);

	// Capte les �v�nements pass�s dans le logger d'Irrlicht pour les envoyer dans le flux de sortie standard cout
	bool OnEventLog(const SEvent& event);

	// Capte les �v�nements de la GUI (g�re les �v�nements en deuxi�me)
	// sendEventToIrrKlangMgr :	Valeur de retour permettant de d�terminer si apr�s cet �v�nement, cet �v�nement de la GUI doit �tre envoy� � l'IrrKlangManager (oui par d�faut), ou s'il a �t� absorb�
    bool OnEventGUI(const SEvent& event
#ifdef USE_IRRKLANG
		, bool& outSendEventToIrrKlangMgr
#endif
		);

	// Capte les �v�nements du jeu (g�re les �v�nements en troisi�me)
    bool OnEventGame(const SEvent& event);

	// Capte les �v�nements lors de l'affichage d'un menu (g�re les �v�nements en premier)
	bool OnEventMenus(const SEvent& event);

	// Initialise le device et d'autres param�tres importants (retourne true si l'initialisation a r�ussi, false sinon)
	bool init();

	// Ajoute toutes les archives du jeu au syst�me de fichier du device
	void addGameArchives();

	// Applique les options de la configuration actuelle du jeu afin de cr�er les shaders PostProcess et XEffects n�cessaires
	void applyGameConfigShadersOptions();

	// Applique les options de la configuration actuelle du jeu au material de remplacement du driver (pour les appliquer � tous les scene nodes) et modifie les param�tres de cr�ation de textures du driver
	void applyGameConfigDriverOptions();

	// Met � jour toute la sc�ne
	// Retourne true si la sc�ne doit �tre dessin�e, false sinon
	bool updateAll();

#ifdef USE_RAKNET
	// Met � jour RakNet et le jeu avec les messages re�us du r�seau
	void updateRakNetGameMessages();
#endif

	EcoWorldSystem system;		// Le syst�me de jeu
	EcoWorldRenderer* renderer; // Le scene node qui affichera le syst�me de jeu

	WeatherID currentWeatherID;	// A n'utiliser que mettre � jour le titre du menu principal !

	/* Anciennes d�clarations des fonctions placerBatiment :
	// Fonction servant � placer un batiment suivant la position de la souris
	// Si batimentID est diff�rent de BI_aucun, alors la position du batiment et son type seront envoy�s au syst�me de jeu
	// Si batimentIndex est sp�cifi�, la position de ce batiment sera prise suivant cet index, et non pas suivant la position actuelle de la souris
	//void placerBatiment(BatimentID batimentID, bool addToSystem, scene::ISceneNode* batiment = 0, core::vector2di batimentIndex = core::vector2di(-1, -1));
#ifdef CAN_BUILD_WITH_SELECTION_RECT
	// Place des batiments suivant un rectangle de s�lection (ils seront ajout�s au syst�me de jeu)
	void placerBatimentRectSelection(BatimentID batimentID);
#endif
	*/
	// Le s�lecteur de b�timents, qui g�re le placement et la s�lection de b�timents
	BatimentSelector batSelector;

#ifdef _DEBUG
	// Variable permettant de d�bloquer tous les b�timents � l'aide d'une combinaison clavier en mode DEBUG
	bool unlockAllBatiments;
#endif

	// Les diff�rentes sc�nes possibles du jeu
	// Attention : lors de l'ajout d'une nouvelle sc�ne, ne pas oublier de mettre � jour la fonction Game::isInGameScene()
	enum E_CURRENT_SCENE
	{
		ECS_MAIN_MENU_LOADING,				// Chargement du menu principal
		ECS_MAIN_MENU,						// Menu principal
		ECS_OPTIONS_MENU,					// Menu options
		ECS_NEW_GAME_MENU,					// Menu pour cr�er une nouvelle partie (mode un joueur)
#ifdef USE_RAKNET
		ECS_MULTIPLAYER_MENU,				// Menu pour rejoindre une partie multijoueur
#endif
		ECS_LOADING_GAME,					// Chargement du jeu
		ECS_PLAY,							// Sc�ne du jeu (mode un joueur)
		ECS_GAME_LOST,						// Ecran indiquant au joueur qu'il a perdu
		ECS_GAME_WON,						// Ecran indiquant au joueur qu'il a gagn�

		ECS_COUNT
	} currentScene;

	// Remet tout � z�ro (les param�tres permettent de d�terminer quelles options sp�cifiques doivent �tre remises � z�ro)
	// resetGameAndSceneManager :	Remet � z�ro le syst�me de jeu et le scene manager (ainsi que le renderer)
	// keepCamera :					Seulement utilis� si resetGameAndSceneManager = true : permet de conserver la cam�ra RTS du jeu ainsi que son pointeur
	// resetGUI :					Supprime et recr�e compl�tement la GUI (D�sactiv� : non utilis�)
	// stopIrrKlang :				Arr�te tous les sons d'IrrKlang actuellement jou�s (mais ne les supprime pas)
	void resetGame(bool resetGameAndSceneManager, bool keepCamera, bool stopIrrKlang);

	// Le temps du device (en secondes) � l'heure de la derni�re sauvegarde automatique
	u32 lastAutoSaveTime;

	// Cr�e une sauvegarde automatique du jeu
	void autoSave();

	// Enregistre la partie en cours dans un fichier
	// Retourne true si une erreur s'est produite, false sinon
	bool saveCurrentGame(const io::path& adresse);

	// Enregistre la partie en cours dans un fichier (mode "Efficace")
	// Ne cr�e pas automatiquement le write file n�cessaire, n'affiche pas de message d'erreur
	// writeFile :			Le fichier dans lequel �crire les donn�es
	// multiplayerSave :	Permet de sp�cifier si l'enregistrement est pour le mode multijoueur : dans ce cas seules les informations importantes seront enregistr�es (ex : la GUI ne sera pas enregistr�e)
	// Retourne true si une erreur s'est produite, false sinon
	bool saveCurrentGame_Eff(io::IWriteFile* writeFile
#ifdef USE_RAKNET
		, bool multiplayerSave = false
#endif
		);

	// Permet de passer � la sc�ne suivante sp�cifi�e dans newCurrentScene
	void switchToNextScene(E_CURRENT_SCENE newCurrentScene);

	// Dessine l'image de fond lors du chargement du menu principal
	void drawMainMenuLoadingBackground();

	// Cr�e la cam�ra RTS (ne devrait �tre appel� qu'une seule fois lors de la vie de cette classe)
	void createCamera();

	// Cr�e le menu principal (la GUI du menu principal doit avoir �tre cr�e auparavant)
	void createMainMenu();

	// Charge la sc�ne compl�te du jeu et cr�e une nouvelle partie (indiquer le nom (avec l'extension) du terrain � charger dans le param�tre terrainFilename)
	void loadGameData(const io::path& terrainFilename, bool startSoundsPaused = false);

	// Lance la cr�ation d'une nouvelle partie en chargeant le terrain sp�cifi�
	// Retourne true si une erreur s'est produite, false sinon
	bool createNewGame(EcoWorldModifiers::E_DIFFICULTY difficulty, const io::path& terrainFilename
#ifdef USE_RAKNET
		, bool multiplayer = false
#endif
		);

	// Charge une partie sauvegard�e depuis un fichier
	// Retourne true si une erreur s'est produite, false sinon
	bool loadSavedGame(const io::path& adresse);

	// Charge une partie sauvegard�e depuis un fichier (mode "Efficace")
	// Ne cr�e pas automatiquement le read file n�cessaire, n'affiche pas de message d'erreur, ne change pas de sc�ne de jeu
	// -> G�re seulement la barre de chargement et le chargement efficace du fichier fourni
	// readFile :			Le fichier � charger, il sera automatiquement lib�r� par un appel � readFile->drop() pendant cette fonction
	// outSameGameVersion :	D�termine si la version de la sauvegarde est la m�me que le version actuelle du jeu
	// outStartPaused :		D�termine si le jeu doit d�marrer en mode pause ou non
	// outTerrainFilename :	D�termine quel terrain � charger est demand� par le fichier
	// multiplayerLoad :	Permet de sp�cifier si le chargement est pour le mode multijoueur : dans ce cas seules les informations importantes seront charg�es (ex : la GUI ne sera pas charg�e)
	// Retourne true si une erreur s'est produite, false sinon
	// Note : le fichier � charger "readfile" sera automatiquement lib�r� par un appel � readFile->drop() pendant cette fonction, d�s qu'il ne sera plus n�cessaire
	bool loadSavedGame_Eff(io::IReadFile* readFile, bool& outSameGameVersion, bool& outStartPaused, core::stringc& outTerrainFilename
#ifdef USE_RAKNET
		, bool multiplayerLoad = false
#endif
		);

#ifdef USE_RAKNET
	// Rejoint une partie multijoueur en r�seau avec l'adresse IP sp�cifi�e
	// Retourne true si une erreur s'est produite, false sinon
	bool joinMultiplayerGame(const char* hostIP);
#endif

	// Affiche une boite de dialogue pour choisir un fichier, � sauvegarder ou � ouvrir, suivant la valeur de type
	void showFileDialog(CGUIFileSelector::E_FILESELECTOR_TYPE type);

	// Affiche la bo�te de dialogue informant l'utilisateur qu'une erreur s'est produite (mise en pause automatique, tra�age de la bo�te de dialogue cr��e, fond modal textur�)
	void showErrorMessageBox(const wchar_t* caption, const wchar_t* text);

	bool hasChangedPauseBeforeShowingFileDialog;		// True si la pause a �t� chang�e avant d'afficher la bo�te de dialogue pour enregistrer ou charger un fichier
	bool hasChangedPauseBeforeShowingErrorMessageBox;	// True si la pause a �t� chang�e avant d'afficher la bo�te de dialogue indiquant qu'une erreur s'est produite

	// Change l'�tat actuel de la pause
	void changePause();

	bool onEchap(bool pressed);	// Fonction appel�e lors d'un appui sur la touche Echap
	bool onEnter(bool pressed);	// Fonction appel�e lors d'un appui sur la touche Entr�e

	void createScreenShot();

	// Variables utilis�es pour le calcul du temps �coul� :
	u32 lastDeviceTime, elapsedTimeMs,			// Le temps du timer �coul� depuis la derni�re frame (en millisecondes)
		lastRealDeviceTime, realElapsedTimeMs;	// Le temps r�el �coul� depuis la derni�re frame (en millisecondes)
	float elapsedTime;							// Le temps du timer �coul� depuis la derni�re frame (en secondes)

	void calculateElapsedTime();	// Permet de calculer le temps �coul� depuis la derni�re frame

	void applyMenuWindowChoice();	// Applique le choix que le joueur a fait dans la fen�tre du menu du jeu qui s'affiche par un appui sur Echap

	void updateGameGUI();			// Actualise la GUI du jeu

	// Permet d'effectuer un rendu � l'�cran
	// reinitScene :	Si true, un tout nouveau rendu sera recommenc� (commencement d'une nouvelle sc�ne du driver vid�o). Passer � false lorsqu'un rendu de la sc�ne � l'int�rieur d'un autre rendu doit �tre effectu�.
	//					A noter que si reinitScene = false, les effets post-rendu ne seront pas appliqu�s � ce rendu final.
	void render(bool drawSceneManager = true, bool drawGUI = true, bool reinitScene = true);

	// Device, driver, sceneManager, timer et fileSystem
	IrrlichtDevice* device;
	video::IVideoDriver* driver;
	scene::ISceneManager* sceneManager;
	gui::IGUIEnvironment* gui;
	ITimer* deviceTimer;
	io::IFileSystem* fileSystem;

	// Le pr�processeur de shader du jeu, utilis� avant la compilation de chaque shader du jeu
	CShaderPreprocessor* shaderPreprocessor;

	// L'�cran de rendu utilis� pour les rendus de PostProcess et de XEffects
	CScreenQuad* screenQuad;

	// Classe principale de PostProcess
	CPostProcessManager* postProcessManager;
	bool postProcessRender;
	u32 realTimeMsCameraStopShaking;	// Le temps r�el en ms auquel l'effet de tremblement de la cam�ra devra �tre arr�t� (pour concorder avec les sons lus par IrrKlang)

	// Classe principale de XEffects
	EffectHandler* xEffects;
	bool xEffectsRender;

	RTSCamera* cameraRTS;	// La camera RTS qui sera utilis�e dans la phase de jeu
	bool lockCamera;		// True si la camera RTS doit �tre bloqu�e

	bool isPaused;			// True si le jeu est en pause

	bool showFPS;			// True si on affiche les FPS en bas de l'�cran (diff�rent des statistiques)

	bool keys[KEY_KEY_CODES_COUNT];	// Les touches du clavier, elles ont la valeur true si elles sont enfonc�es

	MouseState mouseState;	// L'�tat actuel de la souris
	bool isMouseOnGUI;		// True si la souris est sur un �l�ment de la GUI

#ifdef USE_JOYSTICKS
	// Les informations sur les joysticks actuellement connect�s � la machine (on stocke ici toutes les informations, mais seul le premier joystick trouv� est utilis�)
	core::array<SJoystickInfo> joysticksInfos;
	core::array<SEvent::SJoystickEvent> joysticksState;

	// Retourne la valeur normalis�e de position de l'axe d'un joystick (la valeur retourn�e est normalement entre -1.0f et 1.0f)
	float getNormalizedJostickAxis(u32 axis, u32 joystick);
	static float getNormalizedJostickAxis(s16 value);
#endif

	// Calcule si la souris est sur un �l�ment de la GUI
	// parent : �l�ment parent � partir duquel v�rifier si la souris est sur cet �l�ment ou sur un de ses enfants
	// Retourne true si la souris est sur un �l�ment de la GUI, false sinon
	bool calculerIsMouseOnGUI(gui::IGUIElement* parent = 0);

	// Le GUI Manager
	GUIManager* guiManager;

	// True si les �v�nements du clavier peuvent �ter g�r�s (repr�sente principalement si la fen�tre de notes de la GUI du jeu a le focus : dans ce cas, elle seule doit g�rer les �v�nements du clavier => canHandleKeysEvent = false)
	bool canHandleKeysEvent;

	// Gestion du menu inf�rieur de la GUI du jeu :

	// La vitesse de descente ou de mont�e du menu inf�rieur de la GUI du jeu (en pourcentage de l'�cran par milliseconde r�elle (!= des millisecondes de jeu, qui sont d�pendantes de la vitesse du timer)) :
	// = 0 : Le menu inf�rieur reste � sa place actuelle
	// > 0 : Le menu inf�rieur descend
	// < 0 : Le menu inf�rieur remonte
	float vitesseDescenteMenuInferieur;

	// Le temps r�el � partir duquel la descente du menu inf�rieur doit commencer
	u32 realTimeMsStartDescenteMenuInferieur;

	// Bool�en d�terminant si la souris est sur le menu inf�rieur (permet de savoir si elle vient de le quitter)
	bool isMouseOnMenuInferieur;

	// La position de d�part en Y du menu inf�rieur lorsque sa descente a commenc�e
	int menuInferieurStartPosY;

	// D�termine si le menu inf�rieur est en haut :
	// true :			Le menu inf�rieur est en haut
	// false :			Le menu inf�rieur est en bas
	// indeterminate :	Impossible de d�terminer la position du menu inf�rieur : il est s�rement en train de se d�placer (vitesseDescenteMenuInferieur != 0.0f)
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

	// True si on a modifi� le niveau gamma du device actuel, false sinon
	bool hasChangedGammaLevel;

	// Le niveau gamma par d�faut lors de la cr�ation du device
	GammaInfos defaultGammaInfos;

	// Obtient le niveau gamma par d�faut du device (doit seulement �tre appel� juste apr�s la cr�ation du device !)
	void getDefaultGammaRamp()
	{
		// R�cup�re les valeurs par d�faut du niveau gamma
		if (device)
			device->getGammaRamp(defaultGammaInfos.red, defaultGammaInfos.green, defaultGammaInfos.blue, defaultGammaInfos.brightness, defaultGammaInfos.contrast);
	}
	// Indique le nouveau niveau gamma du device d'apr�s des informations de gamma
	void setNewGammaRamp(const GammaInfos& newGammaInfos)
	{
		if (device)
		{
			if (hasChangedGammaLevel)
			{
				// Si le niveau gamma du device a chang�, on compare les nouvelles valeurs gamma avec les valeurs actuelles obtenues d'apr�s le device
				GammaInfos currentGammaInfos;
				device->getGammaRamp(currentGammaInfos.red, currentGammaInfos.green, currentGammaInfos.blue, currentGammaInfos.brightness, currentGammaInfos.contrast);

				if (currentGammaInfos != newGammaInfos)	// D�termine si le niveau gamma est bien diff�rent avant de le changer
				{
					// Si la valeur gamma demand� est diff�rente, on indique au device les nouvelles valeurs du niveau gamma
					device->setGammaRamp(newGammaInfos.red, newGammaInfos.green, newGammaInfos.blue, newGammaInfos.brightness, newGammaInfos.contrast);

					LOG("Niveau gamma modifi�", ELL_INFORMATION);
				}
			}
			// Si le niveau gamma du device n'a pas encore chang�, on compare les nouvelles valeurs gamma avec les valeurs par d�faut qui ont �t� obtenues � la cr�ation du device
			else if (defaultGammaInfos != newGammaInfos)	// D�termine si le niveau gamma est bien diff�rent avant de le changer
			{
				// Si la valeur gamma demand� est diff�rente, on indique au device les nouvelles valeurs du niveau gamma et on indique que le niveau gamma a chang�
				device->setGammaRamp(newGammaInfos.red, newGammaInfos.green, newGammaInfos.blue, newGammaInfos.brightness, newGammaInfos.contrast);
				hasChangedGammaLevel = true;

				LOG("Niveau gamma modifi�", ELL_INFORMATION);
			}
		}
	}
	// R�initialise la valeur gamma du device avec ses valeurs par d�faut si elle a �t� chang�e (doit �tre fait au moins � la fermeture du device, car si le niveau gamma n'est pas r�tabli, les modifications du niveau gamma seront permanentes apr�s la fermeture du jeu !)
	void resetGammaRamp()
	{
		if (hasChangedGammaLevel && device)
		{
			// R�tablit le niveau gamma par d�faut
			device->setGammaRamp(defaultGammaInfos.red, defaultGammaInfos.green, defaultGammaInfos.blue, defaultGammaInfos.brightness, defaultGammaInfos.contrast);
			hasChangedGammaLevel = false;

			LOG("Niveau gamma r�tabli", ELL_INFORMATION);
		}
	}



	// Fonctions inline :

	// Retourne true si la partie est dans une sc�ne du jeu, false sinon (si on est dans une sc�ne du menu)
	bool isInGameScene()
	{
		return (currentScene >= ECS_PLAY);
	}



	// Change la couleur apparente d'un b�timent
	static void changeBatimentColor(scene::ISceneNode* batiment, const video::SColor& color)
	{
		if (!batiment)
			return;

		// D�sactive le brouillard pour le batiment
		batiment->setMaterialFlag(video::EMF_FOG_ENABLE, false);

		// Change la couleur de la lumi�re ambiante sur le b�timent
		const u32 matCount = batiment->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
			batiment->getMaterial(i).AmbientColor = color;

		// Parcours tous les enfants de ce node
		const core::list<scene::ISceneNode*>& children = batiment->getChildren();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it)
		{
			// Change aussi la couleur apparente des autres enfants de ce b�timent
			changeBatimentColor((*it), color);
		}
	}
	// Restaure la couleur apparente d'un b�timent et masque son texte au-dessus de lui
	static void resetBatimentColor(scene::ISceneNode* batiment)
	{
		if (!batiment)
			return;

		// R�active le brouillard pour le batiment
		batiment->setMaterialFlag(video::EMF_FOG_ENABLE, true);

		// Restaure la couleur de la lumi�re ambiante sur le b�timent : ce doit �tre la m�me que sa couleur diffuse pour des raisons de coh�rence des lumi�res
		const u32 matCount = batiment->getMaterialCount();
		for (u32 i = 0; i < matCount; ++i)
			batiment->getMaterial(i).AmbientColor = batiment->getMaterial(i).DiffuseColor;

		// Parcours tous les enfants de ce node
		const core::list<scene::ISceneNode*>& children = batiment->getChildren();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it)
		{
			// Restaure aussi la couleur apparente des autres enfants de ce b�timent
			resetBatimentColor(*it);
		}
	}



	// Applique un material flag � un scene node et � tous ses enfants
	static void applyMaterialFlag(scene::ISceneNode* node, video::E_MATERIAL_FLAG materialFlag, bool newValue = true)
	{
		if (!node)
			return;

		// Applique le material flag � ce scene node
		node->setMaterialFlag(materialFlag, newValue);

		// Puis � chacun de ses enfants
		const core::list<scene::ISceneNode*>& children = node->getChildren();
		if (children.empty())
			return;
		const core::list<scene::ISceneNode*>::ConstIterator END = children.end();
		for (core::list<scene::ISceneNode*>::ConstIterator it = children.begin(); it != END; ++it)
			applyMaterialFlag((*it), materialFlag, newValue);
	}



	// Ajoute � la suite d'un core::stringw un nombre de jours sous un format plus commode � la lecture
	// Exemple : au lieu d'afficher "13787 jours", cette fonction permet d'afficher "38 ann�es 3 mois 17 jours"
	// Cette fonction prend aussi en compte les probl�mes de pluriel dans les dates ("1 ann�e" et "1 jour")
	// Si showZeroData = true, un affichage du type "0 ann�es 3 mois 0 jours" sera autoris�, sinon il sera remplac� par "3 mois"
	// Note : si totalJours = 0 et showZeroData = true, alors la cha�ne fournie se verra tout de m�me ajouter "0 jours"
	static void appendDays(core::stringw& str, u32 totalJours, bool showZeroData = false)
	{
		// Calcule les d�compositions du temps
		const u32 jours = totalJours % 30;
		const u32 totalMois = totalJours / 30;
		const u32 mois = totalMois % 12;
		const u32 annees = totalMois / 12;

		// Indique quelles valeurs seront affich�es
		const bool showAnnees = (annees != 0 || showZeroData);
		const bool showMois = (mois != 0 || showZeroData);
		const bool showJours = (jours != 0 || showZeroData || totalJours == 0);

		// Affiche les ann�es
		if (showAnnees)
		{
			if (annees == 1)
				str.append(L"1 ann�e");
			else
			{
				swprintf_SS(L"%u ann�es", annees);	// %u : Unsigned decimal integer
				str.append(textW_SS);
			}

			// Ajoute une transition (un simple espace) entre ann�es et mois ou entre ann�es et jours
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



	// Ajoute � la suite d'un core::stringw une date sous un format plus commode � la lecture
	// Exemple : au lieu d'afficher "13787 jours", cette fonction permet d'afficher "17 mars 2038"
	// La date correspondant � totalJours = 0 est : "1 janvier 2000"
	static void appendDate(core::stringw& str, u32 totalJours)
	{
		// Calcule les d�compositions du temps
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
		case 1:		str.append(L"f�vrier");		break;
		case 2:		str.append(L"mars");		break;
		case 3:		str.append(L"avril");		break;
		case 4:		str.append(L"mai");			break;
		case 5:		str.append(L"juin");		break;
		case 6:		str.append(L"juillet");		break;
		case 7:		str.append(L"ao�t");		break;
		case 8:		str.append(L"septembre");	break;
		case 9:		str.append(L"octobre");		break;
		case 10:	str.append(L"novembre");	break;
		case 11:	str.append(L"d�cembre");	break;
		default:	str.append(L"MOIS");		break;
		}

		// Ajoute les ann�es
		swprintf_SS(L" %u", annees);		// %u : Unsigned decimal integer
		str.append(textW_SS);
	}



	// Retourne un ISceneNode* converti en un CBatimentSceneNode*
	static CBatimentSceneNode* getBatimentNode(scene::ISceneNode* node)
	{
		CBatimentSceneNode* batNode = NULL;
		if (node && node->getType() == ESNT_BATIMENT_NODE)	// V�rifie que le type de ce node est bien un node de b�timent
			batNode = reinterpret_cast<CBatimentSceneNode*>(node);

		return batNode;
	}
};

#endif
