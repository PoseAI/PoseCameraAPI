// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "ILiveLinkSource.h"
#include "LiveLinkSubjectSettings.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Json.h"
#include "PoseAIStructs.h"

#define LOCTEXT_NAMESPACE "PoseAI"

struct POSEAILIVELINK_API Remapping
{
	FName TargetJointName;
	FQuat RotAdj;
	Remapping(FName TargetJointName, FQuat RotAdj) : TargetJointName(TargetJointName), RotAdj(RotAdj) {};
};



/**
 * Abstract base class for the different rig formats streamable by Pose AI
 */
class POSEAILIVELINK_API PoseAIRig
{
  public:
	FLiveLinkStaticDataStruct rig;
	FLiveLinkStaticDataStruct MakeStaticData();
	bool ProcessFrame(const TSharedPtr<FJsonObject>, FLiveLinkAnimationFrameData& data);
	static bool IsFrameData(const TSharedPtr<FJsonObject> jsonObject);
	static TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> PoseAIRigFactory(const FLiveLinkSubjectName& name, const FPoseAIHandshake& handshake);
	FName RigType() { return rigType; }

	FPoseAIVisibilityFlags visibilityFlags;
    FPoseAILiveValues liveValues;
	FPoseAIScalarStruct scalars;
	FPoseAIEventStruct events;
  
  protected:
	FPoseAIVerbose verbose;
	static const FString fieldBody;
	static const FString fieldRigType;
	static const FString fieldHandLeft;
	static const FString fieldHandRight;
	static const FString fieldRotations;
	static const FString fieldEvents;
	static const FString fieldScalars;
	static const FString fieldVectors;
	
  protected:
	PoseAIRig(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake);
	virtual ~PoseAIRig() {
	};

	/* sets up skeletal heirarchy and provides default locations for each transform based on default skeleton (i.e. UE4 Mannequen or male Metahuman).
	   These bone lengths ensure a sensible animation is created even if user does not retarget from livelink */
	virtual void Configure(); //impure for MacOS compatibility
	
	FLiveLinkSubjectName name;
	FName rigType;
	bool useRootMotion;
	bool includeHands;
	bool isMirrored;
	bool isDesktop;
	int32 numBodyJoints = 21;
	int32 numHandJoints = 17;
	// number of joints to insert in desktop mode (as camera omits quaternions for unused joints)
	int32 lowerBodyNumOfJoints = 8; 

		
	bool isCrouching = false;
	int32 handZoneL = 5;
	int32 handZoneR = 5;
	int32 stableFeet = 0;

	// store translations of deployed rig
	TMap<FName, FVector> boneVectors;
	TArray<FName> jointNames;
	TArray<int32> parentIndices;
	TArray<FTransform> cachedPose = {};
	
	//temporary variable used for convenience in rig construction
	FName lastBoneAdded;

	//ankle to head top height for scaling PoseAI root motion.
	float rigHeight = 170.0f;
	
	//extra offset for hip bone to accomodate mesh thickness from bone sockets.
	float rootHipOffsetZ = 1.0f;

	void AddBone(FName boneName, FName parentName, FVector translation);
	void AddBoneToLast(FName boneName, FVector translation);
	void CachePose(const TArray<FTransform>& transforms);
	void AppendQuatArray(const TArray<FQuat>& quatArray, int32 begin, TArray<FQuat>& componentRotations, FLiveLinkAnimationFrameData& data);
	void AppendCachedRotations(int32 begin, int32 end, TArray<FQuat>& componentRotations, FLiveLinkAnimationFrameData& data);
	bool ProcessVerboseRotations(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data);
	bool ProcessCompactRotations(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data);
	void ProcessVerboseSupplementaryData(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data);
	void ProcessCompactSupplementaryData(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data);
	void TriggerEvents();
	void AssignCharacterMotion(FLiveLinkAnimationFrameData& data);
};

class POSEAILIVELINK_API PoseAIRigUE4 : public PoseAIRig {
  public:
	PoseAIRigUE4(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake) : PoseAIRig(name, handshake) {};
protected:
	void Configure();
};

class POSEAILIVELINK_API PoseAIRigMixamo : public PoseAIRig {
  public:
	PoseAIRigMixamo(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake) :	PoseAIRig(name, handshake) {};
protected:
	void Configure();
};

class POSEAILIVELINK_API PoseAIRigMetaHuman : public PoseAIRig {
public:
	PoseAIRigMetaHuman(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake) : PoseAIRig(name, handshake) {};
protected:
	void Configure();
};

class POSEAILIVELINK_API PoseAIRigDazUE : public PoseAIRig {
public:
	PoseAIRigDazUE(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake) : PoseAIRig(name, handshake) {};
protected:
	void Configure();
};