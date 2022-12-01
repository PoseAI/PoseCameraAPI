// Copyright Pose AI Ltd. All Rights Reserved.

#include "AnimGraphNode_PoseAIHandTarget.h"
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
// FTwoBoneIKDelegate

class FPoseAIHandTargetDelegate : public TSharedFromThis<FPoseAIHandTargetDelegate>
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

TSharedPtr<FPoseAIHandTargetDelegate> UAnimGraphNode_PoseAIHandTarget::PoseAIHandTargetDelegate = NULL;

/////////////////////////////////////////////////////
// UAnimGraphNode_PoseAIHandTarget


UAnimGraphNode_PoseAIHandTarget::UAnimGraphNode_PoseAIHandTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_PoseAIHandTarget::GetControllerDescription() const
{
	return LOCTEXT("PoseAIHandTarget", "PoseAI Hands In BodySpace");
}

FText UAnimGraphNode_PoseAIHandTarget::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_PoseAIHandTarget_Tooltip", "ThIS control applies an inverse kinematic (IK) solver to a 3-joint chain, based on remapped coordinates between different sized avatars.");
}

FText UAnimGraphNode_PoseAIHandTarget::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.IKBone.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.IKBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_PoseAIHandTarget::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_PoseAIHandTarget* PoseAIHandTarget = static_cast<FAnimNode_PoseAIHandTarget*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	PoseAIHandTarget->PoseAiIkVector = Node.PoseAiIkVector;
}

void UAnimGraphNode_PoseAIHandTarget::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
	if (InPin->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, PoseAiIkVector))
	{
		GetDefaultValue(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, PoseAiIkVector), Node.PoseAiIkVector);
	}
}

void UAnimGraphNode_PoseAIHandTarget::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	Super::Super::CustomizeDetails(DetailBuilder);

	// initialize just once
	if (!PoseAIHandTargetDelegate.IsValid())
	{
		PoseAIHandTargetDelegate = MakeShareable(new FPoseAIHandTargetDelegate());
	}

	
	IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
	IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("Effector");
	IDetailCategoryBuilder& JointCategory = DetailBuilder.EditCategory("JointTarget");
	

	EBoneControlSpace Space = Node.EffectorLocationSpace;
	const FString TakeRotationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, bTakeRotationFromEffectorSpace));
	const FString EffectorTargetName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, EffectorTarget));
	const FString EffectorLocationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, EffectorLocation));
	const FString EffectorLocationSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, EffectorLocationSpace));
	// hide all properties in EndEffector category
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*EffectorLocationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*TakeRotationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorTargetName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorLocationSpace, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}

	//Space = Node.JointTargetLocationSpace;
	bool bPinVisibilityChanged = false;
	const FString JointTargetName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, JointTarget));
	const FString JointTargetLocation = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, JointTargetLocation));
	const FString JointTargetLocationSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseAIHandTarget, JointTargetLocationSpace));

	// hide all properties in JointTarget category except for JointTargetLocationSpace
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*JointTargetName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*JointTargetLocationSpace, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*JointTargetLocation, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}


}

void UAnimGraphNode_PoseAIHandTarget::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAnimationCustomVersion::GUID);

	const int32 CustomAnimVersion = Ar.CustomVer(FAnimationCustomVersion::GUID);

	if (CustomAnimVersion < FAnimationCustomVersion::RenamedStretchLimits)
	{
		// fix up deprecated variables
		Node.StartStretchRatio = Node.StretchLimits_DEPRECATED.X;
		Node.MaxStretchScale = Node.StretchLimits_DEPRECATED.Y;
	}

	Ar.UsingCustomVersion(FReleaseObjectVersion::GUID);
	if (Ar.CustomVer(FReleaseObjectVersion::GUID) < FReleaseObjectVersion::RenameNoTwistToAllowTwistInTwoBoneIK)
	{
		Node.bAllowTwist = !Node.bNoTwist_DEPRECATED;
	}

	if (CustomAnimVersion < FAnimationCustomVersion::ConvertIKToSupportBoneSocketTarget)
	{
		if (Node.EffectorSpaceBoneName_DEPRECATED != NAME_None)
		{
			Node.EffectorTarget = FBoneSocketTarget(Node.EffectorSpaceBoneName_DEPRECATED);
		}

		if (Node.JointTargetSpaceBoneName_DEPRECATED != NAME_None)
		{
			Node.JointTarget = FBoneSocketTarget(Node.JointTargetSpaceBoneName_DEPRECATED);
		}
	}
}

void UAnimGraphNode_PoseAIHandTarget::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (bEnableDebugDraw && SkelMeshComp)
	{
		if (FAnimNode_PoseAIHandTarget* ActiveNode = GetActiveInstanceNode<FAnimNode_PoseAIHandTarget>(SkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, SkelMeshComp);
		}
	}
}

#undef LOCTEXT_NAMESPACE
