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
#include "PoseAIEndpoint.h"

#define LOCTEXT_NAMESPACE "PoseAI"

class PoseAILiveLinkRunnable;
class FPoseAISocketSender;
class PoseAILiveLinkSource;

class POSEAILIVELINK_API PoseAILiveLinkServer
{
public:
	void ReceiveUDPDelegate(const FString& recvMessage, const FPoseAIEndpoint& endpoint);
	TSharedPtr<FSocket> GetSocket() const { return serverSocket; }
	
	void CreateServer(int32 port, FPoseAIHandshake myHandshake, PoseAILiveLinkSource* mySource, TSharedRef< PoseAILiveLinkServer> serverRef);
	
	// receiver will be set on a runnable thread and set once started
	void SetReceiver(TSharedPtr<FPoseAIUdpSocketReceiver> receiver) {
		udpSocketReceiver = receiver;
	}

	PoseAILiveLinkServer(bool isIPv6 = false) {
		protocolType = (isIPv6) ? FNetworkProtocolTypes::IPv6 : FNetworkProtocolTypes::IPv4;
	}

	~PoseAILiveLinkServer() {
		CleanUp();
	}

	void CleanUp();
	
	// utility function that identifies host IPv4 address, to be printed in LiveLink console to help user connect to correct address
	bool GetIP(FString& myIP);
	void Disconnect() const;
	void SendConfig(FName target, FPoseAIModelConfig config);
	void SetHandshake(const FPoseAIHandshake& handshake);

	void SendHandshake(const FPoseAIEndpoint& endpoint) const;



private:
	const static FString fieldPrettyName;
	const static FString fieldVersion;
	const static FString fieldUUID;
	const static FString requiredMinVersion;
	PoseAILiveLinkSource* source_ = nullptr;
	FPoseAIHandshake handshake;
	FName protocolType;
	int32 portNum;
	TSharedPtr<FSocket> serverSocket;

	//used to launch receiver without slowing main thread
	TSharedPtr<PoseAILiveLinkRunnable, ESPMode::ThreadSafe> poseAILiveLinkRunnable;
	//Listens for packets
	TSharedPtr<FPoseAIUdpSocketReceiver> udpSocketReceiver;
	//sends instructions to paired app
	TSharedPtr<FPoseAISocketSender> udpSocketSender;
	TMap<FString, FPoseAIEndpoint> endpointFromSessionID;
	TMap<FString, bool> hasRegistered;
	TMap<FString, FName> fnameFromSessionID;
	TMap<FName, FPoseAIEndpoint> endpointFromFName;
	
	bool cleaningUp = false;

	// disconnect message formatted for Pose AI mobile app
	FString disconnect = FString(TEXT("{\"REQUESTS\":[\"DISCONNECT\"]}"));
	bool SendString(const FPoseAIEndpoint& endpoint, FString& message) const;



	FName ExtractPrettyName(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpoint) const;

	// make sure mobile app is sufficiently advanced version as both endpoints of software evolve
	bool CheckAppVersion(FString version) const;

	//split clean up routine by component
	void CleanUpReceiver();
	void CleanUpSender();
	void CleanUpSocket();
};

class POSEAILIVELINK_API PoseAILiveLinkRunnable : public FRunnable
{

public:

	PoseAILiveLinkRunnable(int32 port, TSharedPtr<PoseAILiveLinkServer> server) :
		port(port), poseAILiveLinkServer(server) {
		myName = "PoseAILiveLinkServer_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *myName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {	
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Running server thread"));

		FTimespan inWaitTime = FTimespan::FromMilliseconds(250);
		FString receiverName = "PoseAILiveLink_Receiver_On_Port_" + FString::FromInt(port);
		udpSocketReceiver = MakeShared<FPoseAIUdpSocketReceiver>(poseAILiveLinkServer->GetSocket(), inWaitTime, * receiverName);
		udpSocketReceiver->OnDataReceived().BindSP(poseAILiveLinkServer.ToSharedRef(), &PoseAILiveLinkServer::ReceiveUDPDelegate);
		udpSocketReceiver->Start();
		poseAILiveLinkServer->SetReceiver(udpSocketReceiver);

		if (thread != nullptr)
			thread = nullptr;
		return 0;
	}
	

protected:
	FString myName;
	int32 port;
	FRunnableThread* thread = nullptr;
private:
	TSharedPtr<PoseAILiveLinkServer> poseAILiveLinkServer;
	TSharedPtr<FPoseAIUdpSocketReceiver> udpSocketReceiver;
};


// built in udpSocketSender kept crashing on cleanup so recreated one with sleep instead of tick/update 
class POSEAILIVELINK_API FPoseAISocketSender : public FRunnable
{
	// Structure for outbound packets.
	struct FPacket
	{
		/** Holds the packet's data. */
		TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Data = nullptr;

		/** Holds the recipient. */
		FPoseAIEndpoint Recipient;

		/** Default constructor. */
		FPacket() { }

		/** Creates and initializes a new instance. */
		FPacket(const TSharedRef<TArray<uint8>, ESPMode::ThreadSafe>& InData, const FPoseAIEndpoint& InRecipient)
			: Data(InData)
			, Recipient(InRecipient)
		{ }
	};
public:

	FPoseAISocketSender(TSharedPtr<FSocket> socket, const TCHAR* threadDescription) :
		socket(socket) {
		thread = FRunnableThread::Create(this, threadDescription, 0, EThreadPriority::TPri_Normal);
	}

	
	void ClearQueue() {
		{
			FScopeLock Lock(&QcritSection);
			while (!sendQueue.IsEmpty()) {
				FPacket packet;
				int32 sent = 0;

				if (socket == nullptr) {
					UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: socket missing from sender"));
					break;
				}
				sendQueue.Dequeue(packet);
				if (!packet.Recipient.Address.IsValid())
					continue;

				if (!socket->SendTo(packet.Data->GetData(), packet.Data->Num(), sent, *packet.Recipient.ToInternetAddr()))
					UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to send to %s"), *(packet.Recipient.ToString()));

				if (sent != packet.Data->Num())
					break;
			}
		}
	}

	virtual uint32 Run() override {
		while (running && thread == nullptr) {
			FPlatformProcess::Sleep(0.2);
		}

		while (running ) {
			ClearQueue();
			Sleep(true);
			while (running && sleeping) {
				FPlatformProcess::Sleep(0.005);	
			}
			
		}
		if (socket.IsValid())
			socket->Close();
		thread = nullptr;
		return 0;
	}
	virtual void Exit() override {
		ClearQueue();
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
			sendQueue.Enqueue(FPacket(Data, Recipient));
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
	/** The send queue. */
	TQueue<FPacket, EQueueMode::Mpsc> sendQueue = {};
	/* when the server is closed the main thread may also try to empty the queue (which can only have one consumer). This should guard against that*/
	FCriticalSection QcritSection;
	
		/** The network socket. */
	TSharedPtr<FSocket> socket;
	
	/** The thread object. */
	FRunnableThread* thread = nullptr;

	bool running = true;
	bool sleeping = false;
	bool closeSocket_ = false;
	const FTimespan socketWaitTime = FTimespan::FromMilliseconds(100);
	
};

