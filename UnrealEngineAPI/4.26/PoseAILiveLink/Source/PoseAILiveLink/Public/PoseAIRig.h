// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "ILiveLinkSource.h"
#include "LiveLinkSubjectSettings.h"
#include "Json.h"


#define LOCTEXT_NAMESPACE "PoseAI"


/**
 * Abstract base class for the different rig formats streamable by Pose AI
 */
class POSEAILIVELINK_API PoseAIRig
{
  public:

	FLiveLinkStaticDataStruct rig;
	FLiveLinkStaticDataStruct MakeStaticData();
	bool ProcessFrame(const TSharedPtr<FJsonObject>, FLiveLinkFrameDataStruct& frameData);
	static bool IsFrameData(const TSharedPtr<FJsonObject> jsonObject);
	static TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> PoseAIRigFactory(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop);

  protected:
	static const FString fieldBody;
	static const FString fieldRigType;
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

  protected:
	PoseAIRig(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop);
	virtual ~PoseAIRig() {};

	/* sets up skeletal heirarchy and provides default locations for each transform based on default skeleton (i.e. UE4 Mannequen or male Metahuman).
	   These bone lengths ensure a sensible animation is created even if user does not retarget from livelink */
	virtual void Configure()=0;
	

	FName rigType;
	bool useRootMotion;
	bool includeHands;
	bool isMirrored;
	bool isDesktop;
	int32 numBodyJoints = 21;
	int32 leftShoulderJoint = 15;
	int32 rightShoulderJoint = 18;
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
	
	void AddBone(FName boneName, FName parentName, FVector translation);
	void AddBoneToLast(FName boneName, FVector translation);
	void CachePose(const TArray<FTransform>& transforms);

};

class POSEAILIVELINK_API PoseAIRigUE4 : public PoseAIRig {
  public:
	PoseAIRigUE4(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop) :
		PoseAIRig(rigType, useRootMotion, includeHands, isMirrored, isDesktop) {};
  private:
	void Configure();
};

class POSEAILIVELINK_API PoseAIRigMixamo : public PoseAIRig {
  public:
	PoseAIRigMixamo(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop) :
		PoseAIRig(rigType, useRootMotion, includeHands, isMirrored, isDesktop) {};
private:
	void Configure();
};

class POSEAILIVELINK_API PoseAIRigMetaHuman : public PoseAIRig {
public:
	PoseAIRigMetaHuman(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop) :
		PoseAIRig(rigType, useRootMotion, includeHands, isMirrored, isDesktop) {};
private:
	void Configure();
};
