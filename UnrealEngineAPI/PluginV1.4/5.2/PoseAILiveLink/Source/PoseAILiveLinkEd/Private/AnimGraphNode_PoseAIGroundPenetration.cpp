// Copyright 2022-2023 Pose AI Ltd. All Rights Reserved.

#include "AnimGraphNode_PoseAIGroundPenetration.h"
#include "AnimNodeEditModes.h"
#include "Animation/AnimInstance.h"

// for customization details
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

// version handling
#include "AnimationCustomVersion.h"
#include "UObject/ReleaseObjectVersion.h"

#define LOCTEXT_NAMESPACE "PoseAI"

/////////////////////////////////////////////////////
// 

class FPoseAIGroundPenetrationDelegate : public TSharedFromThis<FPoseAIGroundPenetrationDelegate>
{
public:
	void UpdateLocationSpace(class IDetailLayoutBuilder* DetailBuilder)
	{
		if (DetailBuilder)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
};

TSharedPtr<FPoseAIGroundPenetrationDelegate> UAnimGraphNode_PoseAIGroundPenetration::PoseAIGroundPenetrationDelegate = NULL;

/////////////////////////////////////////////////////
// UAnimGraphNode_PoseAIGroundPenetration


UAnimGraphNode_PoseAIGroundPenetration::UAnimGraphNode_PoseAIGroundPenetration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_PoseAIGroundPenetration::GetControllerDescription() const
{
	return LOCTEXT("PoseAIGroundPenetration", "PoseAI Ground Penetration");
}

FText UAnimGraphNode_PoseAIGroundPenetration::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_PoseAIGroundPenetration_Tooltip", "This control makes sure avatar doesn't penetrate bottom of capsule, and can also pin the avatar lowpoint to capsule floor.");
}

FText UAnimGraphNode_PoseAIGroundPenetration::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.BoneToModify.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_PoseAIGroundPenetration_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_PoseAIGroundPenetration_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_PoseAIGroundPenetration::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_PoseAIGroundPenetration* PoseAIGroundPenetration = static_cast<FAnimNode_PoseAIGroundPenetration*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet

}

void UAnimGraphNode_PoseAIGroundPenetration::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
	
}

void UAnimGraphNode_PoseAIGroundPenetration::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	Super::Super::CustomizeDetails(DetailBuilder);

	// initialize just once
	if (!PoseAIGroundPenetrationDelegate.IsValid())
	{
		PoseAIGroundPenetrationDelegate = MakeShareable(new FPoseAIGroundPenetrationDelegate());
	}

}

void UAnimGraphNode_PoseAIGroundPenetration::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);


}

void UAnimGraphNode_PoseAIGroundPenetration::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (bEnableDebugDraw && SkelMeshComp)
	{
		if (FAnimNode_PoseAIGroundPenetration* ActiveNode = GetActiveInstanceNode<FAnimNode_PoseAIGroundPenetration>(SkelMeshComp->GetAnimInstance()))
		{
			//pass
		}
	}
}

#undef LOCTEXT_NAMESPACE
