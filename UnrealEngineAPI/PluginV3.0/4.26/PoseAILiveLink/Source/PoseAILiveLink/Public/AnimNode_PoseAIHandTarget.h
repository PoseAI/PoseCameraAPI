// Copyright 2022 Pose AI Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"

#include "AnimNode_PoseAIHandTarget.generated.h"


/**
 *	Debugging node that displays the current value of a bone in a specific space.
 */
USTRUCT()
struct POSEAILIVELINK_API FAnimNode_PoseAIHandTarget : public FAnimNode_TwoBoneIK
{
	GENERATED_USTRUCT_BODY()
		/** Name of bone to control. This is the main bone chain to modify from. **/
		UPROPERTY(EditAnywhere, Category = IK)
		FBoneReference SpineFirst;

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category = IK)
		FBoneReference LeftUpperArm;

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category = IK)
		FBoneReference RightUpperArm;
	
	// for now we hide this feature as it can create unwelcome jumps in wrist position
	/** If specified, will use index finger tip for solution. **/
	UPROPERTY()
		FBoneReference UseIndexFingerTip;


	/** Special IK control info from PoseAI movement component. This is NOT a location vector. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effector, meta = (PinShownByDefault))
		FVector PoseAiIkVector = FVector::ZeroVector;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex;
	FCompactPoseBoneIndex SpineFirstIndex;
	FCompactPoseBoneIndex LeftUpperArmIndex;
	FCompactPoseBoneIndex RightUpperArmIndex;
	FCompactPoseBoneIndex IndexFingerTipIndex;
public:
	FAnimNode_PoseAIHandTarget();

	

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

};
