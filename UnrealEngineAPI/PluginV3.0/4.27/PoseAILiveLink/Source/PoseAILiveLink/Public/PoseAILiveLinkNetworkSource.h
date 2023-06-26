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
#include "PoseAILiveLinkServer.h"
#include "PoseAIStructs.h"
#include "PoseAILiveLinkFaceSubSource.h"

#define LOCTEXT_NAMESPACE "PoseAI"


struct POSEAILIVELINK_API PoseAIPortRecord {
	FGuid source;
	FName connectionName;
	FLiveLinkSubjectKey subjectKey;
};

class PoseAILiveLinkSingleSourceListener;


/**
 * Redesigned so that each phone is associated with a single source, on a single port, for simplicity. 
 * Each source maintains its own "server" object, which generates the UDP socket, a listener and a sender class on their own threads.
 * The server feeds into the EventDispatcher system to trigger connection events.  Incoming packets are processed by the Rig class to 
 * trigger frame events and update the LiveLink pose source information.
 */
class POSEAILIVELINK_API PoseAILiveLinkNetworkSource : public ILiveLinkSource
{
public:

	/*
	* method to add a source from code, instead of relying on presets.Exposed to blueprints via the PoseAI movement component.
	* returns true if source was added and fills in subjectNamd with the name of the source's sole subject in the LiveLink system
	*/
	static bool AddSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6, FLiveLinkSubjectName& subjectName);
	static TSharedPtr<ILiveLinkSource> MakeSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6);

	/* Prefer the MakeSource factory method to setup source correctly */
	PoseAILiveLinkNetworkSource(const FPoseAIHandshake& handshake, int32 port, bool useIPv6);
	
	// standard Live Link source methods
	virtual bool CanBeDisplayedInUI() const { return true; }
	virtual TSubclassOf< ULiveLinkSourceSettings > GetSettingsClass() const override { return nullptr; }
	virtual FText GetSourceType() const {
		return LOCTEXT("SourceType", "PoseAI mobile");
	}
	virtual FText GetSourceMachineName() const {
		return LOCTEXT("SourceMachineName", "Unreal Engine");;
	}
	virtual FText GetSourceStatus() const { return status; }
	virtual void InitializeSettings(ULiveLinkSourceSettings* Settings) override;
	virtual bool IsSourceStillValid() const override;
	virtual void OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent) {}
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
	virtual bool RequestSourceShutdown();
	virtual void Update() override {}
	
	// custom methods
	static bool GetPortGuid(int32 port, FGuid& fguid);
	static bool IsValidPort(int32 port);
	static FName GetConnectionName(int32 port);
	static FName GetConnectionName(const FLiveLinkSubjectName& subjectName);
	static FName SubjectNameFromPort(int32 port);
	
	void disable();
	FLiveLinkSubjectName GetSubjectName() const { return subjectKey.SubjectName; }
	void SetConnectionName(FName name);
	void SetHandshake(const FPoseAIHandshake& handshake);

	/* Main processing method */
	void UpdatePose(TSharedPtr<FJsonObject> jsonPose);
	
private:
	// We use a sharedref so that bindSP can be used to create weak references.  This is only owner outside of the delegate system.
	TSharedRef<PoseAILiveLinkSingleSourceListener> listener;

public:
	static const int32 portDefault = 8080;
	PoseAILiveLinkServer udpServer;

private:
	/* stores ports across different sources to avoid conflict from user input */
	static TMap<int32, PoseAIPortRecord> usedPorts;
	ILiveLinkClient* liveLinkClient = nullptr;
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig;
	FPoseAIHandshake handshake;
	int32 port;
	FGuid sourceGuid ;
	FLiveLinkSubjectKey subjectKey;
	TUniquePtr<PoseAILiveLinkFaceSubSource> faceSubSource;
	mutable FText status;
	FCriticalSection InSynchObject;

	void AddSubject();

};

/*
* This class will register for delegates as a smart pointer, allowing the owning source to only have a references from the LiveLinkClient.
*/
class POSEAILIVELINK_API PoseAILiveLinkSingleSourceListener
{
private:
	PoseAILiveLinkNetworkSource* parent;
	bool isMe(const FLiveLinkSubjectName& target) {
		return target == parent->GetSubjectName();
	}
public:
	PoseAILiveLinkSingleSourceListener(PoseAILiveLinkNetworkSource* parent) : parent(parent) {};

	void SetHandshake(const FPoseAIHandshake& handshake) {
		parent->SetHandshake(handshake);
	}

	void CloseTarget(const FLiveLinkSubjectName& target) {
		if (isMe(target))
			parent->RequestSourceShutdown();
	}

	void DisconnectTarget(const FLiveLinkSubjectName& target){
		if (isMe(target)) {
			parent->udpServer.Disconnect();
		}
	}

	void SendConfig(const FLiveLinkSubjectName& target, FPoseAIModelConfig config) {
		if (isMe(target)) {
			FString message_string = config.ToString();
			if (parent->udpServer.SendString(message_string))
				UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent config %s"), *message_string);
		}
	}
		
};