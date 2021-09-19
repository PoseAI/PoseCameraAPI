// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "PoseAIStructs.h"
#include "PoseAIEventDispatcher.generated.h"


#define LOCTEXT_NAMESPACE "PoseAI"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIVisibilityChange, FName, Name, FPoseAIVisibilityFlags, Flags);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAILiveValuesUpdate, FName, Name, FPoseAILiveValues, LiveValues);

/**
 * subsystem to transmit events to blueprints and c++
 */
UCLASS(Blueprintable)
class POSEAILIVELINK_API UPoseAIEventDispatcher : public UObject //: public UEngineSubsystem
{
GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "PoseAI Events")
    static UPoseAIEventDispatcher* GetDispatcher() {
        if (theInstance==nullptr) {
            theInstance = NewObject<UPoseAIEventDispatcher>();
            theInstance->AddToRoot();
            UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: Creating EventDispatcher."));

        }
        return theInstance;
    }

  

    

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIVisibilityChange visibilityChange;
    
    void BroadcastVisibilityChange(FName& rigName, FPoseAIVisibilityFlags& visibilityFlags);
    
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAILiveValuesUpdate liveValues;
    
    void BroadcastLiveValues(FName& rigName, FPoseAILiveValues& values);

private:
    static UPoseAIEventDispatcher* theInstance;
    UPoseAIEventDispatcher() : UObject() {};
};
