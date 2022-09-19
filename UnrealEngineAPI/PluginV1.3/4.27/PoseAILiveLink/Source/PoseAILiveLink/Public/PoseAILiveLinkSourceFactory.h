// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "LiveLinkSourceFactory.h"
#include "ILiveLinkSource.h"
#include "PoseAILiveLinkSourceFactory.generated.h"

#define LOCTEXT_NAMESPACE "PoseAI"

/**
 * 
 */
UCLASS()
class POSEAILIVELINK_API UPoseAILiveLinkSourceFactory : public ULiveLinkSourceFactory
{
	GENERATED_BODY()

	UPoseAILiveLinkSourceFactory() {
		UE_LOG(LogTemp, Display, TEXT("instantiating PoseAILiveLinkSourceFactory"));
	}

	~UPoseAILiveLinkSourceFactory () {
		UE_LOG(LogTemp, Display, TEXT("destroying PoseAILiveLinkSourceFactory"));
	}

public:
	virtual TSharedPtr< SWidget > BuildCreationPanel(FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const override;
	virtual TSharedPtr< ILiveLinkSource > CreateSource(const FString& ConnectionString) const override;
	virtual FText GetSourceDisplayName() const override { return LOCTEXT("Pose AI App", "Pose AI App"); }
	virtual FText GetSourceTooltip() const override { return LOCTEXT("Connect to the Pose AI mobile App", "Connect to the Pose AI mobile App"); }
	virtual EMenuType GetMenuType() const override { return EMenuType::SubPanel; }
};
