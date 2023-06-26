// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLink.h"
#include "Core.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "PoseAI"

void FPoseAILiveLinkModule::StartupModule()
{

}

void FPoseAILiveLinkModule::ShutdownModule()
{
	
}


//#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPoseAILiveLinkModule, PoseAILiveLink)
