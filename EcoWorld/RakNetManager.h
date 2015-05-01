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

#define MAX_CLIENTS		15		// Nombre maximum de clients pouvant être connectés à un serveur (le serveur lui-même n'est pas inclus dans ce total)
#define DEFAULT_PORT	60500	// Port par défaut pour la transmission des données

// Détermine le type des données contenant des ID de bâtiments, d'après leur nombre total
#if BI_COUNT <= 255
#define BATIMENT_ID_TYPE unsigned char
#else
#define BATIMENT_ID_TYPE unsigned int
#endif

// Détermine le type des données contenant des index systèmes, d'après la taille de la carte
#if TAILLE_CARTE <= 255
#define INDEX_TYPE unsigned char
#else
#define INDEX_TYPE unsigned int
#endif

// Détermine le type des données contenant un ID de temps, d'après le nombre total de temps disponibles
#if WI_COUNT <= 255
#define WEATHER_ID_TYPE unsigned char
#else
#define WEATHER_ID_TYPE unsigned int
#endif



// Classe permettant de gérer le moteur de RakNet : moteur de communication réseau
// Schéma des communications réseaux entre Serveur et Client : voir BP1-2 : 1
class RakNetManager
{
public:
	// Structure contenant les informations pour pouvoir charger une partie multijoueur
	struct GameSavedData
	{
		// Les informations nécessaires pour le chargement d'une partie multijoueur (créé avec new[], à détruire avec delete[}) :
		// Toutes les données principales du jeu sous forme chaîne de caractère contenant la partie de l'hôte sauvegardée
		char* data;
		u32 dataSize;

		//GameSavedData() : dataSize(0), data(0)	{ }
	};

	// Structure contenant les informations pour pouvoir construire un bâtiment dans le système de jeu
	struct SystemBatConstructInfos
	{
		// Ces paramêtres seront envoyés à EcoWorldSystem::addBatiment dès leur réception
		BATIMENT_ID_TYPE batimentID;
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;
		float rotation;
		u32 dureeVie;

		//SystemConstructingBatInfos() : batimentID(BI_aucun), indexX(0), indexY(0), rotation(0.0f)	{ }
	};

	// Structure contenant les informations pour pouvoir construire un bâtiment du système de jeu
	struct SystemBatDestroyInfos
	{
		// Ces paramêtres seront envoyés à EcoWorldSystem::destroyBatiment dès leur réception
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;

		//SystemDestroyingBatInfos() : indexX(0), indexY(0)	{ }
	};

	// Structure contenant les informations pour pouvoir modifier le pourcentage de production d'un bâtiment
	struct SystemBatProductionPercentageChangeInfos
	{
		INDEX_TYPE indexX;
		INDEX_TYPE indexY;
		float newProductionPercentage;

		//SystemBatChangingProductionPercentage() : indexX(0), indexY(0), newProductionPercentage(0.0f)	{ }
	};

	// Structure contenant les informations sur le nouveau temps choisi par le WeatherManager de l'hôte
	struct SystemWeatherInfos
	{
		WEATHER_ID_TYPE weatherID;
		float transitionBeginTime;

		//SystemWeatherInfos() : weatherID(WI_sunny), transitionBeginTime(0.0f)	{ }
	};

	// Enum représentant les différents types de message que RakNet a reçu du réseau (ces messages ne sont pas envoyés sur le réseau, ils servent de communication à RakNet et à Game)
	enum E_GAME_PACKET_TYPE
	{
		// Messages pour la gestion des parties et leur chargement :

		// La liste des parties multijoueurs a changée (valide lorsque l'état est ENGS_SEARCHING_GAMES)
		EGPT_MULTIPLAYER_GAMES_LIST_CHANGED,
		// Les informations pour le chargement d'une partie multijoueur ont été reçues (valide lorsque l'état est ENGS_WAITING_FOR_GAME_DATA)
		EGPT_MULTIPLAYER_GAME_DATA_RECEIVED,
		// Les informations pour le chargement d'une partie multijoueur n'ont pas pu être reçues (valide lorsque l'état est ENGS_WAITING_FOR_GAME_DATA)
		EGPT_MULTIPLAYER_GAME_DATA_FAILED,



		// Messages de jeu pour la modification des options de jeu :

		// L'état de la pause a changé (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_GAME_PAUSE_CHANGED,
		// La vitesse du jeu a changée (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_GAME_SPEED_CHANGED,



		// Messages de jeu pour les modifications du système de jeu :

		// Le nouveau temps du jeu a été choisi par le serveur (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_WEATHER_CHOOSEN_NEW,
		// Un bâtiment a été construit (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_CONSTRUCTED,
		// Un bâtiment a été détruit (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_DESTROYED,
		// Le pourcentage de production d'un bâtiment a été modifié (valide lorsque l'état est ENGS_PLAYING_GAME_HOST ou ENGS_PLAYING_GAME_CLIENT)
		EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED,
	};

	// Structure représentant les différents messages que RakNet a reçu du réseau
	struct RakNetGamePacket
	{
		E_GAME_PACKET_TYPE packetType;	// Le type de ce paquet (et son utilité)

		// Union permettant d'économiser l'utilisation mémoire de chaque instance de cette classe (étant donné que toutes les structures ne sont pas utilisées en même temps)
		union
		{
			// Pour EGPT_MULTIPLAYER_GAME_DATA_RECEIVED :
			GameSavedData gameSavedData;

			// Pour EGPT_GAME_PAUSE_CHANGED :
			// Le nouvel état de la pause
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

	// Enum représentant l'état actuel de le partie réseau
	enum E_NETWORK_GAME_STATE
	{
		ENGS_NOT_ENABLED,			// Aucune communication réseau en cours
		ENGS_SEARCHING_GAMES,		// Ecran de recherche de parties réseau disponibles
		ENGS_WAITING_FOR_GAME_DATA,	// Attente des informations sur la partie que le joueur désire rejoindre
		ENGS_PLAYING_GAME_HOST,		// Jeu multijoueur en cours (mode hôte)
		ENGS_PLAYING_GAME_CLIENT,	// Jeu multijoueur en cours (mode client)
	};

	// Constructeur et destructeur
	RakNetManager();
	~RakNetManager();

	// Réinitialise RakNet (interrompt la connexion réseau en cours) (état visé : ENGS_NOT_ENABLED)
	void reset();

	// Recherche des parties réseaux (état visé : ENGS_SEARCHING_GAMES)
	// Retourne true si la connexion a échouée, false sinon
	bool searchGames();

	// Demande les informations d'une partie réseau pour pouvoir ensuite la rejoindre
	// hostIP :	IP de l'hôte de la partie à rejoindre
	// Retourne true si la connexion a échouée, false sinon
	bool queryGameData(const char* hostIP);

	// Crée une nouvelle partie réseau (état visé : ENGS_PLAYING_GAME_HOST)
	// Retourne true si la connexion a échouée, false sinon
	bool createGame();

	// Rejoint une partie réseau en cours (état visé : ENGS_PLAYING_GAME_CLIENT) (doit obligatoirement être précédé par un appel à RakNetManager::queryGameData(hostIP) !)
	// Normalement, RakNet est déjà connecté à cet hôte : par un appel précédent à RakNetManager::queryGameData(hostIP) pour obtenir les informations de chargement de la partie
	// Retourne true si la connexion a échouée, false sinon
	bool joinGame();

	// Met à jour RakNet : reçoit les messages du réseaux, puis envoie les messages du jeu
	void update();



	// Informations fournies par le jeu à envoyer vers le réseau :

	// La liste des informations que le jeu doit envoyer au réseau, valide en permanence (gérée dans la fonction RakNet::update()) (si certaines informations à envoyer ne correspondent pas à l'état actuel de RakNet, elles seront ignorées et supprimées)
	core::list<RakNetGamePacket> sendPackets;	// Paquets à envoyer (non encore effectué)



	// Informations fournies par RakNet reçues du réseau :

	// La liste des informations que RakNet a reçu du réseau, valide en permanence (gérée dans la fonction Game::updateRakNetGameMessages())
	core::list<RakNetGamePacket> receivedPackets;	// Packets reçus (non encore traités dans le jeu)

	// La liste des jeux multijoueurs trouvés, valide si l'état actuel est ENGS_SEARCHING_GAMES
	core::list<MultiplayerGameListInfos> multiplayerGames;



//protected:
	// ID des messages personnalisés de jeu (ces messages sont envoyés sur le réseau)
	enum E_GAME_MESSAGES_ID
	{
		// Messages divers :

		// Message en provenance du serveur vers tous les clients pour indiquer le temps total actuel du jeu, pour une synchronisation complète
		ID_SYNCHRONISE_GAME_TIME = ID_USER_PACKET_ENUM,



		// Messages pour la gestion des parties et leur chargement :

		// Demande les informations sur cette partie multijoueur pour l'affichage dans la liste (demande du client vers l'hôte)
		ID_QUERY_MULTIPLAYER_LIST_INFOS,
		// Envoie les informations sur cette partie multijoueur pour l'affichage dans la liste (réponse de l'hôte vers le client à ID_QUERY_MULTIPLAYER_INFOS)
		ID_SENT_MULTIPLAYER_LIST_INFOS,
		// Demande les informations sur cette partie multijoueur pour le chargement du jeu (demande du client vers l'hôte)
		ID_QUERY_MULTIPLAYER_GAME_DATA,
		// Envoie les informations sur cette partie multijoueur pour le chargement du jeu (réponse de l'hôte vers le client à ID_QUERY_MULTIPLAYER_DATA)
		ID_SENT_MULTIPLAYER_GAME_DATA,



		// Messages de jeu pour la modification des options de jeu :

		// Demande au serveur de mettre le jeu en pause sous l'ordre d'un client
		ID_GAME_PAUSE_CHANGE_ASKING,
		// Demande aux clients de mettre le jeu en pause sous l'ordre du joueur hôte, ou en réponse à ID_GAME_PAUSE_CHANGE_ASKING
		ID_GAME_PAUSE_CHANGED,
		// Demande au serveur de modifier la vitesse du jeu sous l'ordre d'un client
		ID_GAME_SPEED_CHANGE_ASKING,
		// Demande aux clients de modifier la vitesse du jeu sous l'ordre du joueur hôte, ou en réponse à ID_GAME_SPEED_CHANGE_ASKING
		ID_GAME_SPEED_CHANGED,



		// Messages de jeu pour les modifications du système de jeu :

		// Message en provenance du serveur indiquant aux clients le prochain temps du jeu choisi aléatoirement par le WeatherManager.
		// Les clients qui recevront ce message devront forcer leur système de jeu (le WeatherManager) à accepter cette modification
		ID_WEATHER_CHOOSEN_NEW,
		// Demande au serveur pour qu'il détermine suivant son système de jeu si un bâtiment pourra être placé,
		// et qu'il renvoie une réponse affirmative si cette contruction a été acceptée
		ID_BATIMENT_CONSTRUCT_ASKING,
		// Validation de la construction d'un bâtiment par le système de jeu,
		// les clients qui recevront ce message devront forcer leur système de jeu à accepter cette construction
		// (ce message peut être une réponse à ID_CONSTRUCTING_BATIMENT_ASKING, mais pas nécessairement si l'ordre de construction viens du joueur hôte de la partie)
		ID_BATIMENT_CONSTRUCTED,
		// Demande au serveur pour qu'il détermine suivant son système de jeu si un bâtiment pourra être détruit,
		// et qu'il renvoie une réponse affirmative si cette destruction a été acceptée
		ID_BATIMENT_DESTROY_ASKING,
		// Validation de la destruction d'un bâtiment par le système de jeu,
		// les clients qui recevront ce message devront forcer leur système de jeu à accepter cette destruction
		// (ce message peut être une réponse à ID_DESTROYING_BATIMENT_ASKING, mais pas nécessairement si l'ordre de destruction viens du joueur hôte de la partie)
		ID_BATIMENT_DESTROYED,
		// Demande au serveyr pour qu'il détermine suivant son système de jeu si le pourcentage de production d'un bâtiment pourra petre modifié
		// et qu'il renvoie une réponse affirmative si cette modification a été acceptée
		ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING,
		// Validation du changement du pourcentage de production d'un bâtiment par le système de jeu,
		// les clients qui recevront ce message devront forcer leur système de jeu à modifier ce pourcentage
		// (ce message peut être une réponse à ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING, mais pas nécessairement si l'ordre de changement du pourcentage de production viens du joueur hôte de la partie)
		ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED,
	};

protected:
	// Initialise RakNet si ce n'est pas encore fait
	// Retourne false si RakNet est prêt à être contrôlé, sinon retourne true lorsque l'initialisation a échouée
	bool init();

	// Reçoit (traîte) les paquets liés au système de RakNet
	void receivePacket_RakNetSystem(Packet* packet);
	// Reçoit (traîte) les paquets en mode ENGS_SEARCHING_GAMES ou ENGS_WAITING_FOR_GAME_DATA
	void receivePacket_SearchingGames(Packet* packet);
	// Reçoit (traîte) les paquets en mode ENGS_PLAYING_GAME_HOST
	void receivePacket_PlayingGameHost(Packet* packet);
	// Reçoit (traîte) les paquets en mode ENGS_PLAYING_GAME_CLIENT
	void receivePacket_PlayingGameClient(Packet* packet);

	// Envoie les messages (en mode ENGS_SEARCHING_GAMES) aux autres machines connectées
	void sendMessages_SearchingGames();
	// Envoie les messages (en mode ENGS_PLAYING_GAME_HOST) aux autres machines connectées
	void sendMessages_PlayingGameHost();
	// Envoie les messages (en mode ENGS_PLAYING_GAME_CLIENT) aux autres machines connectées
	void sendMessages_PlayingGameClient();

	// Demande les informations nécessaires pour l'affichage dans la liste sur une partie multijoueur (état ENGS_PLAYING_GAME_HOST)
	void queryMultiplayerGameListInfos(const SystemAddress& gameHost);
	// Envoie les informations sur cette partie multijoueur (état ENGS_PLAYING_GAME_HOST) vers une machine recherchant des parties pour l'affichage dans la liste (état ENGS_SEARCHING_GAMES)
	void sendMultiplayerGameListInfos(const SystemAddress& gameClient);
	// Reçoit (traîte) les informations sur une partie multijoueur pour l'affichage dans la liste
	void receiveMultiplayerGameListInfos(Packet* packet);

	// Demande les informations nécessaires pour le chargement sur une partie multijoueur dont l'adresse IP est connue
	void queryMultiplayerGameData(const SystemAddress& gameHost);
	// Envoie les informations sur cette partie multijoueur (état ENGS_PLAYING_GAME_HOST) vers une machine pour le chargement du jeu (état ENGS_WAITING_FOR_GAME_DATA)
	void sendMultiplayerGameData(const SystemAddress& gameClient);
	// Reçoit (traîte) les informations sur une partie multijoueur pour le chargement complet du jeu
	void receiveMultiplayerGameData(Packet* packet);



	// Fonctions de gestion (demande, vérification, réception, envoi) des paquets du jeu (états ENGS_PLAYING_GAME_HOST et ENGS_PLAYING_GAME_CLIENT) :
	// ATTENTION : Fonctions non sécurisées (sans vérification des paramêtres fournis ni des valeurs de retour des fonctions utilisées) !

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
	// Le PacketLogger de RakNet pour EcoWorld permettant d'afficher tous les messages reçus du réseau (seulement activé lorsque le mode de log actuel est ELL_DEBUG)
	EcoWorldPacketLogger* packetLogger;
#endif

	// L'état actuel de la partie en réseau
	E_NETWORK_GAME_STATE currentNetworkState;

	// Les prochains temps auquels certains messages périodique seront renvoyés (permet d'envoyer certains messages régulièrement, et non à chaque frame)
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

	// Crée un BitStream et lui ajoute le TimeStamp si nécessaire
	// Note : le BitStream ainsi reçu devra être détruit avec delete !
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
	// Crée un BitStream suivant un paquet, et lui enlève le TimeStamp si présent
	// Note : le BitStream ainsi reçu devra être détruit avec delete !
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

	// Obtient le temps écoulé (en millisecondes) entre la date de réception d'un paquet et sa date d'envoi (représente le temps de transmission du paquet)
	// Note : Le paquet fourni doit s'être vu ajouté un time stamp au début de ses données lors de son envoi
	RakNet::Time getTimeStampDifference(Packet* packet)
	{
		BitStream bs(packet->data, packet->length, false);

		unsigned char tmpChar;
		RakNet::Time tmpTime, timeDiffMs = 0;
		
		if (bs.Read(tmpChar))
			if (tmpChar == ID_TIMESTAMP)	// Vérifie que l'ID du paquet est bien ID_TIMESTAMP
				if (bs.Read(tmpTime))
					timeDiffMs = max(RakNet::GetTime() - tmpTime, 0);	// Obtient la différence de temps entre l'envoi du paquet et sa réception

		return timeDiffMs;
	}


public:
	// Accesseurs inline :

	// Retourne l'état actuel de la partie en réseau
	E_NETWORK_GAME_STATE getCurrentNetworkState() const	{ return currentNetworkState; }

	// Indique si RakNet est actuellement en mode de jeu en réseau (en mode hôte ou client)
	bool isNetworkPlaying() const						{ return (currentNetworkState == ENGS_PLAYING_GAME_HOST || currentNetworkState == ENGS_PLAYING_GAME_CLIENT); }

	// Retourne l'ID d'un paquet utilisateur sous forme de chaîne de caractères
	static const char* UserIDTOString(unsigned char Id)
	{
		const int realID = Id - ID_USER_PACKET_ENUM;

#ifdef _DEBUG
		// Vérifie que l'ID demandé a bien un nom disponible ici
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
