// Copyright Pose AI Ltd 2021

#include "PoseAIEventDispatcher.h"


void UPoseAIEventDispatcher::BroadcastVisibilityChange(FName& rigName, FPoseAIVisibilityFlags& visibilityFlags){
    visibilityChange.Broadcast(rigName, visibilityFlags);
};

void UPoseAIEventDispatcher::BroadcastLiveValues(FName& rigName, FPoseAILiveValues& values){
    liveValues.Broadcast(rigName, values);
};
