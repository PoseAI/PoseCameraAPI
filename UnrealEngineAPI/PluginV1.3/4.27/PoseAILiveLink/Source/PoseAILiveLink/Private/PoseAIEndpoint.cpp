// Copyright 2021 Pose AI Ltd. .

#include "PoseAIEndpoint.h"


ISocketSubsystem* FPoseAIEndpoint::CachedSocketSubsystem = nullptr;

TSharedPtr<FSocket> BuildUdpSocket(FString& description, FName protocolType, int32 port) {

	FName socketType = NAME_DGram;
	FSocket* socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(socketType, description, protocolType);
	socket->SetNonBlocking();
	socket->SetReuseAddr();
	int actualSize;
	socket->SetReceiveBufferSize(64 * 1024, actualSize);
	socket->SetSendBufferSize(64 * 1024, actualSize);

	TSharedRef<FInternetAddr> sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(protocolType);
	sender->SetIp(0);
	sender->SetPort(port);
	socket->Bind(*sender);
	return MakeShareable(socket);
}


FString FPoseAIEndpoint::ToString() const
{
	return Address->ToString(true);
}


void FPoseAIEndpoint::Initialize()
{
	CachedSocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
}


