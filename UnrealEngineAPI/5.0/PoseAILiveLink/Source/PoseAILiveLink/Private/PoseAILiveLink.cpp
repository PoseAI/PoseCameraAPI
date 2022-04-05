// Copyright Pose AI Ltd 2021

#include "PoseAILiveLink.h"

//#define LOCTEXT_NAMESPACE "FPoseAILiveLinkModule"

void FPoseAILiveLinkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FPoseAILiveLinkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

//#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPoseAILiveLinkModule, PoseAILiveLink)
