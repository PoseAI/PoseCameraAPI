// Copyright Pose AI 2021.  All rights reserved

#include "SPoseAILiveLinkWidget.h"
#include "PoseAILiveLinkSourceFactory.h"
#include "PoseAILiveLinkSingleSource.h"

#define LOCTEXT_NAMESPACE "PoseAI"

TWeakPtr<ILiveLinkSource> SPoseAILiveLinkWidget::source = nullptr;

static TArray<FString> PoseAI_Modes = { "Room", "Desktop", "Portrait", "RoomBodyOnly", "PortraitBodyOnly" };
static TArray<FString> PoseAI_Rigs = { "UE4", "MetaHuman", "Mixamo", "DazUE"};

const FString SPoseAILiveLinkWidget::section = "PoseLiveLink.SourceConfig";
int32 SPoseAILiveLinkWidget::portNum = PoseAILiveLinkSingleSource::portDefault;
int32 SPoseAILiveLinkWidget::syncFPS = 60;
int32 SPoseAILiveLinkWidget::cameraFPS = 60;
int32 SPoseAILiveLinkWidget::modeIndex = 0;
int32 SPoseAILiveLinkWidget::rigIndex = 0;
bool SPoseAILiveLinkWidget::isMirrored = false;
bool SPoseAILiveLinkWidget::useRootMotion = true;
bool SPoseAILiveLinkWidget::isIPv6 = false;

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPoseAILiveLinkWidget::Construct(const FArguments& InArgs)
{
	GConfig->GetBool(*section, TEXT("isIPv6"), isIPv6, GEditorIni);
	GConfig->GetBool(*section, TEXT("isMirrored"), isMirrored, GEditorIni);
	GConfig->GetBool(*section, TEXT("useRootMotion"), useRootMotion, GEditorIni);

	GConfig->GetInt(*section, TEXT("CameraMode"), modeIndex, GEditorIni);
	if (modeIndex < 0 || modeIndex >= PoseAI_Modes.Num())
		modeIndex = 0;
	GConfig->GetInt(*section, TEXT("Rig"), rigIndex, GEditorIni);
	if (rigIndex < 0 || rigIndex >= PoseAI_Rigs.Num())
		rigIndex = 0;

	GConfig->GetInt(*section, TEXT("PortNumber"), portNum, GEditorIni);

	ChildSlot
	[
		SNew(SBox).WidthOverride(300)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.85f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UseIPv6", "Connect via an IPv6 socket"))
				]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.15f)
				[
					SAssignNew(ipv6CheckBox, SCheckBox)
					.IsChecked(isIPv6 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.75f)
				[
					SNew(STextBlock).Text(LOCTEXT("PortIPv4", "Port for IPv4 (0 if unused)"))
				]
				+ SHorizontalBox::Slot().Padding(3, 3, 1, 3).VAlign(VAlign_Center).HAlign(HAlign_Right).FillWidth(0.25f)
				[
					SAssignNew(portInput, SEditableTextBox)
					.OnTextCommitted(this, &SPoseAILiveLinkWidget::UpdatePort)
				.Text(FText::FromString(FString::FromInt(portNum)))
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.65f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToggleMode", "Toggle Camera Mode"))
					
				]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.45f)

				[
					SNew(SButton)
					.OnClicked(this, &SPoseAILiveLinkWidget::OnToggleModeClicked)
				[
					SAssignNew(modeInput, STextBlock)
					.Text(FText::FromString(PoseAI_Modes[modeIndex]))
				]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(1, 3, 3, 3).VAlign(VAlign_Center).HAlign(HAlign_Left).FillWidth(0.6f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToggleRig", "Toggle rig format"))
				]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).FillWidth(0.4f)
				[
					SNew(SButton)
					.OnClicked(this, &SPoseAILiveLinkWidget::OnToggleRigClicked)
					[
						SAssignNew(rigInput, STextBlock)
						.Text(FText::FromString(PoseAI_Rigs[rigIndex]))
					]
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
					SNew(STextBlock).Text(LOCTEXT("CameraFPS", "Request Camera FPS"))
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


void SPoseAILiveLinkWidget::UpdatePort(const FText& InText, ETextCommit::Type type)
{
	portNum = FCString::Atoi(*(InText.ToString()));
}


bool SPoseAILiveLinkWidget::IsPortValid() const
{
	if (portNum < 1028 || portNum > 49151) {
		FLiveLinkLog::Warning(TEXT("PoseAI: %d is an invalid port number. Set a valid port number (>1028 and <49151)."), portNum);
		return false;
	}
	
	if (!PoseAILiveLinkSingleSource::IsValidPort(portNum)) {
		FLiveLinkLog::Warning(TEXT("PoseAI: Cannot set two sources with the same port.  %d is in use already."), portNum);
		return false;
	}
	return true;
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
		((PoseAILiveLinkSingleSource*)linkSource.Get())->disable();
	}
}


TSharedPtr<ILiveLinkSource> SPoseAILiveLinkWidget::CreateSource(const FString& connectionString)
{
	FPoseAIHandshake handshake = FPoseAIHandshake();
	handshake.isMirrored = isMirrored;
	handshake.rig = PoseAI_Rigs[rigIndex];
	handshake.mode = PoseAI_Modes[modeIndex];
	handshake.syncFPS = syncFPS;
	handshake.cameraFPS = cameraFPS;
	handshake.useRootMotion = useRootMotion;
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Set handshake to %s"), *(handshake.ToString()));
	return MakeShared<PoseAILiveLinkSingleSource>(portNum, isIPv6, handshake);
}

FReply SPoseAILiveLinkWidget::OnToggleModeClicked()
{
	modeIndex = (modeIndex+1) % PoseAI_Modes.Num();
	modeInput->SetText(FText::FromString(PoseAI_Modes[modeIndex]));
	GConfig->SetInt(*section, TEXT("CameraMode"), modeIndex, GEditorIni);
	GConfig->Flush(false, GEditorIni);
	return FReply::Handled();
}


FReply SPoseAILiveLinkWidget::OnToggleRigClicked()
{
	rigIndex = (rigIndex + 1) % PoseAI_Rigs.Num();
	rigInput->SetText(FText::FromString(PoseAI_Rigs[rigIndex]));
	GConfig->SetInt(*section, TEXT("Rig"), rigIndex, GEditorIni);
	GConfig->Flush(false, GEditorIni);
	return FReply::Handled();
}


FReply SPoseAILiveLinkWidget::OnOkClicked()
{
	ReadCheckBox(mirroredCheckBox, isMirrored);
	ReadCheckBox(rootMotionCheckBox, useRootMotion);
	ReadCheckBox(ipv6CheckBox, isIPv6);
	
	GConfig->SetBool(*section, TEXT("isMirror"), isMirrored, GEditorIni);
	GConfig->SetBool(*section, TEXT("useRootMotion"), useRootMotion, GEditorIni);

	if (IsPortValid()) {
		GConfig->SetInt(*section, TEXT("PortNumber"), portNum, GEditorIni);
		GConfig->Flush(false, GEditorIni);
		FString connectionString = "";
		TSharedPtr<ILiveLinkSource> src = CreateSource(connectionString);
		callback.Execute(src, connectionString);
		FLiveLinkLog::Info(
			TEXT("PoseAI: Setup source.  Rig is in %s format, %s and %s."),
			*PoseAI_Rigs[rigIndex],
			*FString(isMirrored ? TEXT("mirrored to camera"): TEXT("third person")),
			*FString(useRootMotion ? TEXT("uses root motion"): TEXT("translate motion in the hip/pelvis bone"))
		);
	}
	return FReply::Handled();
}

void SPoseAILiveLinkWidget::ReadCheckBox(TWeakPtr<SCheckBox>& checkBox, bool& readTo)
{
	TSharedPtr<SCheckBox> pin = checkBox.Pin();
	if (pin) 
		readTo = (pin->GetCheckedState() == ECheckBoxState::Checked);
}