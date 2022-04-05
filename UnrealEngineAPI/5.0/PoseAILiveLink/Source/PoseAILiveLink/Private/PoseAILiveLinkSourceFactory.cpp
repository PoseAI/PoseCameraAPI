// Copyright Pose AI Ltd 2021

#include "PoseAILiveLinkSourceFactory.h"
#include "SPoseAILiveLinkWidget.h"


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
