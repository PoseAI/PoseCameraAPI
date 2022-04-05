// Copyright Pose AI 2021.  All rights reserved

#pragma once

#include "CoreMinimal.h"


struct POSEAILIVELINK_API PoseAIHandshake
{
	FString mode;
	FString rig;
	int32 syncFPS;
	int32 cameraFPS;
	bool isMirrored;
	

	FString ToString() const {
		FString handshake = FString::Printf(
			TEXT("{\"HANDSHAKE\":{"
				"\"name\":\"Unreal LiveLink\","
				"\"rig\":\"%s\", " 
				"\"mode\":\"%s\", "
				"\"mirror\":\"%s\", "
				"\"syncFPS\": %d, "
				"\"cameraFPS\": %d"
			"}}"),
			*rig,
			*mode,
			*(YesNo(isMirrored)),
			syncFPS,
			cameraFPS
			);
		return handshake;
	}

	inline FString YesNo(bool val) const {
		return FString(val ? TEXT("YES") : TEXT("NO"));
	}

};
