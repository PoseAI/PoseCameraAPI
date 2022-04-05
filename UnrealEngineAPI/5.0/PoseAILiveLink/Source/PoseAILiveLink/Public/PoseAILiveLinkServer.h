// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "LiveLinkLog.h"
#include "SocketTypes.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "IPAddress.h"
#include "Json.h"
#include "PoseAIStructs.h"
#include "PoseAIUdpSocketReceiver.h"
#include "PoseAIRig.h"
#include "PoseAIEndpoint.h"

#define LOCTEXT_NAMESPACE "PoseAI"

class PoseAILiveLinkReceiverRunnable;
class FPoseAISocketSender;
class PoseAILiveLinkSingleSource;


class POSEAILIVELINK_API PoseAILiveLinkServer
{
	friend PoseAILiveLinkSingleSource;
public:
	PoseAILiveLinkServer(FPoseAIHandshake myHandshake, PoseAILiveLinkSingleSource* mySource, bool isIPv6 = false, int32 portNum = 8080);

	~PoseAILiveLinkServer() {
		CleanUp();
	}

	// utility function that identifies host IPv4 address, to be printed in LiveLink console to help user connect to correct address
	static bool GetIP(FString& myIP);

	void CleanUp();
	void CloseTarget(const FLiveLinkSubjectName& target);
	void Disconnect();
	void DisconnectTarget(const FLiveLinkSubjectName& target);

	TSharedPtr<FSocket> GetSocket() const { return serverSocket; }
	void ReceiveUDPDelegate(const FString& recvMessage, const FPoseAIEndpoint& endpoint);
	void SendConfig(const FLiveLinkSubjectName& target, FPoseAIModelConfig config);
	void SendHandshake() const;
	void SetHandshake(const FPoseAIHandshake& handshake);
	// receiver will be set on a runnable thread and set once started
	void SetReceiver(TSharedPtr<FPoseAIUdpSocketReceiver> receiver) { udpSocketReceiver = receiver; }


private:
	const static FString fieldPrettyName;
	const static FString fieldVersion;
	const static FString fieldUUID;
	static const FString fieldRigType;
	const static FString requiredMinVersion;

	
	PoseAILiveLinkSingleSource* source_ = nullptr;
	
	FPoseAIHandshake handshake;
	FName protocolType;
	int32 port;
	
	// time of last connection.  After timeout seconds a newer connection can takeover the port.
	FDateTime lastConnection;
	const double TIMEOUT_SECONDS = 10.0;

	// dirty flag which source checks during update to see if rig static data needs updating
	bool hasNewRig = false;

	TSharedPtr<FSocket> serverSocket;
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rig;
	//used to launch receiver without slowing main thread
	TSharedPtr<PoseAILiveLinkReceiverRunnable, ESPMode::ThreadSafe> poseAILiveLinkRunnable;
	//Listens for packets
	TSharedPtr<FPoseAIUdpSocketReceiver> udpSocketReceiver;
	//sends instructions to paired app
	TSharedPtr<FPoseAISocketSender> udpSocketSender;
	FPoseAIEndpoint endpoint;
	
	

	// disconnect message formatted for Pose AI mobile app
	FString disconnect = FString(TEXT("{\"REQUESTS\":[\"DISCONNECT\"]}"));
	
	void InitiateConnection(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpointRecv);

	bool SendString(FString& message) const;
	bool HasValidConnection() const;

	FName ExtractConnectionName(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpoint) const;

	// make sure mobile app is sufficiently advanced version as both endpoints of software evolve
	bool CheckAppVersion(FString version) const;

	//split clean up routine by component
	void CleanUpReceiver();
	void CleanUpSender();
	void CleanUpSocket();
	bool cleaningUp = false;
};

class POSEAILIVELINK_API PoseAILiveLinkReceiverRunnable : public FRunnable
{
public:
	PoseAILiveLinkReceiverRunnable(int32 port, PoseAILiveLinkServer* server) :
		port(port), poseAILiveLinkServer(server) {
		myName = "PoseAILiveLinkServer_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *myName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {	
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Running server thread"));
		FTimespan inWaitTime = FTimespan::FromMilliseconds(250);
		FString receiverName = "PoseAILiveLink_Receiver_On_Port_" + FString::FromInt(port);
		udpSocketReceiver = MakeShared<FPoseAIUdpSocketReceiver>(poseAILiveLinkServer->GetSocket(), inWaitTime, * receiverName);
		udpSocketReceiver->OnDataReceived().BindRaw(poseAILiveLinkServer, &PoseAILiveLinkServer::ReceiveUDPDelegate);
		udpSocketReceiver->Start();
		poseAILiveLinkServer->SetReceiver(udpSocketReceiver);
		poseAILiveLinkServer = nullptr;
		thread = nullptr;
		return 0;
	}
	
protected:
	FString myName;
	int32 port;
	FRunnableThread* thread = nullptr;
private:
	PoseAILiveLinkServer* poseAILiveLinkServer;
	TSharedPtr<FPoseAIUdpSocketReceiver> udpSocketReceiver;
};


// built in udpSocketSender kept crashing on cleanup so recreated one with sleep instead of tick/update 
class POSEAILIVELINK_API FPoseAISocketSender : public FRunnable
{
public:
	FPoseAISocketSender(TSharedPtr<FSocket> socket, const TCHAR* threadDescription) :
		socket(socket) {
		thread = FRunnableThread::Create(this, threadDescription, 0, EThreadPriority::TPri_Normal);
	}


	virtual uint32 Run() override {
		while (running && thread == nullptr) {
			FPlatformProcess::Sleep(0.2);
		}

		while (running ) {
			Sleep(true);
			while (running && sleeping) {
				FPlatformProcess::Sleep(0.005);	
			}
			
		}
		
		thread = nullptr;
		return 0;
	}

	virtual void Stop() override {
		running = false;
		if (thread != nullptr) {
			Sleep(false);
		}
	}

	bool Send(const TSharedRef<TArray<uint8>, ESPMode::ThreadSafe>& Data, const FPoseAIEndpoint& Recipient)
	{
		if (running) {
			int32 sent = 0;
			if (socket == nullptr) {
				UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: socket missing from sender"));
				return false;
			}
								
			if (!socket->SendTo(Data->GetData(), Data->Num(), sent, *Recipient.ToInternetAddr()))
				UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to send to %s"), *(Recipient.ToString()));

			if (sent != Data->Num())
				return false;
			
			Sleep(false);
			return true;
		}
		return false;
	}

	void Sleep(bool sleep) {
		sleeping = sleep;
		if (thread != nullptr)
			thread->Suspend(sleep);
	}

protected:
	/** The network socket. */
	TSharedPtr<FSocket> socket;
	
	/** The thread object. */
	FRunnableThread* thread = nullptr;

	bool running = true;
	bool sleeping = false;
};

