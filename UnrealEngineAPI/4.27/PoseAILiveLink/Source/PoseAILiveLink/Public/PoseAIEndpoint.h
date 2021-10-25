// Copyright Pose AI 2021

#pragma once

#include "CoreMinimal.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "SocketSubsystem.h"


TSharedPtr<FSocket> BuildUdpSocket(FString& description, FName protocolType, int32 port);


/**
 * Implements a more generic endpoint to allow for both IPv6 and IPv4 networks, based on Epic Games IPv4 endpoint from the Networking module.
 *
 * Mainly is a wrapper around an FInternetAddr with the helper functions needed to use the networking code with only minor modifications.
 *
 */
struct FPoseAIEndpoint
{
	/** Holds the endpoint's IP address. */
	TSharedPtr<FInternetAddr> Address;

	/** Holds the endpoint's port number. */
	uint16 Port;

public:

	/** Default constructor. */
	FPoseAIEndpoint() { }

	
	/**
	 * Creates and initializes a new endpoint from a given FInternetAddr object.
	 *
	 * Note: this constructor will be removed after the socket subsystem has been refactored.
	 *
	 * @param InternetAddr The Internet address.
	 */

	
	FPoseAIEndpoint(const TSharedPtr<FInternetAddr>& InternetAddr)
	{
		check(InternetAddr.IsValid());

		int32 OutPort;
		Address = InternetAddr;
		InternetAddr->GetPort(OutPort);
		Port = OutPort;
	}

	bool IsValid() const { return Address != nullptr && Address.IsValid(); }

public:

	/**
	 * Compares this endpoint with the given endpoint for equality.
	 *
	 * @param Other The endpoint to compare with.
	 * @return true if the endpoints are equal, false otherwise.
	 */
	bool operator==(const FPoseAIEndpoint& Other) const
	{
		return ((Address == Other.Address));
	}

	/**
	 * Compares this address with the given endpoint for inequality.
	 *
	 * @param Other The endpoint to compare with.
	 * @return true if the endpoints are not equal, false otherwise.
	 */
	bool operator!=(const FPoseAIEndpoint& Other) const
	{
		return (Address != Other.Address);
	}


public:

	
	/**
	 * Gets a string representation for this endpoint.
	 *
	 * @return String representation.
	 * @see Parse, ToText
	 */
	POSEAILIVELINK_API FString ToString() const;

	/**
	 * Gets the display text representation.
	 *
	 * @return Text representation.
	 * @see ToString
	 */
	FText ToText() const
	{
		return FText::FromString(ToString());
	}

	TSharedRef<FInternetAddr> ToInternetAddr() const
	{
		if (CachedSocketSubsystem == nullptr)
			Initialize();

		check(CachedSocketSubsystem != nullptr && "Networking module not loaded and initialized");
		TSharedRef<FInternetAddr> InternetAddr = CachedSocketSubsystem->CreateInternetAddr(Address->GetProtocolType());
		{
			InternetAddr->SetRawIp(Address->GetRawIp());
			InternetAddr->SetPort(Address->GetPort());
		}

		return InternetAddr;
	}

public:

	/** Initializes the IP endpoint functionality. */
	static void Initialize();



private:
	/** ISocketSubsystem::Get() is not thread-safe, so we cache it here. */
	static ISocketSubsystem* CachedSocketSubsystem;
};
