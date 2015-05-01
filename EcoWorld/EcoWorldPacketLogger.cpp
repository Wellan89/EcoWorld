#include "global.h"

#ifdef USE_RAKNET

#include "EcoWorldPacketLogger.h"
#include <RakPeerInterface.h>
#include <InternalPacket.h>

#if _RAKNET_SUPPORT_PacketLogger==1

void EcoWorldPacketLogger::LogHeader()
{
	// Last 5 are splitpacket id, split packet index, split packet count, ordering index, suffix
	AddToLog("Clock,S|R,Typ,Reliable#,Frm #,PktID,BitLn,Time     ,Local IP:Port   ,RemoteIP:Port,SPID,SPIN,SPCO,OI,Suffix,Miscellaneous\n");
}
void EcoWorldPacketLogger::FormatLine(char* into, const char* dir, const char* type, unsigned int reliableMessageNumber, unsigned int frame, unsigned char id,
	const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
	unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex)
{
	const char* idToPrint = NULL;
	if (printId)
	{
		if ((splitPacketCount > 0) && (splitPacketCount != (unsigned int)(-1)))
			idToPrint = "(SPLIT PACKET)";
		else
			idToPrint =	IDTOString(id);
	}

	// If printId is false, idToPrint will be NULL, as it will
	// in the case of an unrecognized id. Testing printId for false
	// would just be redundant.
	if (idToPrint == NULL)
	{
		sprintf_SS("%5u", id);
		idToPrint = text_SS;
	}

	FormatLine(into, dir, type, reliableMessageNumber, frame, idToPrint, bitLen, time, local, remote,splitPacketId,splitPacketIndex,splitPacketCount, orderingIndex);
}
void EcoWorldPacketLogger::FormatLine(char* into, const char* dir, const char* type, unsigned int reliableMessageNumber, unsigned int frame, const char* idToPrint,
	const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
	unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex)
{
	char str1[64], str2[62];
	local.ToString(true, str1);
	remote.ToString(true, str2);
	char localtime[128];
	GetLocalTime(localtime);
	char str3[64];
	if (reliableMessageNumber == (unsigned int)(-1))
	{
		str3[0]='N';
		str3[1]='/';
		str3[2]='A';
		str3[3]=0;
	}
	else
		sprintf_s(str3,64,"%5u",reliableMessageNumber);

	sprintf_s(into, 256, "%s,%s%s,%s,%s,%5u,%s,%u,%"PRINTF_64_BIT_MODIFIER"u,%s,%s,%i,%i,%i,%i,%s,"
					, localtime
					, prefix
					, dir
					, type
					, str3
					, frame
					, idToPrint
					, bitLen
					, time
					, str1
					, str2
					, splitPacketId
					, splitPacketIndex
					, splitPacketCount
					, orderingIndex
					, suffix
					);
}
void EcoWorldPacketLogger::OnDirectSocketSend(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	if (!logDirectMessages)
		return;

#ifdef _DEBUG
	// Pour faciliter le débogage réseau : évite le log de paquets envoyés régulièrement
	if (data[0] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS || data[0] == ID_UNCONNECTED_PONG || data[0] == RakNetManager::ID_SYNCHRONISE_GAME_TIME)
		return;
#endif

	char str[256];
	FormatLine(str, "Snd", "Raw", 0, 0, data[0], bitsUsed, RakNet::GetTimeMS(), rakPeerInterface->GetExternalID(remoteSystemAddress), remoteSystemAddress, (unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}
void EcoWorldPacketLogger::OnDirectSocketReceive(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	if (!logDirectMessages)
		return;

#ifdef _DEBUG
	// Pour faciliter le débogage réseau : évite le log de paquets envoyés régulièrement
	if (data[0] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS || data[0] == ID_UNCONNECTED_PONG || data[0] == RakNetManager::ID_SYNCHRONISE_GAME_TIME)
		return;
#endif

	char str[256];
	FormatLine(str, "Rcv", "Raw", 0, 0, data[0], bitsUsed, RakNet::GetTime(), rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS), remoteSystemAddress,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}
void EcoWorldPacketLogger::OnReliabilityLayerPacketError(const char* errorMessage, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	char str[1024];
	FormatLine(str, "RcvErr", errorMessage, 0, 0, "", bitsUsed, RakNet::GetTime(), rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS), remoteSystemAddress,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}
void EcoWorldPacketLogger::OnAck(unsigned int messageNumber, SystemAddress remoteSystemAddress, RakNet::TimeMS time)
{
	// Pour faciliter le débogage réseau : évite le log des paquets de confirmation de réception
#ifndef _DEBUG
	char str[256];
	char str1[64], str2[62];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);
	localSystemAddress.ToString(true, str1);
	remoteSystemAddress.ToString(true, str2);
	char localtime[128];
	GetLocalTime(localtime);

	sprintf_s(str, 256, "%s,Rcv,Ack,%i,,,,%"PRINTF_64_BIT_MODIFIER"u,%s,%s,,,,,,"
					, localtime
					, messageNumber
					, (unsigned long long) time
					, str1
					, str2
					);
	AddToLog(str);
#endif
}
void EcoWorldPacketLogger::OnPushBackPacket(const char* data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	char str[256];
	char str1[64], str2[62];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);
	localSystemAddress.ToString(true, str1);
	remoteSystemAddress.ToString(true, str2);
	RakNet::TimeMS time = RakNet::GetTimeMS();
	char localtime[128];
	GetLocalTime(localtime);

	sprintf_s(str, 256, "%s,Lcl,PBP,,,%s,%i,%"PRINTF_64_BIT_MODIFIER"u,%s,%s,,,,,,"
					, localtime
					, BaseIDTOString(data[0])
					, bitsUsed
					, (unsigned long long) time
					, str1
					, str2
					);
	AddToLog(str);
}
void EcoWorldPacketLogger::OnInternalPacket(InternalPacket *internalPacket, unsigned frameNumber, SystemAddress remoteSystemAddress, RakNet::TimeMS time, int isSend)
{
	char str[256];
	const char* sendTypes[] =
	{
		"Rcv",
		"Snd",
		"Err1",
		"Err2",
		"Err3",
		"Err4",
		"Err5",
		"Err6",
	};
	const char* sendType = sendTypes[isSend];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);

	unsigned int reliableMessageNumber;
	if (internalPacket->reliability==UNRELIABLE || internalPacket->reliability==UNRELIABLE_SEQUENCED || internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT)
		reliableMessageNumber = (unsigned int)(-1);
	else
		reliableMessageNumber = internalPacket->reliableMessageNumber;

#ifdef _DEBUG
	// Pour faciliter le débogage réseau : évite le log de paquets envoyés régulièrement
	if (internalPacket->data[0] == ID_UNCONNECTED_PING_OPEN_CONNECTIONS || internalPacket->data[0] == ID_UNCONNECTED_PONG || internalPacket->data[0] == RakNetManager::ID_SYNCHRONISE_GAME_TIME)
		return;
#endif

	if (internalPacket->data[0] == ID_TIMESTAMP)
		FormatLine(str, sendType, "Tms", reliableMessageNumber, frameNumber, internalPacket->data[1+sizeof(RakNet::Time)], internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);
	else
		FormatLine(str, sendType, "Nrm", reliableMessageNumber, frameNumber, internalPacket->data[0], internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);

	AddToLog(str);
}
void EcoWorldPacketLogger::WriteMiscellaneous(const char* type, const char* msg)
{
	char str[1024];
	char str1[64];
	SystemAddress localSystemAddress = rakPeerInterface->GetInternalID();
	localSystemAddress.ToString(true, str1);
	RakNet::TimeMS time = RakNet::GetTimeMS();
	char localtime[128];
	GetLocalTime(localtime);

	sprintf_s(str, 1024, "%s,Lcl,%s,,,,,%"PRINTF_64_BIT_MODIFIER"u,%s,,,,,,,%s"
					, localtime
					, type
					, (unsigned long long) time
					, str1
					, msg
					);

	AddToLog(msg);
}

#endif
#endif
