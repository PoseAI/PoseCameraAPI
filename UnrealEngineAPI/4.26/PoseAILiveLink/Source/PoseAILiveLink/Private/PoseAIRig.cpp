// Copyright Pose AI Ltd 2021

#include "PoseAIRig.h"


const FString PoseAIRig::fieldBody = FString(TEXT("Body"));
const FString PoseAIRig::fieldRigType = FString(TEXT("Rig"));
const FString PoseAIRig::fieldHandLeft = FString(TEXT("LeftHand"));
const FString PoseAIRig::fieldHandRight = FString(TEXT("RightHand"));
const FString PoseAIRig::fieldRotations = FString(TEXT("Rotations"));
const FString PoseAIRig::fieldScalars = FString(TEXT("Scalars"));
const FString PoseAIRig::fieldVectors = FString(TEXT("Vectors"));
const FString PoseAIRig::fieldHipScreen = FString(TEXT("HipScreen"));
const FString PoseAIRig::fieldBodyHeight = FString(TEXT("BodyHeight"));
const FString PoseAIRig::fieldStableFoot = FString(TEXT("StableFoot"));
const FString PoseAIRig::fieldShrugLeft = FString(TEXT("ShrugL"));
const FString PoseAIRig::fieldShrugRight = FString(TEXT("ShrugR"));


PoseAIRig::PoseAIRig(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop) :
	rigType(rigType), useRootMotion(useRootMotion), includeHands(includeHands), isMirrored(isMirrored), isDesktop(isDesktop) {
	Configure();
	
}
TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> PoseAIRig::PoseAIRigFactory(FName rigType, bool useRootMotion, bool includeHands, bool isMirrored, bool isDesktop) {
	TSharedPtr<PoseAIRig, ESPMode::ThreadSafe> rigPtr;
	if (rigType == FName("Mixamo")) {
		rigPtr = MakeShared<PoseAIRigMixamo, ESPMode::ThreadSafe>(rigType, useRootMotion, includeHands, isMirrored, isDesktop);
	}
	else if (rigType == FName("MetaHuman")) {
		rigPtr = MakeShared<PoseAIRigMetaHuman, ESPMode::ThreadSafe>(rigType, useRootMotion, includeHands, isMirrored, isDesktop);
	}
	else {
		rigPtr = MakeShared<PoseAIRigUE4, ESPMode::ThreadSafe>(rigType, useRootMotion, includeHands, isMirrored, isDesktop);;
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

bool PoseAIRig::ProcessFrame(const TSharedPtr<FJsonObject> jsonObject, FLiveLinkFrameDataStruct& frameData)
{
	const TSharedPtr < FJsonObject >* objBody;
	const TSharedPtr < FJsonObject >* objHandLeft;
	const TSharedPtr < FJsonObject >* objHandRight;
	const TSharedPtr < FJsonObject >* rotBody;
	const TSharedPtr < FJsonObject >* scaBody;
	const TSharedPtr < FJsonObject >* vecBody;
	const TSharedPtr < FJsonObject >* rotHandLeft;
	const TSharedPtr < FJsonObject >* rotHandRight;

	if (jsonObject->TryGetObjectField(fieldBody, objBody)) {
		if (!(*objBody)->TryGetObjectField(fieldRotations, rotBody))
			rotBody = nullptr;
		if (!(*objBody)->TryGetObjectField(fieldScalars, scaBody))
			scaBody = nullptr;
		if (!(*objBody)->TryGetObjectField(fieldVectors, vecBody))
			vecBody = nullptr;
	} else {
		rotBody = nullptr;
		scaBody = nullptr;
		vecBody = nullptr;
	}

	FLiveLinkAnimationFrameData& data = *frameData.Cast<FLiveLinkAnimationFrameData>();
	data.WorldTime = FPlatformTime::Seconds();

	if (rotBody == nullptr && cachedPose.Num() < 1) {
		return false;

	} else if (rotBody == nullptr) {
		data.Transforms.Append(cachedPose);
		return true;

	} else {

		
		FString rigStringOut;
		if(jsonObject->TryGetStringField(fieldRigType, rigStringOut)){
			if (FName(rigStringOut) != rigType) 
				UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: Rig is streaming in %s format, expected %s format."), *rigStringOut, *rigType.ToString());
		} else {
			UE_LOG(LogTemp, Warning, TEXT("PoseAI LiveLink: Can't get rig field."));
		}


		if (!(jsonObject->TryGetObjectField(fieldHandLeft, objHandLeft) &&
			(*objHandLeft)->TryGetObjectField(fieldRotations, rotHandLeft)))
			rotHandLeft = nullptr;

		if (!(jsonObject->TryGetObjectField(fieldHandRight, objHandRight) &&
			(*objHandRight)->TryGetObjectField(fieldRotations, rotHandRight)))
			rotHandRight = nullptr;

		const TArray < TSharedPtr < FJsonValue > >* hipScreen;
		double bodyHeight;
		FVector baseTranslation = FVector::ZeroVector;
		if (isDesktop) {
			baseTranslation = FVector(0.0f, 0.0f, rigHeight * 0.5f);
		}
		else if (vecBody != nullptr && (*vecBody)->TryGetArrayField(fieldHipScreen, hipScreen) &&
			scaBody != nullptr && (*scaBody)->TryGetNumberField(fieldBodyHeight, bodyHeight) &&
			(bodyHeight > 0.1f)) {
				baseTranslation = FVector(
					-(*hipScreen)[0]->AsNumber() * rigHeight / bodyHeight , //* (isMirrored ? -1.0f : 1.0f)
					0.0,
					-(*hipScreen)[1]->AsNumber() * rigHeight / bodyHeight
				);
		}

		
		TArray<FQuat> componentRotations;
		
		for (int32 i = 0; i < jointNames.Num(); i++) {
			const FName& jointName = jointNames[i];
			int32 parentIdx = parentIndices[i];
			FQuat parentQuat = (parentIdx < 0 ? FQuat::Identity : componentRotations[parentIdx]);
			const FVector& translation = boneVectors.FindRef(jointName);
			FQuat rotation;
			const TArray < TSharedPtr < FJsonValue > >* outArray;
			FString jointString = jointName.ToString();
			if ((rotBody != nullptr && (*rotBody)->TryGetArrayField(jointString, outArray)) ||
				(rotHandLeft != nullptr && (*rotHandLeft)->TryGetArrayField(jointString, outArray)) ||
				(rotHandRight != nullptr && (*rotHandRight)->TryGetArrayField(jointString, outArray))) {
				rotation = FQuat((*outArray)[0]->AsNumber(), (*outArray)[1]->AsNumber(), (*outArray)[2]->AsNumber(), (*outArray)[3]->AsNumber());
			} else if (cachedPose.Num() > i) {
				rotation = parentQuat * cachedPose[i].GetRotation();
			} else {
				rotation = FQuat::Identity;
			}
			componentRotations.Add(rotation);
			FQuat finalRotation = parentQuat.Inverse() * rotation;
			finalRotation.Normalize();
			FTransform transform = FTransform(finalRotation, translation, FVector::OneVector);
			//Live link expects local rotations.  Consider having this sent directly by PoseAI app to save calculations
			data.Transforms.Add(transform);
		}
		
		
		if (scaBody != nullptr) {
			double leftShrug = 0.0;
			double rightShrug = 0.0;
			(*scaBody)->TryGetNumberField(fieldShrugLeft, leftShrug);
			data.Transforms[leftShoulderJoint].AddToTranslation(leftShrug * shrugVector);
			(*scaBody)->TryGetNumberField(fieldShrugRight, rightShrug);
			data.Transforms[rightShoulderJoint].AddToTranslation(rightShrug * shrugVector);
		}


		// ***************************** calculates and assigns character motion **************************************************
		
		// to ensure grounding in the capsule, calculates lowest Z in component space.  doesn't check fingers to save calculations on fingers: if this is important consider using parents.Num()
		TArray<FTransform> componentTransform;
		componentTransform.Emplace(data.Transforms[0]);
		componentTransform.Emplace(data.Transforms[1]);
		float minZ = 0.0f;
		for (int32 j = 2; j < numBodyJoints ; j++) {
			componentTransform.Emplace(data.Transforms[j] * componentTransform[parentIndices[j]]);
			minZ = FGenericPlatformMath::Min(minZ, componentTransform[j].GetTranslation().Z);
		}
		
		//update adjustment only when stable.  this allows character to jump
		double stableFoot = 0.0f;
		if (scaBody != nullptr &&
			(*scaBody)->TryGetNumberField(fieldStableFoot, stableFoot) &&
			(stableFoot > 0.5f)) {
			verticalAdjustment = -minZ - baseTranslation.Z;
			verticalAdjustment = FGenericPlatformMath::Min(verticalAdjustment, rigHeight); // a safety clamp.
		}

		// assigns motion either to root or to hips
		if (useRootMotion) {
			//hip to low point Z distance assigned to hips, rest of movement assigned to root
			data.Transforms[1].SetTranslation(FVector(0.0f, 0.0f, -minZ));
			baseTranslation.Z = FGenericPlatformMath::Min(
				FGenericPlatformMath::Max(baseTranslation.Z + verticalAdjustment + minZ, 0.0f),
				rigHeight*0.5f // a safety min
			); 
			data.Transforms[0].SetTranslation(baseTranslation);
		} else {
			baseTranslation.Z = FGenericPlatformMath::Min(
				FGenericPlatformMath::Max(baseTranslation.Z + verticalAdjustment, -minZ),
				rigHeight * 0.5f - minZ // a safety min
			);
			data.Transforms[1].SetTranslation(baseTranslation);
		}
		CachePose(data.Transforms);
		return true;
	}
}

void PoseAIRig::CachePose(const TArray<FTransform>& transforms) {
	cachedPose.Empty();
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
		numBodyJoints += 2;
		AddBone(TEXT("hand_l"), TEXT("lowerarm_l"), FVector(26.975143, 0, 0));
		AddBone(TEXT("hand_r"), TEXT("lowerarm_r"), FVector(-26.975143, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_l"), TEXT("lowerarm_l"), FVector(14.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_r"), TEXT("lowerarm_r"), FVector(-14.0, 0, 0));

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

	rigHeight = 170.0f;
	shrugVector = FVector(rigHeight * 0.03f, 0.0f, 0.0f);

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
		numBodyJoints += 2;
		AddBone(TEXT("LeftHand"), TEXT("LeftForeArm"), FVector(0, -23.0, 0));
		AddBone(TEXT("RightHand"), TEXT("RightForeArm"), FVector(0, -23.0, 0));
		AddBone(TEXT("LeftForeArmTwist"), TEXT("LeftForeArm"), FVector(0, -14.0, 0));
		AddBone(TEXT("RightForeArmTwist"), TEXT("RightForeArm"), FVector(0, -14.0, 0));

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

	rigHeight = 161.0f;
	shrugVector = FVector(0.0f, -rigHeight * 0.03f, 0.0f);

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
		numBodyJoints += 2;
		AddBone(TEXT("hand_l"), TEXT("lowerarm_l"), FVector(25.2, 0, 0));
		AddBone(TEXT("hand_r"), TEXT("lowerarm_r"), FVector(-25.2, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_l"), TEXT("lowerarm_l"), FVector(14.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_01_r"), TEXT("lowerarm_r"), FVector(-14.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_02_l"), TEXT("lowerarm_l"), FVector(7.0, 0, 0));
		AddBone(TEXT("lowerarm_twist_02_r"), TEXT("lowerarm_r"), FVector(-7.0, 0, 0));

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

	rigHeight = 168.0f;
	shrugVector = FVector(rigHeight * 0.02f, 0.0f, 0.0f);
	//override these as Metahuman has extra three joints in torso.
	leftShoulderJoint = 18;
	rightShoulderJoint = 21;

	rig = MakeStaticData();
}