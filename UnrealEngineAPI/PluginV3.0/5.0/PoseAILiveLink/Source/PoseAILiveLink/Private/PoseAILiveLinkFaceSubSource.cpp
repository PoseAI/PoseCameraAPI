// Copyright Pose AI Ltd 2023.  All Rights Reserved.

#include "PoseAILiveLinkFaceSubSource.h"
#include "PoseAIStructs.h"
#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "PoseAI"


static FName ParseEnumName(FName EnumName)
{
	const int32 BlendShapeEnumNameLength = 22;
	FString EnumString = EnumName.ToString();
	return FName(*EnumString.Right(EnumString.Len() - BlendShapeEnumNameLength));
}


PoseAILiveLinkFaceSubSource::PoseAILiveLinkFaceSubSource(FLiveLinkSubjectKey& poseSubjectKey, ILiveLinkClient* liveLinkClient) : liveLinkClient(liveLinkClient) {

	//Update the subject key to match latest one
	subjectKey = FLiveLinkSubjectKey(poseSubjectKey.Source, FName(*(FString("Face-") + poseSubjectKey.SubjectName.ToString())));
	//Update property names array
	StaticData.PropertyNames.Reset((int32)PoseAIFaceBlendShape::MAX);

	//Iterate through all valid blend shapes to extract names
	const UEnum* EnumPtr = StaticEnum<PoseAIFaceBlendShape>();
	for (int32 Shape = 0; Shape < (int32)PoseAIFaceBlendShape::MAX; Shape++)
	{
		const FName ShapeName = ParseEnumName(EnumPtr->GetNameByValue(Shape));
		StaticData.PropertyNames.Add(ShapeName);
	}
}


bool PoseAILiveLinkFaceSubSource::AddSubject(FCriticalSection& InSynchObject){
	bool success = false;
	if (liveLinkClient && IsInGameThread()) {
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		FLiveLinkSubjectPreset subject;
		subject.bEnabled = true;
		subject.Key = subjectKey;
		subject.Role = TSubclassOf<ULiveLinkRole>(ULiveLinkBasicRole::StaticClass());
		subject.Settings = nullptr;
		subject.VirtualSubject = nullptr;
		FScopeLock ScopeLock(&InSynchObject);

		if (liveLinkClient->CreateSubject(subject)) {
			UE_LOG(LogTemp, Display, TEXT("PoseAI LiveLink: created face subject %s"), *(subjectKey.SubjectName.Name.ToString()));
			FLiveLinkStaticDataStruct StaticDataStruct(FLiveLinkBaseStaticData::StaticStruct());
			FLiveLinkBaseStaticData* BaseStaticData = StaticDataStruct.Cast<FLiveLinkBaseStaticData>();
			BaseStaticData->PropertyNames = StaticData.PropertyNames;
			liveLinkClient->PushSubjectStaticData_AnyThread(subject.Key, ULiveLinkBasicRole::StaticClass(), MoveTemp(StaticDataStruct));
			success = true;
			
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: unable to create subject %s"), *(subjectKey.SubjectName.Name.ToString()));
		}
	}
	return success;
}

bool PoseAILiveLinkFaceSubSource::RequestSubSourceShutdown()
{
	if (liveLinkClient) {
		liveLinkClient->RemoveSubject_AnyThread(subjectKey);
		liveLinkClient = nullptr;
	}	
	return true;
}



void PoseAILiveLinkFaceSubSource::UpdateFace(TSharedPtr<FJsonObject> jsonPose)
{
	if (liveLinkClient) {
		FLiveLinkFrameDataStruct FrameDataStruct(FLiveLinkBaseFrameData::StaticStruct());
		FLiveLinkBaseFrameData* FrameData = FrameDataStruct.Cast<FLiveLinkBaseFrameData>();
		FrameData->WorldTime = FPlatformTime::Seconds();
		//FrameData->MetaData.SceneTime = FrameTime;
		
		FrameData->PropertyValues.Reserve((int32)PoseAIFaceBlendShape::MAX);
		if (jsonPose != nullptr && jsonPose->HasField("Face")) {
			uint32 packetFormat = 1;
			jsonPose->TryGetNumberField("PF", packetFormat);

			if (packetFormat == 0) {
				auto blendShapes = jsonPose->GetArrayField("Face");
				// Iterate through all of the blend shapes copying them into the LiveLink data type
				for (int32 Shape = 0; Shape < (int32)PoseAIFaceBlendShape::MAX; Shape++)
				{
					const float CurveValue = blendShapes[Shape]->AsNumber();
					FrameData->PropertyValues.Add(CurveValue);
				}
			}
			else {
				TArray<float> blendShapes;
				FString compactFace = jsonPose->GetStringField("Face");
				FStringFixed12ToFloat(compactFace, blendShapes);
				// Iterate through all of the blend shapes copying them into the LiveLink data type
				for (int32 Shape = 0; Shape < (int32)PoseAIFaceBlendShape::MAX; Shape++)
				{
					const float CurveValue = blendShapes[Shape];
					FrameData->PropertyValues.Add(CurveValue);
				}
			}
			

			// Share the data locally with the LiveLink client
			liveLinkClient->PushSubjectFrameData_AnyThread(subjectKey, MoveTemp(FrameDataStruct));
		}
	}
}

#undef LOCTEXT_NAMESPACE
