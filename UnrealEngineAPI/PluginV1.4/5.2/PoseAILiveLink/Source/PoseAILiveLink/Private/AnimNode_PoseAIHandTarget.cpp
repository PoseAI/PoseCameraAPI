// Copyright 2022 Pose AI Ltd. All Rights Reserved.

#include "AnimNode_PoseAIHandTarget.h"
#include "Engine/Engine.h"
#include "AnimationRuntime.h"
#include "TwoBoneIK.h"
#include "AnimationCoreLibrary.h"
#include "Animation/AnimInstanceProxy.h"
#include "SceneManagement.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "Animation/AnimTrace.h"

#define LOCTEXT_NAMESPACE "PoseAI"
DECLARE_CYCLE_STAT(TEXT("PoseAIHandTargetIK Eval"), STAT_PoseAIHandTarget_Eval, STATGROUP_Anim);


/////////////////////////////////////////////////////
// FAnimNode_PoseAIHandTarget

FAnimNode_PoseAIHandTarget::FAnimNode_PoseAIHandTarget() 
	: IKBoneCompactPoseIndex(INDEX_NONE)
	, SpineFirstIndex(INDEX_NONE)
	, LeftUpperArmIndex(INDEX_NONE)
	, RightUpperArmIndex(INDEX_NONE)
	, IndexFingerTipIndex(INDEX_NONE)
{
}


void FAnimNode_PoseAIHandTarget::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	// we only care in the zone so fade IK as we move outside the zone
	const float alphaZone = FMath::Clamp(2.0f - FMath::Max(1.0f, FMath::Max(PoseAiIkVector.Y, PoseAiIkVector.Z)), 0.0f, 1.0f);
	if (alphaZone == 0.0f || PoseAiIkVector == FVector::ZeroVector)
		return;

	const FVector BaseCSPos = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex).GetTranslation();
	const FVector Control1CSPos = Output.Pose.GetComponentSpaceTransform(SpineFirstIndex).GetTranslation();
	const FVector Control2CSPos = Output.Pose.GetComponentSpaceTransform(LeftUpperArmIndex).GetTranslation();
	const FVector Control3CSPos = Output.Pose.GetComponentSpaceTransform(RightUpperArmIndex).GetTranslation();
	const FVector Control4CSPos = Control1CSPos + 0.5f * (FVector::Dist(Control2CSPos, Control1CSPos) + FVector::Dist(Control3CSPos, Control1CSPos)) * 
		FVector::CrossProduct(Control3CSPos - Control1CSPos, Control2CSPos - Control1CSPos).GetSafeNormal();
	FVector TargetLocation = 
		PoseAiIkVector.X * Control1CSPos + 
		PoseAiIkVector.Y * Control2CSPos + 
		PoseAiIkVector.Z * Control3CSPos +
		(1.0f - PoseAiIkVector.X - PoseAiIkVector.Y - PoseAiIkVector.Z) * Control4CSPos;
	
	/* disabled currently until hand stability improves
	// adjust wrist IK target by relative position, as angle will be preserved.
	if (IndexFingerTipIndex != INDEX_NONE) {
		const FVector IndexCSPos = Output.Pose.GetComponentSpaceTransform(IndexFingerTipIndex).GetTranslation();
		TargetLocation -= (IndexCSPos - BaseCSPos);
	}
	*/

	EffectorLocation = alphaZone * TargetLocation + (1.0f - alphaZone) * BaseCSPos;
	EffectorLocationSpace = BCS_ComponentSpace;

	JointTargetLocationSpace = BCS_ComponentSpace;
	JointTargetLocation = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex).GetTranslation();
	Super::EvaluateSkeletalControl_AnyThread(Output, OutBoneTransforms);
	
}
bool FAnimNode_PoseAIHandTarget::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
	if (SpineFirstIndex == INDEX_NONE || LeftUpperArmIndex == INDEX_NONE || RightUpperArmIndex == INDEX_NONE)
		return false;
	return Super::IsValidToEvaluate(Skeleton, RequiredBones);
}


void FAnimNode_PoseAIHandTarget::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	IKBone.Initialize(RequiredBones);

	EffectorTarget.InitializeBoneReferences(RequiredBones);
	JointTarget.InitializeBoneReferences(RequiredBones);

	IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(RequiredBones);
	CachedLowerLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
	CachedUpperLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
	if (IKBoneCompactPoseIndex != INDEX_NONE)
	{
		CachedLowerLimbIndex = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex);
		if (CachedLowerLimbIndex != INDEX_NONE)
		{
			CachedUpperLimbIndex = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex);
		}
	}

	SpineFirst.Initialize(RequiredBones);
	LeftUpperArm.Initialize(RequiredBones);
	RightUpperArm.Initialize(RequiredBones);

	SpineFirstIndex = SpineFirst.GetCompactPoseIndex(RequiredBones);
	LeftUpperArmIndex = LeftUpperArm.GetCompactPoseIndex(RequiredBones);
	RightUpperArmIndex = RightUpperArm.GetCompactPoseIndex(RequiredBones);

	UseIndexFingerTip.Initialize(RequiredBones);
	IndexFingerTipIndex = UseIndexFingerTip.GetCompactPoseIndex(RequiredBones);
	 
}
#undef LOCTEXT_NAMESPACE
