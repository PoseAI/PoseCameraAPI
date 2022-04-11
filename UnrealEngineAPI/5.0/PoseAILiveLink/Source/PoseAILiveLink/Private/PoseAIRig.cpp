// Copyright Pose AI Ltd 2021

#include "PoseAIRig.h"
#include "PoseAIEventDispatcher.h"


const FString PoseAIRig::fieldBody = FString(TEXT("Body"));
const FString PoseAIRig::fieldRigType = FString(TEXT("Rig"));
const FString PoseAIRig::fieldHandLeft = FString(TEXT("LeftHand"));
const FString PoseAIRig::fieldHandRight = FString(TEXT("RightHand"));
const FString PoseAIRig::fieldRotations = FString(TEXT("Rotations"));
const FString PoseAIRig::fieldScalars = FString(TEXT("Scalars"));
const FString PoseAIRig::fieldEvents = FString(TEXT("Events"));
const FString PoseAIRig::fieldVectors = FString(TEXT("Vectors"));


bool isDifferentAndSet(int32 newValue, int32& storedValue) {
	bool isDifferent = newValue != storedValue;
	storedValue = newValue;
	return isDifferent;
}


PoseAIRig::PoseAIRig(FLiveLinkSubjectName name, const FPoseAIHandshake& handshake) :
	name(name),
	rigType(FName(handshake.rig)),
	useRootMotion(handshake.useRootMotion), 
	includeHands(!handshake.mode.Contains(TEXT("BodyOnly"))),
	isMirrored(handshake.isMirrored),
	isDesktop(handshake.mode.Contains(TEXT("Desktop"))) {
	Configure();
}


TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> PoseAIRig::PoseAIRigFactory(const FLiveLinkSubjectName& name, const FPoseAIHandshake& handshake) {
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rigPtr;
	FName rigType = FName(handshake.rig);
	if (rigType == FName("Mixamo")) {
		rigPtr = MakeShared<PoseAIRigMixamo, ESPMode::ThreadSafe>(name, handshake);
	}
	else if (rigType == FName("MetaHuman")) {
		rigPtr = MakeShared<PoseAIRigMetaHuman, ESPMode::ThreadSafe>(name, handshake);  
	}
	else if (rigType == FName("DazUE")) {
		rigPtr = MakeShared<PoseAIRigDazUE, ESPMode::ThreadSafe>(name, handshake);  
	}
	else {
		rigPtr = MakeShared<PoseAIRigUE4, ESPMode::ThreadSafe>(name, handshake);;
	}
	rigPtr->Configure();
	return rigPtr;
}

FLiveLinkStaticDataStruct PoseAIRig::MakeStaticData(){
	FLiveLinkStaticDataStruct staticData;
	staticData.InitializeWith(FLiveLinkSkeletonStaticData::StaticStruct(), nullptr);
	FLiveLinkSkeletonStaticData* skelData = staticData.Cast<FLiveLinkSkeletonStaticData>();
	check(skelData);
	skelData->SetBoneNames(jointNames);
	skelData->SetBoneParents(parentIndices);
	return staticData;
}

void PoseAIRig::AddBone(FName boneName, FName parentName, FVector translation) {
	jointNames.Emplace(boneName);
	if (parentName == boneName) {
		parentIndices.Emplace(-1);
	} else {
		parentIndices.Emplace(jointNames.IndexOfByKey(parentName));
	}
	boneVectors.Add(boneName,translation);
	lastBoneAdded = boneName;
}

void PoseAIRig::AddBoneToLast(FName boneName, FVector translation) {
	AddBone(boneName, lastBoneAdded, translation);
}

bool PoseAIRig::IsFrameData(const TSharedPtr<FJsonObject> jsonObject)
{
	return (jsonObject->HasField(fieldBody)) || (jsonObject->HasField(fieldHandLeft)) || (jsonObject->HasField(fieldHandRight));	
}

bool PoseAIRig::ProcessFrame(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data)
{
	double timestamp;
	jsonObject->TryGetNumberField("Timestamp", timestamp);
	// drop packets which are older than latest.  in case clock changes capping staleness test at 600 seconds. 
	if (liveValues.timestamp - 600.0 < timestamp && timestamp < liveValues.timestamp) {
		return false;
	}
	liveValues.timestamp = timestamp;

	FString rigStringOut;
	if (jsonObject->TryGetStringField(fieldRigType, rigStringOut) && FName(rigStringOut) != rigType)
		UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: Rig is streaming in %s format, expected %s format."), *rigStringOut, *rigType.ToString());

	uint32 packetFormat = 0;
	jsonObject->TryGetNumberField("PF", packetFormat);

	if (packetFormat == 1)
		ProcessCompactSupplementaryData(jsonObject, data);
	else
		ProcessVerboseSupplementaryData(jsonObject, data);

	if (visibilityFlags.isTorso && liveValues.bodyHeight > 0.0f)
		liveValues.rootTranslation = FVector(
			-liveValues.hipScreen[0] * rigHeight / liveValues.bodyHeight, //x is left in Unreal so flip
			0.0f, //currently no body distance estimate from pose camera
			0.0f
		);

	TriggerEvents();

	data.WorldTime = FPlatformTime::Seconds();
	bool has_processed = (packetFormat == 1) ? ProcessCompactRotations(jsonObject, data) : ProcessVerboseRotations(jsonObject, data);
	return has_processed;
}

void PoseAIRig::TriggerEvents() {
	/* trigger various events and update the Pose AI Movement Component */
	if (visibilityFlags.HasChanged()) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastVisibilityChange(name, visibilityFlags);
	}
	if (verbose.Events.Jump.CheckTriggerAndUpdate()) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastJumps(name);
	}
	if (verbose.Events.Footstep.CheckTriggerAndUpdate()) {
		float height = FMath::Abs(verbose.Events.Footstep.Magnitude);
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastFootsteps(name, height, verbose.Events.Footstep.Magnitude > 0.0f);
	}
	if (verbose.Events.FeetSplit.CheckTriggerAndUpdate()) {
		float width = FMath::Abs(verbose.Events.FeetSplit.Magnitude);
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastFeetsplits(name, width, verbose.Events.FeetSplit.Magnitude < 0.0f);
	}
	if (verbose.Events.ArmPump.CheckTriggerAndUpdate()) {
		float height = FMath::Abs(verbose.Events.ArmPump.Magnitude);
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmpumps(name, height);
	}
	if (verbose.Events.ArmFlex.CheckTriggerAndUpdate()) {
		float width = FMath::Abs(verbose.Events.ArmFlex.Magnitude);
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmflexes(name, width, verbose.Events.ArmFlex.Magnitude < 0.0f);
	}
	if (verbose.Events.SidestepL.CheckTriggerAndUpdate()) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastSidestepL(name, verbose.Events.SidestepL.Magnitude < 0.0f);
	}
	if (verbose.Events.SidestepR.CheckTriggerAndUpdate()) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastSidestepR(name, verbose.Events.SidestepR.Magnitude < 0.0f);
	}
	if (verbose.Events.ArmGestureL.CheckTriggerAndUpdate()) {
		if (verbose.Events.ArmGestureL.Current == 50)
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmjacks(name, true);
		else if (verbose.Events.ArmGestureL.Current == 51)
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmjacks(name, false);
		else
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmGestureL(name, verbose.Events.ArmGestureL.Current);
	}
	if (verbose.Events.ArmGestureR.CheckTriggerAndUpdate()) {
		if (verbose.Events.ArmGestureR.Current < 50)
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastArmGestureR(name, verbose.Events.ArmGestureR.Current);
	}
	if (isDifferentAndSet(liveValues.handZoneLeft, handZoneL)) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastHandToZoneL(name, handZoneL);
	}
	if (isDifferentAndSet(liveValues.handZoneRight, handZoneR)) {
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastHandToZoneR(name, handZoneR);
	}
	if (isDifferentAndSet(liveValues.stableFeet, stableFeet)) {
		if (stableFeet > 1)
			UPoseAIEventDispatcher::GetDispatcher()->BroadcastStationary(name);
	}
	if (liveValues.isCrouching != isCrouching) {
		isCrouching = !isCrouching;
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastCrouches(name, isCrouching);
	}
	if (visibilityFlags.isTorso)
		UPoseAIEventDispatcher::GetDispatcher()->BroadcastLiveValues(name, liveValues);
}


void PoseAIRig::ProcessCompactSupplementaryData(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data)
{
	TSharedPtr < FJsonObject > objBody;
	TSharedPtr < FJsonObject > objHandLeft;
	TSharedPtr < FJsonObject > objHandRight;

	jsonObject->TryGetNumberField("ModelLatency", liveValues.modelLatency);
	
	objBody = (jsonObject->HasTypedField<EJson::Object>(fieldBody)) ? jsonObject->GetObjectField(fieldBody) : nullptr;
	if (objBody != nullptr && objBody.IsValid()) {
		FString VisA = (objBody->HasTypedField<EJson::String>("VisA")) ? objBody->GetStringField("VisA") : "";
		FString ScaA = (objBody->HasTypedField<EJson::String>("ScaA")) ? objBody->GetStringField("ScaA") : "";
		FString VecA = (objBody->HasTypedField<EJson::String>("VecA")) ? objBody->GetStringField("VecA") : "";
		FString EveA = (objBody->HasTypedField<EJson::String>("EveA")) ? objBody->GetStringField("EveA") : "";
		visibilityFlags.ProcessCompact(VisA);
		liveValues.ProcessCompactScalarsBody(ScaA);
		liveValues.ProcessCompactVectorsBody(VecA);
		verbose.Events.ProcessCompactBody(EveA);
		liveValues.jumpHeight = verbose.Events.Jump.Magnitude;

	}
	objHandLeft = (jsonObject->HasTypedField<EJson::Object>(fieldHandLeft)) ? jsonObject->GetObjectField(fieldHandLeft) : nullptr;
	if (objHandLeft != nullptr && objHandLeft.IsValid()) {
		FString VecA = (objHandLeft->HasTypedField<EJson::String>("VecA")) ? objHandLeft->GetStringField("VecA") : "";
		liveValues.ProcessCompactVectorsHandLeft(VecA);
	}

	objHandRight = (jsonObject->HasTypedField<EJson::Object>(fieldHandRight)) ? jsonObject->GetObjectField(fieldHandRight) : nullptr;
	if (objHandRight != nullptr && objHandRight.IsValid()) {
		FString VecA = (objHandRight->HasTypedField<EJson::String>("VecA")) ? objHandRight->GetStringField("VecA") : "";
		liveValues.ProcessCompactVectorsHandRight(VecA);
	}
}

void PoseAIRig::AssignCharacterMotion(FLiveLinkAnimationFrameData& data) {
	FVector baseTranslation;
	float minZ = 0.0f;
	if (isDesktop)
		baseTranslation = FVector(0.0f, 0.0f, rigHeight * 0.5f);
	else {
		baseTranslation = liveValues.rootTranslation; //careful as this assumes liveValues has been updated already this frame
		TArray<FTransform> componentTransform;
		componentTransform.Emplace(data.Transforms[0]);
		componentTransform.Emplace(data.Transforms[1]);
		// to ensure grounding in the capsule, calculates lowest Z in component space.  doesn't check fingers to save calculations on fingers: if this is important consider using parents.Num()
		for (int32 j = 2; j < numBodyJoints; j++) {
			componentTransform.Emplace(data.Transforms[j] * componentTransform[parentIndices[j]]);
			minZ = FGenericPlatformMath::Min(minZ, componentTransform[j].GetTranslation().Z);
		}
	}
	minZ -= rootHipOffsetZ;

	// assigns motion either to root or to hips
	if (useRootMotion) {
		//hip to low point Z distance assigned to hips, rest of movement assigned to root
		data.Transforms[1].SetTranslation(FVector(0.0f, 0.0f, -minZ));
		data.Transforms[0].SetTranslation(baseTranslation);
	}
	else {
		baseTranslation.Z -= minZ;
		data.Transforms[1].SetTranslation(baseTranslation);
	}
}

bool PoseAIRig::ProcessCompactRotations(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data)
{
	TSharedPtr < FJsonObject > objBody;
	TSharedPtr < FJsonObject > objHandLeft;
	TSharedPtr < FJsonObject > objHandRight;
	FString rotaBody;
	FString rotaHandLeft;
	FString rotaHandRight;

	objBody = (jsonObject->HasTypedField<EJson::Object>(fieldBody)) ? jsonObject->GetObjectField(fieldBody) : nullptr;
	if (objBody != nullptr && objBody.IsValid())
		rotaBody = (objBody->HasTypedField<EJson::String>("RotA")) ? objBody->GetStringField("RotA") : "";

	objHandLeft = (jsonObject->HasTypedField<EJson::Object>(fieldHandLeft)) ? jsonObject->GetObjectField(fieldHandLeft) : nullptr;
	if (objHandLeft != nullptr && objHandLeft.IsValid())
		rotaHandLeft = (objHandLeft->HasTypedField<EJson::String>("RotA")) ? objHandLeft->GetStringField("RotA") : "";

	objHandRight = (jsonObject->HasTypedField<EJson::Object>(fieldHandRight)) ? jsonObject->GetObjectField(fieldHandRight) : nullptr;
	if (objHandRight != nullptr && objHandRight.IsValid())
		rotaHandRight = (objHandRight->HasTypedField<EJson::String>("RotA")) ? objHandRight->GetStringField("RotA") : "";


	bool hasProcessedRotations;

	if ((rotaBody.Len() < 8 && cachedPose.Num() < 1) ) {
		hasProcessedRotations = false;
	}
	else if (rotaBody.Len() < 8 ) {
		data.Transforms.Append(cachedPose);
		hasProcessedRotations = true;
	}
	else {
		TArray<FQuat> componentRotations;
		AppendCachedRotations(0, 1, componentRotations, data);

		if (rotaBody.Len() > 7) {
			TArray<float> flatArray;
			TArray<FQuat> quatArray;
			FStringFixed12ToFloat(rotaBody, flatArray);
			FlatArrayToQuats(flatArray, quatArray);
			AppendQuatArray(quatArray, 1, componentRotations, data); //start at 1 as psoe camera does not include the root joint
		}
		else
			AppendCachedRotations(1, numBodyJoints, componentRotations, data);
		
		if (includeHands) {
			if (rotaHandLeft.Len() > 7) {
				TArray<float> flatArray;
				TArray<FQuat> quatArray;
				FStringFixed12ToFloat(rotaHandLeft, flatArray);
				FlatArrayToQuats(flatArray, quatArray);
				AppendQuatArray(quatArray, numBodyJoints, componentRotations, data);
			}
			else
				AppendCachedRotations(numBodyJoints, numBodyJoints + numHandJoints, componentRotations, data);
			if (rotaHandRight.Len() > 7) {
				TArray<float> flatArray;
				TArray<FQuat> quatArray;
				FStringFixed12ToFloat(rotaHandRight, flatArray);
				FlatArrayToQuats(flatArray, quatArray);
				AppendQuatArray(quatArray, numBodyJoints + numHandJoints, componentRotations, data);
			}
			else
				AppendCachedRotations(numBodyJoints + numHandJoints, numBodyJoints + 2 * numHandJoints, componentRotations, data);
		}
		AssignCharacterMotion(data);
		CachePose(data.Transforms);
		hasProcessedRotations = true;
	}
	return hasProcessedRotations;
}

bool PoseAIRig::ProcessVerboseRotations(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data)
{
	TSharedPtr < FJsonObject > objBody;
	TSharedPtr < FJsonObject > objHandLeft;
	TSharedPtr < FJsonObject > objHandRight;
	TSharedPtr < FJsonObject > rotBody = nullptr;
	TSharedPtr < FJsonObject > rotHandLeft = nullptr;
	TSharedPtr < FJsonObject > rotHandRight = nullptr;

	objBody = (jsonObject->HasTypedField<EJson::Object>(fieldBody)) ? jsonObject->GetObjectField(fieldBody) : nullptr;
	if (objBody != nullptr && objBody.IsValid())
		rotBody = (objBody->HasTypedField<EJson::Object>(fieldRotations)) ? objBody->GetObjectField(fieldRotations) : nullptr;

	objHandLeft = (jsonObject->HasTypedField<EJson::Object>(fieldHandLeft)) ? jsonObject->GetObjectField(fieldHandLeft) : nullptr;
	if (objHandLeft != nullptr && objHandLeft.IsValid())
		rotHandLeft = (objHandLeft->HasTypedField<EJson::Object>(fieldRotations)) ? objHandLeft->GetObjectField(fieldRotations) : nullptr;

	objHandRight = (jsonObject->HasTypedField<EJson::Object>(fieldHandRight)) ? jsonObject->GetObjectField(fieldHandRight) : nullptr;
	if (objHandRight != nullptr && objHandRight.IsValid())
		rotHandRight = (objHandRight->HasTypedField<EJson::Object>(fieldRotations)) ? objHandRight->GetObjectField(fieldRotations) : nullptr;


	bool hasProcessedRotations;
	if (rotBody == nullptr && cachedPose.Num() < 1) {
		hasProcessedRotations = false;
	}
	else if (rotBody == nullptr || !visibilityFlags.isTorso) {
		data.Transforms.Append(cachedPose);
		hasProcessedRotations = true;
	}
	else {
		TArray<FQuat> componentRotations;

		for (int32 i = 0; i < jointNames.Num(); i++) {
			const FName& jointName = jointNames[i];
			int32 parentIdx = parentIndices[i];
			FQuat parentQuat = (parentIdx < 0 ? FQuat::Identity : componentRotations[parentIdx]);
			const FVector& translation = boneVectors.FindRef(jointName);
			FQuat rotation;
			const TArray < TSharedPtr < FJsonValue > >* outArray;
			FString jointString = jointName.ToString();
			if ((rotBody != nullptr && rotBody->TryGetArrayField(jointString, outArray)) ||
				(rotHandLeft != nullptr && rotHandLeft->TryGetArrayField(jointString, outArray)) ||
				(rotHandRight != nullptr && rotHandRight->TryGetArrayField(jointString, outArray))) {
				rotation = FQuat((*outArray)[0]->AsNumber(), (*outArray)[1]->AsNumber(), (*outArray)[2]->AsNumber(), (*outArray)[3]->AsNumber());
			}
			else if (cachedPose.Num() > i) {
				rotation = parentQuat * cachedPose[i].GetRotation();
			}
			else {
				rotation = FQuat::Identity;
			}
			componentRotations.Add(rotation);
			FQuat finalRotation = parentQuat.Inverse() * rotation;
			finalRotation.Normalize();
			FTransform transform = FTransform(finalRotation, translation, FVector::OneVector);
			//Live link expects local rotations.  Consider having this sent directly by PoseAI app to save calculations
			data.Transforms.Add(transform);
		}

		AssignCharacterMotion(data);
		CachePose(data.Transforms);

		hasProcessedRotations = true;
	}
	return hasProcessedRotations;
}

void PoseAIRig::ProcessVerboseSupplementaryData(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkAnimationFrameData& data)
{
	TSharedPtr < FJsonObject > objBody;
	TSharedPtr < FJsonObject > objHandLeft;
	TSharedPtr < FJsonObject > objHandRight;
	TSharedPtr < FJsonObject > scaBody = nullptr;
	TSharedPtr < FJsonObject > eveBody = nullptr;
	TSharedPtr < FJsonObject > vecBody = nullptr;
	TSharedPtr < FJsonObject > vecHandLeft = nullptr;
	TSharedPtr < FJsonObject > vecHandRight = nullptr;

	jsonObject->TryGetNumberField("ModelLatency", liveValues.modelLatency);

	objBody = (jsonObject->HasTypedField<EJson::Object>(fieldBody)) ? jsonObject->GetObjectField(fieldBody) : nullptr;
	objHandLeft = (jsonObject->HasTypedField<EJson::Object>(fieldHandLeft)) ? jsonObject->GetObjectField(fieldHandLeft) : nullptr;
	objHandRight = (jsonObject->HasTypedField<EJson::Object>(fieldHandRight)) ? jsonObject->GetObjectField(fieldHandRight) : nullptr;

	if (objBody != nullptr && objBody.IsValid()) {
		verbose.ProcessJsonObject(objBody);
		//vecBody = (objBody->HasTypedField<EJson::Object>(fieldVectors)) ? objBody->GetObjectField(fieldVectors) : nullptr;
	}

	if (objHandLeft != nullptr && objHandLeft.IsValid()) {
		vecHandLeft = (objHandLeft->HasTypedField<EJson::Object>(fieldVectors)) ? objHandLeft->GetObjectField(fieldVectors) : nullptr;

	}
	if (objHandRight != nullptr && objHandRight.IsValid()) {
		vecHandRight = (objHandRight->HasTypedField<EJson::Object>(fieldVectors)) ? objHandRight->GetObjectField(fieldVectors) : nullptr;
	}

	liveValues.ProcessVerboseBody(verbose);
	liveValues.ProcessVerboseVectorsHandLeft(vecHandLeft);
	liveValues.ProcesssVerboseVectorsHandRight(vecHandRight);
	liveValues.jumpHeight = verbose.Events.Jump.Magnitude;
	visibilityFlags.ProcessVerbose(verbose.Scalars);
}

void PoseAIRig::AppendQuatArray(const TArray<FQuat>& quatArray, int32 begin, TArray<FQuat>& componentRotations, FLiveLinkAnimationFrameData& data) {
	for (int32 i = begin; i < begin + quatArray.Num(); i++) {
		const FName& jointName = jointNames[i];
		int32 parentIdx = parentIndices[i];
		const FQuat& rotation = quatArray[i - begin];
		FQuat parentQuat = (parentIdx < 0 ? FQuat::Identity : componentRotations[parentIdx]);
		const FVector& translation = boneVectors.FindRef(jointName);
		componentRotations.Add(rotation);
		FQuat finalRotation = parentQuat.Inverse() * rotation;
		finalRotation.Normalize();
		FTransform transform = FTransform(finalRotation, translation, FVector::OneVector);
		data.Transforms.Add(transform);
	}
}

void PoseAIRig::AppendCachedRotations(int32 begin, int32 end, TArray<FQuat>& componentRotations, FLiveLinkAnimationFrameData& data) {
	for (int32 i = begin; i < end; i++) {
		const FName& jointName = jointNames[i];
		int32 parentIdx = parentIndices[i];
		FQuat parentQuat = (parentIdx < 0 ? FQuat::Identity : componentRotations[parentIdx]);
		const FQuat& rotation =  (cachedPose.Num() > i) ? parentQuat * cachedPose[i].GetRotation() : FQuat::Identity;
		const FVector& translation = boneVectors.FindRef(jointName);
		componentRotations.Add(rotation);
		FQuat finalRotation = parentQuat.Inverse() * rotation;
		finalRotation.Normalize();
		FTransform transform = FTransform(finalRotation, translation, FVector::OneVector);
		data.Transforms.Add(transform);
	}
}

void PoseAIRig::CachePose(const TArray<FTransform>& transforms) {
	//cachedPose.Empty();
	cachedPose = TArray<FTransform>(transforms);
}

void PoseAIRig::Configure() {}

void PoseAIRigUE4::Configure()
{
	jointNames.Empty();
	boneVectors.Empty();
	parentIndices.Empty();

	AddBone(TEXT("root"), TEXT("root"), FVector(0.0, 0, 0.0));
	AddBoneToLast(TEXT("pelvis"), FVector(0.0, 0, 0.0));

	AddBone(TEXT("thigh_r"), TEXT("pelvis"), FVector(-1.448829, 0.531424, 9.00581));
	AddBoneToLast(TEXT("calf_r"), FVector(42.572037, 0, 0));
	AddBoneToLast(TEXT("foot_r"), FVector(40.19669, 0, 0));
	AddBoneToLast(TEXT("ball_r"), FVector(10.453837, -16.577854, 0.080156));

	AddBone(TEXT("thigh_l"), TEXT("pelvis"), FVector(-1.448829, 0.531424, -9.00581));
	AddBoneToLast(TEXT("calf_l"), FVector(-42.572037, 0, 0));
	AddBoneToLast(TEXT("foot_l"), FVector(-40.19669, 0, 0));
	AddBoneToLast(TEXT("ball_l"), FVector(-10.453837, 16.577854, 0.080156));

	AddBone(TEXT("spine_01"), TEXT("pelvis"), FVector(10.808878, 0.851415, 0));
	AddBoneToLast(TEXT("spine_02"), FVector(18.875349, -3.801159, 0));
	AddBoneToLast(TEXT("spine_03"), FVector(13.407329, -0.420477, 0));
	AddBoneToLast(TEXT("neck_01"), FVector(16.558783, 0.355318, 0));
	AddBoneToLast(TEXT("head"), FVector(9.283613, -0.364157, 0));

	AddBone(TEXT("clavicle_l"), TEXT("spine_03"), FVector(11.883688, 2.732088, -3.781983));
	AddBoneToLast(TEXT("upperarm_l"), FVector(15.784872, 0, 0));
	AddBoneToLast(TEXT("lowerarm_l"), FVector(30.33993, 0, 0));

	AddBone(TEXT("clavicle_r"), TEXT("spine_03"), FVector(11.883688, 2.732102, 3.782003));
	AddBoneToLast(TEXT("upperarm_r"), FVector(-15.784872, 0, 0));
	AddBoneToLast(TEXT("lowerarm_r"), FVector(-30.33993, 0, 0));
	numBodyJoints = jointNames.Num();
	if (includeHands) {
		AddBone(TEXT("hand_l"), TEXT("lowerarm_l"), FVector(26.975143, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_l"), TEXT("lowerarm_l"), FVector(14.0, 0, 0));
		AddBone(TEXT("index_01_l"), TEXT("hand_l"), FVector(12.068114, -1.763462, -2.109398));
		AddBoneToLast(TEXT("index_02_l"), FVector(4.287498, 0, 0));
		AddBoneToLast(TEXT("index_03_l"), FVector(3.39379, 0, 0));
		AddBone(TEXT("middle_01_l"), TEXT("hand_l"), FVector(12.244281, -1.293644, 0.571162));
		AddBoneToLast(TEXT("middle_02_l"), FVector(4.640374, 0, 0));
		AddBoneToLast(TEXT("middle_03_l"), FVector(3.648844, 0, 0));
		AddBone(TEXT("ring_01_l"), TEXT("hand_l"), FVector(11.497885, -1.753527, 2.846912));
		AddBoneToLast(TEXT("ring_02_l"), FVector(4.430177, 0, 0));
		AddBoneToLast(TEXT("ring_03_l"), FVector(3.476652, 0, 0));
		AddBone(TEXT("pinky_01_l"), TEXT("hand_l"), FVector(10.140665, -2.263151, 4.643148));
		AddBoneToLast(TEXT("pinky_02_l"), FVector(3.570981, 0, 0));
		AddBoneToLast(TEXT("pinky_03_l"), FVector(2.985631, 0, 0));
		AddBone(TEXT("thumb_01_l"), TEXT("hand_l"), FVector(4.762036, -2.374981, -2.53782));
		AddBoneToLast(TEXT("thumb_02_l"), FVector(3.869672, 0, 0));
		AddBoneToLast(TEXT("thumb_03_l"), FVector(4.062171, 0, 0));
		
		AddBone(TEXT("hand_r"), TEXT("lowerarm_r"), FVector(-26.975143, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_r"), TEXT("lowerarm_r"), FVector(-14.0, 0, 0));
		AddBone(TEXT("index_01_r"), TEXT("hand_r"), FVector(-12.068114, 1.763462, 2.109398));
		AddBoneToLast(TEXT("index_02_r"), FVector(-4.287498, 0, 0));
		AddBoneToLast(TEXT("index_03_r"), FVector(-3.39379, 0, 0));
		AddBone(TEXT("middle_01_r"), TEXT("hand_r"), FVector(-12.244281, 1.293644, -0.571162));
		AddBoneToLast(TEXT("middle_02_r"), FVector(-4.640374, 0, 0));
		AddBoneToLast(TEXT("middle_03_r"), FVector(-3.648844, 0, 0));
		AddBone(TEXT("ring_01_r"), TEXT("hand_r"), FVector(-11.497885, 1.753527, -2.846912));
		AddBoneToLast(TEXT("ring_02_r"), FVector(-4.430177, 0, 0));
		AddBoneToLast(TEXT("ring_03_r"), FVector(-3.476652, 0, 0));
		AddBone(TEXT("pinky_01_r"), TEXT("hand_r"), FVector(-10.140665, 2.263151, -4.643148));
		AddBoneToLast(TEXT("pinky_02_r"), FVector(-3.570981, 0, 0));
		AddBoneToLast(TEXT("pinky_03_r"), FVector(-2.985631, 0, 0));
		AddBone(TEXT("thumb_01_r"), TEXT("hand_r"), FVector(-4.762036, 2.374981, 2.53782));
		AddBoneToLast(TEXT("thumb_02_r"), FVector(-3.869672, 0, 0));
		AddBoneToLast(TEXT("thumb_03_r"), FVector(-4.062171, 0, 0));
	}
	numHandJoints = (jointNames.Num() - numBodyJoints) / 2;

	rigHeight = 170.0f;
	rig = MakeStaticData();
}


void PoseAIRigMixamo::Configure()
{
	jointNames.Empty();
	boneVectors.Empty();
	parentIndices.Empty();
	AddBone(TEXT("root"), TEXT("root"), FVector(0.0, 0, 0.0));
	AddBoneToLast(TEXT("hips"), FVector(0.0, 0, 0.0));

	AddBone(TEXT("RightUpLeg"), TEXT("hips"), FVector(-9.4, 5.0, 0));
	AddBoneToLast(TEXT("RightLeg"), FVector(0, -44.5, 0));
	AddBoneToLast(TEXT("RightFoot"), FVector(0.7, -35.0, -2.4));
	AddBoneToLast(TEXT("RightToeBase"), FVector(-0.7, -17.8, -5.8));

	AddBone(TEXT("LeftUpLeg"), TEXT("hips"), FVector(9.4, 5.0, 0));
	AddBoneToLast(TEXT("LeftLeg"), FVector(-0, -44.5, 0));
	AddBoneToLast(TEXT("LeftFoot"), FVector(-0.7, -35.0, -2.4));
	AddBoneToLast(TEXT("LeftToeBase"), FVector(0.7, -17.8, -5.8));

	AddBone(TEXT("Spine"), TEXT("hips"), FVector(0, -9.0, -0.3));
	AddBoneToLast(TEXT("Spine1"), FVector(0, -10.5, 0));
	AddBoneToLast(TEXT("Spine2"), FVector(0, -12.0, 0));
	AddBoneToLast(TEXT("Neck"), FVector(0, -13.5, 0));
	AddBoneToLast(TEXT("Head"), FVector(0, -8.2, 2.1));

	AddBone(TEXT("LeftShoulder"), TEXT("Spine2"), FVector(5.7, -11.8, 0));
	AddBoneToLast(TEXT("LeftArm"), FVector(0, -12.0, 0));
	AddBoneToLast(TEXT("LeftForeArm"), FVector(0, -25.7, 0));

	AddBone(TEXT("RightShoulder"), TEXT("Spine2"), FVector(-5.7, -11.8, 0));
	AddBoneToLast(TEXT("RightArm"), FVector(0, -12.0, 0));
	AddBoneToLast(TEXT("RightForeArm"), FVector(0, -25.7, 0));

	numBodyJoints = jointNames.Num();
	if (includeHands) {
		AddBone(TEXT("LeftHand"), TEXT("LeftForeArm"), FVector(0, -23.0, 0));
		AddBone(TEXT("LeftForeArmTwist"), TEXT("LeftForeArm"), FVector(0, -14.0, 0));
		
		AddBone(TEXT("LeftHandIndex1"), TEXT("LeftHand"), FVector(-3.3, -8.3, 0.1));
		AddBoneToLast(TEXT("LeftHandIndex2"), FVector(0, -3.1, 0));
		AddBoneToLast(TEXT("LeftHandIndex3"), FVector(0, -2.9, 0));
		AddBone(TEXT("LeftHandMiddle1"), TEXT("LeftHand"), FVector(-0.9, -8.5, -0.1));
		AddBoneToLast(TEXT("LeftHandMiddle2"), FVector(0, -3.3, 0));
		AddBoneToLast(TEXT("LeftHandMiddle3"), FVector(0, -3.1, 0));
		AddBone(TEXT("LeftHandRing1"), TEXT("LeftHand"), FVector(1.1, -8.7, 0.2));
		AddBoneToLast(TEXT("LeftHandRing2"), FVector(0, -2.7, 0));
		AddBoneToLast(TEXT("LeftHandRing3"), FVector(0, -2.7, 0));
		AddBone(TEXT("LeftHandPinky1"), TEXT("LeftHand"), FVector(3.1, -8.0, 0.2));
		AddBoneToLast(TEXT("LeftHandPinky2"), FVector(0, -2.5, 0));
		AddBoneToLast(TEXT("LeftHandPinky3"), FVector(0, -2.0, 0));
		AddBone(TEXT("LeftHandThumb1"), TEXT("LeftHand"), FVector(-2.9, -2.6, 1.2));
		AddBoneToLast(TEXT("LeftHandThumb2"), FVector(-0.7, -3.2, 0));
		AddBoneToLast(TEXT("LeftHandThumb3"), FVector(0.2, -3.0, 0));

		AddBone(TEXT("RightHand"), TEXT("RightForeArm"), FVector(0, -23.0, 0));
		AddBone(TEXT("RightForeArmTwist"), TEXT("RightForeArm"), FVector(0, -14.0, 0));
		AddBone(TEXT("RightHandIndex1"), TEXT("RightHand"), FVector(3.3, -8.3, 0.1));
		AddBoneToLast(TEXT("RightHandIndex2"), FVector(0, -3.1, 0));
		AddBoneToLast(TEXT("RightHandIndex3"), FVector(0, -2.9, 0));
		AddBone(TEXT("RightHandMiddle1"), TEXT("RightHand"), FVector(0.9, -8.5, -0.1));
		AddBoneToLast(TEXT("RightHandMiddle2"), FVector(0, -3.3, 0));
		AddBoneToLast(TEXT("RightHandMiddle3"), FVector(0, -3.1, 0));
		AddBone(TEXT("RightHandRing1"), TEXT("RightHand"), FVector(-1.1, -8.7, 0.2));
		AddBoneToLast(TEXT("RightHandRing2"), FVector(0, -2.7, 0));
		AddBoneToLast(TEXT("RightHandRing3"), FVector(0, -2.7, 0));
		AddBone(TEXT("RightHandPinky1"), TEXT("RightHand"), FVector(-3.1, -8.0, 0.2));
		AddBoneToLast(TEXT("RightHandPinky2"), FVector(0, -2.5, 0));
		AddBoneToLast(TEXT("RightHandPinky3"), FVector(0, -2.0, 0));
		AddBone(TEXT("RightHandThumb1"), TEXT("RightHand"), FVector(2.9, -2.6, 1.2));
		AddBoneToLast(TEXT("RightHandThumb2"), FVector(0.7, -3.2, 0));
		AddBoneToLast(TEXT("RightHandThumb3"), FVector(-0.2, -3.0, 0));
	}
	numHandJoints = (jointNames.Num() - numBodyJoints) / 2;
	rigHeight = 161.0f;
	rig = MakeStaticData();
}


void PoseAIRigMetaHuman::Configure()
{
	jointNames.Empty();
	boneVectors.Empty();
	parentIndices.Empty();

	AddBone(TEXT("root"), TEXT("root"), FVector(0.0, 0, 0.0));
	AddBoneToLast(TEXT("pelvis"), FVector(0.0, 0, 0.0));

	AddBone(TEXT("thigh_r"), TEXT("pelvis"), FVector(-2.3, 0.4, 9.27));
	AddBoneToLast(TEXT("calf_r"), FVector(41.2, 0, 0));
	AddBoneToLast(TEXT("foot_r"), FVector(40.0, 0, 0));
	AddBoneToLast(TEXT("ball_r"), FVector(7.1, -14.4, -0.4));

	AddBone(TEXT("thigh_l"), TEXT("pelvis"), FVector(-2.3, 0.4, -9.27));
	AddBoneToLast(TEXT("calf_l"), FVector(-41.2, 0, 0));
	AddBoneToLast(TEXT("foot_l"), FVector(-40.0, 0, 0));
	AddBoneToLast(TEXT("ball_l"), FVector(-7.1, 14.4, 0.4));

	AddBone(TEXT("spine_01"), TEXT("pelvis"), FVector(3.4, 0.0, 0));
	AddBoneToLast(TEXT("spine_02"), FVector(6.3, 0.0, 0));
	AddBoneToLast(TEXT("spine_03"), FVector(6.9, 0.0, 0));
	AddBoneToLast(TEXT("spine_04"), FVector(8.1, 0.0, 0));
	AddBoneToLast(TEXT("spine_05"), FVector(18.3, 0.0, 0));
	AddBoneToLast(TEXT("neck_01"), FVector(11.6, 1.0, 0));
	AddBoneToLast(TEXT("neck_02"), FVector(5.0, 0, 0));
	AddBoneToLast(TEXT("head"), FVector(5.0, 0, 0));

	AddBone(TEXT("clavicle_l"), TEXT("spine_05"), FVector(5.5, -0.7, -1.2));
	AddBoneToLast(TEXT("upperarm_l"), FVector(17.0, 0, 0));
	AddBoneToLast(TEXT("lowerarm_l"), FVector(27.0, 0, 0));

	AddBone(TEXT("clavicle_r"), TEXT("spine_05"), FVector(5.5, -0.7, 1.2));
	AddBoneToLast(TEXT("upperarm_r"), FVector(-17.0, 0, 0));
	AddBoneToLast(TEXT("lowerarm_r"), FVector(-27.0, 0, 0));

	numBodyJoints = jointNames.Num();
	if (includeHands) {
		AddBone(TEXT("hand_l"), TEXT("lowerarm_l"), FVector(25.2, 0, 0));		
		AddBone(TEXT("lowerarm_twist_01_l"), TEXT("lowerarm_l"), FVector(14.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_02_l"), TEXT("lowerarm_l"), FVector(7.0, 0, 0));
		
		AddBone(TEXT("index_metacarpal_l"), TEXT("hand_l"), FVector(3.5, 0.4, -2.1));
		AddBoneToLast(TEXT("index_01_l"), FVector(5.9, 0.1, 0.3));
		AddBoneToLast(TEXT("index_02_l"), FVector(3.6, 0, 0));
		AddBoneToLast(TEXT("index_03_l"), FVector(2.4, 0, 0));
		AddBone(TEXT("middle_metacarpal_l"), TEXT("hand_l"), FVector(3.3, 0.3, -0.1));
		AddBoneToLast(TEXT("middle_01_l"), FVector(6.1, 0, 0.2));
		AddBoneToLast(TEXT("middle_02_l"), FVector(4.3, 0, 0));
		AddBoneToLast(TEXT("middle_03_l"), FVector(2.6, 0, 0));
		AddBone(TEXT("ring_metacarpal_l"), TEXT("hand_l"), FVector(3.2, -0.2, 1.2));
		AddBoneToLast(TEXT("ring_01_l"), FVector(6.0, 0.2, 0.4));
		AddBoneToLast(TEXT("ring_02_l"), FVector(3.6, 0, 0));
		AddBoneToLast(TEXT("ring_03_l"), FVector(2.5, 0, 0));
		AddBone(TEXT("pinky_metacarpal_l"), TEXT("hand_l"), FVector(3.1, -0.7, 2.4));
		AddBoneToLast(TEXT("pinky_01_l"), FVector(5.1, 0.1, 0.1));
		AddBoneToLast(TEXT("pinky_02_l"), FVector(3.3, 0, 0));
		AddBoneToLast(TEXT("pinky_03_l"), FVector(1.8, 0, 0));
		AddBone(TEXT("thumb_01_l"), TEXT("hand_l"), FVector(2.0, -1.0, -2.6));
		AddBoneToLast(TEXT("thumb_02_l"), FVector(4.4, 0, 0));
		AddBoneToLast(TEXT("thumb_03_l"), FVector(2.7, 0, 0));

		AddBone(TEXT("hand_r"), TEXT("lowerarm_r"), FVector(-25.2, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_r"), TEXT("lowerarm_r"), FVector(-14.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_02_r"), TEXT("lowerarm_r"), FVector(-7.0, 0, 0));
		AddBone(TEXT("index_metacarpal_r"), TEXT("hand_r"), FVector(-3.5, -0.4, 2.1));
		AddBoneToLast(TEXT("index_01_r"), FVector(-5.9, 0.1, 0.3));
		AddBoneToLast(TEXT("index_02_r"), FVector(-3.6, 0, 0));
		AddBoneToLast(TEXT("index_03_r"), FVector(-2.4, 0, 0));
		AddBone(TEXT("middle_metacarpal_r"), TEXT("hand_r"), FVector(-3.3, -0.3, 0.1));
		AddBoneToLast(TEXT("middle_01_r"), FVector(-6.1, 0, 0.2));
		AddBoneToLast(TEXT("middle_02_r"), FVector(-4.3, 0, 0));
		AddBoneToLast(TEXT("middle_03_r"), FVector(-2.6, 0, 0));
		AddBone(TEXT("ring_metacarpal_r"), TEXT("hand_r"), FVector(-3.2, 0.2, -1.2));
		AddBoneToLast(TEXT("ring_01_r"), FVector(-6.0, 0.2, 0.4));
		AddBoneToLast(TEXT("ring_02_r"), FVector(-3.6, 0, 0));
		AddBoneToLast(TEXT("ring_03_r"), FVector(-2.5, 0, 0));
		AddBone(TEXT("pinky_metacarpal_r"), TEXT("hand_r"), FVector(-3.1, 0.7, -2.4));
		AddBoneToLast(TEXT("pinky_01_r"), FVector(-5.1, 0.1, 0.1));
		AddBoneToLast(TEXT("pinky_02_r"), FVector(-3.3, 0, 0));
		AddBoneToLast(TEXT("pinky_03_r"), FVector(-1.8, 0, 0));
		AddBone(TEXT("thumb_01_r"), TEXT("hand_r"), FVector(-2.0, 1.0, 2.6));
		AddBoneToLast(TEXT("thumb_02_r"), FVector(-4.4, 0, 0));
		AddBoneToLast(TEXT("thumb_03_r"), FVector(-2.7, 0, 0));
	}
	numHandJoints = (jointNames.Num() - numBodyJoints) / 2;
	rigHeight = 168.0f;
	rig = MakeStaticData();
}

void PoseAIRigDazUE::Configure()
{
	
	jointNames.Empty();
	boneVectors.Empty();
	parentIndices.Empty();

	AddBone(TEXT("root"), TEXT("root"), FVector(0.0, 0, 0.0));
	AddBoneToLast(TEXT("hip"), FVector(0.0, -105.0, 0.0));
	AddBoneToLast(TEXT("pelvis"), FVector(0.0, -1.8, 0.0));

	AddBone(TEXT("rThighBend"), TEXT("pelvis"), FVector(-7.9, 10.6, -1.5));
	AddBoneToLast(TEXT("rThighTwist"), FVector(0.0, 21, 0));
	AddBoneToLast(TEXT("rShin"), FVector(0.0, 25.3, -1.2));
	AddBoneToLast(TEXT("rFoot"), FVector(0.0, 42.8, 1));
	AddBoneToLast(TEXT("rToe"), FVector(0.0, 0, 14.0));

	AddBone(TEXT("lThighBend"), TEXT("pelvis"), FVector(7.9, 10.6, -1.5));
	AddBoneToLast(TEXT("lThighTwist"), FVector(0.0, 21, 0));
	AddBoneToLast(TEXT("lShin"), FVector(0.0, 25.3, -1.2));
	AddBoneToLast(TEXT("lFoot"), FVector(0.0, 42.8, 1));
	AddBoneToLast(TEXT("lToe"), FVector(0.0, 0.0, 14.0));

	AddBone(TEXT("abdomenLower"), TEXT("hip"), FVector(0.0, -1.7, -1.5));
	AddBoneToLast(TEXT("abdomenUpper"), FVector(0.0, -8.2, 1.2));
	AddBoneToLast(TEXT("chestLower"), FVector(0.0, -7.9, -0.4));
	AddBoneToLast(TEXT("chestUpper"), FVector(0.0, -13.1, -3.6));
	AddBoneToLast(TEXT("neckLower"), FVector(0.0, -18.3, -1.5));
	AddBoneToLast(TEXT("neckUpper"), FVector(0.0, -3.5, 1.5));
	AddBoneToLast(TEXT("head"), FVector(0.0, -4.9, -0.5));

	AddBone(TEXT("lCollar"), TEXT("chestUpper"), FVector(3.5, -10.9, -1.6));
	AddBoneToLast(TEXT("lShldrBend"), FVector(11.9, 1.7, 0));
	AddBoneToLast(TEXT("lShldrTwist"), FVector(11.6, 0, 0));
	AddBoneToLast(TEXT("lForearmBend"), FVector(14.4, -0.2, -0.5));

	AddBone(TEXT("rCollar"), TEXT("chestUpper"), FVector(-3.5, -10.9, -1.6));
	AddBoneToLast(TEXT("rShldrBend"), FVector(-11.9, 1.7, 0));
	AddBoneToLast(TEXT("rShldrTwist"), FVector(-11.6, 0, 0));
	AddBoneToLast(TEXT("rForearmBend"), FVector(-14.4, -0.2, -0.5));

	numBodyJoints = jointNames.Num();
	if (includeHands) {
		AddBone(TEXT("lForearmTwist"), TEXT("lForearmBend"), FVector(12.1, 0, 0));
		AddBone(TEXT("lHand"), TEXT("lForearmTwist"), FVector(14.2, 0, -0.3));

		AddBone(TEXT("lCarpal1"), TEXT("lHand"), FVector(0.4, -0.4, 1.1));
		AddBoneToLast(TEXT("lIndex1"), FVector(7.6, -0.2, 0.1));
		AddBoneToLast(TEXT("lIndex2"), FVector(3.9, 0, 0));
		AddBoneToLast(TEXT("lIndex3"), FVector(2.1, 0, 0));
		AddBone(TEXT("lCarpal2"), TEXT("lHand"), FVector(0.7, -0.4, 0.2));
		AddBoneToLast(TEXT("lMid1"), FVector(7.5, -0.3, 0));
		AddBoneToLast(TEXT("lMid2"), FVector(4.3, 0, 0));
		AddBoneToLast(TEXT("lMid3"), FVector(2.5, 0, 0));
		AddBone(TEXT("lCarpal3"), TEXT("lHand"), FVector(0.8, -0.4, -0.8));
		AddBoneToLast(TEXT("lRing1"), FVector(6.9, -0.2, 0.0));
		AddBoneToLast(TEXT("lRing2"), FVector(4.0, 0, 0));
		AddBoneToLast(TEXT("lRing3"), FVector(2.2, 0, 0));
		AddBone(TEXT("lCarpal4"), TEXT("lHand"), FVector(0.7, -0.4, 1.7));
		AddBoneToLast(TEXT("lPinky1"), FVector(6.5, 0.2, 0));
		AddBoneToLast(TEXT("lPinky2"), FVector(2.8, 0, 0));
		AddBoneToLast(TEXT("lPinky3"), FVector(1.7, 0, 0));
		AddBone(TEXT("lThumb1"), TEXT("lHand"), FVector(1.4, 0.7, 1.6));
		AddBoneToLast(TEXT("lThumb2"), FVector(4.1, 0, 0));
		AddBoneToLast(TEXT("lThumb3"), FVector(3.0, 0, 0));

		AddBone(TEXT("rForearmTwist"), TEXT("rForearmBend"), FVector(-12.1, 0, 0));
		AddBone(TEXT("rHand"), TEXT("rForearmTwist"), FVector(-14.2, 0, -0.3));

		AddBone(TEXT("rCarpal1"), TEXT("rHand"), FVector(-0.4, -0.4, 1.1));
		AddBoneToLast(TEXT("rIndex1"), FVector(-7.6, -0.2, 0.1));
		AddBoneToLast(TEXT("rIndex2"), FVector(-3.9, 0, 0));
		AddBoneToLast(TEXT("rIndex3"), FVector(-2.1, 0, 0));
		AddBone(TEXT("rCarpal2"), TEXT("rHand"), FVector(0.7, -0.4, 0.2));
		AddBoneToLast(TEXT("rMid1"), FVector(-7.5, -0.3, 0));
		AddBoneToLast(TEXT("rMid2"), FVector(-4.3, 0, 0));
		AddBoneToLast(TEXT("rMid3"), FVector(-2.5, 0, 0));
		AddBone(TEXT("rCarpal3"), TEXT("rHand"), FVector(-0.8, -0.4, 0.8));
		AddBoneToLast(TEXT("rRing1"), FVector(-6.9, -0.2, 0));
		AddBoneToLast(TEXT("rRing2"), FVector(-4.0, 0, 0));
		AddBoneToLast(TEXT("rRing3"), FVector(-2.2, 0, 0));
		AddBone(TEXT("rCarpal4"), TEXT("rHand"), FVector(-0.7, -0.4, 1.7));
		AddBoneToLast(TEXT("rPinky1"), FVector(-6.5, 0.2, 0));
		AddBoneToLast(TEXT("rPinky2"), FVector(-2.8, 0, 0));
		AddBoneToLast(TEXT("rPinky3"), FVector(-1.7, 0, 0));
		AddBone(TEXT("rThumb1"), TEXT("rHand"), FVector(-1.4, 0.7, 1.6));
		AddBoneToLast(TEXT("rThumb2"), FVector(-4.1, 0, 0));
		AddBoneToLast(TEXT("rThumb3"), FVector(-3.0, 0, 0));
	}
	numHandJoints = (jointNames.Num() - numBodyJoints) / 2;
	rigHeight = 168.0f;
	rig = MakeStaticData();
}



