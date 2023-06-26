// Copyright 2022-2023 Pose AI Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_PoseAIGroundPenetration.generated.h"

/**
 *	Debugging node that displays the current value of a bone in a specific space.
 */
USTRUCT()
struct POSEAILIVELINK_API FAnimNode_PoseAIGroundPenetration : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to apply live movement to, usually either root or pelvis/hip. **/
	UPROPERTY(EditAnywhere, Category = SkeletalControl)
		FBoneReference BoneToModify;
	
	/** Set to true to always have contact with ground **/
	UPROPERTY(EditAnywhere, Category = GroundPenetration, meta = (PinShownByDefault))
		bool PinToFloor = false;

	/** These bones will be checked for ground penetration **/
	UPROPERTY(EditAnywhere, Category = GroundPenetration)
		TArray<FBoneReference> BonesToCheck;

	/** These sockets will be checked for ground pnetration. **/
	UPROPERTY(EditAnywhere, Category = GroundPenetration)
		TArray<FName> SocketsToCheck;

	


	TArray<FBoneReference> SocketsBoneReference;
	TArray<FTransform> SocketsLocalTransform;

	FAnimNode_PoseAIGroundPenetration();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

};



