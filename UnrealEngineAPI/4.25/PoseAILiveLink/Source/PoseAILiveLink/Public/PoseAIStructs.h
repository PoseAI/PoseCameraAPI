// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "PoseAIStructs.generated.h"


#define LOCTEXT_NAMESPACE "PoseAI"



/* decoding utilities for compact representation */
float UintB64ToUint(char a, char b);
uint32 UintB64ToUint(char a, char b, char c);
float FixedB64pairToFloat(char a, char b);
void FStringFixed12ToFloat(const FString& data, TArray<float>& flatArray);
void FlatArrayToQuats(const TArray<float>& flatArray, TArray<FQuat>& quatArray);


/* the handshake configures the main parameters of pose camera*/
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIHandshake
{
    GENERATED_BODY()
  
   /* the camera mode. "Room", "Desktop", "Portrait", "RoomBodyOnly", "PortraitBodyOnly" */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    FString mode = "Room";

    /* the skeletal rig to use, based on standard nomenclature and rotations: "UE4", "MetaHuman", "DazUE", "Mixamo" */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    FString rig = "UE4";

    /* the model context.  Will enable new AI models as they are deployed*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    FString context = "Default";

    /* the target frame rate, where phone does interpolation and smoothing for animations. Events are raw.*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    int32 syncFPS = 60;

    /* the desired camera speed.  On many phones only 30 or 60 FPS will be accepted and otherwise you get default*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    int32 cameraFPS = 60;

    /* flips left/right limbs and rotates as if the player is looking at a mirror*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    bool isMirrored = false;

    /* controls compactness of packet.  0 is verbose JSON (mainly use for debugging), 1 is fairly compact JSON (preferred as of this plugin release).  We may add even more condensed formats in the future.*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    int32 packetFormat = 1;

    /* whether to include motion within camera frame in hips or in root*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    bool useRootMotion = false;

    /* Not needed for PoseCam.  Used only for licensee connection and verification.*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    FString whoami = "";

    /* Not needed for PoseCam.  Used only for licencee connection and verification.*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Handshake")
    FString signature = "";


    FString ToString() const;

    bool operator==(const FPoseAIHandshake& Other) const;
    bool operator!=(const FPoseAIHandshake& Other) const {return !operator==(Other);}
    FString YesNoString(bool val) const {
        return val ? FString("YES") : FString("NO");
    }
};


/*adjusts the sensitivity of PoseAI events*/
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIModelConfig
{
    GENERATED_BODY()

    /* alpha where 0.0 is lowest sensitivity (more likely to miss events) and 1.0 is maximum sensitivity (more likely false triggers).*/
    UPROPERTY(BlueprintReadWrite, Category = "PoseAI Model Sensitivity")
    float stepSensitivity = 0.5f;

    /* alpha where 0.0 is lowest sensitivity (more likely to miss events) and 1.0 is maximum sensitivity (more likely false triggers).*/
    UPROPERTY(BlueprintReadWrite, Category = "PoseAI Model Sensitivity")
        float armSensitivity = 0.5f;

    /* alpha where 0.0 is lowest sensitivity (more likely to miss events) and 1.0 is maximum sensitivity (more likely false triggers).*/
    UPROPERTY(BlueprintReadWrite, Category = "PoseAI Model Sensitivity")
    float crouchSensitivity = 0.5f;

    /* alpha where 0.0 is lowest sensitivity (more likely to miss events) and 1.0 is maximum sensitivity (more likely false triggers).*/
    UPROPERTY(BlueprintReadWrite, Category = "PoseAI Model Sensitivity")
    float jumpSensitivity = 0.5f;

    UPROPERTY(BlueprintReadWrite, Category = "PoseAI Model Sensitivity")
    bool isMirrored;

    FString ToString() const; 
    FString YesNoString(bool val) const {
        return val ? FString("YES") : FString("NO");
    }
};


/** base class for the two event notifications formats sent by camera.  All events have a uint count, which upon change signifies a new event has been registered
    and a second property, either a float or uint.
*/
USTRUCT()
struct POSEAILIVELINK_API FPoseAIEventPairBase
{
    GENERATED_BODY()
public:
    /** number of events registered by camera */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI Event")
        uint32 Count = 0;

    virtual void ProcessCompact(const FString& compactString) {};
    bool CheckTriggerAndUpdate();
private:
    uint32 InternalCount = 0;
};

/**
*structure to hold a type of event notification
*/
USTRUCT()
struct POSEAILIVELINK_API FPoseAIEventPair : public FPoseAIEventPairBase
{
    GENERATED_BODY()
public:
    /**magnitude of event where approrpiate */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI Event")
        float Magnitude = 0.0f;

    void ProcessCompact(const FString& compactString) override;

};


USTRUCT()
struct POSEAILIVELINK_API FPoseAIGesturePair : public FPoseAIEventPairBase
{
    GENERATED_BODY()
public:
    /**index code for most recent gesture */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI Event")
        uint32 Current = 0;

    void ProcessCompact(const FString& compactString) override;

};


/**
*structure to receive event notifications
*/
USTRUCT()
struct POSEAILIVELINK_API FPoseAIEventStruct
{
    GENERATED_BODY()
public:
    /** number of footsteps registered by camera, body height magnitude */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair Footstep;

    /** number of left foot sidesteps registered by camera. sign indicates direction */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair SidestepL;

    /** number of right foot sidesteps registered by camera. sign indicates direction */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair SidestepR;

    /** number of jumps registered by camera */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair Jump;

    /** number of footsplits registered by camera, body width magnitude */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair FeetSplit;

    /** number of arm pumps registered by camera, body height magnitude */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair ArmPump;

    /** number of arm flexes registered by camera, body width magnitude */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIEventPair ArmFlex;

    /** number of left or dual arm gestures registered by camera and most recent gesture */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIGesturePair ArmGestureL;

    /** number of right arm gestures registered by camera and most recent gesture */
    UPROPERTY(VisibleAnywhere, Category = "PoseAI")
        FPoseAIGesturePair ArmGestureR;


    void ProcessJsonObject(const TSharedPtr < FJsonObject > eveBody);
    void ProcessCompactBody(const FString& compactString);

};

/**
*structure to share additional information
*/
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIScalarStruct
{
    GENERATED_BODY()
public:
    /** visibility flags by body part.  Correspond to figure in the app */
    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        float VisTorso = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        float VisArmL = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        float VisArmR = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        float VisLegL = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        float VisLegR = 0.0f;

    /** location of left hand relative to body in broad zones */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        int32 HandZoneL = 5;

    /** location of right hand relative to body in broad zones */
    UPROPERTY(BlueprintReadOnly, Category = "PosPeAI")
        int32 HandZoneR = 5;

    /** Heading in degrees of torso.  0 is heading to camera */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float ChestYaw = 0.0f;

    /** Heading in degrees of flattened left foot to right foot vector relative to camera. 0 is parallel to camera */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float StanceYaw = 0.0f;

    /** estimated actual height of the subject in clip coordinates (2.0 = full height of image) */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float BodyHeight = 0.0f;

    /** whether subject is crouching */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float IsCrouching = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
    int32 StableFoot = 0.0f;

    
    void ProcessJsonObject(const TSharedPtr < FJsonObject > scaBody);

};



USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIVerboseBodyVectors
{
    GENERATED_BODY()
public:
    UPROPERTY()
        TArray<float> HipLean;
    UPROPERTY()
        TArray<float> HipScreen;
    UPROPERTY()
        TArray<float> ChestScreen;
};


USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIVerbose
{
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseAIEventStruct Events;
    UPROPERTY()
    FPoseAIScalarStruct Scalars;
    UPROPERTY()
    FPoseAIVerboseBodyVectors Vectors;
    void ProcessJsonObject(const TSharedPtr < FJsonObject > jsonObj);

};

/**
 * structure to store and expose visibility flags for events alerting programmer if subject is out of camera
 */
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAIVisibilityFlags
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        bool isTorso = false;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        bool isLeftArm = false;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        bool isRightArm = false;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        bool isLeftLeg = false;

    UPROPERTY(BlueprintReadOnly, Category = "Flags")
        bool isRightLeg = false;


    bool HasChanged() { return hasChanged; }
    void ProcessVerbose(FPoseAIScalarStruct& scalars);
    void ProcessCompact(const FString& visString);

private:
    bool hasChanged = false;
};

/**
 * structure to share additional information
 */
USTRUCT(BlueprintType)
struct POSEAILIVELINK_API FPoseAILiveValues
{
GENERATED_BODY()
public:
    /** How much subject is leaning in radians, head-to-hips; x to the side, y forward */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D upperBodyLean = FVector2D(0.0f, 0.0f);
    
    /** estimated stance height of the subject in clip coordinates (2.0 = full height of image) */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        bool isCrouching = false;

    /** location of left hand relative to body in broad zones */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        int32 handZoneLeft = 5;

    /** location of left hand relative to body in broad zones */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        int32 handZoneRight = 5;

    /** offset location of subject in camera frame.  Good for lateral movement */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
    FVector rootTranslation = FVector::ZeroVector;

    /** Heading in radians of flattened left foot to right foot vector relative to camera. 0 is parallel to camera */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
    float stanceYaw = 0.0f;

    /** Heading in radians of torso.  0 is heading to camera */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float chestYaw = 0.0f;


    /** Heading of flattened left foot to right foot vector relative to camera */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        int32 modelLatency = 0.0f;

    /** timestamp according to pose camera device (CMTime), in seconds */
    double timestamp = 0.0;

    /** current height of jump in body units */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
        float jumpHeight = 0.0f;

    /** estimated actual height of the subject in clip coordinates (2.0 = full height of image) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    float bodyHeight = 0.0f;

    /** location of hips in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D hipScreen = FVector2D(0.0f, 0.0f);
    
    /** location of chest in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D chestScreen = FVector2D(0.0f, 0.0f);

    /** location of left index finger in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D pointHandLeft = FVector2D(0.0f, 0.0f);
    
    /** location of right index finger in camera frame (clip coordinates -1 to 1, 0 is center ) */
    UPROPERTY(BlueprintReadOnly, Category="PoseAI")
    FVector2D pointHandRight = FVector2D(0.0f, 0.0f);  


    /** if at least one foot has been stationary for a few frames */
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI")
    int32 stableFeet = 0;


    void ProcessVerboseBody(const FPoseAIVerbose& scalars);
    void ProcessVerboseVectorsHandLeft(const TSharedPtr < FJsonObject > vecHand);
    void ProcesssVerboseVectorsHandRight(const TSharedPtr < FJsonObject > vecHand);
    void ProcessCompactScalarsBody(const FString& compactString);
    void ProcessCompactVectorsBody(const FString& compactString);
    void ProcessCompactVectorsHandLeft(const FString& compactString);
    void ProcessCompactVectorsHandRight(const FString& compactString);


private:
    static const FString fieldPointScreen;
};



