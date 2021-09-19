// Copyright Pose AI 2021.  All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "CoreGlobals.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Types/WidgetActiveTimerDelegate.h"
#include "SlateOptMacros.h"
#include "Misc/ConfigCacheIni.h"
#include "LiveLinkLog.h"
#include "iLiveLinkSource.h"
#include "LiveLinkSourceFactory.h"
#include "IPAddress.h"
#include "PoseAIHandshake.h"


constexpr int32 POSEAI_DEFAULT_PORTNUM = 8080;

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
	static int32 rigIndex;
	static bool isMirrored;
	static bool useRootMotion;

	void UpdatePortIPv4(const FText& InText, ETextCommit::Type type);
	void UpdatePortIPv6(const FText& InText, ETextCommit::Type type);
	void UpdateSyncFPS(const FText& InText, ETextCommit::Type type);
	void UpdateCameraFPS(const FText& InText, ETextCommit::Type type);
	bool ArePortsValid() const;

	FReply OnOkClicked();
	FReply OnToggleModeClicked();
	FReply OnToggleRigClicked();

	TSharedPtr<SEditableTextBox> portInput = nullptr;
	TSharedPtr<SEditableTextBox> syncFpsInput = nullptr;
	TSharedPtr<SEditableTextBox> cameraFpsInput = nullptr;
	TSharedPtr<STextBlock> modeInput = nullptr;
	TSharedPtr<STextBlock> rigInput = nullptr;
	TWeakPtr<SCheckBox> mirroredCheckBox = nullptr;
	TWeakPtr<SCheckBox> mixamoCheckBox = nullptr;
	TWeakPtr<SCheckBox> rootMotionCheckBox = nullptr;

	void ReadCheckBox(TWeakPtr<SCheckBox>& checkBox, bool& readTo);
};