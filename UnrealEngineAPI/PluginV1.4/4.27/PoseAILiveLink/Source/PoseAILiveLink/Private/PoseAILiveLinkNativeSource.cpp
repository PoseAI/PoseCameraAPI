// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLinkNativeSource.h"
#include "Features/IModularFeatures.h"
#include "PoseAIEventDispatcher.h"



/* First use the static method to create a source and add it to the LiveLinkClient.
*  The LiveLinkClient must own only shared pointer or UE will crash on cleanup.
*  The client will respond with receive client when it is registered.  We return a weak ptr
*  so the caller can access source if necessary, as the LiveLink system really wants to own the only shared ptr.
*/
TWeakPtr<PoseAILiveLinkNativeSource> PoseAILiveLinkNativeSource::AddSource(FName subjectName, const FPoseAIHandshake& handshake) {
	if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		FLiveLinkClient& LiveLinkClient = IModularFeatures::Get().GetModularFeature<FLiveLinkClient>(ILiveLinkClient::ModularFeatureName);
		TSharedPtr<PoseAILiveLinkNativeSource> PoseAISource = MakeShared<PoseAILiveLinkNativeSource>(subjectName, handshake);
		TWeakPtr<PoseAILiveLinkNativeSource> weakPtr(PoseAISource);
		TSharedPtr<ILiveLinkSource> Source = StaticCastSharedPtr<ILiveLinkSource>(PoseAISource);
		LiveLinkClient.AddSource(Source);
		LiveLinkClient.Tick();
		return weakPtr;
	}
	else {
		return nullptr;
	}
}


 /* the source is initilized by the static method using the name and the handshake parameters. The name
 * governs how the source will appear in the LiveLink UI and how to connect in the LiveLinkPose node in the animation blueprint
 */
PoseAILiveLinkNativeSource::PoseAILiveLinkNativeSource(FName subjectName, const FPoseAIHandshake& handshake) :
	subjectName(subjectName), handshake(handshake), status(LOCTEXT("statusConnecting", "connecting"))
{
	UPoseAIEventDispatcher* dispatcher;
	dispatcher = UPoseAIEventDispatcher::GetDispatcher();
}

/* 
  After the source is added to the LiveLinkClient, the LiveLinkClient calls back the source. We store the assigned guid and client pointer
  and here we add the subject since we will only have one per client
*/
void PoseAILiveLinkNativeSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	status = FText::FormatOrdered(LOCTEXT("statusLocalConnected", "Connected to {0}"), FText::FromName(subjectName));
	sourceGuid = InSourceGuid;
	subjectKey = FLiveLinkSubjectKey(sourceGuid, subjectName);
	liveLinkClient = InClient;

	faceSubSource = TUniquePtr<PoseAILiveLinkFaceSubSource>(new PoseAILiveLinkFaceSubSource(subjectKey, liveLinkClient));
	faceSubSource->AddSubject(InSynchObject);
}

/* 
	After the source is added to the LiveLinkClient and does the ReceiveClient callback, the client creates settings and calls this function.  
*/
void PoseAILiveLinkNativeSource::InitializeSettings(ULiveLinkSourceSettings* Settings) {
	Settings->BufferSettings.MaxNumberOfFrameToBuffered = 1;
	Settings->Mode = ELiveLinkSourceMode::Latest;
}



bool PoseAILiveLinkNativeSource::AddSubject(){

	rig = PoseAIRig::PoseAIRigFactory(subjectName, handshake);
	
	if (rig.IsValid() && liveLinkClient && IsInGameThread()) {
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		FLiveLinkSubjectPreset subject;
		subject.bEnabled = true;
		subject.Key = subjectKey;
		subject.Role = TSubclassOf<ULiveLinkRole>(ULiveLinkAnimationRole::StaticClass());
		subject.Settings = nullptr;
		subject.VirtualSubject = nullptr;
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastSubjectConnected(subjectKey.SubjectName);
		FScopeLock ScopeLock(&InSynchObject);

		if (!liveLinkClient->CreateSubject(subject)) {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to create subject %s"), *(subjectKey.SubjectName.Name.ToString()));
			return false;
		}
		else {
			UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: created subject %s"), *(subjectKey.SubjectName.Name.ToString()));
			FLiveLinkStaticDataStruct rigDefinition = rig->MakeStaticData();
			liveLinkClient->PushSubjectStaticData_AnyThread(subject.Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(rigDefinition));
			return true;
		}
	}
	else {
		return false;
	}
	
}



bool PoseAILiveLinkNativeSource::IsSourceStillValid() const { return true; }


void PoseAILiveLinkNativeSource::disable()
{
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: disabling the source"));
	status = LOCTEXT("statusDisabled", "disabled");
	liveLinkClient = nullptr;
}



bool PoseAILiveLinkNativeSource::RequestSourceShutdown()
{
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: PoseAILiveLinkLocalSource request source shutdown"));
	if (liveLinkClient) {
		faceSubSource->RequestSubSourceShutdown();
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		liveLinkClient->RemoveSource(sourceGuid);
		liveLinkClient = nullptr;
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: removing subject %s"), *(subjectKey.SubjectName.Name.ToString()));
	}
	
	return true;
}

void PoseAILiveLinkNativeSource::ReceivePacket(const FString& recvMessage) {
	static const FGuid GUID_Error = FGuid();

	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(recvMessage);

	if (!FJsonSerializer::Deserialize(Reader, jsonObject)) {
		static const FName NAME_JsonError = "PoseAILiveLink_JsonError";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName("PoseAINativeSource"));
		FLiveLinkLog::WarningOnce(NAME_JsonError, failKey, TEXT("PoseAI: failed to deserialize json object from local posecam, %s"), *Reader->GetErrorMessage());
		return;
	}
	UpdatePose(jsonObject);
}


void PoseAILiveLinkNativeSource::UpdatePose(TSharedPtr<FJsonObject> jsonPose)
{

	if (liveLinkClient && rig && rig.IsValid()) {

		FLiveLinkFrameDataStruct frameData(FLiveLinkAnimationFrameData::StaticStruct());
		FLiveLinkAnimationFrameData& data = *frameData.Cast<FLiveLinkAnimationFrameData>();
		data.Transforms.Reserve(100);

		if (rig->ProcessFrame(jsonPose, data)) {
			liveLinkClient->PushSubjectFrameData_AnyThread(subjectKey, MoveTemp(frameData));
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastFrameReceived(subjectKey.SubjectName);
			faceSubSource->UpdateFace(jsonPose);
		}
	}
}

