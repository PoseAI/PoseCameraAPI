// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLinkRetargetRotations.h"

#include "BonePose.h"
#include "BoneContainer.h"
#include "Engine/Blueprint.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"


UPoseAILiveLinkRetargetRotations::UPoseAILiveLinkRetargetRotations(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	UBlueprint* Blueprint = Cast<UBlueprint>(GetClass()->ClassGeneratedBy);
	if (Blueprint)
	{
		OnBlueprintCompiledDelegate = Blueprint->OnCompiled().AddUObject(this, &UPoseAILiveLinkRetargetRotations::OnBlueprintClassCompiled);
	}
#endif
}

void UPoseAILiveLinkRetargetRotations::BeginDestroy()
{
#if WITH_EDITOR
	if (OnBlueprintCompiledDelegate.IsValid())
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(GetClass()->ClassGeneratedBy);
		check(Blueprint);
		Blueprint->OnCompiled().Remove(OnBlueprintCompiledDelegate);
		OnBlueprintCompiledDelegate.Reset();
	}
#endif

	Super::BeginDestroy();
}

void UPoseAILiveLinkRetargetRotations::OnBlueprintClassCompiled(UBlueprint* TargetBlueprint)
{
	
}


void UPoseAILiveLinkRetargetRotations::BuildPoseFromAnimationData(float DeltaTime, const FLiveLinkSkeletonStaticData* InSkeletonData, const FLiveLinkAnimationFrameData* InFrameData, FCompactPose& OutPose)
{
    auto rootTransform = InFrameData->Transforms[0];
    auto rootTranslation = rootTransform.GetTranslation() * scaleTranslation;
    auto rootZ = FVector3d(0.0f, 0.0f, rootTranslation.Z);
    auto rootXY = FVector3d(rootTranslation.X, rootTranslation.Y, 0.0f);
    for (int32 i = 0; i < InSkeletonData->BoneNames.Num(); i++)
    {
        FName boneName = InSkeletonData->BoneNames[i];
        auto jointTransform = InFrameData->Transforms[i];
        const int32 meshIndex = OutPose.GetBoneContainer().GetPoseBoneIndexForBoneName(boneName);
        if (meshIndex != INDEX_NONE)
        {
            FCompactPoseBoneIndex boneIndex = OutPose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(meshIndex));
            if (i == 0)
            {
                jointTransform.SetLocation(OutPose.GetRefPose(boneIndex).GetTranslation() + rootXY);
                jointTransform.SetScale3D(OutPose.GetRefPose(boneIndex).GetScale3D());
                OutPose[boneIndex] = jointTransform;
            }
            
            else if (i == 1)
            {
                jointTransform.SetLocation(OutPose.GetRefPose(boneIndex).GetTranslation() + rootZ);
                jointTransform.SetScale3D(OutPose.GetRefPose(boneIndex).GetScale3D());
                OutPose[boneIndex] = jointTransform;
            }
            else if (boneIndex != INDEX_NONE)
            {
                jointTransform.SetLocation(OutPose.GetRefPose(boneIndex).GetLocation());
                jointTransform.SetScale3D(OutPose.GetRefPose(boneIndex).GetScale3D());   
                OutPose[boneIndex] = jointTransform;
            }
        }
    }
}
