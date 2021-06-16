// Copyright Pose AI 2021.  All rights reserved

#include "SPoseAILiveLinkWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "PoseAILiveLinkSourceFactory.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "LiveLinkLog.h"

#define LOCTEXT_NAMESPACE "PoseAI"

TWeakPtr<ILiveLinkSource> SPoseAILiveLinkWidget::source;


const FString SPoseAILiveLinkWidget::section = "PoseLiveLink.SourceConfig";
int32 SPoseAILiveLinkWidget::portNum = POSEAI_DEFAULT_PORTNUM;
int32 SPoseAILiveLinkWidget::syncFPS = 60;
int32 SPoseAILiveLinkWidget::cameraFPS = 60;
int32 SPoseAILiveLinkWidget::modeIndex = 0;
bool SPoseAILiveLinkWidget::isMixamo = false;
bool SPoseAILiveLinkWidget::isMirrored = false;
bool SPoseAILiveLinkWidget::useRootMotion = true;

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPoseAILiveLinkWidget::Construct(const FArguments& InArgs)
{

	GConfig->GetBool(*section, TEXT("isMixamo"), isMixamo, GEditorIni);
	GConfig->GetBool(*section, TEXT("isMirrored"), isMirrored, GEditorIni);
	GConfig->GetBool(*section, TEXT("useRootMotion"), useRootMotion, GEditorIni);

	GConfig->GetInt(*section, TEXT("CameraMode"), modeIndex, GEditorIni);
	if (modeIndex < 0 || modeIndex >= PoseAI_Modes.Num())
		modeIndex = 0;

	GConfig->GetInt(*section, TEXT("Port"), portNum, GEditorIni);
	SetPortNum(portNum);
	

	ChildSlot
	[
		SNew(SBox).WidthOverride(300)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.75f)
				[
					SNew(STextBlock).Text(LOCTEXT("Port", "Port:"))
				]
				+ SHorizontalBox::Slot().Padding(3, 3, 1, 3).VAlign(VAlign_Center).HAlign(HAlign_Right).FillWidth(0.25f)
				[
					SAssignNew(portInput, SEditableTextBox)
					.OnTextCommitted(this, &SPoseAILiveLinkWidget::UpdatePort)
				.Text(FText::FromString(FString::FromInt(portNum)))
				]
			]
			+ SVerticalBox::Slot().Padding(0, 4, 0, 2).VAlign(VAlign_Center).HAlign(HAlign_Center).AutoHeight()
			[
				SAssignNew(modeInput, STextBlock)
				.Text(FText::FromString(PoseAI_Modes[modeIndex]))
			]
			+ SVerticalBox::Slot().Padding(0, 2, 0, 4).VAlign(VAlign_Center).HAlign(HAlign_Center).AutoHeight()
			[
				SNew(SButton)
				.OnClicked(this, &SPoseAILiveLinkWidget::OnToggleModeClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToggleMode", "Toggle Camera Mode"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IsMirrored", "Mirror camera (flip left/right)"))
				]
			+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(mirroredCheckBox, SCheckBox)
					.IsChecked(isMirrored ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IsMixamo", "Use Mixamo rig (instead of UE4)"))
				]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(mixamoCheckBox, SCheckBox)
					.IsChecked(isMixamo ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UseRootMotion", "Use root motion"))
				]
			+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(rootMotionCheckBox, SCheckBox)
					.IsChecked(useRootMotion ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock).Text(LOCTEXT("SyncFPS", "Smoothed FPS (by app)"))
				]
				+ SHorizontalBox::Slot().Padding(3, 3, 1, 3).VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(syncFpsInput, SEditableTextBox)
					.OnTextCommitted(this, &SPoseAILiveLinkWidget::UpdateSyncFPS)
				.Text(FText::FromString(FString::FromInt(syncFPS)))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock).Text(LOCTEXT("CameraFPS", "Request Camera FPS (subject to device)"))
				]
			+ SHorizontalBox::Slot().Padding(3, 3, 1, 3).VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(cameraFpsInput, SEditableTextBox)
					.OnTextCommitted(this, &SPoseAILiveLinkWidget::UpdateCameraFPS)
				.Text(FText::FromString(FString::FromInt(cameraFPS)))
				]
				]
			+ SVerticalBox::Slot().Padding(3, 3, 1, 3).VAlign(VAlign_Center).HAlign(HAlign_Right).AutoHeight()
			[
				SNew(SButton)
				.OnClicked(this, &SPoseAILiveLinkWidget::OnOkClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OK", "OK"))
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SPoseAILiveLinkWidget::UpdatePort(const FText & InText, ETextCommit::Type type)
{
	portNum = FCString::Atoi(*(InText.ToString()));
	SetPortNum(portNum);
	GConfig->SetInt(*section, TEXT("Port"), portNum, GEditorIni);
	GConfig->Flush(false, GEditorIni);
}

void SPoseAILiveLinkWidget::SetPortNum(int32 port){
	if (port < 1028 || port > 49151) {
		FLiveLinkLog::Warning(TEXT("PoseAI: %d is an invalid port number, using %d instead."), port, POSEAI_DEFAULT_PORTNUM);
		portNum = POSEAI_DEFAULT_PORTNUM;
	} else {
		portNum = port;
	}
}

void SPoseAILiveLinkWidget::UpdateSyncFPS(const FText& InText, ETextCommit::Type type)
{
	syncFPS = FCString::Atoi(*(InText.ToString()));
	if (syncFPS < 0) {
		syncFPS = 0;
	}
	if (syncFPS < cameraFPS && syncFPS > 0)
		syncFPS = cameraFPS;

	
	GConfig->SetInt(*section, TEXT("syncFPS"), syncFPS, GEditorIni);
	GConfig->Flush(false, GEditorIni);
}

void SPoseAILiveLinkWidget::UpdateCameraFPS(const FText& InText, ETextCommit::Type type)
{
	cameraFPS = FCString::Atoi(*(InText.ToString()));
	if (cameraFPS < 24) {
		cameraFPS = 24;
	}
	if (syncFPS < cameraFPS && syncFPS > 0)
		syncFPS = cameraFPS;
	
	GConfig->SetInt(*section, TEXT("cameraFPS"), cameraFPS, GEditorIni);
	GConfig->SetInt(*section, TEXT("syncFPS"), syncFPS, GEditorIni);
	GConfig->Flush(false, GEditorIni);
}


void SPoseAILiveLinkWidget::disableExistingSource()
{
	TSharedPtr<ILiveLinkSource> linkSource = source.Pin();
	if (linkSource.IsValid()) { 
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: disabling existing source"));
		((PoseAILiveLinkSource*)linkSource.Get())->disable();
	}
}


TSharedPtr<ILiveLinkSource> SPoseAILiveLinkWidget::CreateSource(const FString& port)
{
	PoseAIHandshake handshake = PoseAIHandshake();
	handshake.isMirrored = isMirrored;
	handshake.rig = (isMixamo ? "Mixamo" : "UE4"); 
	handshake.mode = PoseAI_Modes[modeIndex];
	handshake.syncFPS = syncFPS;
	handshake.cameraFPS = cameraFPS;
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Set handshake to %s"), *(handshake.ToString()));
	PoseAILiveLinkSource* src = new PoseAILiveLinkSource(portNum, handshake, useRootMotion);
	TSharedPtr<ILiveLinkSource> sharedPtr(src);
	source = sharedPtr;
	return sharedPtr;
}

FReply SPoseAILiveLinkWidget::OnToggleModeClicked()
{
	modeIndex = (modeIndex+1) % PoseAI_Modes.Num();
	modeInput->SetText(FText::FromString(PoseAI_Modes[modeIndex]));
	GConfig->SetInt(*section, TEXT("CameraMode"), modeIndex, GEditorIni);
	GConfig->Flush(false, GEditorIni);
	return FReply::Handled();
}

FReply SPoseAILiveLinkWidget::OnOkClicked()
{

	ReadCheckBox(mixamoCheckBox, isMixamo);
	ReadCheckBox(mirroredCheckBox, isMirrored);
	ReadCheckBox(rootMotionCheckBox, useRootMotion);
	
	GConfig->SetBool(*section, TEXT("isMixamo"), isMixamo, GEditorIni);
	GConfig->SetBool(*section, TEXT("isMirror"), isMirrored, GEditorIni);
	GConfig->SetBool(*section, TEXT("useRootMotion"), useRootMotion, GEditorIni);

	if (PoseAILiveLinkSource::IsValidPort(portNum)) {
		FString connectionString = "";
		TSharedPtr<ILiveLinkSource> src = CreateSource(connectionString);
		callback.Execute(src, connectionString);
	} else {
		FLiveLinkLog::Warning(TEXT("PoseAI: Cannot set two sources with the same port"));
	}
	return FReply::Handled();
}

void SPoseAILiveLinkWidget::ReadCheckBox(TWeakPtr<SCheckBox>& checkBox, bool& readTo)
{
	TSharedPtr<SCheckBox> pin = checkBox.Pin();
	if (pin) 
		readTo = (pin->GetCheckedState() == ECheckBoxState::Checked);
}