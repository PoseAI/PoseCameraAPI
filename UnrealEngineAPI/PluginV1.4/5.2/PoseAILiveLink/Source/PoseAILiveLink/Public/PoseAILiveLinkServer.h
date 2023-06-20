// Copyright Pose AI Ltd 2022.  All Rights Reserved.

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
#include "SocketSubsystem.h"


class PoseAILiveLinkReceiverRunnable;
class PoseAILiveLinkNetworkSource;
class PoseAILiveLinkServerListener;
class FPoseAISocketSender;

// The networking class needs to be rewritten

class POSEAILIVELINK_API PoseAILiveLinkServer
{
public:
	PoseAILiveLinkServer(FPoseAIHandshake myHandshake, bool isIPv6, int32 portNum);
	void SetSource(TWeakPtr<PoseAILiveLinkNetworkSource> source);

	~PoseAILiveLinkServer() {
		CleanUp();
	}

	// utility function that identifies host IPv4 address, to be printed in LiveLink console to help user connect to correct address
	static bool GetIP(FString& myIP);

	void CleanUp();
	void Disconnect();

	TSharedPtr<FSocket> GetSocket() const { return serverSocket; }

	void ProcessNetworkPacket(const FString& recvMessage, const FPoseAIEndpoint& endpoint);


	bool SendString(FString& message) const;
	void SendHandshake() const;
	void SetHandshake(const FPoseAIHandshake& handshake);
	// receiver will be set on a runnable thread and set once started
	void SetReceiver(TSharedPtr<FPoseAIUdpSocketReceiver> receiver) { udpSocketReceiver = receiver; }


private:
	const static FString fieldPrettyName;
	const static FString fieldVersion;
	const static FString fieldUUID;
	const static FString fieldRigType;
	const static FString requiredMinVersion;

	TSharedPtr<PoseAILiveLinkServerListener> listener;
	TWeakPtr<PoseAILiveLinkNetworkSource> source_;
	FPoseAIHandshake handshake;
	FName protocolType;
	int32 port;
	bool cleaningUp = false;
	
	// time of last connection.  After timeout seconds a newer connection can takeover the port.
	FDateTime lastConnection;
	const double TIMEOUT_SECONDS = 10.0;

	TSharedPtr<FSocket> serverSocket;
	
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
	

	bool HasValidConnection() const;

	FName ExtractConnectionName(TSharedPtr<FJsonObject> jsonObject, const FPoseAIEndpoint& endpoint) const;

	// make sure mobile app is sufficiently advanced version as both endpoints of software evolve
	bool CheckAppVersion(FString version) const;

	//split clean up routine by component
	void CleanUpReceiver();
	void CleanUpSender();
	void CleanUpSocket();
	
};



class POSEAILIVELINK_API PoseAILiveLinkReceiverRunnable : public FRunnable
{
public:
	PoseAILiveLinkReceiverRunnable(int32 port, TSharedPtr<PoseAILiveLinkServerListener> listener, PoseAILiveLinkServer* poseAILiveLinkServer) :
		port(port), poseAILiveLinkServer(poseAILiveLinkServer), listener(listener) {
		myName = "PoseAILiveLinkServer_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *myName, 0, EThreadPriority::TPri_Normal);
	}
	virtual uint32 Run() override;
	
protected:
	FString myName;
	int32 port;
	FRunnableThread* thread = nullptr;
private:
	PoseAILiveLinkServer* poseAILiveLinkServer;
	TSharedPtr<PoseAILiveLinkServerListener> listener;
	TSharedPtr<FPoseAIUdpSocketReceiver> udpSocketReceiver;
};




// built in udpSocketSender kept crashing on cleanup so recreated one with sleep instead of tick/update 
class POSEAILIVELINK_API FPoseAISocketSender : public FRunnable
{
public:
	FPoseAISocketSender(TSharedPtr<FSocket> Socket, const TCHAR* threadDescription) :
		Socket(Socket) {
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
			if (!Socket) {
				UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: socket missing from sender"));
				return false;
			}
								
			if (!Socket->SendTo(Data->GetData(), Data->Num(), sent, *Recipient.ToInternetAddr()))
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
	TSharedPtr<FSocket> Socket;
	
	/** The thread object. */
	FRunnableThread* thread = nullptr;

	bool running = true;
	bool sleeping = false;
};


/* 
* To improve stability with the delegate system we use a listener component class which
* can be wrapped with smart pointers for binding (raw pointer delegate bindings are a potential source of crashes)
*/
class PoseAILiveLinkServerListener {
public:
	void ReceiveUDPDelegate(const FString& recvMessage, const FPoseAIEndpoint& endpoint) {
		parent->ProcessNetworkPacket(recvMessage, endpoint);
	}
	PoseAILiveLinkServerListener(PoseAILiveLinkServer* parent) : parent(parent) {}
private:
	PoseAILiveLinkServer* parent;
};

