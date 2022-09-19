// Copyright Pose AI Ltd 2021

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
#include "PoseAIEventDispatcher.h"

#define LOCTEXT_NAMESPACE "PoseAI"


struct POSEAILIVELINK_API PoseAIPortRecord {
	FGuid source;
	FName connectionName;
	FLiveLinkSubjectKey subjectKey;
};

/**
 * Redesigned so that each phone is associated with a single source, on a single port, for simplicity. 
 * Each source maintains its own "server" object, which generates the UDP socket, a listener and a sender class on their own threads.
 * The server feeds into the EventDispatcher system to trigger connection events.  Incoming packets are processed by the Rig class to 
 * trigger frame events and update the LiveLink pose source information.
 */
class POSEAILIVELINK_API PoseAILiveLinkSingleSource : public ILiveLinkSource
{
public:
	static const int32 portDefault = 8080;

	PoseAILiveLinkSingleSource(int32 port, bool useIPv6, const FPoseAIHandshake& handshake);
	
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
	virtual void InitializeSettings(ULiveLinkSourceSettings* Settings) {}
	virtual bool IsSourceStillValid() const override;
	virtual void OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent) {}
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid);
	virtual bool RequestSourceShutdown();
	virtual void Update() override;
	
	
	// method to add a source from code, instead of relying on presets.  Exposed to blueprints via the PoseAI movement component.
	static bool AddSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6, FLiveLinkSubjectName& subjectName);
	
	static bool GetPortGuid(int32 port, FGuid& fguid);
	static bool IsValidPort(int32 port);

	//Assigns a Livelink subject name from a port number.  Returns FName as that class is used in LiveLinkSubjectKey consturctor
	static FName SubjectNameFromPort(int32 port);

	//Looks up the "connection name" associated with port/subjetname which is the phone name @ IP Address.
	static FName GetConnectionName(int32 port);
	static FName GetConnectionName(const FLiveLinkSubjectName& subjectName);
	void SetConnectionName(FName name);

	void disable();
	FLiveLinkSubjectName GetSubjectName() const { return subjectKey.SubjectName; }
	void UpdatePose(TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig, TSharedPtr<FJsonObject> jsonPose);

private:
	int32 port;
	FGuid sourceGuid ;
	FLiveLinkSubjectKey subjectKey;
	TSharedPtr<PoseAILiveLinkServer> udpServer;
	
	/* stores ports across different sources to avoid conflict from user input */
	static TMap<int32, PoseAIPortRecord> usedPorts;

	ILiveLinkClient* liveLinkClient = nullptr;
	ILiveLinkClient* client = nullptr;
	
	UPoseAIEventDispatcher* dispatcher;

	mutable FText status;
    
	void AddSubject(TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig);
};
