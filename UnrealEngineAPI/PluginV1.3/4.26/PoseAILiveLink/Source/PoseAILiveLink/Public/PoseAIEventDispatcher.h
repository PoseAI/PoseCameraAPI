// Copyright Pose AI Ltd 2021

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Async/Async.h"
#include "LiveLinkTypes.h"
#include "PoseAIStructs.h"
#include "PoseAIEventDispatcher.generated.h"


#define LOCTEXT_NAMESPACE "PoseAI"

DECLARE_MULTICAST_DELEGATE_OneParam(FPoseAIDisconnect, const FLiveLinkSubjectName&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPoseAIHandshakeUpdate, const FPoseAIHandshake&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPoseAIConfigUpdate, const FLiveLinkSubjectName&, FPoseAIModelConfig);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAISubjectConnected, const FLiveLinkSubjectName&, SubjectName, bool, isReconnection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIRegisteredAs, const FLiveLinkSubjectName&, SubjectName, FName, ConnectionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoseAIFrameReceived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIVisibilityChange, const FPoseAIVisibilityFlags&, Flags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAILiveValuesUpdate, const FPoseAILiveValues&, LiveValues);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIFootstepEvent, float, height, bool, isLeftFoot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAISidestepEvent,  bool, isLeftStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIFootsplitEvent, float, width, bool, isExpanding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIArmpumpEvent, float, height);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPoseAIArmflexEvent, float, width, bool, isExpanding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIArmjackEvent,bool, isRising);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoseAIArmflapEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoseAIJumpEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAICrouchEvent, bool, isCrouching);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIHandToZoneEvent, int32, zone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoseAIArmGestureEvent,  int32, armGesture);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoseAIStationaryEvent);



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

UCLASS(ClassGroup = (PoseAI), meta = (BlueprintSpawnableComponent))
class POSEAILIVELINK_API UPoseAIMovementComponent : public UActorComponent
{
    GENERATED_BODY()
    friend UPoseAIEventDispatcher;
 
 public:
     /** Adds a LiveLink source listening for Posecam at the designated port, but will overwrite an existing listener so developer needs to manage if using multiple portss (or use the AddSourceNextOpenPort node instead)*/
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     bool AddSource(const FPoseAIHandshake& handshake, FString& myIP, int32 portNum=8080, bool isIPv6 = false);

     /** Adds a LiveLink source listening for Posecam at the next open port beginning at 8080*/
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     bool AddSourceNextOpenPort(const FPoseAIHandshake& handshake, bool isIPv6, int32& portNum, FString& myIP);

     /** Sends a message to the connected PoseCamera to reconfigure the model with user settings */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void ChangeModelConfig(FPoseAIModelConfig config);

     /** sends disconnect message to app and clsoes source, freeing up Port*/
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void CloseSource();

     /** sends disconnect message to app but does not clsoe source */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void Disconnect();

     /** Get the LiveLink subject name associated with this component */
     UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PoseAI Setup")
     FLiveLinkSubjectName GetSubjectName() { return subjectName; }

     /** Will assign component to the next available Pose AI LiveLink source.  Useful if sources managed with a preswet (Otherwise prefer use of AddSource nodes) */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void RegisterAsFirstAvailable();

     /** sets the handshake for all sources */
     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     static void SetHandshake(const FPoseAIHandshake& handshake);


     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     bool RegisterAs(FLiveLinkSubjectName name, bool siezeIfTaken = true);

     UFUNCTION(BlueprintCallable, Category = "PoseAI Setup")
     void Deregister();


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
        Super::InitializeComponent();
        InitializeObjects();
    }

private:
    FLiveLinkSubjectName subjectName;
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
    FPoseAIDisconnect disconnect;
    FPoseAIDisconnect closeSource;

    UFUNCTION(BlueprintCallable, Category = "PoseAI Events")
     FLiveLinkSubjectName GetFirstUnboundSubject(bool excludeIdleSubjects = true);

    UPROPERTY(BlueprintAssignable, Category = "PoseAI Events")
    FPoseAISubjectConnected subjectConnected;

    // Connection driven events    
    void BroadcastCloseSource(const FLiveLinkSubjectName& subjectName);
    void BroadcastConfigUpdate(const FLiveLinkSubjectName& subjectName, FPoseAIModelConfig config);
    void BroadcastDisconnect(const FLiveLinkSubjectName& subjectName);
    void BroadcastFrameReceived(const FLiveLinkSubjectName& subjectName);
    void BroadcastSubjectConnected(const FLiveLinkSubjectName& subjectName);

    // Pose Camera driven events
    void BroadcastArmpumps(const FLiveLinkSubjectName& subjectName, float stepHeight);
    void BroadcastArmflexes(const FLiveLinkSubjectName& subjectName, float stepHeight, bool isExpanding);
    void BroadcastArmjacks(const FLiveLinkSubjectName& subjectName, bool isRising);
    void BroadcastArmGestureL(const FLiveLinkSubjectName& subjectName, int32 gesture);
    void BroadcastArmGestureR(const FLiveLinkSubjectName& subjectName, int32 gesture);
    void BroadcastCrouches(const FLiveLinkSubjectName& subjectName, bool isCrouching);
    void BroadcastFootsteps(const FLiveLinkSubjectName& subjectName, float stepHeight, bool isLeftStep);
    void BroadcastFeetsplits(const FLiveLinkSubjectName& subjectName, float stepHeight, bool isExpanding);
    void BroadcastHandToZoneL(const FLiveLinkSubjectName& subjectName, int32 zone);
    void BroadcastHandToZoneR(const FLiveLinkSubjectName& subjectName, int32 zone);
    void BroadcastJumps(const FLiveLinkSubjectName& subjectName);
    void BroadcastLiveValues(const FLiveLinkSubjectName& subjectName, FPoseAILiveValues values);
    void BroadcastSidestepL(const FLiveLinkSubjectName& subjectName, bool isLeftStep);
    void BroadcastSidestepR(const FLiveLinkSubjectName& subjectName, bool isLeftStep);   
    void BroadcastStationary(const FLiveLinkSubjectName& subjectName);
    void BroadcastVisibilityChange(const FLiveLinkSubjectName& subjectName, FPoseAIVisibilityFlags visibilityFlags);

    bool RegisterComponentByName(UPoseAIMovementComponent* component, const FLiveLinkSubjectName& name, bool siezeIfTaken);
    void RegisterComponentForFirstAvailableSubject(UPoseAIMovementComponent* component);

    bool HasComponent(const FLiveLinkSubjectName& name, UPoseAIMovementComponent*& component);


private:
    static UPoseAIEventDispatcher* theInstance;
    const double timeoutInSeconds = 60.0;
    TQueue<UPoseAIMovementComponent*> componentQueue;
    UPROPERTY()
    TMap<FLiveLinkSubjectName, UPoseAIMovementComponent*> componentsByName;
    TMap<FLiveLinkSubjectName, FDateTime> knownConnectionsWithTime;
    UPoseAIEventDispatcher() : UObject() {};
};

