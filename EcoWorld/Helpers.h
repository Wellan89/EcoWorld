#ifndef DEF_HELPERS
#define DEF_HELPERS

// Header inclus dans tout le projet, contenant des classes statiques (singleton) permettant de simplifier certaines parties du jeu :
// - L'�tat actuel du jeu (GameState), contenant entre-autres ses param�tres de configuration (GameConfig)
// - La transformation de cha�nes de caract�res (StaticString)
// - L'envoi de textes de log (EcoWorldLogger)

// Note : Les headers n�cessaires � cette classe sont suppos�s d�j� inclus (dans global.h plus particuli�rement)
// (Evite ainsi des bugs avec les outils de d�bogage m�moire (nouvelles d�finitions de new et delete) :
//	Ce header n�cessite des headers de la STL, mais incompatibles avec les nouvelles d�finitions de ces op�rateurs.
//	Or, ce header n�cessite ces outils de d�bogage m�moire (utilisation de new et delete),
//	donc il doit �tre plac� apr�s le header de d�bogage m�moire dans global.h,
//	donc on ne peut pas inclure les headers de la STL dans ce header : ils doivent �tre inclus le header de d�bogage m�moire avant dans global.h)





// Classe statique (singleton) contenant les param�tres constants du jeu, valides m�me lors du red�marrage de la classe Game
// Elle contient entre-autres les param�tres de configuration du jeu, qui sont ainsi accessibles depuis n'importe quelle classe du programme
class Game;
class GameConfiguration;
#ifdef USE_IRRKLANG
class IrrKlangManager;
#endif
#ifdef USE_RAKNET
class RakNetManager;
#endif
class GameState	// Cette classe est en partie impl�ment�e dans Helpers.cpp
{
public:
	// Les param�tres publics du jeu, accessibles directement par l'instance statique de cette classe :

	irr::io::path workingDirectory;		// Le chemin de travail de l'application, modifi� gr�ce � Irrlicht
	irr::io::path fileToOpen;			// Le chemin du fichier � ouvrir : le fichier pass� en argument � l'ex�cutable lors de son lancement (ex : si un fichier *.ewg est pass� en argument au lancement de EcoWorld.exe, alors ce fichier sera charg� par le jeu � son lancement)

	bool restart;						// Lorsque le jeu s'arr�te : indique qu'on doit red�marrer (le passage de cette valeur � true suffit pour fermer le device et red�marrer le jeu)
	bool restartToOptionsMenu;			// Lorsque le jeu red�marre : indique qu'on doit d�marrer au menu options
	int optionsMenuTab;					// Lors du prochain affichage du menu options : indique quel onglet du menu doit �tre activ� (-1 si aucun onglet sp�cifique ne n�cessite d'�tre activ�)
	bool showOptionsConfirmationWindow;	// Lors du prochain affichage du menu options : indique qu'on doit afficher la fen�tre de confirmation des param�tres

	int lastWeather;					// Le temps du menu principal lorsqu'on avait demand� le red�marrage, pour pouvoir le restaurer au red�marrage suivant (-1 si on n'a pas red�marr� ou s'il doit �tre red�fini al�atoirement)
	u32 lastDeviceTime;					// Le temps �coul� au timer du device lorsqu'on avait demand� le red�marrage, pour pouvoir le restaurer au red�marrage suivant (-1.0f si on n'a pas red�marr� ou s'il doit �tre r�initialis�)
	bool keepIrrKlang;					// True si pendant le red�marrage, on ne doit pas stopper IrrKlang (n'est valable que jusqu'au menu principal)

	// Param�tre particulier permettant d'enregistrer les anciennes options de configuration (dans lastGameConfiguration)
	// au lieu des options de configuration actuelles (dans gameConfiguration) lors de l'appel � la m�thode saveGameConfigToFile().
	// Pour plus d'informations, voir le commentaire dans CGUIOptionsMenu::showConfirmationWindow().
	bool saveLastGameConfig;



private:
	// Les param�tres priv�s du jeu, accessibles uniquement gr�ce aux fonctions publiques statiques :

	GameConfiguration* gameConfiguration;		// La configuration actuelle du jeu (utilis� pour le d�marrage, mais aussi tout au long de l'�xecution pour conna�tre certains param�tres)
	GameConfiguration* lastGameConfiguration;	// Si le jeu essaie de d�marrer mais �choue, on restaurera ces param�tres de configuration (peuvent �tre les param�tres par d�faut, mais aussi les derniers param�tres appliqu�s dans le menu options)

#ifdef USE_IRRKLANG
	IrrKlangManager* irrKlangManager;			// L'IrrKlangManager, qui reste valide tout au long du jeu, m�me si IrrKlang n'est pas utilis�
#endif
#ifdef USE_RAKNET
	RakNetManager* rakNetManager;				// Le RakNetManager, qui reste valide tout au long du jeu, m�me si RakNet n'est pas utilis�
#endif



	// Constructeur de copie priv� (permet de le d�sactiver)
	GameState(const GameState& other)	{ }
public:
	// Constructeur et destructeur
	GameState();
	~GameState();



	// M�thodes publiques :

	// Charge le fichier de configuration depuis un fichier (effectu� une seule fois tout au long du jeu, � son d�marrage)
	// Aucun device n'est n�cessaire, un NULL DEVICE sera cr�� pour l'occasion
	void loadGameConfigFromFile();

	// Enregistre le fichier de configuration dans un fichier (effectu� une seule fois tout au long du jeu, � son arr�t)
	// Aucun device n'est n�cessaire, un NULL DEVICE sera cr�� pour l'occasion
	void saveGameConfigToFile();

	// Parcours tous les arguments pass�s au jeu et d�termine ses param�tres
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
// D�clarations globales externes des instances (instance de Game exclue de GameState car souvent utilis�e : optimisation) :
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





// Classe statique (singleton) contenant en permanence une cha�ne de caract�re temporaire (�vitant ainsi des allocations/d�sallocations sucessives de cette cha�ne)
class StaticString
{
public:
	// Cha�nes de caract�res publiques :
#define SS_TEXT_SIZE			100
#define SS_TEXT_SIZE_W			200
	char text[SS_TEXT_SIZE];
	wchar_t textW[SS_TEXT_SIZE_W];

	// Constructeur et destructeur
	StaticString()	{ }
	~StaticString()	{ }
private:
	// Constructeur de copie priv� (permet de le d�sactiver)
	StaticString(const StaticString& other)	{ }
};
// D�claration globale externe de l'instance :
extern StaticString staticString;



// Defines et macros permettant de faciliter l'utilisation des fonctions sprintf_s et swprintf_s gr�ce � cette classe (ces macros permettent de g�rer un nombre infini d'argument, comme les fonctions qu'elles repr�sentent)
#define text_SS						staticString.text
#define textW_SS					staticString.textW
#define sprintf_SS(format, ...)		sprintf_s(staticString.text, SS_TEXT_SIZE, format, __VA_ARGS__)
#define swprintf_SS(format, ...)	swprintf_s(staticString.textW, SS_TEXT_SIZE_W, format, __VA_ARGS__)





// Classe de log pour EcoWorld permettant d'envoyer des informations de log dans un fichier
class EcoWorldLogger	// Cette classe est en partie impl�ment�e dans Helpers.cpp
{
public:
	// Variables n�cessaires pour le log :
	bool canWriteOnDisk;		// D�termine si on peut �crire sur le disque dur de l'ordinateur cible
	ofstream* logFile;			// Le fichier de log

private:
	// Cr�e le fichier de log s'il n'a pas encore �t� cr��
	void verifyLogFile()
	{
		if (!logFile && canWriteOnDisk)
		{
			logFile = new ofstream("EcoWorld Log.log", ios::out | ios::trunc);	// Les nouvelles donn�es remplacent le fichier existant
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
		// Cr�e le fichier de log s'il n'a pas encore �t� cr��
		verifyLogFile();

		// Envoie ce texte dans le fichier de log s'il est valide
		if (logFile)
			(*logFile) << stream.str() << endl;
		else	// Sinon, on essaie tout de m�me d'envoyer ce texte dans la console
			cout << stream.str() << endl;
	}

	// V�rifie le niveau d'importance d'un texte de log pour d�terminer s'il doit �tre affich�
	static bool verifyLogLevel(ELOG_LEVEL logLevel);

	// Constructeur et destructeur
	EcoWorldLogger() : canWriteOnDisk(false), logFile(NULL)
	{
		// On ne cr�e pas le fichier de log dans le constructeur, il sera cr�� lorsqu'un premier texte de log sera envoy� :
		// on cr�e ainsi le fichier de log d�s que n�cessaire, mais pas avant au cas o� il ne serait pas utilis�
	}
	~EcoWorldLogger()
	{
		// Ferme le fichier de log s'il a �t� cr��
		if (logFile)
		{
			if (logFile->is_open())
				logFile->close();
			delete logFile;
		}
	}
private:
	// Constructeur de copie priv� (permet de le d�sactiver)
	EcoWorldLogger(const EcoWorldLogger& other)	{ }
};
// D�claration globale externe de l'instance :
extern EcoWorldLogger ecoWorldLogger;



// D�termine si on peut �crire sur le disque de l'ordinateur cible
#define CAN_WRITE_ON_DISK	ecoWorldLogger.canWriteOnDisk



// Defines permettant de diminuer la taille du code � �crire pour envoyer un simple log de jeu :
#ifdef _DEBUG

// On utilise simplement le flux de sortie standard cout en mode d�bogage, pour �viter la cr�ation d'un fichier de log (facilite ainsi le d�bogage)
#define LOG(text, logLevel)	if (EcoWorldLogger::verifyLogLevel(logLevel)) { cout << text << endl; };

// Envoie les logs de d�bogage uniquement
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
