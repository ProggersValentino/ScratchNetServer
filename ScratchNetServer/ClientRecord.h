#pragma once
#include <Address.h>
#include <ScratchAck.h>
#include <SnaphotRecordKeeper.h>

/// <summary>
/// upkeep of all client related data 
/// </summary>
class ClientRecord
{
public:
	Address* clientAddress;
	ScratchAck* packetAckMaintence;
	SnapshotRecordKeeper* clientSSRecordKeeper;

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

	//to ensure we can perform searches for this class when we need to look up a client 
	bool operator== (const ClientRecord other)
	{
		return clientAddress == other.clientAddress;
	}
};