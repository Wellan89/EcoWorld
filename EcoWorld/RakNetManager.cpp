#include "global.h"

#ifdef USE_RAKNET

#include "RakNetManager.h"
#include "Game.h"
#include "EcoWorldInfos.h"
#include "EcoWorldModifiers.h"
#include "EcoWorldRenderer.h"
#include "WeatherManager.h"

#if _RAKNET_SUPPORT_PacketLogger==1
#include "EcoWorldPacketLogger.h"
#endif
#include <RakPeerInterface.h>

// Macro de LOG personnalisée pour le débogage du module réseau (log avec la priorité ELL_INFORMATION et le préfixe du RakNetManager : RkMgr) :
#define LOG_RAKNET(text)	LOG("RkMgr: " << text, ELL_INFORMATION)

RakNetManager::RakNetManager() : peer(NULL), packetLogger(NULL), currentNetworkState(ENGS_NOT_ENABLED),
	nextTimeSendMsg_searchGamesPing(0), nextTimeSendMsg_synchronizeGameTime(0)
{
	// Réinitialise RakNet
	reset();
}
RakNetManager::~RakNetManager()
{
	if (peer)
	{
		// Attend 200 ms au maximum pour l'envoi des derniers messages (s'il y en a)
		peer->Shutdown(200);

		// Détruit l'instance de RakNet
		RakPeerInterface::DestroyInstance(peer);

		//LOG_RAKNET("RakNet destroyed");	// Désactivé : A cet instant, le logger d'EcoWorld peut avoir été détruit : cela provoque actuellement des bugs en mdoe Release
	}

#if _RAKNET_SUPPORT_PacketLogger==1
	if (packetLogger)
		delete packetLogger;
#endif
}
bool RakNetManager::init()
{
	// Vérifie que RakNet n'est pas déjà en train de tourner
	if (peer && peer->IsActive())
		return false;

	// Initialise l'instance principale de RakNet
	if (!peer)
		peer = RakPeerInterface::GetInstance();
	if (!peer)
		LOG("RkMgr: ERROR : Could not create RakNet instance !", ELL_ERROR);

#if _RAKNET_SUPPORT_PacketLogger==1
	// Initialise le PacketLogger si nécessaire (lorsque le mode de log est ELL_DEBUG)
	if (gameConfig.deviceParams.LoggingLevel <= ELL_DEBUG && !packetLogger)
	{
		LOG_RAKNET("Initializing Packet Logger");

		packetLogger = new EcoWorldPacketLogger();
		if (packetLogger)
		{
			//packetLogger->SetPrefix("RkMgr: PacketLogger : ");
			//packetLogger->SetSuffix("\r\n");
			peer->AttachPlugin(packetLogger);

			packetLogger->LogHeader();
		}
	}
#endif

	// On démarre RakNet en mode serveur dès le début : on peut ainsi être utilisé comme serveur ou client :
	u16 i = 0;
	StartupResult result;
	do
	{
		// Démarre RakNet avec différents ports consécutifs s'il n'a pas pu être démarré avec un port bien précis
		// (permet de lancer EcoWorld plusieurs fois en mode réseau sur la même machine)
		const u16 port = DEFAULT_PORT + i; ++i;
		result = peer->Startup(MAX_CLIENTS, &SocketDescriptor(port, 0), 1);

		LOG_RAKNET("Started result : " << result << " (Port : " << port << ")");

	} while (result != RAKNET_STARTED && i <= 8);	// 8 ports consécutifs testés au maximum

	return (result != RAKNET_STARTED);	// Vérifie que l'instance de RakNet a bien pu démarrer
}
void RakNetManager::reset()
{
	// Demande de quitter le jeu, et attend 200 ms au maximum pour l'envoi des derniers messages (si il y en a)
	if (peer && peer->IsActive())
		peer->Shutdown(200);

	// Vide les liste actuelles des paquets de jeu
	sendPackets.clear();
	receivedPackets.clear();
	multiplayerGames.clear();

	// Indique le nouvel état de RakNet
	currentNetworkState = ENGS_NOT_ENABLED;

	LOG_RAKNET("RakNet stopped");
}
bool RakNetManager::searchGames()
{
	// Arrête l'action en cours et réinitialise RakNet si nécessaire
	if (currentNetworkState == ENGS_SEARCHING_GAMES)		return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Indique le nouvel état de RakNet
	currentNetworkState = ENGS_SEARCHING_GAMES;

	LOG_RAKNET("New mode : Searching games");

	return false;
}
bool RakNetManager::queryGameData(const char* hostIP)
{
	// Arrête l'action en cours et réinitialise RakNet si nécessaire
	if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)	return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Indique le nouvel état de RakNet
	currentNetworkState = ENGS_WAITING_FOR_GAME_DATA;

	// Demande la connexion à cet hôte
	const SystemAddress hostAddress(hostIP);
	const ConnectionAttemptResult connectionResult = peer->Connect(hostIP, hostAddress.GetPort(), 0, 0);

	LOG_RAKNET("New mode : Querying game data (Server address : " << hostIP << " ; Connection result : " << connectionResult << ")");

	// Retourne si cette connexion a réussie
	return (connectionResult == INVALID_PARAMETER || connectionResult == CANNOT_RESOLVE_DOMAIN_NAME || connectionResult == SECURITY_INITIALIZATION_FAILED);
}
bool RakNetManager::createGame()
{
	// Arrête l'action en cours et réinitialise RakNet si nécessaire
	if (currentNetworkState == ENGS_PLAYING_GAME_HOST)		return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Permet la connection d'autres machines à la notre
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);

	// Indique le nouvel état de RakNet
	currentNetworkState = ENGS_PLAYING_GAME_HOST;

	LOG_RAKNET("New mode : Server (creating game)");

	return false;
}
bool RakNetManager::joinGame()
{
	// Vérifie qu'on n'est pas déjà en train de jouer en mode client
	if (currentNetworkState == ENGS_PLAYING_GAME_CLIENT)	return false;

	// Normalement, la précédente étape était déterminée par l'appel à RakNetManager::queryGameData(hostIP) !
	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)	return true;

	// Normalement, RakNet est déjà connecté à cet hôte :
	// par un appel précédent à RakNetManager::queryGameData(hostIP) pour obtenir les informations de chargement de la partie

	// Vérifie qu'on est déjà connecté à un hôte, et un seul :
	if (peer->NumberOfConnections() != 1)					return true;

	// Indique le nouvel état de RakNet
	currentNetworkState = ENGS_PLAYING_GAME_CLIENT;

	LOG_RAKNET("New mode : Client (joining game)");

	return false;
}
void RakNetManager::update()
{
	if (!peer || !peer->IsActive())
		return;

	// Reçois et traîte tout d'abord les messages en file d'attente :
	// Parcours tous les paquets reçus et les traîte
	for (Packet* packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		// Détermine à quelle fonction de réception des paquets on doit envoyer celui-ci :
		const unsigned char packetID = getPacketID(packet);
		if (packetID < ID_USER_PACKET_ENUM)
			receivePacket_RakNetSystem(packet);
		else if (currentNetworkState == ENGS_SEARCHING_GAMES || currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)
			receivePacket_SearchingGames(packet);
		else if (currentNetworkState == ENGS_PLAYING_GAME_HOST)
			receivePacket_PlayingGameHost(packet);
		else if (currentNetworkState == ENGS_PLAYING_GAME_CLIENT)
			receivePacket_PlayingGameClient(packet);
#ifdef _DEBUG
		else
			LOG_DEBUG("RakNetManager::update() : Impossible de determiner quelle fonction doit gerer ce paquet :\r\n    packetID = " << (int)packetID << " (" << PacketLogger::BaseIDTOString(packetID) << ") ; currentNetworkState = " << currentNetworkState, ELL_WARNING);
#endif
	}

	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)
	{
		// Détermine quelle fonction d'envoi des messages on doit appeler :
		if (currentNetworkState == ENGS_SEARCHING_GAMES)
			sendMessages_SearchingGames();
		else if (currentNetworkState == ENGS_PLAYING_GAME_HOST)
			sendMessages_PlayingGameHost();
		else if (currentNetworkState == ENGS_PLAYING_GAME_CLIENT)
			sendMessages_PlayingGameClient();
#ifdef _DEBUG
		else
			LOG_DEBUG("RakNetManager::update() : Impossible de determiner quelle fonction d'envoi de messages appeler : currentNetworkState = " << currentNetworkState, ELL_WARNING);
#endif
	}
}
void RakNetManager::receivePacket_RakNetSystem(Packet* packet)
{
	// Gère le paquet différemment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
		LOG_RAKNET("Pong response sent to : " << packet->systemAddress.ToString());
		break;
	case ID_UNCONNECTED_PONG:	// Réception d'un pong d'un système non connecté
		LOG_RAKNET("Pong response received from : " << packet->systemAddress.ToString());

		if (currentNetworkState == ENGS_SEARCHING_GAMES)	// Ajoute cette machine à la liste des parties multijoueurs
		{
			// Parcours la liste des messages de RakNet pour vérifier que cette machine n'est pas déjà dans la liste des parties
			bool addToList = true;
			const core::stringc packetAddress = packet->systemAddress.ToString();
			const core::list<MultiplayerGameListInfos>::ConstIterator END = multiplayerGames.end();
			for (core::list<MultiplayerGameListInfos>::ConstIterator it = multiplayerGames.begin(); it != END; ++it)
			{
				if ((*it).hostIP == packetAddress)
				{
					addToList = false;
					break;
				}
			}
			if (addToList)
			{
				// Se connecte à l'hôte de cette partie pour pouvoir lui demander les informations sur cette partie multijoueurs plus tard
				const ConnectionAttemptResult connectionResult = peer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), 0, 0);

				LOG_RAKNET("Attempting to connect to : " << packet->systemAddress.ToString() << " ; Connection result : " << connectionResult);
			}
		}
		break;

		// IDs du système de RakNet :
	case ID_CONNECTION_REQUEST_ACCEPTED:	// Notre requête de connexion a été acceptée
		LOG_RAKNET("Connection request accepted : Server address : " << packet->systemAddress.ToString());
	case ID_ALREADY_CONNECTED:
		if (packetID == ID_ALREADY_CONNECTED)
			LOG_RAKNET("Already connected to system");

		if (currentNetworkState == ENGS_SEARCHING_GAMES)			// Demande au nouveau système connecté les informations sur sa partie pour son affichage dans la liste
		{
			// Envoie une demande de description des parties multijoueurs à cet hôte pour l'affichage dans la liste
			queryMultiplayerGameListInfos(packet->systemAddress);
		}
		else if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)	// Demande au nouveau système connecté les informations sur sa partie pour le chargement
		{
			// Envoie une demande de description des parties multijoueurs à cet hôte pour le chargement
			queryMultiplayerGameData(packet->systemAddress);
		}
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		// Indique qu'on n'a pas pu recevoir les informations de la partie pour le chargement
		if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)
			receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));

		LOG_RAKNET("Connection attempt failed");
		break;
	case ID_NEW_INCOMING_CONNECTION:
		LOG_RAKNET("New incoming connection");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Indique qu'on n'a pas pu recevoir les informations de la partie pour le chargement
		if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)
			receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));

		LOG_RAKNET("Server is full");
		break;
	case ID_DISCONNECTION_NOTIFICATION:
		// Indique qu'on n'a pas pu recevoir les informations de la partie pour le chargement
		if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)
		{
			// Vérifie d'abord que la déconnexion n'est pas volontaire : on se déconnecte volontairement du système lorsque les informations multijoueurs ont été reçues
			bool sendMessage = true;
			const core::list<RakNetGamePacket>::ConstIterator END = receivedPackets.end();
			for (core::list<RakNetGamePacket>::ConstIterator it = receivedPackets.begin(); it != END; ++it)
			{
				const E_GAME_PACKET_TYPE packetType = (*it).packetType;
				if (packetType == EGPT_MULTIPLAYER_GAME_DATA_RECEIVED || packetType == EGPT_MULTIPLAYER_GAME_DATA_FAILED)	// On vérifie aussi que le message d'échec n'a pas déjà été envoyé
				{
					sendMessage = false;
					break;
				}
			}
			if (sendMessage)
				receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));
		}

		if (currentNetworkState == ENGS_PLAYING_GAME_HOST)
		{
			LOG_RAKNET("A client has disconnected : Client address :" << packet->systemAddress.ToString());
		}
		else
		{
			LOG_RAKNET("We have been disconnected : Server address :" << packet->systemAddress.ToString());
		}
		break;
	case ID_CONNECTION_LOST:
		// Indique qu'on n'a pas pu recevoir les informations de la partie pour le chargement
		if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)
			receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));

		if (currentNetworkState == ENGS_PLAYING_GAME_HOST)
		{
			LOG_RAKNET("A client lost the connection : Client address : " << packet->systemAddress.ToString());
		}
		else
		{
			LOG_RAKNET("Connection lost : Server address : " << packet->systemAddress.ToString());
		}
		break;
	//case ID_ADVERTISE_SYSTEM:
	//	if (packet->guid != peer->GetMyGUID())	// Se connecte au système qui a envoyé l'avertissement
	//		peer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), 0, 0);
	//	break;

	default:
#if _RAKNET_SUPPORT_PacketLogger==1
		LOG_DEBUG("RakNetManager::receivePacket_RakNetSystem(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID << " (" << PacketLogger::BaseIDTOString(packetID) << ")", ELL_WARNING);
#else
		LOG_DEBUG("RakNetManager::receivePacket_RakNetSystem(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID, ELL_WARNING);
#endif
		break;
	}
}
void RakNetManager::receivePacket_SearchingGames(Packet* packet)
{
	// Gère le paquet différemment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_SENT_MULTIPLAYER_LIST_INFOS:
		// Traîte les informations sur cette partie multijoueur pour son affichage dans la liste
		receiveMultiplayerGameListInfos(packet);
		LOG_RAKNET("Received mutliplayer list infos from : " << packet->systemAddress.ToString());
		break;
	case ID_SENT_MULTIPLAYER_GAME_DATA:
		// Traîte les informations sur cette partie multijoueur pour son chargement
		receiveMultiplayerGameData(packet);
		LOG_RAKNET("Received mutliplayer game data from : " << packet->systemAddress.ToString());
		break;

	default:
#ifdef _DEBUG
		// Ce message peut aussi être une fausse alerte : lors du faible laps de temps où on est connecté au serveur de jeu,
		// on peut aussi recevoir les informations destinées au client actuels (par exemple : les données de construction d'un bâtiment)
		if (packetID != ID_SYNCHRONISE_GAME_TIME)
			LOG_DEBUG("RakNetManager::receivePacket_SearchingGames(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID << " (" << UserIDTOString(packetID) << ")", ELL_WARNING);
#endif
		break;
	}
}
void RakNetManager::receivePacket_PlayingGameHost(Packet* packet)
{
	// Gère le paquet différemment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
		// Paquets de jeu :
		// Note : l'ajout aux paquets reçu sera effectué lors de leur envoi aux clients (dans RakNetManager::sendMessages_PlayingGameHost)
	case ID_GAME_PAUSE_CHANGE_ASKING:
		sendPackets.push_back(receiveChangedGamePause(packet));
		break;
	case ID_GAME_SPEED_CHANGE_ASKING:
		{
			const RakNetGamePacket newPacket = receiveChangedGameSpeed(packet);
			if (checkCanChangeGameSpeed(newPacket))
				sendPackets.push_back(newPacket);
		}
		break;
	case ID_BATIMENT_CONSTRUCT_ASKING:
		{
			const RakNetGamePacket newPacket = receiveConstructedBatiment(packet);
			if (checkCanConstructBatiment(newPacket))
				sendPackets.push_back(newPacket);
		}
		break;
	case ID_BATIMENT_DESTROY_ASKING:
		{
			const RakNetGamePacket newPacket = receiveDestroyedBatiment(packet);
			if (checkCanDestroyBatiment(newPacket))
				sendPackets.push_back(newPacket);
		}
		break;
	case ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING:
		{
			const RakNetGamePacket newPacket = receiveChangedBatimentProductionPercentage(packet);
			if (checkCanChangeBatimentProductionPercentage(newPacket))
				sendPackets.push_back(newPacket);
		}
		break;


		// Paquets de chargement de partie multijoueur :
	case ID_QUERY_MULTIPLAYER_LIST_INFOS:
		// Envoie les informations sur cette partie multijoueur au client qui l'a demandé, pour l'affichage dans la liste
		sendMultiplayerGameListInfos(packet->systemAddress);
		LOG_RAKNET("Multiplayer list infos sent to : " << packet->systemAddress.ToString());
		break;
	case ID_QUERY_MULTIPLAYER_GAME_DATA:
		// Envoie les informations sur cette partie multijoueur au client qui l'a demandé, pour le chargement de la partie
		sendMultiplayerGameData(packet->systemAddress);
		LOG_RAKNET("Multiplayer game data sent to : " << packet->systemAddress.ToString());
		break;

	default:
		LOG_DEBUG("RakNetManager::receivePacket_PlayingGameHost(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID << " (" << UserIDTOString(packetID) << ")", ELL_WARNING);
		break;
	}
}
void RakNetManager::receivePacket_PlayingGameClient(Packet* packet)
{
	// Gère le paquet différemment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_SYNCHRONISE_GAME_TIME:
		// Modifie directement le temps du système de jeu :
		{
			BitStream* const bs = createBitStreamReader(packet);
			float totalTime;
			if (bs->Read(totalTime))
				game->system.synchroniseGameTime(totalTime);
			delete bs;
		}
		break;

	case ID_GAME_PAUSE_CHANGED:
		receivedPackets.push_back(receiveChangedGamePause(packet));
		break;
	case ID_GAME_SPEED_CHANGED:
		receivedPackets.push_back(receiveChangedGameSpeed(packet));
		break;
	case ID_WEATHER_CHOOSEN_NEW:
		receivedPackets.push_back(receiveChoosenNewWeather(packet));
		break;
	case ID_BATIMENT_CONSTRUCTED:
		receivedPackets.push_back(receiveConstructedBatiment(packet));
		break;
	case ID_BATIMENT_DESTROYED:
		receivedPackets.push_back(receiveDestroyedBatiment(packet));
		break;
	case ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED:
		receivedPackets.push_back(receiveChangedBatimentProductionPercentage(packet));
		break;

	default:
		LOG_DEBUG("RakNetManager::receivePacket_PlayingGameClient(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID << " (" << UserIDTOString(packetID) << ")", ELL_WARNING);
		break;
	}
}
void RakNetManager::sendMessages_SearchingGames()
{
	if (currentNetworkState != ENGS_SEARCHING_GAMES)
		return;

	// Envoie le message de Ping à toutes les machines du réseau (adresse de Broadcast) et par le port par défaut
	// pour connaître les machines qui hébergent des parties sur ce réseau local
	const TimeMS currentTime = GetTime();
	if (currentTime >= nextTimeSendMsg_searchGamesPing)
	{
		peer->Ping("255.255.255.255", DEFAULT_PORT, true);
		nextTimeSendMsg_searchGamesPing = currentTime + SEND_MSG_PERIOD_SEARCH_GAMES_PING;	// Envoie ce message toutes les 500 ms
	}



	// Efface tous les messages à envoyer vers le réseau car ils ne sont pas supportés ici
#ifdef _DEBUG
	const core::list<RakNetGamePacket>::ConstIterator END = sendPackets.end();
	for (core::list<RakNetGamePacket>::ConstIterator it = sendPackets.begin(); it != END; ++it)
		LOG_DEBUG("RakNetManager::sendMessages_SearchingGames() : Le type d'un packet de jeu à envoyer est inconnu : \r\n    packetType = " << (int)((*it).packetType), ELL_WARNING);
#endif
	sendPackets.clear();
}
void RakNetManager::sendMessages_PlayingGameHost()
{
	if (currentNetworkState != ENGS_PLAYING_GAME_HOST)
		return;

	// Envoie régulièrement le temps total du jeu vers tous les clients pour s'assurer qu'ils soient bien synchronisés :
	const TimeMS currentTime = GetTime();
	if (currentTime >= nextTimeSendMsg_synchronizeGameTime)
	{
		BitStream* const bs = createBitStreamWriter();
		bs->Write((MessageID)ID_SYNCHRONISE_GAME_TIME);
		bs->Write(game->system.getTime().getTotalTime());
		peer->Send(bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 1, UNASSIGNED_SYSTEM_ADDRESS, true);	// Temps de jeu : Ordering channel 1
		delete bs;

		nextTimeSendMsg_synchronizeGameTime = currentTime + SEND_MSG_PERIOD_SYNCHRONIZE_GAME_TIME;
	}



	// Gère les messages à envoyer vers le réseau :
	const core::list<RakNetGamePacket>::ConstIterator END = sendPackets.end();
	for (core::list<RakNetGamePacket>::ConstIterator it = sendPackets.begin(); it != END; ++it)
	{
		const RakNetGamePacket& packet = (*it);

		switch (packet.packetType)
		{
		case EGPT_GAME_PAUSE_CHANGED:
			sendChangedGamePause(packet);
			receivedPackets.push_back(packet);
			break;
		case EGPT_GAME_SPEED_CHANGED:
			sendChangedGameSpeed(packet);
			receivedPackets.push_back(packet);
			break;
		case EGPT_WEATHER_CHOOSEN_NEW:
			sendChoosenNewWeather(packet);
			//receivedPackets.push_back(packet);	// Pour le serveur, il est inutile de simuler un reçu de ce paquet, étant donné que les valeurs sont déjà inscrites dans le WeatherManager
			break;
		case EGPT_BATIMENT_CONSTRUCTED:
			sendConstructedBatiment(packet);
			receivedPackets.push_back(packet);
			break;
		case EGPT_BATIMENT_DESTROYED:
			sendDestroyedBatiment(packet);
			receivedPackets.push_back(packet);
			break;
		case EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED:
			sendChangedBatimentProductionPercentage(packet);
			receivedPackets.push_back(packet);
			break;

		default:
			LOG_DEBUG("RakNetManager::sendMessages_PlayingGameHost() : Le type d'un packet de jeu à envoyer est inconnu : \r\n    packetType = " << (int)(packet.packetType), ELL_WARNING);
			break;
		}
	}
	sendPackets.clear();
}
void RakNetManager::sendMessages_PlayingGameClient()
{
	if (currentNetworkState != ENGS_PLAYING_GAME_CLIENT)
		return;

	// Gère les messages à envoyer vers le réseau :
	const core::list<RakNetGamePacket>::ConstIterator END = sendPackets.end();
	for (core::list<RakNetGamePacket>::ConstIterator it = sendPackets.begin(); it != END; ++it)
	{
		const RakNetGamePacket& packet = (*it);

		switch (packet.packetType)
		{
		case EGPT_GAME_PAUSE_CHANGED:
			queryChangeGamePause(packet);
			break;
		case EGPT_GAME_SPEED_CHANGED:
			queryChangeGameSpeed(packet);
			break;
		case EGPT_BATIMENT_CONSTRUCTED:
			queryConstructBatiment(packet);
			break;
		case EGPT_BATIMENT_DESTROYED:
			queryDestroyBatiment(packet);
			break;
		case EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED:
			queryChangeBatimentProductionPercentage(packet);
			break;

		default:
			LOG_DEBUG("RakNetManager::sendMessages_PlayingGameClient() : Le type d'un packet de jeu à envoyer est inconnu : \r\n    packetType = " << (int)(packet.packetType), ELL_WARNING);
			break;
		}
	}
	sendPackets.clear();
}
void RakNetManager::queryMultiplayerGameListInfos(const SystemAddress& gameHost)
{
	if (currentNetworkState != ENGS_SEARCHING_GAMES)
		return;

	// Demande des informations sur cette partie à cet hôte, pour l'affichage dans la liste
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_QUERY_MULTIPLAYER_LIST_INFOS);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, gameHost, false);
	delete bs;
}
void RakNetManager::sendMultiplayerGameListInfos(const SystemAddress& gameClient)
{
	if (currentNetworkState != ENGS_PLAYING_GAME_HOST || !game)
		return;

	// Envoie les informations sur cette partie multijoueur :
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_SENT_MULTIPLAYER_LIST_INFOS);

	// terrainName
	{
		core::stringc terrainName;
		const core::stringc& rendererTerrainName = game->renderer->getTerrainManager().getTerrainInfos().terrainName;

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

		// Enlève l'extension ".ewt" du nom du terrain
		terrainName.remove(L".ewt");

		// Envoie le nom du terrain dans le paquet :
		const u8 terrainNameSize = (u8)min(terrainName.size(), UCHAR_MAX);
		bs->Write(terrainNameSize);																// Ecrit la taille du nom du terrain
		bs->WriteBits(reinterpret_cast<const u8*>(terrainName.c_str()), terrainNameSize * 8);	// Ecrit le nom du terrain (attention : unité de longueur en bits !)
	}

	// currentParticipants
	// Attention : dans cette valeur, le système "gameClient" à qui on envoie les données, qui nous est connecté, est aussi inclus !
	// (mais on ne fait pas "- 1" car on peut ainsi inclure aussi la machine hôte dans ce total)
	bs->Write((u8)peer->NumberOfConnections());

	// maxParticipants
	bs->Write((u8)(peer->GetMaximumIncomingConnections() + 1));	// + 1 car le serveur n'est pas inclus dans peer->GetMaximumIncomingConnections()

	// hostGameVersion
	{
		// Envoie notre version d'EcoWorld dans le paquet :
		const core::stringc hostGameVersion(ECOWORLD_VERSION);
		const u8 hostGameVersionSize = (u8)min(hostGameVersion.size(), UCHAR_MAX);
		bs->Write(hostGameVersionSize);																	// Ecrit la taille de la version d'EcoWorld
		bs->WriteBits(reinterpret_cast<const u8*>(hostGameVersion.c_str()), hostGameVersionSize * 8);	// Ecrit la version d'EcoWorld (attention : unité de longueur en Bits !)
	}

	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, gameClient, false);
	delete bs;
}
void RakNetManager::receiveMultiplayerGameListInfos(Packet* packet)
{
	if (currentNetworkState != ENGS_SEARCHING_GAMES)
		return;

	// Traîte les informations sur cette partie multijoueur :
	MultiplayerGameListInfos gameInfos;
	BitStream* const bs = createBitStreamReader(packet);
	u8 tmpU8 = 0;

	// hostIP
	gameInfos.hostIP = packet->systemAddress.ToString();

	// terrainName
	if (bs->Read(tmpU8))	// Obtient la taille du nom du terrain
	{
		const u8 terrainNameSize = tmpU8;
		char* tmpTerrainName = new char[terrainNameSize + 1];	// Taille + 1 pour pouvoir contenir le caractère de fin de chaîne '\0'
		if (tmpTerrainName)
		{
			// Obtient le nom du terrain (attention : unité de longueur en Bits !)
			if (bs->ReadBits(reinterpret_cast<u8*>(tmpTerrainName), terrainNameSize * 8))
			{
				tmpTerrainName[terrainNameSize] = '\0';	// Ajoute le caractère de fin de chaîne comme dernier caractère
				gameInfos.terrainName = tmpTerrainName;
			}

			delete[] tmpTerrainName;
		}
	}

	// currentParticipants
	if (bs->Read(tmpU8))
		gameInfos.currentParticipants = tmpU8;

	// maxParticipants
	if (bs->Read(tmpU8))
		gameInfos.maxParticipants = tmpU8;

	// hostGameVersion
	if (bs->Read(tmpU8))	// Obtient la taille de la version du jeu de l'hôte
	{
		const u8 hostGameVersionSize = tmpU8;
		char* tmpHostGameVerion = new char[hostGameVersionSize + 1];	// Taille + 1 pour pouvoir contenir le caractère de fin de chaîne '\0'
		if (tmpHostGameVerion)
		{
			// Obtient la version du jeu de l'hôte (attention : unité de longueur en Bits !)
			if (bs->ReadBits(reinterpret_cast<u8*>(tmpHostGameVerion), hostGameVersionSize * 8))
			{
				tmpHostGameVerion[hostGameVersionSize] = '\0';	// Ajoute le caractère de fin de chaîne comme dernier caractère
				gameInfos.hostGameVersion = tmpHostGameVerion;
			}

			delete[] tmpHostGameVerion;
		}
	}

	delete bs;
	multiplayerGames.push_back(gameInfos);

	// Indique qu'une nouvelle partie a été trouvée
	receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAMES_LIST_CHANGED));

	// Se déconnecte du système qui a envoyé ce paquet maintenant qu'on a reçu ses informations (évite qu'on ne les reçoive indéfiniment)
	peer->CloseConnection(packet->systemAddress, true);
}
void RakNetManager::queryMultiplayerGameData(const SystemAddress& gameHost)
{
	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)
		return;

	// Demande des informations sur cette partie à cet hôte, pour le chargement
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_QUERY_MULTIPLAYER_GAME_DATA);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, gameHost, false);
	delete bs;
}
void RakNetManager::sendMultiplayerGameData(const SystemAddress& gameClient)
{
	if (currentNetworkState != ENGS_PLAYING_GAME_HOST || !game)
		return;

	// Envoie les informations sur cette partie multijoueur :
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_SENT_MULTIPLAYER_GAME_DATA);

	// Crée une sauvegarde du jeu actuel et l'envoie au client
	if (CAN_WRITE_ON_DISK)	// Ecrit la sauvegarde actuelle sur le disque puis la relit et l'envoie au client
	{
		const io::path tmpPath("tmp_ServerSave.ewg");	// "tmp_MultiplayerServerSavedGame.ewg"

		LOG("Information : Enregistrement de la partie actuelle sur le disque dur (\"" << tmpPath.c_str() << "\") pour l'envoyer au client.", ELL_INFORMATION);

		// Vérifie que le fichier temporaire qu'on va créer n'existe pas, sinon les nouvelles données seront ajoutées à celles déjà existantes
		::remove(tmpPath.c_str());

		// Crée les données pour le fichier
		io::IWriteFile* const writeFile = game->fileSystem->createAndWriteFile(tmpPath);
		if (writeFile)
		{
			// Enregistre le jeu en mode "Efficace" et multijoueur
			game->saveCurrentGame_Eff(writeFile, true);

			// Ferme le fichier
			writeFile->drop();

			// Ouvre le fichier en mode lecture
			io::IReadFile* const readFile = game->fileSystem->createAndOpenFile(tmpPath);
			if (readFile)
			{
				const long fileLenght = readFile->getSize();
				if (fileLenght > 0)
				{
					char* gameSaveData = new char[fileLenght];
					if (gameSaveData)
					{
						readFile->read(gameSaveData, fileLenght);

						// Ajoute la sauvegarde du jeu au paquet
						bs->WriteAlignedBytes(reinterpret_cast<u8*>(gameSaveData), fileLenght);

						delete[] gameSaveData;
					}
				}
				readFile->drop();
			}

			// Supprime le fichier temporaire créé
			::remove(tmpPath.c_str());
		}
	}
	else					// Ecrit la sauvegarde actuelle dans la mémoire (taille limitée !), puis l'envoie au client
	{
#define SAVE_DATA_LENGHT	262144	// 256 Ko de mémoire max !
		LOG("Avertissement : Impossible d'enregistrer la partie actuelle sur le disque dur pour l'envoyer au client : Les droits d'écriture sur le disque ont été refusés !"
			<< "    Essai en mode mémoire seulement (" << SAVE_DATA_LENGHT << " octets max) !", ELL_WARNING);

		char* gameSaveData = new char[SAVE_DATA_LENGHT];
		if (gameSaveData)
		{
			io::IWriteFile* const writeFile = game->fileSystem->createMemoryWriteFile(gameSaveData, SAVE_DATA_LENGHT, "Multiplayer_ServeurSavedGame_Save");

			// Enregistre le jeu en mode "Efficace" et multijoueur
			game->saveCurrentGame_Eff(writeFile, true);

			// Ajoute la sauvegarde du jeu au paquet
			bs->WriteAlignedBytes(reinterpret_cast<u8*>(gameSaveData), strlen(gameSaveData));

			// Vérifie que toute la mémoire disponible n'a pas été utilisée, sinon il y a sûrement eu un manque de mémoire pour la sauvegarde
			const long writeFilePos = writeFile->getPos();
			if (writeFilePos >= 250000)
				LOG("Erreur : L'enregistrement en mode mémoire de la partie actuelle pour l'envoi vers le client a sûrement échoué : Octets utilisés : " << writeFilePos << " / " << SAVE_DATA_LENGHT << " !", ELL_ERROR);

			writeFile->drop();
			delete[] gameSaveData;
		}
	}

	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, gameClient, false);
	delete bs;
}
void RakNetManager::receiveMultiplayerGameData(Packet* packet)
{
	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)
		return;

	// Traîte les informations sur cette partie multijoueur :
	BitStream* const bs = createBitStreamReader(packet);
	const BitSize_t saveLenght = packet->length;

	// Vérifie que la taille de la sauvegarde est valide
	if (saveLenght < 2)
	{
		// Indique que les informations sur cette partie n'ont pas pu être reçues
		receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));

		delete bs;
		return;
	}

	// Obtient le texte de la sauvegarde envoyé
	char* gameSaveData = new char[saveLenght];
	if (gameSaveData)
	{
		if (bs->ReadAlignedBytes(reinterpret_cast<u8*>(gameSaveData), saveLenght - 1))
		{
			gameSaveData[saveLenght - 1] = '\0';	// Ajoute le caractère de fin de chaîne comme dernier caractère
		}
		else
		{
			delete[] gameSaveData;
			gameSaveData = NULL;
		}
	}
	delete bs;

	// Indique que les informations sur cette partie ont été reçues
	RakNetGamePacket newPacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED);
	if (gameSaveData)
	{
		newPacket.packetType = EGPT_MULTIPLAYER_GAME_DATA_RECEIVED;
		newPacket.gameSavedData.dataSize = saveLenght;
		newPacket.gameSavedData.data = gameSaveData;		// Attention : on ne libère pas gameSaveData, car on copie ici seulement son pointeur : il devra être libéré manuellement à la destruction de ce message
	}
	receivedPackets.push_back(newPacket);
}
void RakNetManager::queryChangeGamePause(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_PAUSE_CHANGE_ASKING);
	bs->Write(packet.gameIsPaused);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendChangedGamePause(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_PAUSE_CHANGED);
	bs->Write(packet.gameIsPaused);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveChangedGamePause(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	bool tmpBool, gameIsPaused = false;

	if (bs->Read(tmpBool))
		gameIsPaused = tmpBool;

	delete bs;
	RakNetGamePacket newPacket(EGPT_GAME_PAUSE_CHANGED);
	newPacket.gameIsPaused = gameIsPaused;
	return newPacket;
}
bool RakNetManager::checkCanChangeGameSpeed(const RakNetGamePacket& packet)
{
	// Valeurs déterminées d'après GUIManager::getGameGUITimerSpeed() :
	return (packet.gameSpeed >= 0.1f &&
		(packet.gameSpeed <= 10.0f || (game->guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getMax() > 6 && packet.gameSpeed <= 50.0f)));
}
void RakNetManager::queryChangeGameSpeed(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_SPEED_CHANGE_ASKING);
	bs->WriteFloat16(packet.gameSpeed, 0.1f, 50.0f);	// Valeurs déterminées d'après les valeurs maximales et minimales retournées par GUIManager::getGameGUITimerSpeed()
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendChangedGameSpeed(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_SPEED_CHANGED);
	bs->WriteFloat16(packet.gameSpeed, 0.1f, 50.0f);	// Valeurs déterminées d'après les valeurs maximales et minimales retournées par GUIManager::getGameGUITimerSpeed()
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveChangedGameSpeed(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	float tmpFloat, gameSpeed = 1.0f;

	if (bs->ReadFloat16(tmpFloat, 0.1f, 50.0f))			// Valeurs déterminées d'après les valeurs maximales et minimales retournées par GUIManager::getGameGUITimerSpeed()
		gameSpeed = tmpFloat;

	delete bs;
	RakNetGamePacket newPacket(EGPT_GAME_SPEED_CHANGED);
	newPacket.gameSpeed = gameSpeed;
	return newPacket;
}
void RakNetManager::sendChoosenNewWeather(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_WEATHER_CHOOSEN_NEW);
	bs->Write(packet.newChoosenWeatherInfos.weatherID);
	bs->Write(packet.newChoosenWeatherInfos.transitionBeginTime);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveChoosenNewWeather(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	WEATHER_ID_TYPE tmpId, weatherID = WI_sunny;
	float tmpFloat, transitionBeginTime = game->system.getTime().getTotalTime();

	if (bs->Read(tmpId))
		weatherID = tmpId;
	if (bs->Read(tmpFloat))
		transitionBeginTime = tmpFloat;

	delete bs;
	RakNetGamePacket newPacket(EGPT_WEATHER_CHOOSEN_NEW);
	newPacket.newChoosenWeatherInfos.weatherID = weatherID;
	newPacket.newChoosenWeatherInfos.transitionBeginTime = transitionBeginTime;
	return newPacket;
}
bool RakNetManager::checkCanConstructBatiment(const RakNetGamePacket& packet)
{
	return (((game->system.canCreateBatiment(
			(BatimentID)packet.gameConstructInfos.batimentID,
			core::vector2di(packet.gameConstructInfos.indexX, packet.gameConstructInfos.indexY),
			packet.gameConstructInfos.rotation))
		& ~EBCE_ENERGIE) == EBCE_AUCUNE);	// Le manque d'énergie n'est plus une erreur fatale
}
void RakNetManager::queryConstructBatiment(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_CONSTRUCT_ASKING);
	bs->Write(packet.gameConstructInfos.batimentID);
	bs->Write(packet.gameConstructInfos.indexX);
	bs->Write(packet.gameConstructInfos.indexY);

	// Répare la rotation du bâtiment pour qu'elle soit comprise entre 0.0f et 360.0f :
	float rot = packet.gameConstructInfos.rotation;
	while (rot < 0.0f)		rot += 360.0f;
	while (rot >= 360.0f)	rot -= 360.0f;
	bs->WriteFloat16(rot, 0.0f, 360.0f);

	bs->Write(packet.gameConstructInfos.dureeVie);

	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendConstructedBatiment(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_CONSTRUCTED);
	bs->Write(packet.gameConstructInfos.batimentID);
	bs->Write(packet.gameConstructInfos.indexX);
	bs->Write(packet.gameConstructInfos.indexY);

	// Répare la rotation du bâtiment pour qu'elle soit comprise entre 0.0f et 360.0f :
	float rot = packet.gameConstructInfos.rotation;
	while (rot < 0.0f)		rot += 360.0f;
	while (rot >= 360.0f)	rot -= 360.0f;
	bs->WriteFloat16(rot, 0.0f, 360.0f);

	bs->Write(packet.gameConstructInfos.dureeVie);

	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveConstructedBatiment(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	BATIMENT_ID_TYPE tmpID, batID = BI_aucun;
	INDEX_TYPE tmpIndex, indexX = 0, indexY = 0;
	float tmpFloat, rotation = 0.0f;
	u32 tmpU32, dureeVie = 1;

	if (bs->Read(tmpID))
		batID = tmpID;
	if (bs->Read(tmpIndex))
		indexX = tmpIndex;
	if (bs->Read(tmpIndex))
		indexY = tmpIndex;
	if (bs->ReadFloat16(tmpFloat, 0.0f, 360.0f))
		rotation = tmpFloat;
	if (bs->Read(tmpU32))
		dureeVie = tmpU32;

	delete bs;
	RakNetGamePacket newPacket(EGPT_BATIMENT_CONSTRUCTED);
	newPacket.gameConstructInfos.batimentID = batID;
	newPacket.gameConstructInfos.indexX = indexX;
	newPacket.gameConstructInfos.indexY = indexY;
	newPacket.gameConstructInfos.rotation = rotation;
	newPacket.gameConstructInfos.dureeVie = dureeVie;
	return newPacket;
}
bool RakNetManager::checkCanDestroyBatiment(const RakNetGamePacket& packet)
{
	return (game->system.canDestroyBatiment(core::vector2di(packet.gameDestroyInfos.indexX, packet.gameDestroyInfos.indexY)) == EBDE_AUCUNE);
}
void RakNetManager::queryDestroyBatiment(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_DESTROY_ASKING);
	bs->Write(packet.gameDestroyInfos.indexX);
	bs->Write(packet.gameDestroyInfos.indexY);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendDestroyedBatiment(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_DESTROYED);
	bs->Write(packet.gameDestroyInfos.indexX);
	bs->Write(packet.gameDestroyInfos.indexY);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveDestroyedBatiment(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	INDEX_TYPE tmpIndex, indexX = 0, indexY = 0;

	if (bs->Read(tmpIndex))
		indexX = tmpIndex;
	if (bs->Read(tmpIndex))
		indexY = tmpIndex;

	delete bs;
	RakNetGamePacket newPacket(EGPT_BATIMENT_DESTROYED);
	newPacket.gameDestroyInfos.indexX = indexX;
	newPacket.gameDestroyInfos.indexY = indexY;
	return newPacket;
}
bool RakNetManager::checkCanChangeBatimentProductionPercentage(const RakNetGamePacket& packet)
{
	const Batiment* const bat = game->system.carte[packet.gameBatPPChangeInfos.indexX][packet.gameBatPPChangeInfos.indexY].batiment;
	return (bat && StaticBatimentInfos::needPourcentageProduction(bat->getID()));
}
void RakNetManager::queryChangeBatimentProductionPercentage(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGE_ASKING);
	bs->Write(packet.gameBatPPChangeInfos.indexX);
	bs->Write(packet.gameBatPPChangeInfos.indexY);
	bs->WriteFloat16(packet.gameBatPPChangeInfos.newProductionPercentage, 0.0f, 1.0f);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendChangedBatimentProductionPercentage(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED);
	bs->Write(packet.gameBatPPChangeInfos.indexX);
	bs->Write(packet.gameBatPPChangeInfos.indexY);
	bs->WriteFloat16(packet.gameBatPPChangeInfos.newProductionPercentage, 0.0f, 1.0f);
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveChangedBatimentProductionPercentage(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	INDEX_TYPE tmpIndex, indexX = 0, indexY = 0;
	float tmpFloat, newPP = 1.0f;

	if (bs->Read(tmpIndex))
		indexX = tmpIndex;
	if (bs->Read(tmpIndex))
		indexY = tmpIndex;
	if (bs->Read(tmpFloat))
		newPP = tmpFloat;

	delete bs;
	RakNetGamePacket newPacket(EGPT_BATIMENT_PRODUCTION_PERCENTAGE_CHANGED);
	newPacket.gameBatPPChangeInfos.indexX = indexX;
	newPacket.gameBatPPChangeInfos.indexY = indexY;
	newPacket.gameBatPPChangeInfos.newProductionPercentage = newPP;
	return newPacket;
}
#endif
