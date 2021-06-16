// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "ILiveLinkSource.h"
#include "LiveLinkSubjectSettings.h"
#include "Json.h"


#define LOCTEXT_NAMESPACE "PoseAI"

constexpr int32 kPoseAILeftShoulderJoint = 15;
constexpr int32 kPoseAIRightShoulderJoint = 18;
/**
 * 
 */
class POSEAILIVELINK_API PoseAIRig
{
  public:
	PoseAIRig(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop);
	FLiveLinkStaticDataStruct rig;
	FLiveLinkStaticDataStruct MakeStaticData();
	bool ProcessFrame(const TSharedPtr<FJsonObject>, FLiveLinkFrameDataStruct& frameData);
	static bool IsFrameData(const TSharedPtr<FJsonObject> jsonObject);


  private:
	static const FString fieldBody;
	static const FString fieldHandLeft;
	static const FString fieldHandRight;
	static const FString fieldRotations;
	static const FString fieldScalars;
	static const FString fieldVectors;
	static const FString fieldHipScreen;
	static const FString fieldBodyHeight;
	static const FString fieldStableFoot;
	static const FString fieldShrugLeft;
	static const FString fieldShrugRight;

	FName rigType;
	bool useRootMotion;
	bool includeHands;
	bool isMirrored;
	bool isDesktop;
	// store translations of deployed rig
	TMap<FName, FVector> boneVectors;
	TArray<FName> jointNames;
	TArray<int32> parentIndices;
	TArray<FTransform> cachedPose = {};
	FVector shrugVector = FVector::ZeroVector;
	
	//temporary variable used for convenience in rig construction
	FName lastBoneAdded;

	//ankle to head top height for scaling PoseAI root motion.
	float rigHeight = 170.0f;

	//cached adjustment calculated when pose is stable to handle jumping.
	float verticalAdjustment = 0.0f;
	
	FLiveLinkStaticDataStruct MakeUE4Rig();
	FLiveLinkStaticDataStruct MakeMixamoRig();

	void AddBone(FName boneName, FName parentName, FVector translation);
	void AddBoneToLast(FName boneName, FVector translation);
	void CachePose(const TArray<FTransform>& transforms);

	
};
