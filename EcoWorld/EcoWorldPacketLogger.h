#ifndef ECO_WORLD_PACKET_LOGGER
#define ECO_WORLD_PACKET_LOGGER

#include "global.h"

#ifdef USE_RAKNET

#include <NativeFeatureIncludes.h>
#if _RAKNET_SUPPORT_PacketLogger==1

#include "RakNetManager.h"

#include <PacketLogger.h>
using namespace RakNet;

// Classe de log pour les paquets de RakNet : écrit les textes de log dans le logger d'EcoWorld au lieu de les écrire dans la console
// Cette classe est basée sur la classe PacketLogger de RakNet 4.020
// Modification par rapport au logger de base de RakNet : affichage des informations sous forme de tableau

/*	Format actuel :

	Comma delimited log format:
	1. Send or receive,
	2. Raw (direct socket send) OR Ack (Acknowledgement) OR Tms (Timestamped packet),
	3. Message number,
	4. Packet Number (Independent for send & receive). (Each Packet may contain multiple messages),
	5. Packet ID (or a string for RPC calls),
	6. Bits used by the message (does not include 2-4 byte RakNet header),
	7. Time the message is sent,
	8. Local System (binary IP followed by port),
	9. Remote System (binary IP followed by port).
*/

class EcoWorldPacketLogger : public PacketLogger
{
public:
	// Constructeur
	EcoWorldPacketLogger()			{ }

	// Translate the supplied parameters into an output line - overloaded version that takes a MessageIdentifier
	// and translates it into a string (numeric or textual representation based on printId); this calls the
	// second version which takes a const char* argument for the messageIdentifier
	virtual void FormatLine(char* into, const char* dir, const char* type, unsigned int reliableMessageNumber, unsigned int frame,
		unsigned char messageIdentifier, const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
		unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex);
	virtual void FormatLine(char* into, const char* dir, const char* type, unsigned int reliableMessageNumber, unsigned int frame,
		const char* idToPrint, const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
		unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex);

	/// Events on low level sends and receives.  These functions may be called from different threads at the same time.
	virtual void OnDirectSocketSend(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress);
	virtual void OnDirectSocketReceive(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress);
	virtual void OnReliabilityLayerPacketError(const char* errorMessage, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress);
	virtual void OnInternalPacket(InternalPacket *internalPacket, unsigned frameNumber, SystemAddress remoteSystemAddress, RakNet::TimeMS time, int isSend);
	virtual void OnAck(unsigned int messageNumber, SystemAddress remoteSystemAddress, RakNet::TimeMS time);
	virtual void OnPushBackPacket(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress);

	/// Logs out a header for all the data
	virtual void LogHeader();

	// Write informational messages
	virtual void WriteMiscellaneous(const char* type, const char* msg);

	// Ecrit un texte de log dans le logger d'EcoWorld :
	// Override this to log strings to wherever. Log should be threadsafe
	virtual void WriteLog(const char* str)
	{
		// Les logs des paquets ne sont gérés qu'en mode de log ELL_DEBUG
		LOG("RkMgr: Packet Logger : " << str, ELL_DEBUG);
	}

protected:
	// Retourne l'ID d'un paquet utilisateur sous forme de chaîne de caractères
	virtual const char* UserIDTOString(unsigned char Id)
	{
		return RakNetManager::UserIDTOString(Id);
	}
};

#endif
#endif
#endif
