// Copyright 2021 Pose AI Ltd. .

#include "PoseAIEndpoint.h"

/* helper function to generate address*/
TSharedRef<FInternetAddr> GetAny() {
	FName protocolType = FNetworkProtocolTypes::IPv6;
	TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(protocolType);
	Sender->SetIp(0);
	Sender->SetPort(0);
	return Sender;
}

const FPoseAIEndpoint FPoseAIEndpoint::Any(GetAny());
ISocketSubsystem* FPoseAIEndpoint::CachedSocketSubsystem = nullptr;


FString FPoseAIEndpoint::ToString() const
{
	return Address->ToString(true);
}


void FPoseAIEndpoint::Initialize()
{
	CachedSocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
}


