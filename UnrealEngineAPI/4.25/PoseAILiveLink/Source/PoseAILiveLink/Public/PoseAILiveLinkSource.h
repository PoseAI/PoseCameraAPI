// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "LiveLinkSubjectSettings.h"
#include "LiveLinkFrameInterpolationProcessor.h"
#include "LiveLinkFramePreProcessor.h"
#include "LiveLinkFrameTranslator.h"
#include "PoseAIRig.h"
#include "PoseAILiveLinkServer.h"
#include "PoseAIHandshake.h"



#define LOCTEXT_NAMESPACE "PoseAI"


/**
 * 
 */
class POSEAILIVELINK_API PoseAILiveLinkSource : public ILiveLinkSource
{
public:
	PoseAILiveLinkSource(int32 inIPv4port, int32 inIPv6port, const PoseAIHandshake& handshake, bool useRootMotion);
	
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
		return LOCTEXT("SourceNachineName", "Unreal Engine");;
	}
	virtual FText GetSourceStatus() const { return status; }

	virtual TSubclassOf< ULiveLinkSourceSettings > GetSettingsClass() const override { return nullptr; }
	
	virtual void Update() override;
	virtual void OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent) {}

	void AddSubject(FName name);
	void UpdatePose(FName& name, TSharedPtr<FJsonObject> jsonPose);
	static bool IsValidPort(int32 port);

private:
	int32 portIPv4;
	int32 portIPv6;
	/* stores ports across different sources to avoid conflict */
	static TArray<int32> usedPorts;

	ILiveLinkClient* liveLinkClient = nullptr;
	ILiveLinkClient* client;
	const PoseAIHandshake handshake;
	bool useRootMotion = true;

	bool enabled;
	mutable FText status;

	/* create two different server objects if user wants both IPv4 and IPv6, as not all systems allow dual sockets*/
	TSharedPtr<PoseAILiveLinkServer, ESPMode::ThreadSafe> udpServerIPv4;
	TSharedPtr<PoseAILiveLinkServer, ESPMode::ThreadSafe> udpServerIPv6;
	
	TMap<FName, FLiveLinkSubjectKey> subjectKeys = {};
	TMap<FName, PoseAIRig> rigs = {};
	TQueue<FName> newConnections = {};
	FGuid sourceGuid;

	PoseAIRig MakeRig();
	
};
