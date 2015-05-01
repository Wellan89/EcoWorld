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

// Macro de LOG personnalis�e pour le d�bogage du module r�seau (log avec la priorit� ELL_INFORMATION et le pr�fixe du RakNetManager : RkMgr) :
#define LOG_RAKNET(text)	LOG("RkMgr: " << text, ELL_INFORMATION)

RakNetManager::RakNetManager() : peer(NULL), packetLogger(NULL), currentNetworkState(ENGS_NOT_ENABLED),
	nextTimeSendMsg_searchGamesPing(0), nextTimeSendMsg_synchronizeGameTime(0)
{
	// R�initialise RakNet
	reset();
}
RakNetManager::~RakNetManager()
{
	if (peer)
	{
		// Attend 200 ms au maximum pour l'envoi des derniers messages (s'il y en a)
		peer->Shutdown(200);

		// D�truit l'instance de RakNet
		RakPeerInterface::DestroyInstance(peer);

		//LOG_RAKNET("RakNet destroyed");	// D�sactiv� : A cet instant, le logger d'EcoWorld peut avoir �t� d�truit : cela provoque actuellement des bugs en mdoe Release
	}

#if _RAKNET_SUPPORT_PacketLogger==1
	if (packetLogger)
		delete packetLogger;
#endif
}
bool RakNetManager::init()
{
	// V�rifie que RakNet n'est pas d�j� en train de tourner
	if (peer && peer->IsActive())
		return false;

	// Initialise l'instance principale de RakNet
	if (!peer)
		peer = RakPeerInterface::GetInstance();
	if (!peer)
		LOG("RkMgr: ERROR : Could not create RakNet instance !", ELL_ERROR);

#if _RAKNET_SUPPORT_PacketLogger==1
	// Initialise le PacketLogger si n�cessaire (lorsque le mode de log est ELL_DEBUG)
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

	// On d�marre RakNet en mode serveur d�s le d�but : on peut ainsi �tre utilis� comme serveur ou client :
	u16 i = 0;
	StartupResult result;
	do
	{
		// D�marre RakNet avec diff�rents ports cons�cutifs s'il n'a pas pu �tre d�marr� avec un port bien pr�cis
		// (permet de lancer EcoWorld plusieurs fois en mode r�seau sur la m�me machine)
		const u16 port = DEFAULT_PORT + i; ++i;
		result = peer->Startup(MAX_CLIENTS, &SocketDescriptor(port, 0), 1);

		LOG_RAKNET("Started result : " << result << " (Port : " << port << ")");

	} while (result != RAKNET_STARTED && i <= 8);	// 8 ports cons�cutifs test�s au maximum

	return (result != RAKNET_STARTED);	// V�rifie que l'instance de RakNet a bien pu d�marrer
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

	// Indique le nouvel �tat de RakNet
	currentNetworkState = ENGS_NOT_ENABLED;

	LOG_RAKNET("RakNet stopped");
}
bool RakNetManager::searchGames()
{
	// Arr�te l'action en cours et r�initialise RakNet si n�cessaire
	if (currentNetworkState == ENGS_SEARCHING_GAMES)		return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Indique le nouvel �tat de RakNet
	currentNetworkState = ENGS_SEARCHING_GAMES;

	LOG_RAKNET("New mode : Searching games");

	return false;
}
bool RakNetManager::queryGameData(const char* hostIP)
{
	// Arr�te l'action en cours et r�initialise RakNet si n�cessaire
	if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)	return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Indique le nouvel �tat de RakNet
	currentNetworkState = ENGS_WAITING_FOR_GAME_DATA;

	// Demande la connexion � cet h�te
	const SystemAddress hostAddress(hostIP);
	const ConnectionAttemptResult connectionResult = peer->Connect(hostIP, hostAddress.GetPort(), 0, 0);

	LOG_RAKNET("New mode : Querying game data (Server address : " << hostIP << " ; Connection result : " << connectionResult << ")");

	// Retourne si cette connexion a r�ussie
	return (connectionResult == INVALID_PARAMETER || connectionResult == CANNOT_RESOLVE_DOMAIN_NAME || connectionResult == SECURITY_INITIALIZATION_FAILED);
}
bool RakNetManager::createGame()
{
	// Arr�te l'action en cours et r�initialise RakNet si n�cessaire
	if (currentNetworkState == ENGS_PLAYING_GAME_HOST)		return false;
	if (currentNetworkState != ENGS_NOT_ENABLED)			reset();
	if (init())												return true;

	// Permet la connection d'autres machines � la notre
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);

	// Indique le nouvel �tat de RakNet
	currentNetworkState = ENGS_PLAYING_GAME_HOST;

	LOG_RAKNET("New mode : Server (creating game)");

	return false;
}
bool RakNetManager::joinGame()
{
	// V�rifie qu'on n'est pas d�j� en train de jouer en mode client
	if (currentNetworkState == ENGS_PLAYING_GAME_CLIENT)	return false;

	// Normalement, la pr�c�dente �tape �tait d�termin�e par l'appel � RakNetManager::queryGameData(hostIP) !
	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)	return true;

	// Normalement, RakNet est d�j� connect� � cet h�te :
	// par un appel pr�c�dent � RakNetManager::queryGameData(hostIP) pour obtenir les informations de chargement de la partie

	// V�rifie qu'on est d�j� connect� � un h�te, et un seul :
	if (peer->NumberOfConnections() != 1)					return true;

	// Indique le nouvel �tat de RakNet
	currentNetworkState = ENGS_PLAYING_GAME_CLIENT;

	LOG_RAKNET("New mode : Client (joining game)");

	return false;
}
void RakNetManager::update()
{
	if (!peer || !peer->IsActive())
		return;

	// Re�ois et tra�te tout d'abord les messages en file d'attente :
	// Parcours tous les paquets re�us et les tra�te
	for (Packet* packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		// D�termine � quelle fonction de r�ception des paquets on doit envoyer celui-ci :
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
		// D�termine quelle fonction d'envoi des messages on doit appeler :
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
	// G�re le paquet diff�remment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
		LOG_RAKNET("Pong response sent to : " << packet->systemAddress.ToString());
		break;
	case ID_UNCONNECTED_PONG:	// R�ception d'un pong d'un syst�me non connect�
		LOG_RAKNET("Pong response received from : " << packet->systemAddress.ToString());

		if (currentNetworkState == ENGS_SEARCHING_GAMES)	// Ajoute cette machine � la liste des parties multijoueurs
		{
			// Parcours la liste des messages de RakNet pour v�rifier que cette machine n'est pas d�j� dans la liste des parties
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
				// Se connecte � l'h�te de cette partie pour pouvoir lui demander les informations sur cette partie multijoueurs plus tard
				const ConnectionAttemptResult connectionResult = peer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), 0, 0);

				LOG_RAKNET("Attempting to connect to : " << packet->systemAddress.ToString() << " ; Connection result : " << connectionResult);
			}
		}
		break;

		// IDs du syst�me de RakNet :
	case ID_CONNECTION_REQUEST_ACCEPTED:	// Notre requ�te de connexion a �t� accept�e
		LOG_RAKNET("Connection request accepted : Server address : " << packet->systemAddress.ToString());
	case ID_ALREADY_CONNECTED:
		if (packetID == ID_ALREADY_CONNECTED)
			LOG_RAKNET("Already connected to system");

		if (currentNetworkState == ENGS_SEARCHING_GAMES)			// Demande au nouveau syst�me connect� les informations sur sa partie pour son affichage dans la liste
		{
			// Envoie une demande de description des parties multijoueurs � cet h�te pour l'affichage dans la liste
			queryMultiplayerGameListInfos(packet->systemAddress);
		}
		else if (currentNetworkState == ENGS_WAITING_FOR_GAME_DATA)	// Demande au nouveau syst�me connect� les informations sur sa partie pour le chargement
		{
			// Envoie une demande de description des parties multijoueurs � cet h�te pour le chargement
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
			// V�rifie d'abord que la d�connexion n'est pas volontaire : on se d�connecte volontairement du syst�me lorsque les informations multijoueurs ont �t� re�ues
			bool sendMessage = true;
			const core::list<RakNetGamePacket>::ConstIterator END = receivedPackets.end();
			for (core::list<RakNetGamePacket>::ConstIterator it = receivedPackets.begin(); it != END; ++it)
			{
				const E_GAME_PACKET_TYPE packetType = (*it).packetType;
				if (packetType == EGPT_MULTIPLAYER_GAME_DATA_RECEIVED || packetType == EGPT_MULTIPLAYER_GAME_DATA_FAILED)	// On v�rifie aussi que le message d'�chec n'a pas d�j� �t� envoy�
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
	//	if (packet->guid != peer->GetMyGUID())	// Se connecte au syst�me qui a envoy� l'avertissement
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
	// G�re le paquet diff�remment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_SENT_MULTIPLAYER_LIST_INFOS:
		// Tra�te les informations sur cette partie multijoueur pour son affichage dans la liste
		receiveMultiplayerGameListInfos(packet);
		LOG_RAKNET("Received mutliplayer list infos from : " << packet->systemAddress.ToString());
		break;
	case ID_SENT_MULTIPLAYER_GAME_DATA:
		// Tra�te les informations sur cette partie multijoueur pour son chargement
		receiveMultiplayerGameData(packet);
		LOG_RAKNET("Received mutliplayer game data from : " << packet->systemAddress.ToString());
		break;

	default:
#ifdef _DEBUG
		// Ce message peut aussi �tre une fausse alerte : lors du faible laps de temps o� on est connect� au serveur de jeu,
		// on peut aussi recevoir les informations destin�es au client actuels (par exemple : les donn�es de construction d'un b�timent)
		if (packetID != ID_SYNCHRONISE_GAME_TIME)
			LOG_DEBUG("RakNetManager::receivePacket_SearchingGames(" << packet << ") : L'ID d'un paquet est inconnu :\r\n    packetID = " << (int)packetID << " (" << UserIDTOString(packetID) << ")", ELL_WARNING);
#endif
		break;
	}
}
void RakNetManager::receivePacket_PlayingGameHost(Packet* packet)
{
	// G�re le paquet diff�remment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
		// Paquets de jeu :
		// Note : l'ajout aux paquets re�u sera effectu� lors de leur envoi aux clients (dans RakNetManager::sendMessages_PlayingGameHost)
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
		// Envoie les informations sur cette partie multijoueur au client qui l'a demand�, pour l'affichage dans la liste
		sendMultiplayerGameListInfos(packet->systemAddress);
		LOG_RAKNET("Multiplayer list infos sent to : " << packet->systemAddress.ToString());
		break;
	case ID_QUERY_MULTIPLAYER_GAME_DATA:
		// Envoie les informations sur cette partie multijoueur au client qui l'a demand�, pour le chargement de la partie
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
	// G�re le paquet diff�remment suivant son ID :
	const unsigned char packetID = getPacketID(packet);
	switch (packetID)
	{
	case ID_SYNCHRONISE_GAME_TIME:
		// Modifie directement le temps du syst�me de jeu :
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

	// Envoie le message de Ping � toutes les machines du r�seau (adresse de Broadcast) et par le port par d�faut
	// pour conna�tre les machines qui h�bergent des parties sur ce r�seau local
	const TimeMS currentTime = GetTime();
	if (currentTime >= nextTimeSendMsg_searchGamesPing)
	{
		peer->Ping("255.255.255.255", DEFAULT_PORT, true);
		nextTimeSendMsg_searchGamesPing = currentTime + SEND_MSG_PERIOD_SEARCH_GAMES_PING;	// Envoie ce message toutes les 500 ms
	}



	// Efface tous les messages � envoyer vers le r�seau car ils ne sont pas support�s ici
#ifdef _DEBUG
	const core::list<RakNetGamePacket>::ConstIterator END = sendPackets.end();
	for (core::list<RakNetGamePacket>::ConstIterator it = sendPackets.begin(); it != END; ++it)
		LOG_DEBUG("RakNetManager::sendMessages_SearchingGames() : Le type d'un packet de jeu � envoyer est inconnu : \r\n    packetType = " << (int)((*it).packetType), ELL_WARNING);
#endif
	sendPackets.clear();
}
void RakNetManager::sendMessages_PlayingGameHost()
{
	if (currentNetworkState != ENGS_PLAYING_GAME_HOST)
		return;

	// Envoie r�guli�rement le temps total du jeu vers tous les clients pour s'assurer qu'ils soient bien synchronis�s :
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



	// G�re les messages � envoyer vers le r�seau :
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
			//receivedPackets.push_back(packet);	// Pour le serveur, il est inutile de simuler un re�u de ce paquet, �tant donn� que les valeurs sont d�j� inscrites dans le WeatherManager
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
			LOG_DEBUG("RakNetManager::sendMessages_PlayingGameHost() : Le type d'un packet de jeu � envoyer est inconnu : \r\n    packetType = " << (int)(packet.packetType), ELL_WARNING);
			break;
		}
	}
	sendPackets.clear();
}
void RakNetManager::sendMessages_PlayingGameClient()
{
	if (currentNetworkState != ENGS_PLAYING_GAME_CLIENT)
		return;

	// G�re les messages � envoyer vers le r�seau :
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
			LOG_DEBUG("RakNetManager::sendMessages_PlayingGameClient() : Le type d'un packet de jeu � envoyer est inconnu : \r\n    packetType = " << (int)(packet.packetType), ELL_WARNING);
			break;
		}
	}
	sendPackets.clear();
}
void RakNetManager::queryMultiplayerGameListInfos(const SystemAddress& gameHost)
{
	if (currentNetworkState != ENGS_SEARCHING_GAMES)
		return;

	// Demande des informations sur cette partie � cet h�te, pour l'affichage dans la liste
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

		// Enl�ve l'extension ".ewt" du nom du terrain
		terrainName.remove(L".ewt");

		// Envoie le nom du terrain dans le paquet :
		const u8 terrainNameSize = (u8)min(terrainName.size(), UCHAR_MAX);
		bs->Write(terrainNameSize);																// Ecrit la taille du nom du terrain
		bs->WriteBits(reinterpret_cast<const u8*>(terrainName.c_str()), terrainNameSize * 8);	// Ecrit le nom du terrain (attention : unit� de longueur en bits !)
	}

	// currentParticipants
	// Attention : dans cette valeur, le syst�me "gameClient" � qui on envoie les donn�es, qui nous est connect�, est aussi inclus !
	// (mais on ne fait pas "- 1" car on peut ainsi inclure aussi la machine h�te dans ce total)
	bs->Write((u8)peer->NumberOfConnections());

	// maxParticipants
	bs->Write((u8)(peer->GetMaximumIncomingConnections() + 1));	// + 1 car le serveur n'est pas inclus dans peer->GetMaximumIncomingConnections()

	// hostGameVersion
	{
		// Envoie notre version d'EcoWorld dans le paquet :
		const core::stringc hostGameVersion(ECOWORLD_VERSION);
		const u8 hostGameVersionSize = (u8)min(hostGameVersion.size(), UCHAR_MAX);
		bs->Write(hostGameVersionSize);																	// Ecrit la taille de la version d'EcoWorld
		bs->WriteBits(reinterpret_cast<const u8*>(hostGameVersion.c_str()), hostGameVersionSize * 8);	// Ecrit la version d'EcoWorld (attention : unit� de longueur en Bits !)
	}

	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, gameClient, false);
	delete bs;
}
void RakNetManager::receiveMultiplayerGameListInfos(Packet* packet)
{
	if (currentNetworkState != ENGS_SEARCHING_GAMES)
		return;

	// Tra�te les informations sur cette partie multijoueur :
	MultiplayerGameListInfos gameInfos;
	BitStream* const bs = createBitStreamReader(packet);
	u8 tmpU8 = 0;

	// hostIP
	gameInfos.hostIP = packet->systemAddress.ToString();

	// terrainName
	if (bs->Read(tmpU8))	// Obtient la taille du nom du terrain
	{
		const u8 terrainNameSize = tmpU8;
		char* tmpTerrainName = new char[terrainNameSize + 1];	// Taille + 1 pour pouvoir contenir le caract�re de fin de cha�ne '\0'
		if (tmpTerrainName)
		{
			// Obtient le nom du terrain (attention : unit� de longueur en Bits !)
			if (bs->ReadBits(reinterpret_cast<u8*>(tmpTerrainName), terrainNameSize * 8))
			{
				tmpTerrainName[terrainNameSize] = '\0';	// Ajoute le caract�re de fin de cha�ne comme dernier caract�re
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
	if (bs->Read(tmpU8))	// Obtient la taille de la version du jeu de l'h�te
	{
		const u8 hostGameVersionSize = tmpU8;
		char* tmpHostGameVerion = new char[hostGameVersionSize + 1];	// Taille + 1 pour pouvoir contenir le caract�re de fin de cha�ne '\0'
		if (tmpHostGameVerion)
		{
			// Obtient la version du jeu de l'h�te (attention : unit� de longueur en Bits !)
			if (bs->ReadBits(reinterpret_cast<u8*>(tmpHostGameVerion), hostGameVersionSize * 8))
			{
				tmpHostGameVerion[hostGameVersionSize] = '\0';	// Ajoute le caract�re de fin de cha�ne comme dernier caract�re
				gameInfos.hostGameVersion = tmpHostGameVerion;
			}

			delete[] tmpHostGameVerion;
		}
	}

	delete bs;
	multiplayerGames.push_back(gameInfos);

	// Indique qu'une nouvelle partie a �t� trouv�e
	receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAMES_LIST_CHANGED));

	// Se d�connecte du syst�me qui a envoy� ce paquet maintenant qu'on a re�u ses informations (�vite qu'on ne les re�oive ind�finiment)
	peer->CloseConnection(packet->systemAddress, true);
}
void RakNetManager::queryMultiplayerGameData(const SystemAddress& gameHost)
{
	if (currentNetworkState != ENGS_WAITING_FOR_GAME_DATA)
		return;

	// Demande des informations sur cette partie � cet h�te, pour le chargement
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

	// Cr�e une sauvegarde du jeu actuel et l'envoie au client
	if (CAN_WRITE_ON_DISK)	// Ecrit la sauvegarde actuelle sur le disque puis la relit et l'envoie au client
	{
		const io::path tmpPath("tmp_ServerSave.ewg");	// "tmp_MultiplayerServerSavedGame.ewg"

		LOG("Information : Enregistrement de la partie actuelle sur le disque dur (\"" << tmpPath.c_str() << "\") pour l'envoyer au client.", ELL_INFORMATION);

		// V�rifie que le fichier temporaire qu'on va cr�er n'existe pas, sinon les nouvelles donn�es seront ajout�es � celles d�j� existantes
		::remove(tmpPath.c_str());

		// Cr�e les donn�es pour le fichier
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

			// Supprime le fichier temporaire cr��
			::remove(tmpPath.c_str());
		}
	}
	else					// Ecrit la sauvegarde actuelle dans la m�moire (taille limit�e !), puis l'envoie au client
	{
#define SAVE_DATA_LENGHT	262144	// 256 Ko de m�moire max !
		LOG("Avertissement : Impossible d'enregistrer la partie actuelle sur le disque dur pour l'envoyer au client : Les droits d'�criture sur le disque ont �t� refus�s !"
			<< "    Essai en mode m�moire seulement (" << SAVE_DATA_LENGHT << " octets max) !", ELL_WARNING);

		char* gameSaveData = new char[SAVE_DATA_LENGHT];
		if (gameSaveData)
		{
			io::IWriteFile* const writeFile = game->fileSystem->createMemoryWriteFile(gameSaveData, SAVE_DATA_LENGHT, "Multiplayer_ServeurSavedGame_Save");

			// Enregistre le jeu en mode "Efficace" et multijoueur
			game->saveCurrentGame_Eff(writeFile, true);

			// Ajoute la sauvegarde du jeu au paquet
			bs->WriteAlignedBytes(reinterpret_cast<u8*>(gameSaveData), strlen(gameSaveData));

			// V�rifie que toute la m�moire disponible n'a pas �t� utilis�e, sinon il y a s�rement eu un manque de m�moire pour la sauvegarde
			const long writeFilePos = writeFile->getPos();
			if (writeFilePos >= 250000)
				LOG("Erreur : L'enregistrement en mode m�moire de la partie actuelle pour l'envoi vers le client a s�rement �chou� : Octets utilis�s : " << writeFilePos << " / " << SAVE_DATA_LENGHT << " !", ELL_ERROR);

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

	// Tra�te les informations sur cette partie multijoueur :
	BitStream* const bs = createBitStreamReader(packet);
	const BitSize_t saveLenght = packet->length;

	// V�rifie que la taille de la sauvegarde est valide
	if (saveLenght < 2)
	{
		// Indique que les informations sur cette partie n'ont pas pu �tre re�ues
		receivedPackets.push_back(RakNetGamePacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED));

		delete bs;
		return;
	}

	// Obtient le texte de la sauvegarde envoy�
	char* gameSaveData = new char[saveLenght];
	if (gameSaveData)
	{
		if (bs->ReadAlignedBytes(reinterpret_cast<u8*>(gameSaveData), saveLenght - 1))
		{
			gameSaveData[saveLenght - 1] = '\0';	// Ajoute le caract�re de fin de cha�ne comme dernier caract�re
		}
		else
		{
			delete[] gameSaveData;
			gameSaveData = NULL;
		}
	}
	delete bs;

	// Indique que les informations sur cette partie ont �t� re�ues
	RakNetGamePacket newPacket(EGPT_MULTIPLAYER_GAME_DATA_FAILED);
	if (gameSaveData)
	{
		newPacket.packetType = EGPT_MULTIPLAYER_GAME_DATA_RECEIVED;
		newPacket.gameSavedData.dataSize = saveLenght;
		newPacket.gameSavedData.data = gameSaveData;		// Attention : on ne lib�re pas gameSaveData, car on copie ici seulement son pointeur : il devra �tre lib�r� manuellement � la destruction de ce message
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
	// Valeurs d�termin�es d'apr�s GUIManager::getGameGUITimerSpeed() :
	return (packet.gameSpeed >= 0.1f &&
		(packet.gameSpeed <= 10.0f || (game->guiManager->guiElements.gameGUI.vitesseJeuScrollBar->getMax() > 6 && packet.gameSpeed <= 50.0f)));
}
void RakNetManager::queryChangeGameSpeed(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_SPEED_CHANGE_ASKING);
	bs->WriteFloat16(packet.gameSpeed, 0.1f, 50.0f);	// Valeurs d�termin�es d'apr�s les valeurs maximales et minimales retourn�es par GUIManager::getGameGUITimerSpeed()
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
void RakNetManager::sendChangedGameSpeed(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_GAME_SPEED_CHANGED);
	bs->WriteFloat16(packet.gameSpeed, 0.1f, 50.0f);	// Valeurs d�termin�es d'apr�s les valeurs maximales et minimales retourn�es par GUIManager::getGameGUITimerSpeed()
	peer->Send(bs, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	delete bs;
}
RakNetManager::RakNetGamePacket RakNetManager::receiveChangedGameSpeed(Packet* packet)
{
	BitStream* const bs = createBitStreamReader(packet);
	float tmpFloat, gameSpeed = 1.0f;

	if (bs->ReadFloat16(tmpFloat, 0.1f, 50.0f))			// Valeurs d�termin�es d'apr�s les valeurs maximales et minimales retourn�es par GUIManager::getGameGUITimerSpeed()
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
		& ~EBCE_ENERGIE) == EBCE_AUCUNE);	// Le manque d'�nergie n'est plus une erreur fatale
}
void RakNetManager::queryConstructBatiment(const RakNetGamePacket& packet)
{
	BitStream* const bs = createBitStreamWriter();
	bs->Write((MessageID)ID_BATIMENT_CONSTRUCT_ASKING);
	bs->Write(packet.gameConstructInfos.batimentID);
	bs->Write(packet.gameConstructInfos.indexX);
	bs->Write(packet.gameConstructInfos.indexY);

	// R�pare la rotation du b�timent pour qu'elle soit comprise entre 0.0f et 360.0f :
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

	// R�pare la rotation du b�timent pour qu'elle soit comprise entre 0.0f et 360.0f :
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
