// Copyright Pose AI Ltd 2023.  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "ILiveLinkClient.h"
#include "LiveLinkClient.h"
#include "LiveLinkSourceSettings.h"
#include "LiveLinkSubjectSettings.h"
#include "LiveLinkFrameInterpolationProcessor.h"
#include "LiveLinkFramePreProcessor.h"
#include "LiveLinkFrameTranslator.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "LiveLinkTypes.h"
#include "LiveLinkLog.h"
#include "Json.h"



/**
 *A child object for the LiveLink sources to manage the face animation as a supplementary livelink subject
 */
class POSEAILIVELINK_API PoseAILiveLinkFaceSubSource
{

public:
	/* Prefer using the AddSource method for setup */
	PoseAILiveLinkFaceSubSource(FLiveLinkSubjectKey& poseSubjectKey, ILiveLinkClient* liveLinkClient);
	bool AddSubject(FCriticalSection& InSynchObject);
	bool RequestSubSourceShutdown();
	void UpdateFace(TSharedPtr<FJsonObject> jsonPose);

private:

	FLiveLinkSubjectKey subjectKey;
	FName subjectName = "FacePoseAI"; // will be overwritten on initialization
	ILiveLinkClient* liveLinkClient = nullptr;
	FLiveLinkSkeletonStaticData StaticData;
};


UENUM(BlueprintType, Category = "PoseAI Animation", meta = (Experimental))
enum class PoseAIFaceBlendShape : uint8
{
	// Left eye blend shapes
	EyeBlinkLeft,
	EyeLookDownLeft,
	EyeLookInLeft,
	EyeLookOutLeft,
	EyeLookUpLeft,
	EyeSquintLeft,
	EyeWideLeft,
	// Right eye blend shapes
	EyeBlinkRight,
	EyeLookDownRight,
	EyeLookInRight,
	EyeLookOutRight,
	EyeLookUpRight,
	EyeSquintRight,
	EyeWideRight,
	// Jaw blend shapes
	JawForward,
	JawLeft,
	JawRight,
	JawOpen,
	// Mouth blend shapes
	MouthClose,
	MouthFunnel,
	MouthPucker,
	MouthLeft,
	MouthRight,
	MouthSmileLeft,
	MouthSmileRight,
	MouthFrownLeft,
	MouthFrownRight,
	MouthDimpleLeft,
	MouthDimpleRight,
	MouthStretchLeft,
	MouthStretchRight,
	MouthRollLower,
	MouthRollUpper,
	MouthShrugLower,
	MouthShrugUpper,
	MouthPressLeft,
	MouthPressRight,
	MouthLowerDownLeft,
	MouthLowerDownRight,
	MouthUpperUpLeft,
	MouthUpperUpRight,
	// Brow blend shapes
	BrowDownLeft,
	BrowDownRight,
	BrowInnerUp,
	BrowOuterUpLeft,
	BrowOuterUpRight,
	// Cheek blend shapes
	CheekPuff,
	CheekSquintLeft,
	CheekSquintRight,
	// Nose blend shapes
	NoseSneerLeft,
	NoseSneerRight,
	TongueOut,
	MAX
};

