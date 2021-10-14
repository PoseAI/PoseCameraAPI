// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"
#include "PoseAIStructs.h"
#include "PoseAIEventDispatcher.generated.h"


#define LOCTEXT_NAMESPACE "PoseAI"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAISubjectConnected, FName, Name, bool, isReconnection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIRegisteredAs, FName, Name);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIFrameReceived, FName, Name);
DECLARE_MULTICAST_DELEGATE_OneParam(FPoseAIHandshakeUpdate, const FPoseAIHandshake&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPoseAIConfigUpdate, FName, FPoseAIModelConfig);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIVisibilityChange, FName, Name, FPoseAIVisibilityFlags, Flags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAILiveValuesUpdate, FName, Name, FPoseAILiveValues, LiveValues);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPoseAIFootstepEvent, FName, Name, float, height, bool, isLeftFoot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAISidestepEvent, FName, Name, bool, isLeftStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPoseAIFootsplitEvent, FName, Name, float, width, bool, isExpanding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIArmpumpEvent, FName, Name, float, height);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPoseAIArmflexEvent, FName, Name, float, width, bool, isExpanding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIArmjackEvent, FName, Name, bool, isRising);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIArmflapEvent, FName, Name);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIJumpEvent, FName, Name);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAICrouchEvent, FName, Name, bool, isCrouching);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIHandToZoneEvent, FName, Name, int32, zone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIArmGestureEvent, FName, Name, int32, armGesture);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIStationaryEvent, FName, Name);



UCLASS(ClassGroup = (PoseAI))
class POSEAILIVELINK_API UStepCounter : public UObject
{
    GENERATED_BODY()

public:
    /** Convenience setter for timeout and fadeduration at the same time*/
    UFUNCTION(BlueprintCallable, Category = "PoseAI Steptracker")
        UStepCounter* SetProperties(float timeoutIn = 0.5f, float fadeDuration = 0.2f);

    /** clears all steps.  Optionally fades speed over <timeout>*/
    UFUNCTION(BlueprintCallable, Category = "PoseAI Steptracker")
        void Halt(bool fade);

    /** Average step distance in [body width, body height] units per second.  If most recent step was longer than <timout> ago, speed is faded to zero */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Steptracker")
        float DistancePerSecond();
    
    /** Average number of steps per second.  If most recent step was longer than <timout> ago, speed is faded to zero */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Steptracker")
        float StepsPerSecond();

    /** Time since last step in seconds*/
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Steptracker")
        float TimeSinceLastStep();

    /** most recent step distance*/
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Steptracker")
        float LastDistance();


    /** time since last step when motion begins to slow*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Steptracker")
        float timeout = 0.5f;

    /** after a next step timeout, the speed is faded to zero over this duration*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PoseAI Steptracker")
        float fadeDurationOnTimeout = 0.2f;

    /** total steps registered by the stepcounter*/
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Steptracker")
        int32 totalSteps = 0;

    /** total distance registered by the stepcounter*/
    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Steptracker")
        float totalDistance = 0.0f;

    void RegisterStep(float stepDistance);

   UStepCounter(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
    {
        
        times_.SetNumUninitialized(num_to_track);
        heights_.SetNumUninitialized(num_to_track);
    }


private:
    float CheckIfActiveAndFade();
    const int32 num_to_track = 4;
    int32 num_ = 0;
    int32 tail_ = -1;

    float steps_per_second_ = 0.0f;
    float distance_per_second_ = 0.0f;
    FDateTime last_time_ = FDateTime::Now();
    TArray<FDateTime> times_;
    TArray<float> heights_;
};


class UPoseAIEventDispatcher;

UCLASS(ClassGroup = (PoseAI))
class POSEAILIVELINK_API UPoseAIMovementComponent : public UActorComponent
{
    GENERATED_BODY()
    friend UPoseAIEventDispatcher;
 
 public:
     /** sets this component as the receipient for events for the specific name */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void RegisterAsFirstAvailable();
     
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     bool RegisterAs(FName name, bool siezeIfTaken=true);
     
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void Deregister();

     UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Setup")
     FName GetSubjectName() { return subjectName; }

     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void ChangeModelConfig(FPoseAIModelConfig config);

     /** sets the handshake for all sources */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     static void SetHandshake(const FPoseAIHandshake& handshake);

     UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PoseAI Events")
     FDateTime lastFrameReceived;

     UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PoseAI Events")
         FPoseAIVisibilityFlags visibilityFlags;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PoseAI Events")
        FPoseAILiveValues mostRecentValues;  

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* footsteps;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* leftsteps;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* rightsteps;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* feetsplits;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* armpumps ;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* armflexes;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* armjacks;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* armflapL;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* armflapR;

    UPROPERTY(BlueprintReadOnly, Category = "PoseAI Events")
        UStepCounter* jumps;

    /**********  events ********************/

    /** when the component succeesfully registered with a connection (i.e. a pose camera joined) */
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIRegisteredAs onRegistered;
    
    /** when any body part visisbility flag changes */
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIVisibilityChange onVisibilityChange;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAILiveValuesUpdate onLiveValues;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIFootstepEvent onFootstep;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIFootsplitEvent onFeetsplit;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAISidestepEvent onSidestepLeftFoot;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAISidestepEvent onSidestepRightFoot;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmpumpEvent onArmpump;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmflexEvent onArmflex;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmflapEvent onArmflapR;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmflapEvent onArmflapL;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmjackEvent onArmjack;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmGestureEvent onArmGestureLeft;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmGestureEvent onArmGestureRight;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIJumpEvent onJump;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAICrouchEvent onCrouch;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIHandToZoneEvent onHandToZoneL;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIHandToZoneEvent onHandToZoneR;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIStationaryEvent onStationary;

    void SetLiveValues(FPoseAILiveValues values) {
        mostRecentValues = values;
    }

    virtual void InitializeComponent() override {
        InitializeObjects();
    }

private:
    FName subjectName;
    void InitializeObjects();

};


/**
 * Dispatcher to transmit events to blueprints and c++, can access events from all sources
 */
UCLASS(Blueprintable)
class POSEAILIVELINK_API UPoseAIEventDispatcher : public UObject
{
GENERATED_BODY()
public:
    /** Gets the singleton dispatcher.  Use this for binding or to get named components */
    UFUNCTION(BlueprintCallable, Category = "PoseAI Events")
    static UPoseAIEventDispatcher* GetDispatcher() {
        if (theInstance==nullptr) {
            theInstance = NewObject<UPoseAIEventDispatcher>();
            theInstance->AddToRoot();
            UE_LOG(LogTemp, Display, TEXT("PoseAILiveLink: Creating EventDispatcher."));
        }
        return theInstance;
    }

    /** sets the handshake for all sources */
    UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
    void SetHandshake(const FPoseAIHandshake& handshake);

    FPoseAIHandshakeUpdate handshakeUpdate;
    FPoseAIConfigUpdate modelConfigUpdate;


    UFUNCTION(BlueprintCallable, Category = "PoseAI Events")
    FName GetFirstUnboundSubject(bool excludeIdleSubjects = true);

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAISubjectConnected subjectConnected;


    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIFrameReceived frameReceived;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIVisibilityChange visibilityChange;
    
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAILiveValuesUpdate liveValues;
    
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIFootstepEvent footsteps;
    
    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIFootsplitEvent footsplits;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAISidestepEvent sidestepLeftFoot;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAISidestepEvent sidestepRightFoot;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIArmpumpEvent armpumps;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIArmflexEvent armflexes;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIArmjackEvent armjacks;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmGestureEvent armGestureLeftOrDual;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
        FPoseAIArmGestureEvent armGestureRight;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIJumpEvent jumps;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAICrouchEvent crouches;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIHandToZoneEvent handToZoneL;

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAIHandToZoneEvent handToZoneR;

    void BroadcastSubjectConnected(FName rigName);
    void BroadcastConfigUpdate(FName rigName, FPoseAIModelConfig config);
    void BroadcastFrameReceived(FName rigName);
    void BroadcastVisibilityChange(FName rigName, FPoseAIVisibilityFlags visibilityFlags);
    void BroadcastLiveValues(FName rigName, FPoseAILiveValues values);
    void BroadcastFootsteps(FName rigName, float stepHeight, bool isLeftStep);
    void BroadcastFeetsplits(FName rigName, float stepHeight, bool isExpanding);
    void BroadcastSidestepL(FName rigName, bool isLeftStep);
    void BroadcastSidestepR(FName rigName, bool isLeftStep);
    void BroadcastArmpumps(FName rigName, float stepHeight);
    void BroadcastArmflexes(FName rigName, float stepHeight, bool isExpanding);
    void BroadcastArmjacks(FName rigName, bool isRising);
    void BroadcastArmGestureL(FName rigName, int32 gesture);
    void BroadcastArmGestureR(FName rigName, int32 gesture);
    void BroadcastJumps(FName rigName);
    void BroadcastCrouches(FName rigName, bool isCrouching);
    void BroadcastHandToZoneL(FName rigName, int32 zone);
    void BroadcastHandToZoneR(FName rigName, int32 zone);
    void BroadcastStationary(FName rigName);
    
    bool RegisterComponentByName(UPoseAIMovementComponent* component, FName name, bool siezeIfTaken);
    void RegisterComponentForFirstAvailableSubject(UPoseAIMovementComponent* component);

    bool HasComponent(FName name, UPoseAIMovementComponent*& component);


private:
    static UPoseAIEventDispatcher* theInstance;
    const double timeoutInSeconds = 60.0;
    TQueue<UPoseAIMovementComponent*> componentQueue;
    UPROPERTY()
    TMap<FName, UPoseAIMovementComponent*> componentsByName;
    TMap<FName, FDateTime> knownConnectionsWithTime;
    UPoseAIEventDispatcher() : UObject() {};
};

