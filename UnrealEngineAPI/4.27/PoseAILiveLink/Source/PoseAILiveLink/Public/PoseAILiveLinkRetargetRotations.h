// Copyright PoseAI Ltd 2021

#pragma once
#include "CoreMinimal.h"
#include "LiveLinkRetargetAsset.h"
#include "PoseAILiveLinkRetargetRotations.generated.h"

// Rretarget asset for data coming from Live Link. Remaps rotations onto all bones and only translations for root and pelvis/hip.
UCLASS(Blueprintable)
class POSEAILIVELINK_API UPoseAILiveLinkRetargetRotations : public ULiveLinkRetargetAsset
{
	GENERATED_UCLASS_BODY()

	virtual ~UPoseAILiveLinkRetargetRotations() {}

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface

	//~ Begin ULiveLinkRetargetAsset interface
	virtual void BuildPoseFromAnimationData(float DeltaTime, const FLiveLinkSkeletonStaticData* InSkeletonData, const FLiveLinkAnimationFrameData* InFrameData, FCompactPose& OutPose) override;
	//~ End ULiveLinkRetargetAsset interface

	// allow user to scale translations for differences in skeleton sizes
	UPROPERTY(EditAnywhere, Category = Settings)
	float scaleTranslation = 1.0f;

private:
	
	void OnBlueprintClassCompiled(UBlueprint* TargetBlueprint);


#if WITH_EDITOR
	/** Blueprint.OnCompiled delegate handle */
	FDelegateHandle OnBlueprintCompiledDelegate;
#endif
};
