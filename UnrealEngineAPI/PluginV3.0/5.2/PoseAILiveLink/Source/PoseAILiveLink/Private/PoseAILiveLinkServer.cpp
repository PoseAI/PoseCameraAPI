// Copyright Pose AI Ltd 2022.  All Rights Reserved.

#include "PoseAILiveLinkServer.h"
#include "Async/Async.h"
#include "PoseAIRig.h"
#include "PoseAIEventDispatcher.h"
#include "PoseAILiveLinkNetworkSource.h"

#define LOCTEXT_NAMESPACE "PoseAI"

const FString PoseAILiveLinkServer::requiredMinVersion = FString(TEXT("1.2.5"));
const FString PoseAILiveLinkServer::fieldPrettyName = FString(TEXT("userName"));
const FString PoseAILiveLinkServer::fieldUUID = FString(TEXT("UUID"));
const FString PoseAILiveLinkServer::fieldRigType = FString(TEXT("Rig"));
const FString PoseAILiveLinkServer::fieldVersion = FString(TEXT("version"));


PoseAILiveLinkServer::PoseAILiveLinkServer(FPoseAIHandshake myHandshake, bool isIPv6, int32 portNum) :
	listener(MakeShared<PoseAILiveLinkServerListener>(this)),
	handshake(myHandshake),
	port(portNum)
{

	protocolType = (isIPv6) ? FNetworkProtocolTypes::IPv6 : FNetworkProtocolTypes::IPv4;
	
	UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Creating Server"));
	
	FString serverName = "PoseAIServerSocketOnPort_" + FString::FromInt(port);
	FString senderName = "PoseAILiveLinkSenderOnPort_" + FString::FromInt(port);
	serverSocket = BuildUdpSocket(serverName, protocolType, port);
	poseAILiveLinkRunnable = MakeShared<PoseAILiveLinkReceiverRunnable, ESPMode::ThreadSafe>(port, listener, this);
	udpSocketSender = MakeShared<FPoseAISocketSender>(serverSocket, *senderName);
		
	FString myIP;
	if (protocolType == FNetworkProtocolTypes::IPv6) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server on IPv6 link-Local address (begins with fe80:) and Port:%d"), port);
	} else if (GetIP(myIP)) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server on %s Port:%d"), *myIP, port);				
	} else {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Created Server but can't determine your IP address.  You may not have a valid network adapter."));
	}
}

void PoseAILiveLinkServer::SetSource(TWeakPtr<PoseAILiveLinkNetworkSource> source) {
	source_ = source;
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
		TSharedPtr<FSocket> socketForClosure = serverSocket;
		uint32 portnum = port;
		// using delayed closure to try to resolve issue where Epic built plugin creates crashes while project plugin doesn't.
		Async(EAsyncExecution::Thread, [socketForClosure, portnum]() {
			FPlatformProcess::Sleep(2.0);
			socketForClosure->Close();
			UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Closed socket on Port:%d"), portnum);
			});		
	}	
}

void PoseAILiveLinkServer::CleanUpReceiver() {
	if (udpSocketReceiver && udpSocketReceiver.IsValid()) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketReceiver"));
		udpSocketReceiver->Stop();
	}

	if (poseAILiveLinkRunnable && poseAILiveLinkRunnable.IsValid()) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up serverThread"));
		poseAILiveLinkRunnable.Reset();
	}
}
void PoseAILiveLinkServer::CleanUpSender() {
	if (udpSocketSender && udpSocketSender.IsValid()) {
		Disconnect();
		UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Cleaning up socketSender"));
		udpSocketSender->Stop(); 
		udpSocketSender->Exit();
	}
}


bool PoseAILiveLinkServer::HasValidConnection() const {
	return endpoint.IsValid() && (FDateTime::Now() - lastConnection).GetTotalSeconds() < TIMEOUT_SECONDS;
}

void PoseAILiveLinkServer::ProcessNetworkPacket(const FString& recvMessage, const FPoseAIEndpoint& endpointRecv) {
	static const FGuid GUID_Error = FGuid();
	if (cleaningUp) return;

	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(recvMessage);
	
	if (!FJsonSerializer::Deserialize(Reader, jsonObject)) {
		static const FName NAME_JsonError = "PoseAILiveLink_JsonError";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(endpointRecv.ToString()));
		FLiveLinkLog::WarningOnce(NAME_JsonError, failKey, TEXT("PoseAI: failed to deserialize json object from %s, %s"), *endpointRecv.ToString(), *Reader->GetErrorMessage());
		return;
	}

	bool sameAsCurrent = endpoint.IsValid() && (endpoint.ToString() == endpointRecv.ToString());
	if (HasValidConnection() && !sameAsCurrent) {
		if (ExtractConnectionName(jsonObject, endpointRecv) == PoseAILiveLinkNetworkSource::GetConnectionName(port)) {
				endpoint = endpointRecv; //port has changed but IP and phone nmae same so just update endpoint
				SendHandshake();
		}
		else { //reject
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Ignoring contact from %s as already engaged."), *endpointRecv.ToString());
			//consider sending rejected connection a warning message
		}
	}
	else if (!HasValidConnection() && !sameAsCurrent) { // new connection
		InitiateConnection(jsonObject, endpointRecv);
		
	} 
	else {
		if (PoseAIRig::IsFrameData(jsonObject)) {
			lastConnection = FDateTime::Now();
			if (source_.IsValid()) {
				auto shared_ptr = source_.Pin();
				shared_ptr->UpdatePose(jsonObject);
				UPoseAIEventDispatcher::GetDispatcher()->BroadcastFrameReceived(shared_ptr->GetSubjectName());
			}
		}
		else if (ExtractConnectionName(jsonObject, endpointRecv) == PoseAILiveLinkNetworkSource::GetConnectionName(port)) { //is likely a repeat hello message
			SendHandshake();
		}
	}
}

void PoseAILiveLinkServer::InitiateConnection(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpointRecv) {
	static const FGuid GUID_Error = FGuid();
	FString version;
	if (!(jsonObject->TryGetStringField(fieldVersion, version))) {
		static const FName NAME_NoAppVersion = "PoseAILiveLink_NoAppVersion";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(endpointRecv.ToString()));
		FLiveLinkLog::WarningOnce(NAME_NoAppVersion, failKey, TEXT("PoseAI: Incoming connection does not have a hello handshake"));
		return;
	}
	else if (!CheckAppVersion(version)) {
		static const FName NAME_AppVersionFail = "PoseAILiveLink_AppVersionFail";
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_Error, FName(endpointRecv.ToString()));
		FLiveLinkLog::WarningOnce(NAME_AppVersionFail, failKey, TEXT("PoseAI: Please update the mobile app to at least version %s"), *requiredMinVersion);
		UE_LOG(LogTemp, Error, TEXT("PoseAILiveLink: Unknown app version.  Can not safely connect."), *version);
		return;
	}
	FName connectionName = ExtractConnectionName(jsonObject, endpointRecv);
	UE_LOG(LogTemp, Display, TEXT("PoseAI: received new contact from %s on port %d"), *(connectionName.ToString()), endpointRecv.Port);
	if (source_.IsValid()) {
		source_.Pin()->SetConnectionName(connectionName);
		endpoint = endpointRecv;
		SendHandshake();
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastSubjectConnected(source_.Pin()->GetSubjectName());
		lastConnection = FDateTime::Now();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("PoseAI: Unable to setup Source."));
	}
}

bool PoseAILiveLinkServer::SendString(FString& message) const {
	if (endpoint.IsValid()) {
		FTCHARToUTF8 byteConvert(*message);
		TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
		bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
		return udpSocketSender->Send(bytedata, endpoint);
	}
	else {
		return false;
	}
}


void PoseAILiveLinkServer::SendHandshake() const {
	FString message_string = handshake.ToString();
	if (SendString(message_string)) {
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent handshake %s to %s"), *message_string, *(endpoint.ToString()));
	} else { //unsuccessful
		static const FName NAME_HandshakeFail = "PoseAILiveLink_HandshakeFail";
		static const FGuid GUID_HandshakeFail = FGuid();
		FLiveLinkSubjectKey failKey = FLiveLinkSubjectKey(GUID_HandshakeFail, FName(endpoint.ToString()));
		FLiveLinkLog::WarningOnce(NAME_HandshakeFail, failKey, TEXT("PoseAI: Unable to send the handshake to %s"), *(endpoint.ToString()));
	}
}

void PoseAILiveLinkServer::SetHandshake(const FPoseAIHandshake& newHandshake) {
	handshake = newHandshake;
	if (endpoint.IsValid()) 
		SendHandshake();
}



void PoseAILiveLinkServer::Disconnect()  {
	if (endpoint.IsValid()) {
		FTCHARToUTF8 byteConvert(*disconnect);
		TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> bytedata = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
		bytedata->Append((uint8*)byteConvert.Get(), byteConvert.Length());;
		if (udpSocketSender->Send(bytedata, endpoint)) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Sent disconnect request to %s"), *(endpoint.ToString()));
		}
		else { //unsuccesful
			UE_LOG(LogTemp, Display, TEXT("PoseAI: Unable to disconnect from %s"), *(endpoint.ToString()));
		}
	}
}


FName PoseAILiveLinkServer::ExtractConnectionName(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpointRecv) const
{
	FString prettyString;
	if (!(jsonObject->TryGetStringField(fieldPrettyName, prettyString))) {
		prettyString = "Unknown";
	}
	return FName(*(prettyString.Append("@").Append(endpointRecv.Address->ToString(false))));
}

bool PoseAILiveLinkServer::CheckAppVersion(FString version) const
{
	UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: App version %s vs required version %s."), *version, *requiredMinVersion);
	TArray<FString> appArray;
	version.ParseIntoArray(appArray, TEXT("."), false);
	if (appArray.Num() < 3) {
		UE_LOG(LogTemp, Warning, TEXT("PoseAILiveLink: App version %s format error"), *version);
	}

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


uint32 PoseAILiveLinkReceiverRunnable::Run() {
	FTimespan inWaitTime = FTimespan::FromMilliseconds(250);
	FString receiverName = "PoseAILiveLink_Receiver_On_Port_" + FString::FromInt(port);
	udpSocketReceiver = MakeShared<FPoseAIUdpSocketReceiver>(poseAILiveLinkServer->GetSocket(), inWaitTime, *receiverName);
	udpSocketReceiver->OnDataReceived().BindSP(listener.ToSharedRef(), &PoseAILiveLinkServerListener::ReceiveUDPDelegate);
	udpSocketReceiver->Start();
	poseAILiveLinkServer->SetReceiver(udpSocketReceiver);
	poseAILiveLinkServer = nullptr;
	listener = nullptr;
	thread = nullptr;
	return 0;
}

#undef LOCTEXT_NAMESPACE