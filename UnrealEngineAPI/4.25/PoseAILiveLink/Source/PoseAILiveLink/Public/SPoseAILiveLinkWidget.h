// Copyright Pose AI 2021.  All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "LiveLinkSourceFactory.h"
#include "IPAddress.h"
#include "PoseAIHandshake.h"


constexpr int32 POSEAI_DEFAULT_PORTNUM = 8080;
static TArray<FString> PoseAI_Modes = {"Room", "Desktop", "Portrait", "RoomBodyOnly", "PortraitBodyOnly"};
/**
 *
 */
class POSEAILIVELINK_API SPoseAILiveLinkWidget : public SCompoundWidget, public FWidgetActiveTimerDelegate
{
public:
	SLATE_BEGIN_ARGS(SPoseAILiveLinkWidget) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void setCallback(ULiveLinkSourceFactory::FOnLiveLinkSourceCreated whenCreated) { callback = whenCreated; }

	static void disableExistingSource();
	static TSharedPtr<ILiveLinkSource> CreateSource(const FString& port);

protected:
	static TWeakPtr<ILiveLinkSource> source; 
	ULiveLinkSourceFactory::FOnLiveLinkSourceCreated callback;

	UPROPERTY(EditAnywhere, Config, Category = Custom)

private:
	const static FString section;
	static int32 portNumIPv4;
	static int32 portNumIPv6;
	static int32 syncFPS;
	static int32 cameraFPS;
	static int32 modeIndex;
	static bool isMixamo;
	static bool isMirrored;
	static bool useRootMotion;

	void UpdatePortIPv4(const FText& InText, ETextCommit::Type type);
	void UpdatePortIPv6(const FText& InText, ETextCommit::Type type);
	void UpdateSyncFPS(const FText& InText, ETextCommit::Type type);
	void UpdateCameraFPS(const FText& InText, ETextCommit::Type type);
	bool ArePortsValid() const;

	FReply OnOkClicked();
	FReply OnToggleModeClicked();

	TSharedPtr<SEditableTextBox> portInput;
	TSharedPtr<SEditableTextBox> syncFpsInput;
	TSharedPtr<SEditableTextBox> cameraFpsInput;
	TSharedPtr<STextBlock> modeInput;
	TWeakPtr<SCheckBox> mirroredCheckBox;
	TWeakPtr<SCheckBox> mixamoCheckBox;
	TWeakPtr<SCheckBox> rootMotionCheckBox;

	void ReadCheckBox(TWeakPtr<SCheckBox>& checkBox, bool& readTo);
};