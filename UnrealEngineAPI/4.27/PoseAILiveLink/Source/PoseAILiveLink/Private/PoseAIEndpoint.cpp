// Copyright 2021 Pose AI Ltd. .

#include "PoseAIEndpoint.h"


ISocketSubsystem* FPoseAIEndpoint::CachedSocketSubsystem = nullptr;


FString FPoseAIEndpoint::ToString() const
{
	return Address->ToString(true);
}


void FPoseAIEndpoint::Initialize()
{
	CachedSocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
}


