// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "ILiveLinkClient.h"
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


/**
 * 
 */
class POSEAILIVELINK_API PoseAILiveLinkSource : public ILiveLinkSource
{
public:
	PoseAILiveLinkSource(int32 inIPv4port, int32 inIPv6port, const FPoseAIHandshake& handshake, bool useRootMotion);
	
	~PoseAILiveLinkSource ();

		void disable();
	void setIPAddress(const FText & ip);

	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid);
	virtual void InitializeSettings(ULiveLinkSourceSettings* Settings) {}

	virtual bool CanBeDisplayedInUI() const { return true; }

	virtual bool IsSourceStillValid() const override;

	virtual bool RequestSourceShutdown();

	virtual FText GetSourceType() const {
		return LOCTEXT("SourceType", "PoseAI mobile");
	}
	virtual FText GetSourceMachineName() const {
		return LOCTEXT("SourceMachineName", "Unreal Engine");;
	}
	virtual FText GetSourceStatus() const { return status; }

	virtual TSubclassOf< ULiveLinkSourceSettings > GetSettingsClass() const override { return nullptr; }
	
	virtual void Update() override;
	virtual void OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent) {}

	void AddSubject(FName name);
	void UpdatePose(FName& name, TSharedPtr<FJsonObject> jsonPose);
	static bool IsValidPort(int32 port);
	void BindServers();
    
    

private:
	int32 portIPv4;
	int32 portIPv6;
	/* stores ports across different sources to avoid conflict from user input */
	static TArray<int32> usedPorts;

	ILiveLinkClient* liveLinkClient = nullptr;
	ILiveLinkClient* client = nullptr;
	
	FPoseAIHandshake handshake;
	UPoseAIEventDispatcher* dispatcher;
	// stores user selection for how character motion is assigned
	bool useRootMotion = true;

	bool enabled;
	mutable FText status;
    
	/* create two different server objects if user wants both IPv4 and IPv6, as not all systems allow dual sockets*/
	TSharedPtr<PoseAILiveLinkServer> udpServerIPv4;
	TSharedPtr<PoseAILiveLinkServer> udpServerIPv6;
	
	TMap<FName, FLiveLinkSubjectKey> subjectKeys = {};
	TMap<FName, TSharedPtr<PoseAIRig, ESPMode::ThreadSafe>> rigs = {};
	TQueue<FName> newConnections = {};
	FGuid sourceGuid;

	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> MakeRig(FName name);
    
    
	
};
