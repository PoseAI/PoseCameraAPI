// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLinkSourceFactory.h"
#include "SPoseAILiveLinkWidget.h"

#define LOCTEXT_NAMESPACE "PoseAI"


TSharedPtr<SWidget> UPoseAILiveLinkSourceFactory::BuildCreationPanel(FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const
{
	auto rawWidget = SNew(SPoseAILiveLinkWidget);
	rawWidget->setCallback(OnLiveLinkSourceCreated);
	TSharedPtr<SWidget> myWidget = rawWidget;
	return myWidget;
}

TSharedPtr< ILiveLinkSource > UPoseAILiveLinkSourceFactory::CreateSource(const FString & ConnectionString) const
{
	return SPoseAILiveLinkWidget::CreateSource(ConnectionString);
}

FText UPoseAILiveLinkSourceFactory::GetSourceDisplayName() const  { return LOCTEXT("Pose AI App", "Pose AI App"); }
FText UPoseAILiveLinkSourceFactory::GetSourceTooltip() const  { return LOCTEXT("Connect to the Pose AI mobile App", "Connect to the Pose AI mobile App"); }

#undef LOCTEXT_NAMESPACE