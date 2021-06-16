// Copyright Pose AI Ltd 2021

#include "PoseAILiveLinkServer.h"
#include "Json.h"
#include "PoseAIRig.h"
#include "LiveLinkLog.h"


const FString PoseAILiveLinkServer::requiredMinVersion = FString(TEXT("0.6.0"));
const FString PoseAILiveLinkServer::fieldPrettyName = FString(TEXT("userName"));
const FString PoseAILiveLinkServer::fieldVersion = FString(TEXT("version"));


void PoseAILiveLinkServer::CreateServer(int32 port, PoseAIHandshake myHandshake) {
	portNum = port;
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Creating Server"));
	serverSocket = FUdpSocketBuilder(FString("PoseAIServerSocket")).AsNonBlocking().AsReusable().BoundToPort(port).WithReceiveBufferSize(64 * 1024).WithSendBufferSize(64 * 1024).Build();
	poseAILiveLinkRunnable = MakeShared<PoseAILiveLinkRunnable, ESPMode::ThreadSafe>(port, this);
	FString senderName = "PoseAILiveLink_Sender_On_Port_" + FString::FromInt(port);
	udpSocketSender = MakeShared<FPoseAISocketSender, ESPMode::ThreadSafe>(serverSocket, *senderName);
	handshake = myHandshake;

	FString myIP;
	if (GetIP(myIP)) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server on %s Port:%d"), *myIP, port);
	} else {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server but can't determine your IP address.  You may not have a valid network adapter."));
	}
}

bool PoseAILiveLinkServer::GetIP(FString& myIP) {
	bool canBind = false;
	TSharedRef<FInternetAddr> localIp = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, canBind);
	if (localIp->IsValid()) {
		myIP = (localIp->ToString(false));
		return true;
	} else {
		myIP = LOCTEXT("undeterminedIP", "Can't determine your local host IP address").ToString();
		return false;
	}
}

void PoseAILiveLinkServer::CleanUp() {
	if (!cleaningUp) {
		cleaningUp = true;
		CleanUpReceiver();
		CleanUpSender();
		//CleanUpSocket();
	}	
}

void PoseAILiveLinkServer::CleanUpReceiver() {

	if (OnPoseReceived().IsBound())
		OnPoseReceived().Unbind();

	if (udpSocketReceiver != nullptr) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketReceiver"));
		udpSocketReceiver->Stop();
		udpSocketReceiver->Exit();
		udpSocketReceiver = nullptr;
	}

	if (poseAILiveLinkRunnable && poseAILiveLinkRunnable != nullptr) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up serverThread"));
		poseAILiveLinkRunnable = nullptr;
	}
}
void PoseAILiveLinkServer::CleanUpSender() {
	if (udpSocketSender != nullptr) {
		Disconnect();
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketSender"));
		udpSocketSender->StopThread(true); //closing socket here to make sure outgoing message is sent
		udpSocketSender = nullptr;
	}

}
void PoseAILiveLinkServer::CleanUpSocket() {
	if (serverSocket && serverSocket != nullptr) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socket"));
		serverSocket->Close();
		//ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(serverSocket);
		serverSocket = nullptr;
	}
}

void PoseAILiveLinkServer::ReceiveUDPDelegate(const FArrayReaderPtr& arrayReaderPtr, const FIPv4Endpoint& endpoint) {
	if (cleaningUp)
		return;

	FString sessionID = endpoint.ToString();
	
	FString recvMessage;
	char* bytedata = (char*)arrayReaderPtr->GetData();
	bytedata[arrayReaderPtr->Num()] = '\0';
	recvMessage = FString(UTF8_TO_TCHAR(bytedata));

	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(recvMessage);

	static const FGuid GUID_Error = FGuid();
	if (!FJsonSerializer::Deserialize(Reader, jsonObject)) {
		
		static const FName NAME_JsonError = "PoseAILiveLink_JsonError";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(sessionID));
		FLiveLinkLog::WarningOnce(NAME_JsonError, failKey, TEXT("PoseAI: failed to deserialize json object from %s"), *sessionID);
		return;
	}

	if (!knownSockets.Contains(sessionID)) {
		/* initialize new connection */

		if (!CheckAppVersion(jsonObject)) {
			static const FName NAME_AppVersionFail = "PoseAILiveLink_AppVersionFail";
			FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(sessionID));
			FLiveLinkLog::WarningOnce(NAME_AppVersionFail, failKey, TEXT("PoseAI: Please update the mobile app to at least version %s"), *requiredMinVersion);
			return;
		}

		FName prettyName;
		if (ExtractPrettyName(jsonObject, prettyName)) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI: received new contact from %s on %s"), *(prettyName.ToString()), *sessionID);
		}
		else {
			prettyName = FName(*sessionID);
			UE_LOG(LogTemp, Display, TEXT("PoseAI: received new contact from %s"), *sessionID);
		}

		knownSockets.Emplace(sessionID, endpoint);
		prettyNames.Emplace(sessionID, prettyName);
		SendHandshake(endpoint);
	} else if (!PoseAIRig::IsFrameData(jsonObject)){
		SendHandshake(endpoint);
	} else {
		FName sourceName = prettyNames[sessionID];
		poseFrameDelegate.ExecuteIfBound(sourceName, jsonObject);
	}
}

void PoseAILiveLinkServer::SendHandshake(const FIPv4Endpoint& endpoint) const {
	FTCHARToUTF8 byteConvert(*handshake.ToString());
	TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
	bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
	if (udpSocketSender->Send(bytedata, endpoint)) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent handshake to %s"), *(endpoint.ToString()));
	} else { //unsuccesful
		static const FName NAME_HandshakenFail = "PoseAILiveLink_HandshakeFail";
		static const FGuid GUID_HandshakeFail = FGuid();
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_HandshakeFail, FName(endpoint.ToString()));
		FLiveLinkLog::WarningOnce(NAME_HandshakenFail, failKey, TEXT("PoseAI: Unable to send the handhsake to %s"), *(endpoint.ToString()));
	}
}

void PoseAILiveLinkServer::Disconnect() const {
	FTCHARToUTF8 byteConvert(*disconnect);
	TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
	bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
	for (auto& endpoint : knownSockets) {
		if (udpSocketSender->Send(bytedata, endpoint.Value)) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent disconnect request to %s"), *(endpoint.Value.ToString()));
		}
		else { //unsuccesful
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Unable to disconnect from %s"), *(endpoint.Value.ToString()));
		}
	}
}


bool PoseAILiveLinkServer::ExtractPrettyName(TSharedPtr<FJsonObject> jsonObject, FName& prettyName) const
{
	FString prettyString;
	if (!(jsonObject->TryGetStringField(fieldPrettyName, prettyString))) {
		return false;
	}
	prettyName = FName(*prettyString);
	return true;
	
}

bool PoseAILiveLinkServer::CheckAppVersion(TSharedPtr<FJsonObject> jsonObject) const
{
	FString version;
	if (!(jsonObject->TryGetStringField(fieldVersion, version))) {
		UE_LOG(LogTemp, Error, TEXT("PoseAILiveLink: Unknown app version.  Can not safely connect."), *version);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: App version %s vs required version %s."), *version, *requiredMinVersion);
	TArray<FString> appArray;
	version.ParseIntoArray(appArray, TEXT("."), false);
	TArray<FString> requiredArray;
	requiredMinVersion.ParseIntoArray(requiredArray, TEXT("."), false);
	for (int32 i = 0; i < 3; i++) {
		int32 app = FCString::Atoi(*appArray[i]);
		int32 req = FCString::Atoi(*requiredArray[i]);
		if (app < req)
			return false;
		else if (app > req)
			return true;
	}
	return true;
}
