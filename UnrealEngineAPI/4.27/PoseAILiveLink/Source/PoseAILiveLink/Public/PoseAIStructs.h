// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "PoseAIStructs.generated.h"


#define LOCTEXT_NAMESPACE "PoseAI"

/**
 * structure to store and expose visibility flags for events alerting programmer if subject is out of camera
 */
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIVisibilityFlags
{
GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Flags")
    bool isTorso = false;
    
    UPROPERTY(BlueprintReadOnly, Category="Flags")
    bool isLeftArm = false;
    
    UPROPERTY(BlueprintReadOnly, Category="Flags")
    bool isRightArm = false;
    
    UPROPERTY(BlueprintReadOnly, Category="Flags")
    bool isLeftLeg = false;
    
    UPROPERTY(BlueprintReadOnly, Category="Flags")
    bool isRightLeg = false;
    
    
    bool HasChanged(){return hasChanged;}
    void ProcessUpdate(const TSharedPtr < FJsonObject >* scaBody);
    
private:
    static const FString fieldTorso;
    static const FString fieldLeftLeg;
    static const FString fieldRightLeg;
    static const FString fieldLeftArm;
    static const FString fieldRightArm;
    bool hasChanged = false;
    void ProcessField(const TSharedPtr < FJsonObject >* scaBody, const FString& fieldName, bool& fieldBool);

};


/**
 * structure to share additional information
 */
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAILiveValues
{
GENERATED_BODY()
public:
    /** Heading of flattened left foot to right foot vector relative to camera */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    float stanceYaw = 0.0f;
    
    /** How much subject is leaning in radians, head-to-feet; x to the side, y forward */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D fullBodyLean = FVector2D(0.0f, 0.0f);
    
    /** How much subject is leaning in radians, head-to-hips; x to the side, y forward */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D upperBodyLean = FVector2D(0.0f, 0.0f);
    
    /** estimated height of the subject in clip coordinates (2.0 = full height of image) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    float bodyHeight = 0.0f;
   
    /** location of hips in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D hipScreen = FVector2D(0.0f, 0.0f);
    
    /** location of chest in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D chestScreen = FVector2D(0.0f, 0.0f);
   
    /** location of left index finger in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D pointHandLeft = FVector2D(0.0f, 0.0f);
    
    /** location of right index finger in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D pointHandRight = FVector2D(0.0f, 0.0f);
    
    /** true if at least one foot has not recently moved in the camera frame */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    bool isStableFoot = false;
    
    void ProcessUpdateScalarsBody(const TSharedPtr < FJsonObject >* scaBody);
    void ProcessUpdateVectorsBody(const TSharedPtr < FJsonObject >* vecBody);
    void ProcessUpdateVectorsHandLeft(const TSharedPtr < FJsonObject >* vecHand);
    void ProcessUpdateVectorsHandRight(const TSharedPtr < FJsonObject >* vecHand);

private:
    static const FString fieldBodyHeight;
    static const FString fieldStanceYaw;
    static const FString fieldStableFoot;
    static const FString fieldFullLean;
    static const FString fieldUpperLean;
    static const FString fieldHipScreen;
    static const FString fieldChestScreen;
    static const FString fieldPointScreen;
};
