// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Runtime/Networking/Public/Common/UdpSocketBuilder.h"
#include "Runtime/Networking/Public/Common/UdpSocketReceiver.h"
#include "Runtime/Networking/Public/Common/UdpSocketSender.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "IPAddress.h"
#include "Json.h"
#include "PoseAIHandshake.h"


#define LOCTEXT_NAMESPACE "PoseAI"


DECLARE_DELEGATE_TwoParams(FPoseFrameDelegate, FName&, TSharedPtr<FJsonObject>);

class PoseAILiveLinkRunnable;
class FPoseAISocketSender;


class POSEAILIVELINK_API PoseAILiveLinkServer
{
public:
	void ReceiveUDPDelegate(const FArrayReaderPtr& arrayReaderPtr, const FIPv4Endpoint& endpoint);
	FSocket* GetSocket() const { return serverSocket; }
	
	void CreateServer(int32 port, PoseAIHandshake myHandshake);
	void SetReceiver(TSharedPtr<FUdpSocketReceiver, ESPMode::ThreadSafe> receiver) {
		udpSocketReceiver = receiver;
	}

	FPoseFrameDelegate& OnPoseReceived() {
		return poseFrameDelegate;
	}

	~PoseAILiveLinkServer() {
		CleanUp();
	}

	void CleanUp();
	

	bool GetIP(FString& myIP);
	void Disconnect() const;

	PoseAIHandshake handshake;

private:
	const static FString fieldPrettyName;
	const static FString fieldVersion;
	const static FString requiredMinVersion;

	int32 portNum;
	FSocket* serverSocket;
	TSharedPtr<FUdpSocketReceiver, ESPMode::ThreadSafe> udpSocketReceiver;
	TSharedPtr<FPoseAISocketSender, ESPMode::ThreadSafe> udpSocketSender;
	TSharedPtr<PoseAILiveLinkRunnable, ESPMode::ThreadSafe> poseAILiveLinkRunnable;
	FPoseFrameDelegate poseFrameDelegate;
	TMap<FString, FIPv4Endpoint> knownSockets;
	TMap<FString, FName> prettyNames;
	
	bool cleaningUp = false;
	FString disconnect = FString(TEXT("{\"REQUESTS\":[\"DISCONNECT\"]}"));

	void SendHandshake(const FIPv4Endpoint& endpoint) const;

	bool ExtractPrettyName(TSharedPtr<FJsonObject> jsonObject, FName& prettyName) const;
	bool CheckAppVersion(TSharedPtr<FJsonObject> jsonObject) const;
	void CleanUpReceiver();
	void CleanUpSender();
	void CleanUpSocket();
};

class POSEAILIVELINK_API PoseAILiveLinkRunnable : public FRunnable
{

public:

	PoseAILiveLinkRunnable(int32 port, PoseAILiveLinkServer* server) :
		port(port), poseAILiveLinkServer(server) {
		myName = "PoseAILiveLinkServer_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *myName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {	
		UE_LOG(LogTemp, Display, TEXT("PoseAI: Running server thread"));

		FTimespan inWaitTime = FTimespan::FromMilliseconds(250);
		FString receiverName = "PoseAILiveLink_Receiver_On_Port_" + FString::FromInt(port);
		TSharedPtr<FUdpSocketReceiver, ESPMode::ThreadSafe> udpSocketReceiver = MakeShared<FUdpSocketReceiver, ESPMode::ThreadSafe>(poseAILiveLinkServer->GetSocket(), inWaitTime, * receiverName);
		udpSocketReceiver->OnDataReceived().BindRaw(poseAILiveLinkServer, &PoseAILiveLinkServer::ReceiveUDPDelegate);
		udpSocketReceiver->Start();
		poseAILiveLinkServer->SetReceiver(udpSocketReceiver);

		if (thread != nullptr)
			thread = nullptr;
		return 0;
	}
	

protected:
	FString myName;
	int32 port;
	FRunnableThread* thread;
private:
	PoseAILiveLinkServer* poseAILiveLinkServer;
};


// built in udpSocetSender kept crashng on cleanup so recreated one with sleep instead of tick/update 
class POSEAILIVELINK_API FPoseAISocketSender : public FRunnable
{
	// Structure for outbound packets.
	struct FPacket
	{
		/** Holds the packet's data. */
		TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Data;

		/** Holds the recipient. */
		FIPv4Endpoint Recipient;

		/** Default constructor. */
		FPacket() { }

		/** Creates and initializes a new instance. */
		FPacket(const TSharedRef<TArray<uint8>, ESPMode::ThreadSafe>& InData, const FIPv4Endpoint& InRecipient)
			: Data(InData)
			, Recipient(InRecipient)
		{ }
	};
public:

	FPoseAISocketSender(FSocket* socket, const TCHAR* threadDescription) :
		socket(socket) {
		thread = FRunnableThread::Create(this, threadDescription, 0, EThreadPriority::TPri_Normal);
	}

	
	bool ClearQueue() {
		while (sendQueue.IsEmpty() == false) {
			if (socket == nullptr)
				return false;
			/*
			if (!socket->Wait(ESocketWaitConditions::WaitForWrite, socketWaitTime))
			{
				break;
			}*/
			FPacket packet;
			int32 sent = 0;
			sendQueue.Dequeue(packet);
			socket->SendTo(packet.Data->GetData(), packet.Data->Num(), sent, *packet.Recipient.ToInternetAddr());

			if (sent != packet.Data->Num())
			{
				return false;
			}
		}
		return true;
	}

	virtual uint32 Run() override {
		while (running && thread == nullptr) {
			FPlatformProcess::Sleep(0.2);
		}

		while (running ) {
			if (!ClearQueue())
				return false;
			
			if (running) {
				Sleep(true);
				while (running && sleeping) {
					FPlatformProcess::Sleep(0.005);	
				}
			}
		}
		thread = nullptr;
		return 0;
	}

	void StopThread(bool closeSocket) {
		closeSocket_ = closeSocket;
		running = false;
		ClearQueue();
		if (thread != nullptr) {
			Sleep(false);
		}
		if (closeSocket_) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: Closing socket"));
			socket->Close();
		}
		socket = nullptr;
	}

	bool Send(const TSharedRef<TArray<uint8>, ESPMode::ThreadSafe>& Data, const FIPv4Endpoint& Recipient)
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
	TQueue<FPacket, EQueueMode::Mpsc> sendQueue;

		/** The network socket. */
	FSocket* socket;
	
	/** The thread object. */
	FRunnableThread* thread = nullptr;

	bool running = true;
	bool sleeping = false;
	bool closeSocket_ = false;
	const FTimespan socketWaitTime = FTimespan::FromMilliseconds(100);
	
};

