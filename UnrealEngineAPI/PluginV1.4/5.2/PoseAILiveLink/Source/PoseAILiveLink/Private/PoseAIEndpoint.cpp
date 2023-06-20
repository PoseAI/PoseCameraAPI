// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAIEndpoint.h"
#define LOCTEXT_NAMESPACE "PoseAI"


ISocketSubsystem* FPoseAIEndpoint::CachedSocketSubsystem = nullptr;

TSharedPtr<FSocket> BuildUdpSocket(FString& description, FName protocolType, int32 port) {

	FName socketType = NAME_DGram;
	FSocket* socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(socketType, description, protocolType);
	if (!socket->SetNonBlocking(true)) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI Could not set socket to non-blocking"));

	}
	
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

#undef LOCTEXT_NAMESPACE

