// Copyright 2023 Pose AI Ltd. All Rights Reserved.

#include "AnimNode_PoseAIGroundPenetration.h"
#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"


DECLARE_CYCLE_STAT(TEXT("PoseAIGroundPenetration Eval"), STAT_PoseAIGroundPenetration_Eval, STATGROUP_Anim);


/////////////////////////////////////////////////////
// FAnimNode_PoseAIGroundPenetration

FAnimNode_PoseAIGroundPenetration::FAnimNode_PoseAIGroundPenetration()
	
{
	
}

void FAnimNode_PoseAIGroundPenetration::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_PoseAIGroundPenetration::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	check(OutBoneTransforms.Num() == 0);
		
	if (BonesToCheck.Num() + SocketsBoneReference.Num() > 0) {
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

		FCompactPoseBoneIndex CompactPoseBoneToModify = BoneToModify.GetCompactPoseIndex(BoneContainer);
		FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
		FVector appliedTranslation = FVector::Zero();
		float minZ = 99999999.0;

		for (auto& b : BonesToCheck)
		{
			FCompactPoseBoneIndex CompactPoseBoneToCheck = b.GetCompactPoseIndex(BoneContainer);
			float z = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToCheck).GetTranslation().Z;
			minZ = FMath::Min(minZ, z);

		}
		for (int i = 0; i < SocketsBoneReference.Num(); ++i)
		{
			const FCompactPoseBoneIndex SocketBoneIndex = SocketsBoneReference[i].GetCompactPoseIndex(BoneContainer);
			const FTransform SocketTransform = SocketsLocalTransform[i] * Output.Pose.GetComponentSpaceTransform(SocketBoneIndex) ;
			float z = SocketTransform.GetTranslation().Z;
			minZ = FMath::Min(minZ, z);
		}
		if (PinToFloor) appliedTranslation.Z = -minZ;
		else appliedTranslation.Z += FMath::Max(0.0f, -minZ);
		NewBoneTM.AddToTranslation(appliedTranslation);
		OutBoneTransforms.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));
	}
	TRACE_ANIM_NODE_VALUE(Output, TEXT("Target"), BoneToModify.BoneName);
}

bool FAnimNode_PoseAIGroundPenetration::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (const auto& b : BonesToCheck) {
		if (!b.IsValidToEvaluate(RequiredBones))
			return false;
	}
	// if both bones are valid
	return (BoneToModify.IsValidToEvaluate(RequiredBones));
}


void FAnimNode_PoseAIGroundPenetration::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	BoneToModify.Initialize(RequiredBones);
	for (auto& b : BonesToCheck) {
		b.Initialize(RequiredBones);
	}
	
	if (USkeletalMesh* SkelMesh = RequiredBones.GetSkeletalMeshAsset())
	{
		SocketsBoneReference.Empty();
		SocketsLocalTransform.Empty();
		for (auto& socketName : SocketsToCheck) {
			if (const USkeletalMeshSocket* Socket = SkelMesh->FindSocket(socketName))
			{
				FBoneReference socketBoneReference(Socket->BoneName);
				socketBoneReference.Initialize(RequiredBones);
				SocketsLocalTransform.Add(Socket->GetSocketLocalTransform());
				SocketsBoneReference.Add(socketBoneReference);
			}
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI: Required bones missing from FAnimNode_PoseAIGroundPenetration"));

	}
	

}

