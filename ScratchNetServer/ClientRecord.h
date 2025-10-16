#pragma once
#include <Address.h>
#include <ScratchAck.h>
#include <SnaphotRecordKeeper.h>
#include <unordered_map>

/// <summary>
/// upkeep of all client related data 
/// </summary>
class ClientRecord
{
public:
	Address* clientAddress;
	ScratchAck* packetAckMaintence;
	SnapshotRecordKeeper* clientSSRecordKeeper;
	std::unordered_map<int, Snapshot> networkedObjects;

	ClientRecord()
	{
		//initialization of variables 
		clientAddress = CreateAddress();
		packetAckMaintence = GenerateScratchAck();
		clientSSRecordKeeper = InitRecordKeeper();
	}

	ClientRecord(Address* inputtedAddress)
	{
		//initialization of variables 
		clientAddress = inputtedAddress;
		packetAckMaintence = GenerateScratchAck();
		clientSSRecordKeeper = InitRecordKeeper();
	}

	bool TryInsertNewNetworkObject(int objectID, Snapshot SnapshotToAdd);
	bool TryUpdatingNetworkedObject(int objectID, Snapshot SnapshotToUpdate);

	//to ensure we can perform searches for this class when we need to look up a client 
	bool operator== (const ClientRecord other)
	{
		return clientAddress == other.clientAddress;
	}
};