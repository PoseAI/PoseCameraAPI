// Copyright Pose AI Ltd 2021

#include "PoseAIEventDispatcher.h"




void UStepCounter::Halt(bool fade) {
    num_ = 0;
    tail_ = -1;
    if (fade)
        last_time_ = FMath::Min(last_time_, FDateTime::Now() - FTimespan::FromSeconds(static_cast<double>(timeout)));
    else {
        last_time_ = FMath::Min(last_time_, FDateTime::Now() - FTimespan::FromSeconds(static_cast<double>(timeout + fadeDurationOnTimeout)));
        steps_per_second_ = 0.0f;
        distance_per_second_ = 0.0f;
    }
}

float UStepCounter::CheckIfActiveAndFade() {
    float time_since_last_step = TimeSinceLastStep();
    if (time_since_last_step > timeout) {
        num_ = 0;
        tail_ = -1;
        return (fadeDurationOnTimeout > 0.0f) ?
            FMath::Max(0.0f, 1.0f - (time_since_last_step - timeout) / fadeDurationOnTimeout)
            : 0.0f;
    }
    else
        return 1.0f;
}

float UStepCounter::StepsPerSecond() {
    return steps_per_second_ * CheckIfActiveAndFade();
}

float UStepCounter::DistancePerSecond() {
    return distance_per_second_ * CheckIfActiveAndFade();
}

float UStepCounter::LastDistance() {
    return (num_ > 0) ? heights_[tail_] : 0.0f;
}

float UStepCounter::TimeSinceLastStep() {
    return (FDateTime::Now() - last_time_).GetTotalSeconds();;
}

void UStepCounter::RegisterStep(float stepDistance) {
    totalSteps++;
    totalDistance += stepDistance;
    tail_ = (tail_ + 1) % num_to_track;
    last_time_ = FDateTime::Now();
    times_[tail_] = last_time_;
    heights_[tail_] = stepDistance;
    num_ = FGenericPlatformMath::Min(num_ + 1, num_to_track);

    float elapsed_time;
    if (num_ < 2)
        elapsed_time = 1.0f;
    else {
        int head_ = (tail_ + 1) % num_;
        elapsed_time = (times_[tail_] - times_[head_]).GetTotalSeconds();
        if (elapsed_time <= 0.0) elapsed_time = 1.0f;
    }
    steps_per_second_ = static_cast<float>(times_.Num()) / elapsed_time;
    distance_per_second_ = 0.0f;
    for (int h = 0; h < num_; ++h)
        distance_per_second_ += heights_[h];
    distance_per_second_ /= elapsed_time;

}
UStepCounter* UStepCounter::SetProperties(float timeoutIn, float fadeDuration) {
    timeout = timeoutIn;
    fadeDurationOnTimeout = fadeDuration;
    return this;
}


void UPoseAIMovementComponent::InitializeObjects() {
     footsteps = NewObject<UStepCounter>();
    leftsteps = NewObject<UStepCounter>()->SetProperties(0.3f, 0.1f);
    rightsteps = NewObject<UStepCounter>()->SetProperties(0.3f, 0.1f);
    feetsplits = NewObject<UStepCounter>();
    armpumps = NewObject<UStepCounter>();
    armflexes = NewObject<UStepCounter>();
    armjacks = NewObject<UStepCounter>()->SetProperties(1.0f, 0.5f);
    armflapL = NewObject<UStepCounter>()->SetProperties(0.5f, 1.5f);
    armflapR = NewObject<UStepCounter>()->SetProperties(0.5f, 1.5f);
    jumps = NewObject<UStepCounter>();
}

bool UPoseAIMovementComponent::RegisterAs(FName name, bool siezeIfTaken) {
    InitializeObjects();
    bool success = UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentByName(this, name, siezeIfTaken);
    if (success)
        onRegistered.Broadcast(name);
    return success;
}

void UPoseAIMovementComponent::RegisterAsFirstAvailable() {
    InitializeObjects();
    UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentForFirstAvailableSubject(this);
}


void UPoseAIMovementComponent::Deregister() {
    UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentByName(this, NAME_None,true);
    subjectName = NAME_None;
}

void UPoseAIMovementComponent::ChangeModelConfig(FPoseAIModelConfig config) {
    UPoseAIEventDispatcher::GetDispatcher()->BroadcastConfigUpdate(subjectName, config);
}

void UPoseAIMovementComponent::SetHandshake(const FPoseAIHandshake& handshake) {
    UPoseAIEventDispatcher::GetDispatcher()->SetHandshake(handshake);
}


bool UPoseAIEventDispatcher::RegisterComponentByName(UPoseAIMovementComponent* component, FName name, bool siezeIfTaken) {
    UE_LOG(LogTemp, Display, TEXT("PoseAI: Event dispatcher, registering %s"), *(name.ToString()));

    UPoseAIMovementComponent* existing_component;
    if (HasComponent(name, existing_component)) {
        if (!siezeIfTaken) return false;
        existing_component->Deregister();
    }

    FName prev = component->GetSubjectName();
    componentsByName.Remove(prev);
    if (name != NAME_None)
        componentsByName.Emplace(name, component);
    component->lastFrameReceived = FDateTime::Now();
    component->subjectName = name;
    return true;
}


UPoseAIEventDispatcher* UPoseAIEventDispatcher::theInstance = nullptr;


void UPoseAIEventDispatcher::RegisterComponentForFirstAvailableSubject(UPoseAIMovementComponent* component) {
    FName firstUnbound = GetFirstUnboundSubject();
    if (firstUnbound != NAME_None)
        component->RegisterAs(firstUnbound, true);
    else
        componentQueue.Enqueue(component);
}

FName UPoseAIEventDispatcher::GetFirstUnboundSubject(bool excludeIdleSubjects) {
    for (auto& elem : knownConnectionsWithTime) {
        if (excludeIdleSubjects && (FDateTime::Now() - elem.Value).GetTotalSeconds() > timeoutInSeconds) continue;
        UPoseAIMovementComponent* component;
        if (HasComponent(elem.Key, component) && component->GetSubjectName() == elem.Key) continue;
        return elem.Key;
    }
    return NAME_None;
}


void UPoseAIEventDispatcher::BroadcastSubjectConnected(FName rigName) {
    knownConnectionsWithTime.Emplace(rigName, FDateTime::Now());
    AsyncTask(ENamedThreads::GameThread, [this, rigName]() {
        UPoseAIMovementComponent* existing_component;
        bool isReconnection = HasComponent(rigName, existing_component);
        if (isReconnection) {
            if (existing_component != nullptr && IsValid(existing_component) )  existing_component->onRegistered.Broadcast(rigName);;
        } else if (!componentQueue.IsEmpty()) {
            
            UPoseAIMovementComponent* component;
            componentQueue.Dequeue(component);
            if (component != nullptr && IsValid(component))  component->RegisterAs(rigName, true);
            
        }
        subjectConnected.Broadcast(rigName, isReconnection);
     });
}

void UPoseAIEventDispatcher::BroadcastConfigUpdate(FName rigName, FPoseAIModelConfig config) {
    modelConfigUpdate.Broadcast(rigName, config);
}

void UPoseAIEventDispatcher::SetHandshake(const FPoseAIHandshake& handshake) {
    handshakeUpdate.Broadcast(handshake);
}


void UPoseAIEventDispatcher::BroadcastFrameReceived(FName rigName) {

    knownConnectionsWithTime.Add(rigName,FDateTime::Now());
    AsyncTask(ENamedThreads::GameThread, [this, rigName]() {
        frameReceived.Broadcast(rigName);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) component->lastFrameReceived = FDateTime::Now();
     });
}

void UPoseAIEventDispatcher::BroadcastVisibilityChange(FName rigName, FPoseAIVisibilityFlags visibilityFlags){
    AsyncTask(ENamedThreads::GameThread, [this, rigName, visibilityFlags]() {
        visibilityChange.Broadcast(rigName, visibilityFlags);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->onVisibilityChange.Broadcast(rigName, visibilityFlags);
            component->visibilityFlags = visibilityFlags;
        }
     });
}

void UPoseAIEventDispatcher::BroadcastLiveValues(FName rigName, FPoseAILiveValues values){
    AsyncTask(ENamedThreads::GameThread, [this, rigName, values]() {
        liveValues.Broadcast(rigName, values);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->onLiveValues.Broadcast(rigName, values);
            component->SetLiveValues(values);
        }
    });
}

void UPoseAIEventDispatcher::BroadcastFootsteps(FName rigName, float stepHeight, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, stepHeight, isLeftStep]() {
        footsteps.Broadcast(rigName, stepHeight, isLeftStep);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->footsteps->RegisterStep(stepHeight);
            component->onFootstep.Broadcast(rigName, stepHeight, isLeftStep);
        }
    });
}
void UPoseAIEventDispatcher::BroadcastFeetsplits(FName rigName, float width, bool isExpanding) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, width, isExpanding]() {
        footsplits.Broadcast(rigName, width, isExpanding);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->feetsplits->RegisterStep(width);
            component->onFeetsplit.Broadcast(rigName, width, isExpanding);
        }
        });
}
void UPoseAIEventDispatcher::BroadcastArmpumps(FName rigName, float stepHeight) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, stepHeight]() {
        armpumps.Broadcast(rigName, stepHeight);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->armpumps->RegisterStep(stepHeight);
            component->onArmpump.Broadcast(rigName, stepHeight);
        }
    });
}

void UPoseAIEventDispatcher::BroadcastArmflexes(FName rigName, float width, bool isExpanding) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, width, isExpanding]() {
        armflexes.Broadcast(rigName, width, isExpanding);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->armflexes->RegisterStep(width);
            component->onArmflex.Broadcast(rigName, width, isExpanding);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastArmjacks(FName rigName, bool isRising) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, isRising]() {
        armjacks.Broadcast(rigName, isRising);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->armjacks->RegisterStep(0.5f);
            component->onArmjack.Broadcast(rigName, isRising);
        }
        });
}



void UPoseAIEventDispatcher::BroadcastSidestepL(FName rigName, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, isLeftStep]() {
        sidestepLeftFoot.Broadcast(rigName, isLeftStep);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            ((isLeftStep) ? component->leftsteps : component->rightsteps)->RegisterStep(1.0f);
            component->onSidestepLeftFoot.Broadcast(rigName, isLeftStep);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastSidestepR(FName rigName, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, isLeftStep]() {
        sidestepRightFoot.Broadcast(rigName, isLeftStep);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            ((isLeftStep) ? component->leftsteps : component->rightsteps)->RegisterStep(1.0f);
            component->onSidestepRightFoot.Broadcast(rigName, isLeftStep);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastJumps(FName rigName) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName]() {
        jumps.Broadcast(rigName);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->onJump.Broadcast(rigName);
            component->jumps->RegisterStep(1.0f);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastCrouches(FName rigName, bool isCrouching) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, isCrouching]() {
        crouches.Broadcast(rigName, isCrouching);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) component->onCrouch.Broadcast(rigName, isCrouching);
        });
}

void UPoseAIEventDispatcher::BroadcastArmGestureL(FName rigName, int32 gesture) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, gesture]() {
        armGestureLeftOrDual.Broadcast(rigName, gesture);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->onArmGestureLeft.Broadcast(rigName, gesture);
            if (gesture == 10) {
                component->armflapL->RegisterStep(1.0f);
                component->onArmflapL.Broadcast(rigName);
            }
        }
        });
}

void UPoseAIEventDispatcher::BroadcastArmGestureR(FName rigName, int32 gesture) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, gesture]() {
        armGestureRight.Broadcast(rigName, gesture);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) {
            component->onArmGestureRight.Broadcast(rigName, gesture);
            if (gesture == 10) {
                component->armflapR->RegisterStep(1.0f);
                component->onArmflapR.Broadcast(rigName);
            }
        }
        });
}

void UPoseAIEventDispatcher::BroadcastHandToZoneL(FName rigName, int32 zone) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, zone]() {
        handToZoneL.Broadcast(rigName, zone);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) component->onHandToZoneL.Broadcast(rigName, zone);
        });
}


void UPoseAIEventDispatcher::BroadcastHandToZoneR(FName rigName, int32 zone) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName, zone]() {
        handToZoneR.Broadcast(rigName, zone);
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) component->onHandToZoneR.Broadcast(rigName, zone);
        });
}

void UPoseAIEventDispatcher::BroadcastStationary(FName rigName) {
    AsyncTask(ENamedThreads::GameThread, [this, rigName]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(rigName, component)) component->onStationary.Broadcast(rigName);
        });
}


bool UPoseAIEventDispatcher::HasComponent(FName name, UPoseAIMovementComponent*& component) {
    if (!componentsByName.Contains(name))
        return false;
    component = componentsByName[name];
    return component != nullptr && IsValid(component);
}