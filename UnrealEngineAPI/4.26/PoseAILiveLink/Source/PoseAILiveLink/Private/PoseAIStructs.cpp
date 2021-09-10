// Copyright Pose AI Ltd 2021

#include "PoseAIStructs.h"


void ProcessFieldAsBool(const TSharedPtr < FJsonObject >* jsonObj, const FString& fieldName, bool& fieldBool){
    double value;
    if ((*jsonObj)->TryGetNumberField(fieldName, value)){
        fieldBool = value > 0.5;
    }
}

void ProcessFieldAsFloat(const TSharedPtr < FJsonObject >* jsonObj, const FString& fieldName, float& fieldFloat){
    double value;
    if ((*jsonObj)->TryGetNumberField(fieldName, value)){
        fieldFloat = (float) value;
    }
}

void ProcessFieldAsVector2D(const TSharedPtr < FJsonObject >* jsonObj, const FString& fieldName, FVector2D& fieldVector2D){
    const TArray < TSharedPtr < FJsonValue > >* value;
    if ((*jsonObj)->TryGetArrayField(fieldName, value)){
        fieldVector2D.X = (*value)[0]->AsNumber();
        fieldVector2D.Y = (*value)[1]->AsNumber();

    }
}


const FString FPoseAIVisibilityFlags::fieldLeftArm = FString(TEXT("VisArmL"));
const FString FPoseAIVisibilityFlags::fieldRightArm = FString(TEXT("VisArmR"));
const FString FPoseAIVisibilityFlags::fieldLeftLeg = FString(TEXT("VisLegL"));
const FString FPoseAIVisibilityFlags::fieldRightLeg = FString(TEXT("VisLegR"));
const FString FPoseAIVisibilityFlags::fieldTorso = FString(TEXT("VisTorso"));

void FPoseAIVisibilityFlags::ProcessUpdate(const TSharedPtr < FJsonObject >* scaBody){
    hasChanged = false;
    if (scaBody==nullptr)
        return;
    ProcessField(scaBody, fieldTorso, isTorso);
    ProcessField(scaBody, fieldLeftLeg, isLeftLeg);
    ProcessField(scaBody, fieldRightLeg, isRightLeg);
    ProcessField(scaBody, fieldLeftArm, isLeftArm);
    ProcessField(scaBody, fieldRightArm, isRightArm);
}

void FPoseAIVisibilityFlags::ProcessField(const TSharedPtr < FJsonObject >* scaBody, const FString& fieldName, bool& fieldBool){
    double value;
    if ((*scaBody)->TryGetNumberField(fieldName, value)){
        bool boolValue = value > 0.5;
        if (boolValue != fieldBool)
            hasChanged = true;
        fieldBool = boolValue;
    }
}

const FString FPoseAILiveValues::fieldBodyHeight = FString(TEXT("BodyHeight"));
const FString FPoseAILiveValues::fieldStanceYaw = FString(TEXT("StanceYaw"));
const FString FPoseAILiveValues::fieldStableFoot = FString(TEXT("StableFoot"));
const FString FPoseAILiveValues::fieldFullLean = FString(TEXT("FullLean"));
const FString FPoseAILiveValues::fieldUpperLean = FString(TEXT("HipLean"));
const FString FPoseAILiveValues::fieldHipScreen = FString(TEXT("HipScreen"));
const FString FPoseAILiveValues::fieldChestScreen = FString(TEXT("ChestScreen"));
const FString FPoseAILiveValues::fieldPointScreen = FString(TEXT("PointScreen"));


void FPoseAILiveValues::ProcessUpdateScalarsBody(const TSharedPtr < FJsonObject >* scaBody){
    if (scaBody==nullptr)
        return;
    ProcessFieldAsBool(scaBody, fieldStableFoot, isStableFoot);
    ProcessFieldAsFloat(scaBody, fieldStanceYaw, stanceYaw);
    ProcessFieldAsFloat(scaBody, fieldBodyHeight, bodyHeight);
}

void FPoseAILiveValues::ProcessUpdateVectorsBody(const TSharedPtr < FJsonObject >* vecBody){
    if (vecBody==nullptr)
        return;
    ProcessFieldAsVector2D(vecBody, fieldFullLean, fullBodyLean);
    ProcessFieldAsVector2D(vecBody, fieldUpperLean, upperBodyLean);
    ProcessFieldAsVector2D(vecBody, fieldHipScreen, hipScreen);
    ProcessFieldAsVector2D(vecBody, fieldChestScreen, chestScreen);
}

void FPoseAILiveValues::ProcessUpdateVectorsHandLeft(const TSharedPtr < FJsonObject >* vecHand){
    if (vecHand==nullptr)
        return;
    ProcessFieldAsVector2D(vecHand, fieldPointScreen, pointHandLeft);
}

void FPoseAILiveValues::ProcessUpdateVectorsHandRight(const TSharedPtr < FJsonObject >* vecHand){
    if (vecHand==nullptr)
        return;
    ProcessFieldAsVector2D(vecHand, fieldPointScreen, pointHandRight);
}

