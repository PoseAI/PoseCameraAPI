// Copyright Pose AI Ltd 2021

#include "PoseAIStructs.h"

float UintB64ToUint(char a, char b) {
    static const float reverse_map[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6, 7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,	0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

    return reverse_map[static_cast<uint8>(a)] * 64 + reverse_map[static_cast<uint8>(b)];
}
uint32 UintB64ToUint(char a, char b, char c) {
    static const float reverse_map[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6, 7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,	0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
    return reverse_map[static_cast<uint8>(a)] * 4096 + reverse_map[static_cast<uint8>(b)] * 64 + reverse_map[static_cast<uint8>(c)];
}

float FixedB64pairToFloat(char a, char b) {
    static const float firstByte[256] = { 0.0f, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.9384465070835368, 0.9697117733268197, 0.9384465070835368, 0.9384465070835368, 0.9697117733268197, 0.6257938446507083, 0.6570591108939912, 0.688324377137274, 0.7195896433805569, 0.7508549096238397, 0.7821201758671226, 0.8133854421104054, 0.8446507083536883, 0.8759159745969711, 0.907181240840254, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, -0.9687347337567171, -0.9374694675134343, -0.9062042012701514, -0.8749389350268686, -0.8436736687835857, -0.8124084025403029, -0.78114313629702, -0.7498778700537372, -0.7186126038104543, -0.6873473375671715, -0.6560820713238886, -0.6248168050806058, -0.5935515388373229, -0.5622862725940401, -0.5310210063507572, -0.49975574010747437, -0.4684904738641915, -0.43722520762090866, -0.4059599413776258, -0.37469467513434296, -0.3434294088910601, -0.31216414264777725, -0.2808988764044944, -0.24963361016121155, -0.2183683439179287, 0.0, 0.0, 0.0, 0.0, 0.9697117733268197, 0.0, -0.18710307767464585, -0.155837811431363, -0.12457254518808014, -0.09330727894479729, -0.06204201270151444, -0.030776746458231585, 0.0004885197850512668, 0.03175378602833412, 0.06301905227161697, 0.09428431851489982, 0.12554958475818268, 0.15681485100146553, 0.18808011724474838, 0.21934538348803123, 0.2506106497313141, 0.28187591597459694, 0.3131411822178798, 0.34440644846116264, 0.3756717147044455, 0.40693698094772834, 0.4382022471910112, 0.46946751343429405, 0.5007327796775769, 0.5319980459208598, 0.5632633121641426, 0.5945285784074255 };
    static const float secondByte[256] = { 0.0f, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.030288226673180263, 0.030776746458231558, 0.030288226673180263, 0.030288226673180263, 0.030776746458231558, 0.025403028822667317, 0.025891548607718612, 0.026380068392769906, 0.0268685881778212, 0.027357107962872496, 0.02784562774792379, 0.028334147532975085, 0.02882266731802638, 0.029311187103077674, 0.02979970688812897, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0004885197850512946, 0.0009770395701025891, 0.0014655593551538837, 0.0019540791402051783, 0.002442598925256473, 0.0029311187103077674, 0.003419638495359062, 0.0039081582804103565, 0.004396678065461651, 0.004885197850512946, 0.00537371763556424, 0.005862237420615535, 0.006350757205666829, 0.006839276990718124, 0.0073277967757694185, 0.007816316560820713, 0.008304836345872008, 0.008793356130923302, 0.009281875915974597, 0.009770395701025891, 0.010258915486077186, 0.01074743527112848, 0.011235955056179775, 0.01172447484123107, 0.012212994626282364, 0.0, 0.0, 0.0, 0.0, 0.030776746458231558, 0.0, 0.012701514411333659, 0.013190034196384953, 0.013678553981436248, 0.014167073766487542, 0.014655593551538837, 0.015144113336590131, 0.015632633121641426, 0.01612115290669272, 0.016609672691744015, 0.01709819247679531, 0.017586712261846604, 0.0180752320468979, 0.018563751831949193, 0.019052271617000488, 0.019540791402051783, 0.020029311187103077, 0.02051783097215437, 0.021006350757205666, 0.02149487054225696, 0.021983390327308255, 0.02247191011235955, 0.022960429897410845, 0.02344894968246214, 0.023937469467513434, 0.024425989252564728, 0.024914509037616023 };
    return firstByte[static_cast<uint8>(a)] + secondByte[static_cast<uint8>(b)];
}

void FStringFixed12ToFloat(const FString& data, TArray<float>& flatArray) {
    check(data.Len() % 2 == 0);
    flatArray.Reserve(flatArray.Num() + data.Len() / 2);
    for (int i = 0; i < data.Len(); i += 2)
        flatArray.Add(FixedB64pairToFloat(data[i], data[i + 1]));
}

void FlatArrayToQuats(const TArray<float>& flatArray, TArray<FQuat>& quatArray) {
    check(flatArray.Num() % 4 == 0);
    quatArray.Reserve(quatArray.Num() + flatArray.Num() / 4);
    for (int i = 0; i < flatArray.Num(); i += 4)
        quatArray.Add(FQuat(flatArray[i], flatArray[i + 1], flatArray[i + 2], flatArray[i + 3]));
}


void ProcessFieldAsVector2D(const TSharedPtr < FJsonObject > jsonObj, const FString& fieldName, FVector2D& fieldVector2D){
    const TArray < TSharedPtr < FJsonValue > >* value;
    if (jsonObj->TryGetArrayField(fieldName, value)){
        fieldVector2D.X = (*value)[0]->AsNumber();
        fieldVector2D.Y = (*value)[1]->AsNumber();
    }
}

void ProcessArrayAsVector2D(const TArray < float > value, FVector2D& fieldVector2D) {
    if (value.Num() == 2) {
        fieldVector2D.X = value[0];
        fieldVector2D.Y = value[1];
    }
}

void SetAndCheckForChange(bool newValue, bool& field, bool& changeFlag) {
    if (newValue != field)
        changeFlag = true;
    field = newValue;
}
void SetAndCheckForChange(float newValue, bool& field, bool& changeFlag) {
    SetAndCheckForChange(newValue > 0.5f, field, changeFlag);
}


FString FPoseAIHandshake::ToString() const {
    return FString::Printf(
        TEXT("{\"HANDSHAKE\":{"
            "\"name\":\"Unreal LiveLink\","
            "\"rig\":\"%s\", "
            "\"mode\":\"%s\", "
            "\"context\":\"%s\", "
            "\"whoami\":\"%s\", "
            "\"signature\":\"%s\", "
            "\"mirror\":\"%s\", "
            "\"syncFPS\": %d, "
            "\"cameraFPS\": %d, "
            "\"packetFormat\": %d"
            "}}"),
        *rig,
        *mode,
        *context,
        *whoami,
        *signature,
        *(YesNoString(isMirrored)),
        syncFPS,
        cameraFPS,
        packetFormat
    );
}

bool FPoseAIHandshake::operator==(const FPoseAIHandshake& Other) const
{
    return rig == Other.rig && mode == Other.mode && syncFPS == Other.syncFPS && cameraFPS == Other.cameraFPS && isMirrored == Other.isMirrored && packetFormat == Other.packetFormat;
}

FString FPoseAIModelConfig::ToString() const {
    return FString::Printf(
        TEXT("{\"CONFIG\":{"
            "\"mirror\":\"%s\", "
            "\"stepSensitivity\":%f, "
            "\"armSensitivity\":%f, "
            "\"crouchSensitivity\": %f, "
            "\"jumpSensitivity\":%f"
            "}}"),
        *(YesNoString(isMirrored)),
        stepSensitivity,
        armSensitivity,
        crouchSensitivity,
        jumpSensitivity
    );
}

bool FPoseAIEventPairBase::CheckTriggerAndUpdate() {
    bool hasChanged = Count != InternalCount;
    InternalCount = Count;
    return hasChanged;
}

void FPoseAIEventPair::ProcessCompact(const FString& compactString) {
   Count = UintB64ToUint(compactString[0], compactString[1], compactString[2]);
   Magnitude = FixedB64pairToFloat(compactString[3], compactString[4]);
}

void FPoseAIGesturePair::ProcessCompact(const FString& compactString) {
    Count = UintB64ToUint(compactString[0], compactString[1], compactString[2]);
    Current = UintB64ToUint(compactString[3], compactString[4]);
}

void FPoseAIEventStruct::ProcessCompactBody(const FString& compactString) {
    TArray<FPoseAIEventPairBase*> compactOrder = { &Footstep, &SidestepL, &SidestepR, &Jump, &FeetSplit, &ArmPump, &ArmFlex, &ArmGestureL, &ArmGestureR};
    if (compactString.Len() % 5 != 0) {
        UE_LOG(LogTemp, Warning, TEXT("PoseAILiveLink: Invalid event string: %s."), *compactString);
        return;
    }
    for (int i = 0; i < compactOrder.Num(); ++i) {
        if (compactString.Len() < 5 * i)
            break;
        compactOrder[i]->ProcessCompact(compactString.Mid(i * 5, 5));
    }
}

void  FPoseAIVisibilityFlags::ProcessCompact(const FString& visString) {
    hasChanged = false;
    SetAndCheckForChange(visString[0] != '0', isTorso, hasChanged);
    SetAndCheckForChange(visString[1] != '0', isLeftLeg, hasChanged);
    SetAndCheckForChange(visString[2] != '0', isRightLeg, hasChanged);
    SetAndCheckForChange(visString[3] != '0', isLeftArm, hasChanged);
    SetAndCheckForChange(visString[4] != '0', isRightArm, hasChanged);
}

void FPoseAILiveValues::ProcessCompactScalarsBody(const FString& compactString) {
    if(compactString.Len() < 14) return;
    bodyHeight = FixedB64pairToFloat(compactString[0], compactString[1]) + 1.0f;
    chestYaw = FixedB64pairToFloat(compactString[2], compactString[3]) * 180.0f;
    stanceYaw = FixedB64pairToFloat(compactString[4], compactString[5]) * 180.0f;
    stableFeet = UintB64ToUint(compactString[6], compactString[7]);
    handZoneLeft = UintB64ToUint(compactString[8], compactString[9]);
    handZoneRight = UintB64ToUint(compactString[10], compactString[11]);
    isCrouching = UintB64ToUint(compactString[12], compactString[13]) > 0;
}

void FPoseAILiveValues::ProcessCompactVectorsBody(const FString& compactString) {
    if (compactString.Len() < 12) return;
    upperBodyLean.Set(FixedB64pairToFloat(compactString[0], compactString[1]) * 180.0f,
                      FixedB64pairToFloat(compactString[2], compactString[3]) * 180.0f);
    hipScreen.Set(FixedB64pairToFloat(compactString[4], compactString[5]),
                  FixedB64pairToFloat(compactString[6], compactString[7]));
    chestScreen.Set(FixedB64pairToFloat(compactString[8], compactString[9]),
                    FixedB64pairToFloat(compactString[10], compactString[11]));
}

void FPoseAILiveValues::ProcessCompactVectorsHandLeft(const FString& compactString) {
    if (compactString.Len() < 4) return;
    pointHandLeft.Set(FixedB64pairToFloat(compactString[0], compactString[1]),
                      FixedB64pairToFloat(compactString[2], compactString[3]));
}

void FPoseAILiveValues::ProcessCompactVectorsHandRight(const FString& compactString) {
    if (compactString.Len() < 4) return;
    pointHandRight.Set(FixedB64pairToFloat(compactString[0], compactString[1]),
        FixedB64pairToFloat(compactString[2], compactString[3]));
}



void FPoseAIVisibilityFlags::ProcessVerbose(FPoseAIScalarStruct& scalars) {
    hasChanged = false;
    SetAndCheckForChange(scalars.VisTorso, isTorso, hasChanged);
    SetAndCheckForChange(scalars.VisLegL, isLeftLeg, hasChanged);
    SetAndCheckForChange(scalars.VisLegR, isRightLeg, hasChanged);
    SetAndCheckForChange(scalars.VisArmL, isLeftArm, hasChanged);
    SetAndCheckForChange(scalars.VisArmR, isRightArm, hasChanged);
}


const FString FPoseAILiveValues::fieldPointScreen = FString(TEXT("PointScreen"));

void FPoseAIScalarStruct::ProcessJsonObject(const TSharedPtr < FJsonObject > scaBody) {
    FJsonObjectConverter::JsonObjectToUStruct(scaBody.ToSharedRef(), this);
}

void FPoseAIEventStruct::ProcessJsonObject(const TSharedPtr < FJsonObject > eveBody) {
    FJsonObjectConverter::JsonObjectToUStruct(eveBody.ToSharedRef(), this);
}

void FPoseAIVerbose::ProcessJsonObject(const TSharedPtr < FJsonObject > jsonObj) {
    FJsonObjectConverter::JsonObjectToUStruct(jsonObj.ToSharedRef(), this);
}


void FPoseAILiveValues::ProcessVerboseBody(const FPoseAIVerbose& verbose){
    bodyHeight = verbose.Scalars.BodyHeight;
    stableFeet = FMath::RoundToInt((float)verbose.Scalars.StableFoot);
    stanceYaw = verbose.Scalars.StanceYaw * 180.0f;
    chestYaw = verbose.Scalars.ChestYaw * 180.0f;
    isCrouching = verbose.Scalars.IsCrouching > 0.5f;
    handZoneLeft = verbose.Scalars.HandZoneL;
    handZoneRight= verbose.Scalars.HandZoneR;
    ProcessArrayAsVector2D(verbose.Vectors.HipLean, upperBodyLean);
    upperBodyLean *= 180.0f;
    ProcessArrayAsVector2D(verbose.Vectors.HipScreen, hipScreen);
    ProcessArrayAsVector2D(verbose.Vectors.ChestScreen, chestScreen);
}



void FPoseAILiveValues::ProcessVerboseVectorsHandLeft(const TSharedPtr < FJsonObject > vecHand){
    if (vecHand==nullptr)
        return;
    ProcessFieldAsVector2D(vecHand, fieldPointScreen, pointHandLeft);
}

void FPoseAILiveValues::ProcesssVerboseVectorsHandRight(const TSharedPtr < FJsonObject > vecHand){
    if (vecHand==nullptr)
        return;
    ProcessFieldAsVector2D(vecHand, fieldPointScreen, pointHandRight);
}

