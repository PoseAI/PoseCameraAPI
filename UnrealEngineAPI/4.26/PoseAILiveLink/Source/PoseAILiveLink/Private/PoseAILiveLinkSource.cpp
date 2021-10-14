// Copyright Pose AI Ltd 2021

#include "PoseAILiveLinkSource.h"


static int lockedAt;
static int unlockedAt;
static FCriticalSection critSection; 


PoseAILiveLinkSource::PoseAILiveLinkSource(int32 inIPv4port, int32 inIPv6port, const FPoseAIHandshake& handshake, bool useRootMotion) :
	portIPv4(inIPv4port), portIPv6(inIPv6port), handshake(handshake), useRootMotion(useRootMotion), enabled(true), status(LOCTEXT("statusConnecting", "connecting"))
{
	dispatcher = UPoseAIEventDispatcher::GetDispatcher();
	dispatcher->AddToRoot();
}

void PoseAILiveLinkSource::BindServers() {
	if (portIPv6 > 0) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI: connecting to %d using IPv6"), portIPv6);
		udpServerIPv6 = MakeShared<PoseAILiveLinkServer>(true);
		if (udpServerIPv6.IsValid()) {
			usedPorts.Emplace(portIPv6);
			dispatcher->handshakeUpdate.AddSP(udpServerIPv6.ToSharedRef(), &PoseAILiveLinkServer::SetHandshake);
			dispatcher->modelConfigUpdate.AddSP(udpServerIPv6.ToSharedRef(), &PoseAILiveLinkServer::SendConfig);
			udpServerIPv6->CreateServer(portIPv6, handshake, this, udpServerIPv6.ToSharedRef());
			status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on IPv6 local-link Port:{1}"), FText::FromString(FString::FromInt(portIPv6)));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI: unable to create a server on %d using IPv4"), portIPv4);
		}
	}

	if (portIPv4 > 0) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI: connecting to %d using IPv4"), portIPv4);		
		udpServerIPv4 = MakeShared<PoseAILiveLinkServer>(false);
		if (udpServerIPv4.IsValid()) {
			usedPorts.Emplace(portIPv4);
			dispatcher->handshakeUpdate.AddSP(udpServerIPv4.ToSharedRef(), &PoseAILiveLinkServer::SetHandshake);
			dispatcher->modelConfigUpdate.AddSP(udpServerIPv4.ToSharedRef(), &PoseAILiveLinkServer::SendConfig);
			udpServerIPv4->CreateServer(portIPv4, handshake, this, udpServerIPv4.ToSharedRef());
			FString myIP;
			udpServerIPv4->GetIP(myIP);
			status = FText::FormatOrdered(LOCTEXT("statusConnected", "listening on {0} Port:{1}"), FText::FromString(myIP), FText::FromString(FString::FromInt(portIPv4)));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI: unable to create a server on %d using IPv4"), portIPv4);
		}
	}
}

TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> PoseAILiveLinkSource::MakeRig(FName name) {
	return PoseAIRig::PoseAIRigFactory(
		name,
		FName(handshake.rig),
		useRootMotion,
		!handshake.mode.Contains(TEXT("BodyOnly")), //includeHands
		handshake.isMirrored,
		handshake.mode.Contains(TEXT("Desktop")) //isDesktop
		);
}

void PoseAILiveLinkSource::AddSubject(FName name)
{
	check(IsInGameThread());
	FLiveLinkSubjectPreset subject;
	subject.bEnabled = true;
	subject.Key = FLiveLinkSubjectKey(sourceGuid, name);
	subject.Role = TSubclassOf<ULiveLinkRole>(ULiveLinkAnimationRole::StaticClass());
	subject.Settings = nullptr;
	subject.VirtualSubject = nullptr;
	
	critSection.Lock(); lockedAt = __LINE__;
	liveLinkClient->RemoveSubject_AnyThread(subject.Key); // try to remove from client even if untracked here for case where preset loaded name
	bool is_reconnection = false;
	if (subjectKeys.Find(name) != 0) {
		UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: replacing %s with new connection"), *(name.ToString()));
		subjectKeys.Remove(name);
		is_reconnection = true;

	}
	UE_LOG(LogTemp, Display, TEXT("PoseAIiveLink: adding %s to subjects"), *(name.ToString()));
	if (!liveLinkClient->CreateSubject(subject)) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAILiveLink: unable to create subject %s"), *(name.ToString()));
	}
	else {
		rigs.Add(name, MakeRig(name));
		FLiveLinkStaticDataStruct rigDefinition = rigs[name]->MakeStaticData();
		liveLinkClient->RemoveSubject_AnyThread(subject.Key);
		liveLinkClient->PushSubjectStaticData_AnyThread(subject.Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(rigDefinition));
		subjectKeys.Add(name, subject.Key);
	}
	critSection.Unlock(); unlockedAt = __LINE__;
}

void PoseAILiveLinkSource::Update()
{
	while (!newConnections.IsEmpty())
	{
		FName newSubject;
		newConnections.Dequeue(newSubject);
		AddSubject(newSubject);
	}
	
}

TArray<int32> PoseAILiveLinkSource::usedPorts = {};

bool PoseAILiveLinkSource::IsValidPort(int32 port) {
	return !usedPorts.Contains(port);
}

bool PoseAILiveLinkSource::IsSourceStillValid() const
{
	return true;
}

PoseAILiveLinkSource::~PoseAILiveLinkSource()
{
	if (portIPv4 > 0) {
		usedPorts.Remove(portIPv4);
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: PoseAILiveLinkSource on port %d closed"), portIPv4);
	}
	if (portIPv6 > 0) {
		usedPorts.Remove(portIPv6);
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: PoseAILiveLinkSource on port %d closed"), portIPv6);
	}
	
}

void PoseAILiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	sourceGuid = InSourceGuid;
	client = InClient;
	liveLinkClient = InClient;

	UE_LOG(LogTemp, Display, TEXT("Pose AI LiveLink: receive client %s"), *client->ModularFeatureName.ToString());
	for (auto& elem : subjectKeys) {
		AddSubject(elem.Key);
	}
 }

void PoseAILiveLinkSource::disable()
{
	UE_LOG(LogTemp, Display, TEXT("Pose AI LiveLink: disabling the source"));
	status = LOCTEXT("statusDisabled", "disabled");

	critSection.Lock(); lockedAt = __LINE__;
	liveLinkClient = nullptr;
	critSection.Unlock(); unlockedAt = __LINE__;

	enabled = false;
}


bool PoseAILiveLinkSource::RequestSourceShutdown()
{
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: requested source shutdown"));
	if (liveLinkClient != nullptr) {
		for (auto& elem : subjectKeys) {
			liveLinkClient->RemoveSubject_AnyThread(elem.Value);
			UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: removing subject %s"), *(elem.Key.ToString()));
		}
	}
	subjectKeys.Empty();
	if (udpServerIPv4)
		udpServerIPv4->CleanUp();
	if (udpServerIPv6)
		udpServerIPv6->CleanUp();
	udpServerIPv4 = nullptr;
	udpServerIPv6 = nullptr;

	critSection.Lock(); lockedAt = __LINE__;
	liveLinkClient = nullptr;
	critSection.Unlock(); unlockedAt = __LINE__;
	return true;
}

void PoseAILiveLinkSource::UpdatePose(FName& name, TSharedPtr<FJsonObject> jsonPose)
{
	if (liveLinkClient == nullptr)
		return;
	if (subjectKeys.Find(name) == nullptr) {
		UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: cannot find %s to update frame.  Adding new subject."), *(name.ToString()));
		newConnections.Enqueue(name);
		return;
	}
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe>* rig = rigs.Find(name);
	if (rig == nullptr)
		return;
	if(!rig->IsValid()) {
		rigs.Remove(name);
		return;
	}
	FLiveLinkFrameDataStruct frameData(FLiveLinkAnimationFrameData::StaticStruct());
	FLiveLinkAnimationFrameData& data = *frameData.Cast<FLiveLinkAnimationFrameData>();
	data.Transforms.Reserve(100);
	if ((*rig)->ProcessFrame(jsonPose, data)) {
		liveLinkClient->PushSubjectFrameData_AnyThread(subjectKeys[name], MoveTemp(frameData));
	}
}
