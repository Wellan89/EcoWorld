#ifndef DEF_HELPERS
#define DEF_HELPERS

// Header inclus dans tout le projet, contenant des classes statiques (singleton) permettant de simplifier certaines parties du jeu :
// - L'état actuel du jeu (GameState), contenant entre-autres ses paramêtres de configuration (GameConfig)
// - La transformation de chaînes de caractères (StaticString)
// - L'envoi de textes de log (EcoWorldLogger)

// Note : Les headers nécessaires à cette classe sont supposés déjà inclus (dans global.h plus particulièrement)
// (Evite ainsi des bugs avec les outils de débogage mémoire (nouvelles définitions de new et delete) :
//	Ce header nécessite des headers de la STL, mais incompatibles avec les nouvelles définitions de ces opérateurs.
//	Or, ce header nécessite ces outils de débogage mémoire (utilisation de new et delete),
//	donc il doit être placé après le header de débogage mémoire dans global.h,
//	donc on ne peut pas inclure les headers de la STL dans ce header : ils doivent être inclus le header de débogage mémoire avant dans global.h)





// Classe statique (singleton) contenant les paramêtres constants du jeu, valides même lors du redémarrage de la classe Game
// Elle contient entre-autres les paramêtres de configuration du jeu, qui sont ainsi accessibles depuis n'importe quelle classe du programme
class Game;
class GameConfiguration;
#ifdef USE_IRRKLANG
class IrrKlangManager;
#endif
#ifdef USE_RAKNET
class RakNetManager;
#endif
class GameState	// Cette classe est en partie implémentée dans Helpers.cpp
{
public:
	// Les paramêtres publics du jeu, accessibles directement par l'instance statique de cette classe :

	irr::io::path workingDirectory;		// Le chemin de travail de l'application, modifié grâce à Irrlicht
	irr::io::path fileToOpen;			// Le chemin du fichier à ouvrir : le fichier passé en argument à l'exécutable lors de son lancement (ex : si un fichier *.ewg est passé en argument au lancement de EcoWorld.exe, alors ce fichier sera chargé par le jeu à son lancement)

	bool restart;						// Lorsque le jeu s'arrête : indique qu'on doit redémarrer (le passage de cette valeur à true suffit pour fermer le device et redémarrer le jeu)
	bool restartToOptionsMenu;			// Lorsque le jeu redémarre : indique qu'on doit démarrer au menu options
	int optionsMenuTab;					// Lors du prochain affichage du menu options : indique quel onglet du menu doit être activé (-1 si aucun onglet spécifique ne nécessite d'être activé)
	bool showOptionsConfirmationWindow;	// Lors du prochain affichage du menu options : indique qu'on doit afficher la fenêtre de confirmation des paramêtres

	int lastWeather;					// Le temps du menu principal lorsqu'on avait demandé le redémarrage, pour pouvoir le restaurer au redémarrage suivant (-1 si on n'a pas redémarré ou s'il doit être redéfini aléatoirement)
	u32 lastDeviceTime;					// Le temps écoulé au timer du device lorsqu'on avait demandé le redémarrage, pour pouvoir le restaurer au redémarrage suivant (-1.0f si on n'a pas redémarré ou s'il doit être réinitialisé)
	bool keepIrrKlang;					// True si pendant le redémarrage, on ne doit pas stopper IrrKlang (n'est valable que jusqu'au menu principal)

	// Paramêtre particulier permettant d'enregistrer les anciennes options de configuration (dans lastGameConfiguration)
	// au lieu des options de configuration actuelles (dans gameConfiguration) lors de l'appel à la méthode saveGameConfigToFile().
	// Pour plus d'informations, voir le commentaire dans CGUIOptionsMenu::showConfirmationWindow().
	bool saveLastGameConfig;



private:
	// Les paramêtres privés du jeu, accessibles uniquement grâce aux fonctions publiques statiques :

	GameConfiguration* gameConfiguration;		// La configuration actuelle du jeu (utilisé pour le démarrage, mais aussi tout au long de l'éxecution pour connaître certains paramêtres)
	GameConfiguration* lastGameConfiguration;	// Si le jeu essaie de démarrer mais échoue, on restaurera ces paramêtres de configuration (peuvent être les paramêtres par défaut, mais aussi les derniers paramêtres appliqués dans le menu options)

#ifdef USE_IRRKLANG
	IrrKlangManager* irrKlangManager;			// L'IrrKlangManager, qui reste valide tout au long du jeu, même si IrrKlang n'est pas utilisé
#endif
#ifdef USE_RAKNET
	RakNetManager* rakNetManager;				// Le RakNetManager, qui reste valide tout au long du jeu, même si RakNet n'est pas utilisé
#endif



	// Constructeur de copie privé (permet de le désactiver)
	GameState(const GameState& other)	{ }
public:
	// Constructeur et destructeur
	GameState();
	~GameState();



	// Méthodes publiques :

	// Charge le fichier de configuration depuis un fichier (effectué une seule fois tout au long du jeu, à son démarrage)
	// Aucun device n'est nécessaire, un NULL DEVICE sera créé pour l'occasion
	void loadGameConfigFromFile();

	// Enregistre le fichier de configuration dans un fichier (effectué une seule fois tout au long du jeu, à son arrêt)
	// Aucun device n'est nécessaire, un NULL DEVICE sera créé pour l'occasion
	void saveGameConfigToFile();

	// Parcours tous les arguments passés au jeu et détermine ses paramêtres
	void parseMainArguments(int argc, char* argv[]);

	// Obtient la configuration actuelle du jeu
	GameConfiguration& getGameConfig()
	{
		return (*gameConfiguration);
	}
	// Obtient l'ancienne configuration du jeu
	GameConfiguration& getLastGameConfig()
	{
		return (*lastGameConfiguration);
	}
#ifdef USE_IRRKLANG
	// Obtient l'IrrKlangManager du jeu
	IrrKlangManager& getIrrKlangManager()
	{
		return (*irrKlangManager);
	}
#endif
#ifdef USE_RAKNET
	// Obtient le RakNetManager du jeu
	RakNetManager& getRakNetManager()
	{
		return (*rakNetManager);
	}
#endif
};
// Déclarations globales externes des instances (instance de Game exclue de GameState car souvent utilisée : optimisation) :
extern GameState gameState;
extern Game* game;



// Defines permettant de faciliter l'utilisation de la classe GameState
#define gameConfig		gameState.getGameConfig()
#define lastGameConfig	gameState.getLastGameConfig()
#ifdef USE_IRRKLANG
#define ikMgr			gameState.getIrrKlangManager()
#endif
#ifdef USE_RAKNET
#define rkMgr			gameState.getRakNetManager()
#endif





// Classe statique (singleton) contenant en permanence une chaîne de caractère temporaire (évitant ainsi des allocations/désallocations sucessives de cette chaîne)
class StaticString
{
public:
	// Chaînes de caractères publiques :
#define SS_TEXT_SIZE			100
#define SS_TEXT_SIZE_W			200
	char text[SS_TEXT_SIZE];
	wchar_t textW[SS_TEXT_SIZE_W];

	// Constructeur et destructeur
	StaticString()	{ }
	~StaticString()	{ }
private:
	// Constructeur de copie privé (permet de le désactiver)
	StaticString(const StaticString& other)	{ }
};
// Déclaration globale externe de l'instance :
extern StaticString staticString;



// Defines et macros permettant de faciliter l'utilisation des fonctions sprintf_s et swprintf_s grâce à cette classe (ces macros permettent de gérer un nombre infini d'argument, comme les fonctions qu'elles représentent)
#define text_SS						staticString.text
#define textW_SS					staticString.textW
#define sprintf_SS(format, ...)		sprintf_s(staticString.text, SS_TEXT_SIZE, format, __VA_ARGS__)
#define swprintf_SS(format, ...)	swprintf_s(staticString.textW, SS_TEXT_SIZE_W, format, __VA_ARGS__)





// Classe de log pour EcoWorld permettant d'envoyer des informations de log dans un fichier
class EcoWorldLogger	// Cette classe est en partie implémentée dans Helpers.cpp
{
public:
	// Variables nécessaires pour le log :
	bool canWriteOnDisk;		// Détermine si on peut écrire sur le disque dur de l'ordinateur cible
	ofstream* logFile;			// Le fichier de log

private:
	// Crée le fichier de log s'il n'a pas encore été créé
	void verifyLogFile()
	{
		if (!logFile && canWriteOnDisk)
		{
			logFile = new ofstream("EcoWorld Log.log", ios::out | ios::trunc);	// Les nouvelles données remplacent le fichier existant
			if (logFile)
			{
				if (!logFile->is_open())
				{
					delete logFile;
					logFile = NULL;
				}
			}
		}
	}

public:
	// Ecrit un texte sous forme de flux C++ dans le fichier de log
	void log(const stringstream& stream)
	{
		// Crée le fichier de log s'il n'a pas encore été créé
		verifyLogFile();

		// Envoie ce texte dans le fichier de log s'il est valide
		if (logFile)
			(*logFile) << stream.str() << endl;
		else	// Sinon, on essaie tout de même d'envoyer ce texte dans la console
			cout << stream.str() << endl;
	}

	// Vérifie le niveau d'importance d'un texte de log pour déterminer s'il doit être affiché
	static bool verifyLogLevel(ELOG_LEVEL logLevel);

	// Constructeur et destructeur
	EcoWorldLogger() : canWriteOnDisk(false), logFile(NULL)
	{
		// On ne crée pas le fichier de log dans le constructeur, il sera créé lorsqu'un premier texte de log sera envoyé :
		// on crée ainsi le fichier de log dès que nécessaire, mais pas avant au cas où il ne serait pas utilisé
	}
	~EcoWorldLogger()
	{
		// Ferme le fichier de log s'il a été créé
		if (logFile)
		{
			if (logFile->is_open())
				logFile->close();
			delete logFile;
		}
	}
private:
	// Constructeur de copie privé (permet de le désactiver)
	EcoWorldLogger(const EcoWorldLogger& other)	{ }
};
// Déclaration globale externe de l'instance :
extern EcoWorldLogger ecoWorldLogger;



// Détermine si on peut écrire sur le disque de l'ordinateur cible
#define CAN_WRITE_ON_DISK	ecoWorldLogger.canWriteOnDisk



// Defines permettant de diminuer la taille du code à écrire pour envoyer un simple log de jeu :
#ifdef _DEBUG

// On utilise simplement le flux de sortie standard cout en mode débogage, pour éviter la création d'un fichier de log (facilite ainsi le débogage)
#define LOG(text, logLevel)	if (EcoWorldLogger::verifyLogLevel(logLevel)) { cout << text << endl; };

// Envoie les logs de débogage uniquement
#define LOG_DEBUG(text, logLevel)	LOG(text, logLevel)
#define LOG_RELEASE(text, logLevel)

#else

// Utilise le logger du jeu pour envoyer ce texte de log
#define LOG(text, logLevel)	if (EcoWorldLogger::verifyLogLevel(logLevel)) { stringstream log_str; log_str << text; ecoWorldLogger.log(log_str); };

// Envoie les logs de release uniquement
#define LOG_DEBUG(text, logLevel)
#define LOG_RELEASE(text, logLevel)	LOG(text, logLevel)

#endif





#endif
