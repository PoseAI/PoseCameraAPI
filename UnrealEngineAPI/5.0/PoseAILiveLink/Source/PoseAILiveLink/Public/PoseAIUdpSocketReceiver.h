// Pose AI Ltd Copyright 2021
// This is a minor edit of Epic Games FUdpSocetReceiver class to allow different protocols (like IPv6)

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/SingleThreadRunnable.h"
#include "Serialization/ArrayReader.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

#include "PoseAIEndpoint.h"
#include "IPAddress.h"





/**
 * Temporary fix for concurrency crashes. This whole class will be redesigned.
 */
typedef TSharedPtr<FArrayReader, ESPMode::ThreadSafe> FArrayReaderPtr;

/**
 * Delegate type for received data.
 *
 * The first parameter is the received data.
 * The second parameter is sender's IP endpoint.
 */
DECLARE_DELEGATE_TwoParams(FPoseAIOnSocketDataReceived, const FString&, const FPoseAIEndpoint&);  //Change delegate name and use our endpoint


/**
 * Asynchronously receives data from an UDP socket.
 */
class FPoseAIUdpSocketReceiver
	: public FRunnable
	, private FSingleThreadRunnable
{
public:

	/**
	 * Creates and initializes a new socket receiver.
	 *
	 * @param InSocket The UDP socket to receive data from.
	 * @param InWaitTime The amount of time to wait for the socket to be readable.
	 * @param InThreadName The receiver thread name (for debugging).
	 */
	FPoseAIUdpSocketReceiver(TSharedPtr<FSocket> InSocket, const FTimespan& InWaitTime, const TCHAR* InThreadName)
		: Socket(InSocket)
		, Stopping(false)
		, Thread(nullptr)
		, ThreadName(InThreadName)
		, WaitTime(InWaitTime)
	{
		check(Socket != nullptr);
		check(Socket->GetSocketType() == SOCKTYPE_Datagram);
		Reader->SetNumUninitialized(MaxReadBufferSize);
		SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	}

	/** Virtual destructor. */
	virtual ~FPoseAIUdpSocketReceiver()
	{
		if (Thread != nullptr)
		{
			Thread->Kill(true);
			delete Thread;
		}
	}

public:

	/** Set the maximum size allocated to read off of the socket. */
	void SetMaxReadBufferSize(uint32 InMaxReadBufferSize)
	{
		MaxReadBufferSize = InMaxReadBufferSize;
	}

	/** Start the receiver thread. */
	void Start()
	{
		Thread = FRunnableThread::Create(this, *ThreadName, 128 * 1024, TPri_AboveNormal, FPlatformAffinity::GetPoolThreadMask());
	}

	/**
	 * Returns a delegate that is executed when data has been received.
	 *
	 * This delegate must be bound before the receiver thread is started with
	 * the Start() method. It cannot be unbound while the thread is running.
	 *
	 * @return The delegate.
	 */
	FPoseAIOnSocketDataReceived& OnDataReceived()
	{
		check(Thread == nullptr);
		return DataReceivedDelegate;
	}

public:

	//~ FRunnable interface

	virtual FSingleThreadRunnable* GetSingleThreadInterface() override
	{
		return this;
	}

	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		while (!Stopping)
		{
			isUpdating = true;
			Update(WaitTime);
			isUpdating = false;
		}

		return 0;
	}

	virtual void Stop() override
	{
		Stopping = true;
	}

	virtual void Exit() override { }

protected:

	/** Update this socket receiver. */
	void Update(const FTimespan& SocketWaitTime)
	{
		if (!Socket->Wait(ESocketWaitConditions::WaitForRead, SocketWaitTime))
		{
			return;
		}
		
		/*************************************************
			Hwere we make changes to specify address with protocol
			**********************************************************/
		if (Stopping)
			return;

		TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr(Socket->GetProtocol());
		uint32 Size;
		
		while (Socket!=nullptr && Socket.IsValid() && Socket->HasPendingData(Size))
		{			
			
			
			int32 Read = 0;
			if (Socket->RecvFrom(Reader->GetData(), FMath::Min(Size, MaxReadBufferSize), Read, *Sender))
			{
				// we also send the messages via delegate as FStrings instead of FArrayReaderPtrs
				UTF8CHAR* bytedata = (UTF8CHAR*)Reader->GetData();
				FString recvMessage = FString(Read, bytedata);
				DataReceivedDelegate.ExecuteIfBound(recvMessage, FPoseAIEndpoint(Sender));
			}
		}
		
		
	}

protected:

	//~ FSingleThreadRunnable interface

	virtual void Tick() override
	{
		Update(FTimespan::Zero());
	}

private:
	FArrayReaderPtr Reader = MakeShared<FArrayReader, ESPMode::ThreadSafe>(true);
	/** The network socket. */
	TSharedPtr<FSocket> Socket = nullptr;

	/** Pointer to the socket sub-system. */
	ISocketSubsystem* SocketSubsystem = nullptr;

	/** Flag indicating that the thread is stopping. */
	bool Stopping;

	/** The thread object. */
	FRunnableThread* Thread = nullptr;

	/** The receiver thread's name. */
	FString ThreadName;

	/** The amount of time to wait for inbound packets. */
	FTimespan WaitTime;

	/** The maximum read buffer size used to read the socket. */
	uint32 MaxReadBufferSize = 65507u;

	bool isUpdating = false;

private:

	/** Holds the data received delegate. */
	FPoseAIOnSocketDataReceived DataReceivedDelegate;
};

