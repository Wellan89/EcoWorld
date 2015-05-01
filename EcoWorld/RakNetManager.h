#ifndef DEF_RAKNET_MANAGER
#define DEF_RAKNET_MANAGER

#include "global.h"

#ifdef USE_RAKNET

#include "GUIManager.h"
#include "Batiments.h"
#include "Weathers.h"

#include <MessageIdentifiers.h>
#include <RakNetTime.h>
#include <BitStream.h>
#include <GetTime.h>

#include <NativeFeatureIncludes.h>
#if _RAKNET_SUPPORT_PacketLogger==1
class EcoWorldPacketLogger;
#endif

using namespace RakNet;

#define MAX_CLIENTS		15		// Nombre maximum de clients pouvant �tre connect�s � un serveur (le serveur lui-m�me n'est pas inclus dans ce total)
#define DEFAULT_PORT	60500	// Port par d�faut pour la transmission des donn�es

// D�termine le type des donn�es contenant des ID de b�timents, d'apr�s leur nombre total
#if BI_COUNT <= 255
#define BATIMENT_ID_TYPE unsigned char
#else
#define BATIMENT_ID_TYPE unsigned int
#endif

// D�termine le type des donn�es contenant des index syst�mes, d'apr�s la taille de la carte
#if TAILLE_CARTE <= 255
#define INDEX_TYPE unsigned char
#else
#define INDEX_TYPE unsigned int
#endif

// D�termine le type des donn�es contenant un ID de temps, d'apr�s le nombre total de temps disponibles
#if WI_COUNT <= 255
#define WEATHER_ID_TYPE unsigned char
#else
#define WEATHER_ID_TYPE unsigned int
#endif



// Classe permettant de g�rer le moteur de RakNet : moteur de communication r�seau
// Sch�ma des communications r�seaux entre Serveur et Client : voir BP1-2 : 1
class RakNetManager
{
public:
	// Structure contenant les informations pour pouvoir charger une partie multijoueur
	struct GameSavedData
	{
		// Les informations n�cessaires pour le chargement d'une partie multijoueur (cr�� avec new[], � d�truire avec delete[}) :
		// Toutes les donn�es principales du jeu sous forme cha�ne de caract�re contenant la partie de l'h�te sauvegard�e
		char* data;
		u32 dataSize;

		//GameSavedData() : dataSize(0), data(0)	{ }
	};

	// Structure contenant les informations pour pouvoir construire un b�timent dans le syst�me de jeu
	struct SystemBatConstructInfos
	{
		// Ces param�tres seront envoy�s � EcoWorldSystem::addBatiment d�s leur r�ception
		BATIMENT_ID_TYPE batimentID;
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;
		float rotation;
		u32 dureeVie;

		//SystemConstructingBatInfos() : batimentID(BI_aucun), indexX(0), indexY(0), rotation(0.0f)	{ }
	};

	// Structure contenant les informations pour pouvoir construire un b�timent du syst�me de jeu
	struct SystemBatDestroyInfos
	{
		// Ces param�tres seront envoy�s � EcoWorldSystem::destroyBatiment d�s leur r�ception
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;

		//SystemDestroyingBatInfos() : indexX(0), indexY(0)	{ }
	};

	// Structure contenant les informations pour pouvoir modifier le pourcentage de production d'un b�timent
	struct SystemBatProductionPercentageChangeInfos
	{
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;
		float newProductionPercentage;

		//SystemBatChangingProductionPercentage() : indexX(0), indexY(0), newProductionPercentage(0.0f)	{ }
	};

	// Structure contenant les informations sur le nouveau temps choisi par le WeatherManager de l'h�te
	struct SystemWeatherInfos
	{
		WEATHER_ID_TYPE weatherID;
		float transitionBeginTime;

		//SystemWeatherInfos() : weatherID(WI_sunny), transitionBeginTime(0.0f)	{ }
	};

	// Enum repr�sentant les diff�rents types de message que RakNet a re�u du r�seau (ces messages ne sont pas envoy�s sur le r�seau, ils servent de communication � RakNet et � Game)
	enum E_GAME_PACKET_TYPE
	{
		// Messages pour la gestion des parties et leur chargement :

		// La liste des parties multijoueurs a chang�e (valide lorsque l'�tat est ENGS_SEARCHING_GAMES)
		EGPT_MULTIPLAYER_GAMES_LIST_CHANGED,
		// Les informations pour le chargement d'une partie multijoueur ont �t� re�ues (valide lorsque l'�tat est ENGS_WAITING_FOR_GAME_DATA)
		EGPT_MULTIPLAYER_GAME_DATA_RECEIVED,
		// Les informations pour le chargement d'une partie multijoueur n'ont pas pu �tre re�ues (valide lorsque l'�tat est ENGS_WAITING_FOR_GAME_DATA)
		EGPT_MULTIPLAYER_GAME_DATA_FAILED,



		// Messages de jeu pour la modification des options de jeu :

		// L'�tat de la pause a chang� (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_GAME_PAUSE_CHANGED,
		// La vitesse du jeu a chang�e (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_GAME_SPEED_CHANGED,



		// Messages de jeu pour les modifications du syst�me de jeu :

		// Le nouveau temps du jeu a �t� choisi par le serveur (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_WEATHER_CHOOSEN_NEW,
		// Un b�timent a �t� construit (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_CONSTRUCTED,
		// Un b�timent a �t� d�truit (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_DESTROYED,
		// Le pourcentage de production d'un b�timent a �t� modifi� (valide lorsque l'�tat est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED,
	};

	// Structure repr�sentant les diff�rents messages que RakNet a re�u du r�seau
	struct RakNetGamePacket
	{
		E_GAME_PACKET_TYPE packetType;	// Le type de ce paquet (et son utilit�)

		// Union permettant d'�conomiser l'utilisation m�moire de chaque instance de cette classe (�tant donn� que toutes les structures ne sont pas utilis�es en m�me temps)
		union
		{
			// Pour EGPT_MULTIPLAYER_GAME_DATA_RECEIVED :
			GameSavedData gameSavedData;

			// Pour EGPT_GAME_PAUSE_CHANGED :
			// Le nouvel �tat de la pause
			bool gameIsPaused;

			// Pour EGPT_GAME_SPEED_CHANGED :
			// La nouvelle vitesse du jeu
			float gameSpeed;

			// Pour EGPT_WEATHER_CHOOSEN_NEW :
			SystemWeatherInfos newChoosenWeatherInfos;

			// Pour EGPT_CONSTRUCTED_BATIMENT :
			SystemBatConstructInfos gameConstructInfos;

			// Pour EGPT_DESTROYED_BATIMENT :
			SystemBatDestroyInfos gameDestroyInfos;

			// Pour EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED :
			SystemBatProductionPercentageChangeInfos gameBatPPChangeInfos;
		};

		RakNetGamePacket(E_GAME_PACKET_TYPE type) : packetType(type)	{ }
	};

	// Enum repr�sentant l'�tat actuel de le partie r�seau
	enum E_NETWORK_GAME_STATE
	{
		ENGS_NOT_ENABLED,			// Aucune communication r�seau en cours
		ENGS_SEARCHING_GAMES,		// Ecran de recherche de parties r�seau disponibles
		ENGS_WAITING_FOR_GAME_DATA,	// Attente des informations sur la partie que le joueur d�sire rejoindre
		ENGS_PLAYING_GAME_HOST,		// Jeu multijoueur en cours (mode h�te)
		ENGS_PLAYING_GAME_CLIENT,	// Jeu multijoueur en cours (mode client)
	};

	// Constructeur et destructeur
	RakNetManager();
	~RakNetManager();

	// R�initialise RakNet (interrompt la connexion r�seau en cours) (�tat vis� : ENGS_NOT_ENABLED)
	void reset();

	// Recherche des parties r�seaux (�tat vis� : ENGS_SEARCHING_GAMES)
	// Retourne true si la connexion a �chou�e, false sinon
	bool searchGames();

	// Demande les informations d'une partie r�seau pour pouvoir ensuite la rejoindre
	// hostIP :	IP de l'h�te de la partie � rejoindre
	// Retourne true si la connexion a �chou�e, false sinon
	bool queryGameData(const char* hostIP);

	// Cr�e une nouvelle partie r�seau (�tat vis� : ENGS_PLAYING_GAME_HOST)
	// Retourne true si la connexion a �chou�e, false sinon
	bool createGame();

	// Rejoint une partie r�seau en cours (�tat vis� : ENGS_PLAYING_GAME_CLIENT) (doit obligatoirement �tre pr�c�d� par un appel � RakNetManager::queryGameData(hostIP) !)
	// Normalement, RakNet est d�j� connect� � cet h�te : par un appel pr�c�dent � RakNetManager::queryGameData(hostIP) pour obtenir les informations de chargement de la partie
	// Retourne true si la connexion a �chou�e, false sinon
	bool joinGame();

	// Met � jour RakNet : re�oit les messages du r�seaux, puis envoie les messages du jeu
	void update();



	// Informations fournies par le jeu � envoyer vers le r�seau :

	// La liste des informations que le jeu doit envoyer au r�seau, valide en permanence (g�r�e dans la fonction RakNet::update()) (si certaines informations � envoyer ne correspondent pas � l'�tat actuel de RakNet, elles seront ignor�es et supprim�es)
	core::list<RakNetGamePacket> sendPackets;	// Paquets � envoyer (non encore effectu�)



	// Informations fournies par RakNet re�ues du r�seau :

	// La liste des informations que RakNet a re�u du r�seau, valide en permanence (g�r�e dans la fonction Game::updateRakNetGameMessages())
	core::list<RakNetGamePacket> receivedPackets;	// Packets re�us (non encore trait�s dans le jeu)

	// La liste des jeux multijoueurs trouv�s, valide si l'�tat actuel est ENGS_SEARCHING_GAMES
	core::list<MultiplayerGameListInfos> multiplayerGames;



//protected:
	// ID des messages personnalis�s de jeu (ces messages sont envoy�s sur le r�seau)
	enum E_GAME_MESSAGES_ID
	{
		// Messages divers :

		// Message en provenance du serveur vers tous les clients pour indiquer le temps total actuel du jeu, pour une synchronisation compl�te
		ID_SYNCHRONISE_GAME_TIME = ID_USER_PACKET_ENUM,



		// Messages pour la gestion des parties et leur chargement :

		// Demande les informations sur cette partie multijoueur pour l'affichage dans la liste (demande du client vers l'h�te)
		ID_QUERY_MULTIPLAYER_LIST_INFOS,
		// Envoie les informations sur cette partie multijoueur pour l'affichage dans la liste (r�ponse de l'h�te vers le client � ID_QUERY_MULTIPLAYER_INFOS)
		ID_SENT_MULTIPLAYER_LIST_INFOS,
		// Demande les informations sur cette partie multijoueur pour le chargement du jeu (demande du client vers l'h�te)
		ID_QUERY_MULTIPLAYER_GAME_DATA,
		// Envoie les informations sur cette partie multijoueur pour le chargement du jeu (r�ponse de l'h�te vers le client � ID_QUERY_MULTIPLAYER_DATA)
		ID_SENT_MULTIPLAYER_GAME_DATA,



		// Messages de jeu pour la modification des options de jeu :

		// Demande au serveur de mettre le jeu en pause sous l'ordre d'un client
		ID_GAME_PAUSE_CHANGE_ASKING,
		// Demande aux clients de mettre le jeu en pause sous l'ordre du joueur h�te, ou en r�ponse � ID_GAME_PAUSE_CHANGE_ASKING
		ID_GAME_PAUSE_CHANGED,
		// Demande au serveur de modifier la vitesse du jeu sous l'ordre d'un client
		ID_GAME_SPEED_CHANGE_ASKING,
		// Demande aux clients de modifier la vitesse du jeu sous l'ordre du joueur h�te, ou en r�ponse � ID_GAME_SPEED_CHANGE_ASKING
		ID_GAME_SPEED_CHANGED,



		// Messages de jeu pour les modifications du syst�me de jeu :

		// Message en provenance du serveur indiquant aux clients le prochain temps du jeu choisi al�atoirement par le WeatherManager.
		// Les clients qui recevront ce message devront forcer leur syst�me de jeu (le WeatherManager) � accepter cette modification
		ID_WEATHER_CHOOSEN_NEW,
		// Demande au serveur pour qu'il d�termine suivant son syst�me de jeu si un b�timent pourra �tre plac�,
		// et qu'il renvoie une r�ponse affirmative si cette contruction a �t� accept�e
		ID_BATIMENT_CONSTRUCT_ASKING,
		// Validation de la construction d'un b�timent par le syst�me de jeu,
		// les clients qui recevront ce message devront forcer leur syst�me de jeu � accepter cette construction
		// (ce message peut �tre une r�ponse � ID_CONSTRUCTING_BATIMENT_ASKING, mais pas n�cessairement si l'ordre de construction viens du joueur h�te de la partie)
		ID_BATIMENT_CONSTRUCTED,
		// Demande au serveur pour qu'il d�termine suivant son syst�me de jeu si un b�timent pourra �tre d�truit,
		// et qu'il renvoie une r�ponse affirmative si cette destruction a �t� accept�e
		ID_BATIMENT_DESTROY_ASKING,
		// Validation de la destruction d'un b�timent par le syst�me de jeu,
		// les clients qui recevront ce message devront forcer leur syst�me de jeu � accepter cette destruction
		// (ce message peut �tre une r�ponse � ID_DESTROYING_BATIMENT_ASKING, mais pas n�cessairement si l'ordre de destruction viens du joueur h�te de la partie)
		ID_BATIMENT_DESTROYED,
		// Demande au serveyr pour qu'il d�termine suivant son syst�me de jeu si le pourcentage de production d'un b�timent pourra petre modifi�
		// et qu'il renvoie une r�ponse affirmative si cette modification a �t� accept�e
		ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING,
		// Validation du changement du pourcentage de production d'un b�timent par le syst�me de jeu,
		// les clients qui recevront ce message devront forcer leur syst�me de jeu � modifier ce pourcentage
		// (ce message peut �tre une r�ponse � ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING, mais pas n�cessairement si l'ordre de changement du pourcentage de production viens du joueur h�te de la partie)
		ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED,
	};

protected:
	// Initialise RakNet si ce n'est pas encore fait
	// Retourne false si RakNet est pr�t � �tre contr�l�, sinon retourne true lorsque l'initialisation a �chou�e
	bool init();

	// Re�oit (tra�te) les paquets li�s au syst�me de RakNet
	void receivePacket_RakNetSystem(Packet* packet);
	// Re�oit (tra�te) les paquets en mode ENGS_SEARCHING_GAMES ou ENGS_WAITING_FOR_GAME_DATA
	void receivePacket_SearchingGames(Packet* packet);
	// Re�oit (tra�te) les paquets en mode ENGS_PLAYING_GAME_HOST
	void receivePacket_PlayingGameHost(Packet* packet);
	// Re�oit (tra�te) les paquets en mode ENGS_PLAYING_GAME_CLIENT
	void receivePacket_PlayingGameClient(Packet* packet);

	// Envoie les messages (en mode ENGS_SEARCHING_GAMES) aux autres machines connect�es
	void sendMessages_SearchingGames();
	// Envoie les messages (en mode ENGS_PLAYING_GAME_HOST) aux autres machines connect�es
	void sendMessages_PlayingGameHost();
	// Envoie les messages (en mode ENGS_PLAYING_GAME_CLIENT) aux autres machines connect�es
	void sendMessages_PlayingGameClient();

	// Demande les informations n�cessaires pour l'affichage dans la liste sur une partie multijoueur (�tat ENGS_PLAYING_GAME_HOST)
	void queryMultiplayerGameListInfos(const SystemAddress& gameHost);
	// Envoie les informations sur cette partie multijoueur (�tat ENGS_PLAYING_GAME_HOST) vers une machine recherchant des parties pour l'affichage dans la liste (�tat ENGS_SEARCHING_GAMES)
	void sendMultiplayerGameListInfos(const SystemAddress& gameClient);
	// Re�oit (tra�te) les informations sur une partie multijoueur pour l'affichage dans la liste
	void receiveMultiplayerGameListInfos(Packet* packet);

	// Demande les informations n�cessaires pour le chargement sur une partie multijoueur dont l'adresse IP est connue
	void queryMultiplayerGameData(const SystemAddress& gameHost);
	// Envoie les informations sur cette partie multijoueur (�tat ENGS_PLAYING_GAME_HOST) vers une machine pour le chargement du jeu (�tat ENGS_WAITING_FOR_GAME_DATA)
	void sendMultiplayerGameData(const SystemAddress& gameClient);
	// Re�oit (tra�te) les informations sur une partie multijoueur pour le chargement complet du jeu
	void receiveMultiplayerGameData(Packet* packet);



	// Fonctions de gestion (demande, v�rification, r�ception, envoi) des paquets du jeu (�tats ENGS_PLAYING_GAME_HOST et ENGS_PLAYING_GAME_CLIENT) :
	// ATTENTION : Fonctions non s�curis�es (sans v�rification des param�tres fournis ni des valeurs de retour des fonctions utilis�es) !

	void queryChangeGamePause(const RakNetGamePacket& packet);
	void sendChangedGamePause(const RakNetGamePacket& packet);
	RakNetGamePacket receiveChangedGamePause(Packet* packet);

	bool checkCanChangeGameSpeed(const RakNetGamePacket& packet);
	void queryChangeGameSpeed(const RakNetGamePacket& packet);
	void sendChangedGameSpeed(const RakNetGamePacket& packet);
	RakNetGamePacket receiveChangedGameSpeed(Packet* packet);

	void sendChoosenNewWeather(const RakNetGamePacket& packet);
	RakNetGamePacket receiveChoosenNewWeather(Packet* packet);

	bool checkCanConstructBatiment(const RakNetGamePacket& packet);
	void queryConstructBatiment(const RakNetGamePacket& packet);
	void sendConstructedBatiment(const RakNetGamePacket& packet);
	RakNetGamePacket receiveConstructedBatiment(Packet* packet);

	bool checkCanDestroyBatiment(const RakNetGamePacket& packet);
	void queryDestroyBatiment(const RakNetGamePacket& packet);
	void sendDestroyedBatiment(const RakNetGamePacket& packet);
	RakNetGamePacket receiveDestroyedBatiment(Packet* packet);

	bool checkCanChangeBatimentProductionPercentage(const RakNetGamePacket& packet);
	void queryChangeBatimentProductionPercentage(const RakNetGamePacket& packet);
	void sendChangedBatimentProductionPercentage(const RakNetGamePacket& packet);
	RakNetGamePacket receiveChangedBatimentProductionPercentage(Packet* packet);



	// L'instance principale de RakNet
	RakPeerInterface* peer;

#if _RAKNET_SUPPORT_PacketLogger==1
	// Le PacketLogger de RakNet pour EcoWorld permettant d'afficher tous les messages re�us du r�seau (seulement activ� lorsque le mode de log actuel est ELL_DEBUG)
	EcoWorldPacketLogger* packetLogger;
#endif

	// L'�tat actuel de la partie en r�seau
	E_NETWORK_GAME_STATE currentNetworkState;

	// Les prochains temps auquels certains messages p�riodique seront renvoy�s (permet d'envoyer certains messages r�guli�rement, et non � chaque frame)
#define SEND_MSG_PERIOD_SEARCH_GAMES_PING		500	// Envoie le message de ping de recherche de parties toutes les 500 ms
	TimeMS nextTimeSendMsg_searchGamesPing;			// Ping de recherche de parties

#define SEND_MSG_PERIOD_SYNCHRONIZE_GAME_TIME	50	// Envoie le message de synchronisation de temps du jeu toutes les 50 ms
	TimeMS nextTimeSendMsg_synchronizeGameTime;		// Message de synchronisation du temps de jeu



	// Fonctions inline :

	// Retourne l'ID d'un paquet
	unsigned char getPacketID(Packet *packet)
	{
		if (packet->data[0] == ID_TIMESTAMP)
			return packet->data[sizeof(unsigned char) + sizeof(RakNet::Time)];

		return packet->data[0];
	}

	// Cr�e un BitStream et lui ajoute le TimeStamp si n�cessaire
	// Note : le BitStream ainsi re�u devra �tre d�truit avec delete !
	BitStream* createBitStreamWriter(bool addTimeStamp = false)
	{
		BitStream* bs = new BitStream();
		if (addTimeStamp)
		{
			// Ajoute le TimeStamp
			bs->Write(ID_TIMESTAMP);
			bs->Write(RakNet::GetTime());
		}
		return bs;
	}
	// Cr�e un BitStream suivant un paquet, et lui enl�ve le TimeStamp si pr�sent
	// Note : le BitStream ainsi re�u devra �tre d�truit avec delete !
	BitStream* createBitStreamReader(Packet* packet)
	{
		const bool timeStamp = (packet->data[0] == ID_TIMESTAMP);
		const unsigned int timeStampSize = sizeof(unsigned char) + sizeof(RakNet::Time);

		BitStream* bs = new BitStream(
			timeStamp ? &packet->data[timeStampSize] : &packet->data[1],	// Exclut l'ID du paquet dans le BitStream
			timeStamp ? packet->length - timeStampSize : packet->length - sizeof(unsigned char),
			false);

		return bs;
	}

	// Obtient le temps �coul� (en millisecondes) entre la date de r�ception d'un paquet et sa date d'envoi (repr�sente le temps de transmission du paquet)
	// Note : Le paquet fourni doit s'�tre vu ajout� un time stamp au d�but de ses donn�es lors de son envoi
	RakNet::Time getTimeStampDifference(Packet* packet)
	{
		BitStream bs(packet->data, packet->length, false);

		unsigned char tmpChar;
		RakNet::Time tmpTime, timeDiffMs = 0;
		
		if (bs.Read(tmpChar))
			if (tmpChar == ID_TIMESTAMP)	// V�rifie que l'ID du paquet est bien ID_TIMESTAMP
				if (bs.Read(tmpTime))
					timeDiffMs = max(RakNet::GetTime() - tmpTime, 0);	// Obtient la diff�rence de temps entre l'envoi du paquet et sa r�ception

		return timeDiffMs;
	}


public:
	// Accesseurs inline :

	// Retourne l'�tat actuel de la partie en r�seau
	E_NETWORK_GAME_STATE getCurrentNetworkState() const	{ return currentNetworkState; }

	// Indique si RakNet est actuellement en mode de jeu en r�seau (en mode h�te ou client)
	bool isNetworkPlaying() const						{ return (currentNetworkState == ENGS_PLAYING_GAME_HOST || currentNetworkState == ENGS_PLAYING_GAME_CLIENT); }

	// Retourne l'ID d'un paquet utilisateur sous forme de cha�ne de caract�res
	static const char* UserIDTOString(unsigned char Id)
	{
		const int realID = Id - ID_USER_PACKET_ENUM;

#ifdef _DEBUG
		// V�rifie que l'ID demand� a bien un nom disponible ici
		_IRR_DEBUG_BREAK_IF(realID < 0 || realID > 14)
#endif
		if (realID < 0 || realID > 14)
			return NULL;

		static const char* UserIDTable[] =
		{
			"ID_SYNCHRONISE_GAME_TIME",

			"ID_QUERY_MULTIPLAYER_LIST_INFOS",
			"ID_SENT_MULTIPLAYER_LIST_INFOS",
			"ID_QUERY_MULTIPLAYER_GAME_DATA",
			"ID_SENT_MULTIPLAYER_GAME_DATA",

			"ID_GAME_PAUSE_CHANGE_ASKING",
			"ID_GAME_PAUSE_CHANGED",
			"ID_GAME_SPEED_CHANGE_ASKING",
			"ID_GAME_SPEED_CHANGED",

			"ID_WEATHER_CHOOSEN_NEW",
			"ID_BATIMENT_CONSTRUCT_ASKING",
			"ID_BATIMENT_CONSTRUCTED",
			"ID_BATIMENT_DESTROY_ASKING",
			"ID_BATIMENT_DESTROYED",
			"ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING",
			"ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED",
		};

		return UserIDTable[realID];
	}
};

#endif
#endif
