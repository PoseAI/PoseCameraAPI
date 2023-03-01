// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "ILiveLinkClient.h"
#include "LiveLinkClient.h"
#include "LiveLinkSourceSettings.h"
#include "LiveLinkSubjectSettings.h"
#include "LiveLinkFrameInterpolationProcessor.h"
#include "LiveLinkFramePreProcessor.h"
#include "LiveLinkFrameTranslator.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "LiveLinkTypes.h"
#include "LiveLinkLog.h"
#include "HAL/RunnableThread.h"
#include "Json.h"
#include "PoseAIRig.h"
#include "PoseAIStructs.h"
#include "PoseAILiveLinkFaceSubSource.h"

#define LOCTEXT_NAMESPACE "PoseAI"


/**
 * Source from in game engine framework.
 * The server feeds into the EventDispatcher system to trigger connection events.  Incoming packets are processed by the Rig class to 
 * trigger frame events and update the LiveLink pose source information.
 */
class POSEAILIVELINK_API PoseAILiveLinkNativeSource : public ILiveLinkSource
{

	/*
	Use a static method to add source via a sole shared ptr with only ownership by LiveLinkClient, and pass back weak pointer to caller.  
	LiveLink really seems to want to own the only shared pointer or cleanup can crash.
	*/
public:
	static TWeakPtr<PoseAILiveLinkNativeSource> AddSource(FName subjectName, const FPoseAIHandshake& handshake);
	bool AddSubject();
	void ReceivePacket(const FString& recvMessage);

public:
	/* Prefer using the AddSource method for setup */
	PoseAILiveLinkNativeSource(FName subjectName, const FPoseAIHandshake& handshake);

	// standard Live Link source methods
	virtual bool CanBeDisplayedInUI() const { return true; }
	virtual TSubclassOf< ULiveLinkSourceSettings > GetSettingsClass() const override { return nullptr; }
	virtual FText GetSourceType() const {
		return LOCTEXT("SourceType", "PoseAI Local");
	}
	virtual FText GetSourceMachineName() const {
		return LOCTEXT("SourceMachineName", "Unreal Engine Local");;
	}
	virtual FText GetSourceStatus() const { return status; }
	virtual void InitializeSettings(ULiveLinkSourceSettings* Settings) override;
	virtual bool IsSourceStillValid() const override;
	virtual void OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent) {}
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
	virtual bool RequestSourceShutdown();
	virtual void Update() override {};
	
public:
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig;
	void disable();
	void UpdatePose(TSharedPtr<FJsonObject> jsonPose);

private:
	FGuid sourceGuid ;
	FLiveLinkSubjectKey subjectKey;
	FName subjectName = "PoseAILocalCam";
	ILiveLinkClient* liveLinkClient = nullptr;
	FCriticalSection InSynchObject;
	FPoseAIHandshake handshake;
	TUniquePtr<PoseAILiveLinkFaceSubSource> faceSubSource;

	mutable FText status;
	
};
