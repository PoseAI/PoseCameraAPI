// Copyright Pose AI Ltd 2021

#include "PoseAIEventDispatcher.h"
#include "PoseAILiveLinkSingleSource.h"

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


bool UPoseAIMovementComponent::AddSourceNextOpenPort(const FPoseAIHandshake& handshake, bool isIPv6, int32& portNum, FString& myIP) {
    portNum = PoseAILiveLinkSingleSource::portDefault;
    while (!PoseAILiveLinkSingleSource::IsValidPort(portNum)) {
        portNum++;
        if (portNum > 49151)
            return false;
    }
    return AddSource(handshake, myIP, portNum, isIPv6);
}

bool UPoseAIMovementComponent::AddSource(const FPoseAIHandshake& handshake, FString& myIP, int32 portNum, bool isIPv6) {
    InitializeObjects();
   FLiveLinkSubjectName addedSubjectName;
   PoseAILiveLinkServer::GetIP(myIP);
    return PoseAILiveLinkSingleSource::AddSource(handshake, portNum, isIPv6, addedSubjectName) &&
        UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentByName(this, addedSubjectName, true);
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

bool UPoseAIMovementComponent::RegisterAs(FLiveLinkSubjectName name, bool siezeIfTaken) {
    InitializeObjects();
    bool success = UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentByName(this, name, siezeIfTaken);
    if (success)
        onRegistered.Broadcast(name, PoseAILiveLinkSingleSource::GetConnectionName(name));
    return success;
}

void UPoseAIMovementComponent::RegisterAsFirstAvailable() {
    InitializeObjects();
    UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentForFirstAvailableSubject(this);
}

void UPoseAIMovementComponent::CloseSource() {
    UPoseAIEventDispatcher::GetDispatcher()->BroadcastCloseSource(subjectName);
}

void UPoseAIMovementComponent::Disconnect() {
    UPoseAIEventDispatcher::GetDispatcher()->BroadcastDisconnect(subjectName);
}

void UPoseAIMovementComponent::Deregister() {
    UPoseAIEventDispatcher::GetDispatcher()->RegisterComponentByName(this, FLiveLinkSubjectName(NAME_None) ,true);
    subjectName = FLiveLinkSubjectName(NAME_None);
}

void UPoseAIMovementComponent::ChangeModelConfig(FPoseAIModelConfig config) {
    UPoseAIEventDispatcher::GetDispatcher()->BroadcastConfigUpdate(subjectName, config);
}

void UPoseAIMovementComponent::SetHandshake(const FPoseAIHandshake& handshake) {
    UPoseAIEventDispatcher::GetDispatcher()->SetHandshake(handshake);
}


bool UPoseAIEventDispatcher::RegisterComponentByName(UPoseAIMovementComponent* component, const FLiveLinkSubjectName& name, bool siezeIfTaken) {
    UE_LOG(LogTemp, Display, TEXT("PoseAI: Event dispatcher, registering %s"), *(name.ToString()));

    UPoseAIMovementComponent* existing_component;
    if (HasComponent(name, existing_component)) {
        if (!siezeIfTaken) return false;
        existing_component->Deregister();
    }

    FName prev = component->GetSubjectName();
    componentsByName.Remove(prev);
    if (name.Name != NAME_None)
        componentsByName.Emplace(name, component);
    component->lastFrameReceived = FDateTime::Now();
    component->subjectName = name;
    return true;
}


UPoseAIEventDispatcher* UPoseAIEventDispatcher::theInstance = nullptr;


void UPoseAIEventDispatcher::RegisterComponentForFirstAvailableSubject(UPoseAIMovementComponent* component) {
   FLiveLinkSubjectName firstUnbound = GetFirstUnboundSubject();
    if (firstUnbound.Name != NAME_None)
        component->RegisterAs(firstUnbound, true);
    else
        componentQueue.Enqueue(component);
}

FLiveLinkSubjectName UPoseAIEventDispatcher::GetFirstUnboundSubject(bool excludeIdleSubjects) {
    for (auto& elem : knownConnectionsWithTime) {
        if (excludeIdleSubjects && (FDateTime::Now() - elem.Value).GetTotalSeconds() > timeoutInSeconds) continue;
        UPoseAIMovementComponent* component;
        if (HasComponent(elem.Key, component) && component->GetSubjectName() == elem.Key) continue;
        return elem.Key;
    }
    return NAME_None;
}


void UPoseAIEventDispatcher::BroadcastSubjectConnected(const FLiveLinkSubjectName& subjectName) {
    knownConnectionsWithTime.Emplace(subjectName, FDateTime::Now());
    AsyncTask(ENamedThreads::GameThread, [this, subjectName]() {
        UPoseAIMovementComponent* existing_component;
        bool isReconnection = HasComponent(subjectName, existing_component);
        if (isReconnection) {
            if (existing_component != nullptr && IsValid(existing_component) )  existing_component->onRegistered.Broadcast(subjectName,  PoseAILiveLinkSingleSource::GetConnectionName(subjectName));
        } else if (!componentQueue.IsEmpty()) {
            UPoseAIMovementComponent* component;
            componentQueue.Dequeue(component);
            if (component != nullptr && IsValid(component))  component->RegisterAs(subjectName, true);
        }
        subjectConnected.Broadcast(subjectName, isReconnection);
     });
}

void UPoseAIEventDispatcher::BroadcastConfigUpdate(const FLiveLinkSubjectName& subjectName, FPoseAIModelConfig config) {
    modelConfigUpdate.Broadcast(subjectName, config);
}

void UPoseAIEventDispatcher::BroadcastDisconnect(const FLiveLinkSubjectName& subjectName) {
    disconnect.Broadcast(subjectName);
}

void UPoseAIEventDispatcher::BroadcastCloseSource(const FLiveLinkSubjectName& subjectName) {
    closeSource.Broadcast(subjectName);
}


void UPoseAIEventDispatcher::SetHandshake(const FPoseAIHandshake& handshake) {
    handshakeUpdate.Broadcast(handshake);
}

void UPoseAIEventDispatcher::BroadcastFrameReceived(const FLiveLinkSubjectName& subjectName) {
    knownConnectionsWithTime.Add(subjectName,FDateTime::Now());
    AsyncTask(ENamedThreads::GameThread, [this, subjectName]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) component->lastFrameReceived = FDateTime::Now();
     });
}

void UPoseAIEventDispatcher::BroadcastVisibilityChange(const FLiveLinkSubjectName& subjectName, FPoseAIVisibilityFlags visibilityFlags){
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, visibilityFlags]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->onVisibilityChange.Broadcast(visibilityFlags);
            component->visibilityFlags = visibilityFlags;
        }
     });
}

void UPoseAIEventDispatcher::BroadcastLiveValues(const FLiveLinkSubjectName& subjectName, FPoseAILiveValues values){
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, values]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->onLiveValues.Broadcast(values);
            component->SetLiveValues(values);
        }
    });
}

void UPoseAIEventDispatcher::BroadcastFootsteps(const FLiveLinkSubjectName& subjectName, float stepHeight, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, stepHeight, isLeftStep]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->footsteps->RegisterStep(stepHeight);
            component->onFootstep.Broadcast(stepHeight, isLeftStep);
        }
    });
}
void UPoseAIEventDispatcher::BroadcastFeetsplits(const FLiveLinkSubjectName& subjectName, float width, bool isExpanding) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, width, isExpanding]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->feetsplits->RegisterStep(width);
            component->onFeetsplit.Broadcast(width, isExpanding);
        }
        });
}
void UPoseAIEventDispatcher::BroadcastArmpumps(const FLiveLinkSubjectName& subjectName, float stepHeight) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, stepHeight]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->armpumps->RegisterStep(stepHeight);
            component->onArmpump.Broadcast(stepHeight);
        }
    });
}

void UPoseAIEventDispatcher::BroadcastArmflexes(const FLiveLinkSubjectName& subjectName, float width, bool isExpanding) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, width, isExpanding]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->armflexes->RegisterStep(width);
            component->onArmflex.Broadcast(width, isExpanding);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastArmjacks(const FLiveLinkSubjectName& subjectName, bool isRising) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, isRising]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->armjacks->RegisterStep(0.5f);
            component->onArmjack.Broadcast(isRising);
        }
        });
}


void UPoseAIEventDispatcher::BroadcastSidestepL(const FLiveLinkSubjectName& subjectName, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, isLeftStep]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            ((isLeftStep) ? component->leftsteps : component->rightsteps)->RegisterStep(1.0f);
            component->onSidestepLeftFoot.Broadcast(isLeftStep);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastSidestepR(const FLiveLinkSubjectName& subjectName, bool isLeftStep) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, isLeftStep]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            ((isLeftStep) ? component->leftsteps : component->rightsteps)->RegisterStep(1.0f);
            component->onSidestepRightFoot.Broadcast(isLeftStep);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastJumps(const FLiveLinkSubjectName& subjectName) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->onJump.Broadcast();
            component->jumps->RegisterStep(1.0f);
        }
        });
}

void UPoseAIEventDispatcher::BroadcastCrouches(const FLiveLinkSubjectName& subjectName, bool isCrouching) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, isCrouching]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) component->onCrouch.Broadcast(isCrouching);
        });
}

void UPoseAIEventDispatcher::BroadcastArmGestureL(const FLiveLinkSubjectName& subjectName, int32 gesture) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, gesture]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->onArmGestureLeft.Broadcast(gesture);
            if (gesture == 10) {
                component->armflapL->RegisterStep(1.0f);
                component->onArmflapL.Broadcast();
            }
        }
        });
}

void UPoseAIEventDispatcher::BroadcastArmGestureR(const FLiveLinkSubjectName& subjectName, int32 gesture) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, gesture]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) {
            component->onArmGestureRight.Broadcast(gesture);
            if (gesture == 10) {
                component->armflapR->RegisterStep(1.0f);
                component->onArmflapR.Broadcast();
            }
        }
        });
}

void UPoseAIEventDispatcher::BroadcastHandToZoneL(const FLiveLinkSubjectName& subjectName, int32 zone) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, zone]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) component->onHandToZoneL.Broadcast(zone);
        });
}


void UPoseAIEventDispatcher::BroadcastHandToZoneR(const FLiveLinkSubjectName& subjectName, int32 zone) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName, zone]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) component->onHandToZoneR.Broadcast(zone);
        });
}

void UPoseAIEventDispatcher::BroadcastStationary(const FLiveLinkSubjectName& subjectName) {
    AsyncTask(ENamedThreads::GameThread, [this, subjectName]() {
        UPoseAIMovementComponent* component;
        if (HasComponent(subjectName, component)) component->onStationary.Broadcast();
        });
}


bool UPoseAIEventDispatcher::HasComponent(const FLiveLinkSubjectName& name, UPoseAIMovementComponent*& component) {
    if (!componentsByName.Contains(name))
        return false;
    component = componentsByName[name];
    return component != nullptr && IsValid(component);
}