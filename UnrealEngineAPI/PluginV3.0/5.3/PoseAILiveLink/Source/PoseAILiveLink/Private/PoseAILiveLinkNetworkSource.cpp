// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLinkNetworkSource.h"
#include "Features/IModularFeatures.h"
#include "PoseAIEventDispatcher.h"

#define LOCTEXT_NAMESPACE "PoseAI"

 
/*
* Static method for creating and adding a networked source to LiveLink, as alternative to the menu UI.  
*/
bool PoseAILiveLinkNetworkSource::AddSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6, FLiveLinkSubjectName& subjectName) {

	 if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	 {
		 FLiveLinkClient& LiveLinkClient = IModularFeatures::Get().GetModularFeature<FLiveLinkClient>(ILiveLinkClient::ModularFeatureName);

		 if (!PoseAILiveLinkNetworkSource::IsValidPort(portNum)) {
			 UE_LOG(LogTemp, Warning, TEXT("PoseAI: Port %d already assigned to another source.  Cancelling"), portNum);
			 FGuid existingSource;
			 if (PoseAILiveLinkNetworkSource::GetPortGuid(portNum, existingSource))
				 LiveLinkClient.RemoveSource(existingSource);
		 }
		 TSharedPtr<ILiveLinkSource> Source = MakeSource(handshake, portNum, isIPv6);
		 LiveLinkClient.AddSource(Source);
		 LiveLinkClient.Tick();
		 subjectName = SubjectNameFromPort(portNum);
		 return true;
	 }
	 else {
		 return false;
	 }
}

/*
*  Factory method to set up smart pointers and link the udp server weakly to its owner.
 * The resulting pointer then needs to be added by the caller to the LiveLink.
 * To avoid crashes, only LiveLinkClient should have non-weak references to the source
 */
TSharedPtr<ILiveLinkSource> PoseAILiveLinkNetworkSource::MakeSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6) {
	TSharedPtr<PoseAILiveLinkNetworkSource> PoseAISource = MakeShared<PoseAILiveLinkNetworkSource>(handshake, portNum, isIPv6);
	TWeakPtr<PoseAILiveLinkNetworkSource> weakSource(PoseAISource);
	PoseAISource->udpServer.SetSource(weakSource);
	return StaticCastSharedPtr<ILiveLinkSource>(PoseAISource);
}

/*
*  Should only be wrapped in a smart pointer and created by MakeSource.
 */
PoseAILiveLinkNetworkSource::PoseAILiveLinkNetworkSource(const FPoseAIHandshake& handshake, int32 port, bool useIPv6) :
	listener(MakeShared<PoseAILiveLinkSingleSourceListener>(this)),
	udpServer(PoseAILiveLinkServer(handshake, useIPv6, port)),
	handshake(handshake),
	port(port),
	status(LOCTEXT("statusConnecting", "connecting"))
{
	subjectKey = FLiveLinkSubjectKey(sourceGuid, SubjectNameFromPort(port));

	UE_LOG(LogTemp, Display, TEXT("PoseAI: connecting to %d"), port);
	
	UPoseAIEventDispatcher* dispatcher = UPoseAIEventDispatcher::GetDispatcher();
	dispatcher->handshakeUpdate.AddSP(listener, &PoseAILiveLinkSingleSourceListener::SetHandshake);
	dispatcher->modelConfigUpdate.AddSP(listener, &PoseAILiveLinkSingleSourceListener::SendConfig);
	dispatcher->disconnect.AddSP(listener, &PoseAILiveLinkSingleSourceListener::DisconnectTarget);
	dispatcher->closeSource.AddSP(listener, &PoseAILiveLinkSingleSourceListener::CloseTarget);
	if (useIPv6) {
		status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on IPv6 local-link Port:{1}"), FText::FromString(FString::FromInt(port)));
	}
	else {
		FString myIP;
		udpServer.GetIP(myIP);
		status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on {0} Port:{1}"), FText::FromString(myIP), FText::FromString(FString::FromInt(port)));
	}
}


/*
* This method is called by the livelink client when the source has been submitted.  At this point the Guid and client have been asigned
*/
void PoseAILiveLinkNetworkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	sourceGuid = InSourceGuid;
	subjectKey = FLiveLinkSubjectKey(sourceGuid, SubjectNameFromPort(port));
	PoseAIPortRecord record = PoseAIPortRecord();
	record.source = InSourceGuid;
	record.subjectKey = subjectKey;
	usedPorts.Add(port, record);
	liveLinkClient = InClient;

	AddSubject();
	faceSubSource = TUniquePtr<PoseAILiveLinkFaceSubSource>(new PoseAILiveLinkFaceSubSource(subjectKey, liveLinkClient));
	faceSubSource->AddSubject(InSynchObject);

}

/* 
* This method is called by the LiveLink client after the source has been submitted and after the receiveclient call.
* Still to be confirmed if any call needs to be made to apply the changes we make here to the actual livelink system
*/
void PoseAILiveLinkNetworkSource::InitializeSettings(ULiveLinkSourceSettings* Settings) {
	Settings->BufferSettings.MaxNumberOfFrameToBuffered = 1;
	Settings->Mode = ELiveLinkSourceMode::Latest;
}


/*
* Once the source is setup and received we can add subjects.  Here we also create the rig that corresponds to the subject.  We will only have one subject per source
*/
void PoseAILiveLinkNetworkSource::AddSubject(){
	rig = PoseAIRig::PoseAIRigFactory(SubjectNameFromPort(port), handshake);
	if (!rig) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to create rig %s"), *handshake.GetRigString());
		return;
	}
	check(IsInGameThread());
	liveLinkClient->RemoveSubject_AnyThread(subjectKey);
	FLiveLinkSubjectPreset subject;
	subject.bEnabled = true;
	subject.Key =  subjectKey;
	subject.Role = TSubclassOf<ULiveLinkRole>(ULiveLinkAnimationRole::StaticClass());
	subject.Settings = nullptr;
	subject.VirtualSubject = nullptr;
	
	FScopeLock ScopeLock(&InSynchObject);
	if (!liveLinkClient->CreateSubject(subject)) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to create subject %s"), *(subjectKey.SubjectName.Name.ToString()));
	}
	else {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: created subject %s"), *(subjectKey.SubjectName.Name.ToString()));
		FLiveLinkStaticDataStruct rigDefinition = rig->MakeStaticData();
		liveLinkClient->PushSubjectStaticData_AnyThread(subject.Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(rigDefinition));
	}
}


/*
*  The main processing function. For this source the update is called by the udpclient when it receives a frame.
*/
void PoseAILiveLinkNetworkSource::UpdatePose(TSharedPtr<FJsonObject> jsonPose)
{
	if (!liveLinkClient ||!rig || !rig.IsValid()) {
		return;
	}
	FLiveLinkFrameDataStruct frameData(FLiveLinkAnimationFrameData::StaticStruct());
	FLiveLinkAnimationFrameData& data = *frameData.Cast<FLiveLinkAnimationFrameData>();
	data.Transforms.Reserve(100);
	if (rig->ProcessFrame(jsonPose, data)) {
		liveLinkClient->PushSubjectFrameData_AnyThread(subjectKey, MoveTemp(frameData));
		faceSubSource->UpdateFace(jsonPose);
	}
	else {
		static const FName NAME_JsonError = "PoseAILiveLink_ProcessFrameError";
		FLiveLinkLog::WarningOnce(NAME_JsonError, subjectKey, TEXT("PoseAI: Error processing frame (for instance, rig type mismatch)"));
	}
}



void PoseAILiveLinkNetworkSource::SetHandshake(const FPoseAIHandshake& newHandshake) {
	bool dirty = handshake != newHandshake;
	bool rigChange = handshake.rig != newHandshake.rig;
	handshake = newHandshake;
	if (rigChange) {
		AddSubject();
	}
	if (dirty)
		udpServer.SetHandshake(handshake);
}


bool PoseAILiveLinkNetworkSource::GetPortGuid(int32 port, FGuid& fguid) {
	bool has = usedPorts.Contains(port);
	if (has)
		fguid = usedPorts[port].source;
	return has;
}

FName PoseAILiveLinkNetworkSource::SubjectNameFromPort(int32 port) {
	FString NewString = FString("PoseCam@port:") + FString::FromInt(port);
	return FName(*NewString);

}

void PoseAILiveLinkNetworkSource::SetConnectionName(FName name) {
	if (usedPorts.Contains(port))
		usedPorts[port].connectionName = name;
}

FName PoseAILiveLinkNetworkSource::GetConnectionName(int32 port) {
	return (usedPorts.Contains(port)) ? usedPorts[port].connectionName : NAME_None;
}

FName PoseAILiveLinkNetworkSource::GetConnectionName(const FLiveLinkSubjectName& name) {
	for (const auto& elem : usedPorts) {
		if (elem.Value.subjectKey.SubjectName == name)
			return elem.Value.connectionName;
	}
	return NAME_None;
}

bool PoseAILiveLinkNetworkSource::IsSourceStillValid() const { return true; }

bool PoseAILiveLinkNetworkSource::IsValidPort(int32 port) {  return !usedPorts.Contains(port); }

void PoseAILiveLinkNetworkSource::disable()
{
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: disabling the source"));
	status = LOCTEXT("statusDisabled", "disabled");
	liveLinkClient = nullptr;
}

bool PoseAILiveLinkNetworkSource::RequestSourceShutdown()
{
	usedPorts.Remove(port);
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: PoseAILiveLinkNetworkSource on port %d closed"), port);
	if (liveLinkClient != nullptr) {
		faceSubSource->RequestSubSourceShutdown();
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		liveLinkClient->RemoveSource(sourceGuid);
		liveLinkClient = nullptr;
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: removing subject %s"), *(subjectKey.SubjectName.Name.ToString()));
	}
	return true;
}

FText PoseAILiveLinkNetworkSource::GetSourceType() const {
	return LOCTEXT("SourceType", "PoseAI Local");
}

FText PoseAILiveLinkNetworkSource::GetSourceMachineName() const {
	return LOCTEXT("SourceMachineName", "Unreal Engine Local");;
}

TMap<int32, PoseAIPortRecord> PoseAILiveLinkNetworkSource::usedPorts = {};

#undef LOCTEXT_NAMESPACE