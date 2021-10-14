// Copyright Pose AI Ltd 2021

#include "PoseAILiveLinkServer.h"
#include "PoseAIRig.h"
#include "PoseAIEventDispatcher.h"
#include "PoseAILiveLinkSource.h"


const FString PoseAILiveLinkServer::requiredMinVersion = FString(TEXT("0.8.24"));
const FString PoseAILiveLinkServer::fieldPrettyName = FString(TEXT("userName"));
const FString PoseAILiveLinkServer::fieldUUID = FString(TEXT("UUID"));
const FString PoseAILiveLinkServer::fieldVersion = FString(TEXT("version"));



TSharedPtr<FSocket> BuildUdpSocket(FString& description, FName protocolType, int32 port) {
	
	FName socketType = NAME_DGram;
	FSocket* socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(socketType, description, protocolType);
	socket->SetNonBlocking();
	socket->SetReuseAddr();
	int actualSize;
	socket->SetReceiveBufferSize(64 * 1024, actualSize);
	socket->SetSendBufferSize(64 * 1024, actualSize);

	TSharedRef<FInternetAddr> sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(protocolType);
	sender->SetIp(0);
	sender->SetPort(port);
	socket->Bind(*sender);
	return MakeShareable(socket);
}


void PoseAILiveLinkServer::CreateServer(int32 port, FPoseAIHandshake myHandshake, PoseAILiveLinkSource* mySource, TSharedRef< PoseAILiveLinkServer> serverRef) {
	portNum = port;
	source_ = mySource;
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Creating Server"));
	
	FString serverName = "PoseAIServerSocketOnPort_" + FString::FromInt(port);
	serverSocket = BuildUdpSocket(serverName, protocolType, port);
	poseAILiveLinkRunnable = MakeShared<PoseAILiveLinkRunnable, ESPMode::ThreadSafe>(port, serverRef); 
	FString senderName = "PoseAILiveLinkSenderOnPort_" + FString::FromInt(port);
	udpSocketSender = MakeShared<FPoseAISocketSender>(serverSocket, *senderName);
	handshake = myHandshake;


	FString myIP;
	if (protocolType == FNetworkProtocolTypes::IPv6) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server on IPv6 link-Local address (begins with fe80:) and Port:%d"), port);
	} else if (GetIP(myIP)) {
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
	source_ = nullptr;
	if (!cleaningUp) {
		cleaningUp = true;
		CleanUpReceiver();
		CleanUpSender();
		//CleanUpSocket();
	}	

}

void PoseAILiveLinkServer::CleanUpReceiver() {
	if (udpSocketReceiver != nullptr && udpSocketReceiver.IsValid()) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketReceiver"));
		udpSocketReceiver->Stop();
		udpSocketReceiver->Exit();
		//udpSocketReceiver.Reset();
	}

	if (poseAILiveLinkRunnable != nullptr && poseAILiveLinkRunnable.IsValid()) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up serverThread"));
		poseAILiveLinkRunnable.Reset();
	}
}
void PoseAILiveLinkServer::CleanUpSender() {
	if (udpSocketSender != nullptr && udpSocketSender.IsValid()) {
		Disconnect();
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketSender"));
		udpSocketSender->Stop(); 
		udpSocketSender->Exit();
		//udpSocketSender.Reset();

	}

}
void PoseAILiveLinkServer::CleanUpSocket() {
	if (serverSocket != nullptr && serverSocket.IsValid()) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socket"));
		serverSocket->Close();
		//ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(serverSocket);
		serverSocket = nullptr;
	}
}

void PoseAILiveLinkServer::ReceiveUDPDelegate(const FString& recvMessage, const FPoseAIEndpoint& endpoint) {
	if (cleaningUp)
		return;

	FString sessionID = endpoint.ToString();
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(recvMessage);

	static const FGuid GUID_Error = FGuid();
	if (!FJsonSerializer::Deserialize(Reader, jsonObject)) {
		
		static const FName NAME_JsonError = "PoseAILiveLink_JsonError";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(sessionID));
		FLiveLinkLog::WarningOnce(NAME_JsonError, failKey, TEXT("PoseAI: failed to deserialize json object from %s"), *sessionID);
		return;
	}

	if (!endpointFromSessionID.Contains(sessionID)) {
		/* initialize new connection */
		FString version;
		if (!(jsonObject->TryGetStringField(fieldVersion, version))) {
			static const FName NAME_NoAppVersion = "PoseAILiveLink_NoAppVersion";
			FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(sessionID));
			FLiveLinkLog::WarningOnce(NAME_NoAppVersion, failKey, TEXT("PoseAI: Incoming connection does not have a hello handshake"));
			return;
		} else if (!CheckAppVersion(version)) {
			static const FName NAME_AppVersionFail = "PoseAILiveLink_AppVersionFail";
			FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(sessionID));
			FLiveLinkLog::WarningOnce(NAME_AppVersionFail, failKey, TEXT("PoseAI: Please update the mobile app to at least version %s"), *requiredMinVersion);
			UE_LOG(LogTemp, Error, TEXT("PoseAILiveLink: Unknown app version.  Can not safely connect."), *version);
			return;
		}

		FName prettyName = ExtractPrettyName(jsonObject, endpoint);
		UE_LOG(LogTemp, Display, TEXT("PoseAI: received new contact from %s on port %d"), *(prettyName.ToString()), endpoint.Port);
		endpointFromSessionID.Emplace(sessionID, endpoint);
		hasRegistered.Emplace(sessionID, false);

		fnameFromSessionID.Emplace(sessionID, prettyName);
		endpointFromFName.Emplace(prettyName, endpoint);
		SendHandshake(endpoint);
	} else if (!PoseAIRig::IsFrameData(jsonObject)){
		SendHandshake(endpoint);
	} else {
		FName subjectName = fnameFromSessionID[sessionID];
		if (!hasRegistered[sessionID]) {
			hasRegistered[sessionID] = true;
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastSubjectConnected(subjectName);
		}

		
		source_->UpdatePose(subjectName, jsonObject);
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastFrameReceived(subjectName);
	}
	
}

bool PoseAILiveLinkServer::SendString(const FPoseAIEndpoint& endpoint, FString& message) const {
	FTCHARToUTF8 byteConvert(*message);
	TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
	bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
	return udpSocketSender->Send(bytedata, endpoint);
}

void PoseAILiveLinkServer::SendHandshake(const FPoseAIEndpoint& endpoint) const {
	FString message_string = handshake.ToString();
	if (SendString(endpoint, message_string)) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent handshake %s to %s"), *message_string, *(endpoint.ToString()));
	} else { //unsuccesful
		static const FName NAME_HandshakeFail = "PoseAILiveLink_HandshakeFail";
		static const FGuid GUID_HandshakeFail = FGuid();
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_HandshakeFail, FName(endpoint.ToString()));
		FLiveLinkLog::WarningOnce(NAME_HandshakeFail, failKey, TEXT("PoseAI: Unable to send the handshake to %s"), *(endpoint.ToString()));
	}
}

void PoseAILiveLinkServer::SetHandshake(const FPoseAIHandshake& newHandshake) {
	bool dirty = handshake != newHandshake;
	handshake = newHandshake;
	if (dirty) {
		TArray<FPoseAIEndpoint> endpoints;
		endpointFromFName.GenerateValueArray(endpoints);
		for (auto& endpoint : endpoints) SendHandshake(endpoint);
	}
}


void PoseAILiveLinkServer::SendConfig(FName target, FPoseAIModelConfig config) {
	FPoseAIEndpoint* endpoint = endpointFromFName.Find(target);
	FString message_string = config.ToString();
	if (endpoint != nullptr) {
		if (SendString(*endpoint, message_string))
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent config %s to %s"), *message_string, *(endpoint->ToString()));
	}
}



void PoseAILiveLinkServer::Disconnect() const {
	FTCHARToUTF8 byteConvert(*disconnect);
	TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
	bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
	for (auto& endpoint : endpointFromSessionID) {
		if (udpSocketSender->Send(bytedata, endpoint.Value)) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent disconnect request to %s"), *(endpoint.Value.ToString()));
		}
		else { //unsuccesful
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Unable to disconnect from %s"), *(endpoint.Value.ToString()));
		}
	}
}


FName PoseAILiveLinkServer::ExtractPrettyName(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpoint) const
{
	//consider using uuid field instead to ensure uniqueness, in case multiple users connect from same IP address
	FString prettyString;
	if (!(jsonObject->TryGetStringField(fieldPrettyName, prettyString))) {
		prettyString = "Unknown";
	}
	return FName(*(prettyString.Append("@").Append(endpoint.Address->ToString(false))));
	
	
}

bool PoseAILiveLinkServer::CheckAppVersion(FString version) const
{
	
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
