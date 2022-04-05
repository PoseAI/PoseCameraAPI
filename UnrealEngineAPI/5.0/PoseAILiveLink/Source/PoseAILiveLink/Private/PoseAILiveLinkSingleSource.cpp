// Copyright Pose AI Ltd 2021

#include "PoseAILiveLinkSingleSource.h"
#include "Features/IModularFeatures.h"


 FCriticalSection critSingleSection;

 TMap<int32, PoseAIPortRecord> PoseAILiveLinkSingleSource::usedPorts = {};


PoseAILiveLinkSingleSource::PoseAILiveLinkSingleSource(int32 port, bool useIPv6, const FPoseAIHandshake& handshake) :
	port(port), status(LOCTEXT("statusConnecting", "connecting"))
{
	dispatcher = UPoseAIEventDispatcher::GetDispatcher();

	UE_LOG(LogTemp, Display, TEXT("PoseAI: connecting to %d"), port);
	udpServer = MakeShared<PoseAILiveLinkServer>(handshake, this, useIPv6, port);
	if (udpServer.IsValid()) {
		dispatcher->handshakeUpdate.AddSP(udpServer.ToSharedRef(), &PoseAILiveLinkServer::SetHandshake);
		dispatcher->modelConfigUpdate.AddSP(udpServer.ToSharedRef(), &PoseAILiveLinkServer::SendConfig);
		dispatcher->disconnect.AddSP(udpServer.ToSharedRef(), &PoseAILiveLinkServer::DisconnectTarget);
		dispatcher->closeSource.AddSP(udpServer.ToSharedRef(), &PoseAILiveLinkServer::CloseTarget);
		if (useIPv6) {
			status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on IPv6 local-link Port:{1}"), FText::FromString(FString::FromInt(port)));
		}
		else {
			FString myIP;
			udpServer->GetIP(myIP);
			status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on {0} Port:{1}"), FText::FromString(myIP), FText::FromString(FString::FromInt(port)));
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI: unable to create a server on %d using IPv4"), port);
	}
}
void PoseAILiveLinkSingleSource::Update() {
	if (udpServer->hasNewRig) {
		AddSubject(udpServer->rig);
		udpServer->hasNewRig = false;
	}
}


bool PoseAILiveLinkSingleSource::AddSource(const FPoseAIHandshake& handshake, int32 portNum, bool isIPv6, FLiveLinkSubjectName& subjectName) {

	bool bResult = false;
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		FLiveLinkClient& LiveLinkClient = IModularFeatures::Get().GetModularFeature<FLiveLinkClient>(ILiveLinkClient::ModularFeatureName);

		if (!PoseAILiveLinkSingleSource::IsValidPort(portNum)) {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI: Port %d already assigned to another source.  Cancelling"), portNum);
			FGuid existingSource;
			if (PoseAILiveLinkSingleSource::GetPortGuid(portNum, existingSource))
				LiveLinkClient.RemoveSource(existingSource);
		}
		TSharedPtr<PoseAILiveLinkSingleSource> PoseAISource = MakeShared<PoseAILiveLinkSingleSource>(portNum, isIPv6, handshake);
		TSharedPtr<ILiveLinkSource> Source = StaticCastSharedPtr<ILiveLinkSource>(PoseAISource);
		LiveLinkClient.AddSource(Source);
		LiveLinkClient.Tick();
		subjectName = PoseAISource->GetSubjectName();
		bResult = true;
	}
	return bResult;
}

void PoseAILiveLinkSingleSource::AddSubject(TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig){
	
	check(IsInGameThread());
	liveLinkClient->RemoveSubject_AnyThread(subjectKey);
	FLiveLinkSubjectPreset subject;
	subject.bEnabled = true;
	subject.Key =  subjectKey;
	subject.Role = TSubclassOf<ULiveLinkRole>(ULiveLinkAnimationRole::StaticClass());
	subject.Settings = nullptr;
	subject.VirtualSubject = nullptr;
	
	critSingleSection.Lock();
	if (!liveLinkClient->CreateSubject(subject)) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAILiveLink: unable to create subject %s"), *(subjectKey.SubjectName.Name.ToString()));
	}
	else {
		UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: created subject %s"), *(subjectKey.SubjectName.Name.ToString()));
		FLiveLinkStaticDataStruct rigDefinition = rig->MakeStaticData();
		liveLinkClient->PushSubjectStaticData_AnyThread(subject.Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(rigDefinition));

	}
	critSingleSection.Unlock();
}

bool PoseAILiveLinkSingleSource::GetPortGuid(int32 port, FGuid& fguid) {
	bool has = usedPorts.Contains(port);
	if (has)
		fguid = usedPorts[port].source;
	return has;
}

FName PoseAILiveLinkSingleSource::SubjectNameFromPort(int32 port) {
	FString NewString = FString("PoseCam@port:") + FString::FromInt(port);
	return FName(*NewString);

}

void PoseAILiveLinkSingleSource::SetConnectionName(FName name) {
	if (usedPorts.Contains(port))
		usedPorts[port].connectionName = name;
}

FName PoseAILiveLinkSingleSource::GetConnectionName(int32 port) {
	return (usedPorts.Contains(port)) ? usedPorts[port].connectionName : NAME_None;
}

FName PoseAILiveLinkSingleSource::GetConnectionName(const FLiveLinkSubjectName& name) {
	for (const auto& elem : usedPorts) {
		if (elem.Value.subjectKey.SubjectName == name)
			return elem.Value.connectionName;
	}
	return NAME_None;
}

bool PoseAILiveLinkSingleSource::IsSourceStillValid() const { return true; }

bool PoseAILiveLinkSingleSource::IsValidPort(int32 port) {  return !usedPorts.Contains(port); }

void PoseAILiveLinkSingleSource::disable()
{
	UE_LOG(LogTemp, Display, TEXT("Pose AI LiveLink: disabling the source"));
	status = LOCTEXT("statusDisabled", "disabled");
	liveLinkClient = nullptr;
}


void PoseAILiveLinkSingleSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	sourceGuid = InSourceGuid;
	subjectKey = FLiveLinkSubjectKey(sourceGuid, SubjectNameFromPort(port));
	PoseAIPortRecord record = PoseAIPortRecord();
	record.source = InSourceGuid;
	record.subjectKey = subjectKey;
	usedPorts.Add(port, record);
	client = InClient;
	liveLinkClient = InClient;
	
	if (udpServer->rig != nullptr && udpServer->rig.IsValid())
		AddSubject(udpServer->rig);

	UE_LOG(LogTemp, Display, TEXT("Pose AI LiveLink: receive client %s"), *client->ModularFeatureName.ToString());
}

bool PoseAILiveLinkSingleSource::RequestSourceShutdown()
{
	usedPorts.Remove(port);
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: PoseAILiveLinkSingleSource on port %d closed"), port);
	if (liveLinkClient != nullptr) {
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		liveLinkClient->RemoveSource(sourceGuid);
		liveLinkClient = nullptr;
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: removing subject %s"), *(subjectKey.SubjectName.Name.ToString()));
	}
	return true;
}

void PoseAILiveLinkSingleSource::UpdatePose(TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig, TSharedPtr<FJsonObject> jsonPose)
{
	if (liveLinkClient == nullptr)
		return;
	if(rig == nullptr || !rig.IsValid()) {
		return;
	}
	FLiveLinkFrameDataStruct frameData(FLiveLinkAnimationFrameData::StaticStruct());
	FLiveLinkAnimationFrameData& data = *frameData.Cast<FLiveLinkAnimationFrameData>();
	data.Transforms.Reserve(100);
	if (rig->ProcessFrame(jsonPose, data)) {
		liveLinkClient->PushSubjectFrameData_AnyThread(subjectKey, MoveTemp(frameData));
	}
}
