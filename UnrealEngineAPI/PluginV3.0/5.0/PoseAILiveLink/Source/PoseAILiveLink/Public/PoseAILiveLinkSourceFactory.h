// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LiveLinkSourceFactory.h"
#include "ILiveLinkSource.h"
#include "PoseAILiveLinkSourceFactory.generated.h"

/**
 * 
 */
UCLASS()
class POSEAILIVELINK_API UPoseAILiveLinkSourceFactory : public ULiveLinkSourceFactory
{
	GENERATED_BODY()

	UPoseAILiveLinkSourceFactory() {
	}

	~UPoseAILiveLinkSourceFactory () {
	}

public:
	virtual TSharedPtr< SWidget > BuildCreationPanel(FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const override;
	virtual TSharedPtr< ILiveLinkSource > CreateSource(const FString& ConnectionString) const override;
	virtual FText GetSourceDisplayName() const override;
	virtual FText GetSourceTooltip() const override;
	virtual EMenuType GetMenuType() const override { return EMenuType::SubPanel; }
};
