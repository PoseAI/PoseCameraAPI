// Copyright Pose AI Ltd 2021. All Rights Reserved.

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
    for (int32 i = 0; i < InSkeletonData->BoneNames.Num(); i++)
    {
        FName boneName = InSkeletonData->BoneNames[i];
        auto jointTransform = InFrameData->Transforms[i];
        const int32 meshIndex = OutPose.GetBoneContainer().GetPoseBoneIndexForBoneName(boneName);
        if (meshIndex != INDEX_NONE)
        {
            FCompactPoseBoneIndex boneIndex = OutPose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(meshIndex));
            if (boneIndex != INDEX_NONE)
            {
                // root and hip we care about location so use livelink location and scale by property which can be edited in blueprints (to compensate for different skeleton sizes)
                if (i < 2) {
                    jointTransform.SetLocation(jointTransform.GetLocation() * scaleTranslation);
                    jointTransform.SetScale3D(OutPose.GetRefPose(boneIndex).GetScale3D());
                }
               // for the rest we use ref pose to set bone location, to preserve skeleton dimensions
                else
                {
                    jointTransform.SetLocation(OutPose.GetRefPose(boneIndex).GetLocation());
                    jointTransform.SetScale3D(OutPose.GetRefPose(boneIndex).GetScale3D());
                }
                OutPose[boneIndex] = jointTransform;
            }
        }
    }
}
