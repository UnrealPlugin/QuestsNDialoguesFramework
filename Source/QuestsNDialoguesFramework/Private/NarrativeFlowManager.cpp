#include "NarrativeFlowManager.h"

#include "ArticyFlowPlayer.h"
#include "ArticyObject.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------
// USubsystem lifecycle
// ---------------------------------------------------------------------------

void UNarrativeFlowManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // FlowPlayer is created lazily in EnsureFlowPlayer() once a world exists.
}

void UNarrativeFlowManager::Deinitialize()
{
    StopFlow();

    if (FlowPlayer)
    {
        FlowPlayer->OnPlayerPaused.RemoveDynamic(this, &UNarrativeFlowManager::HandlePlayerPaused);
        FlowPlayer->OnBranchesUpdated.RemoveDynamic(this, &UNarrativeFlowManager::HandleBranchesUpdated);

        // Destroy the host actor we spawned to own the component.
        AActor* Host = FlowPlayer->GetOwner();
        if (Host)
        {
            Host->Destroy();
        }

        FlowPlayer = nullptr;
    }

    Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UNarrativeFlowManager::EnsureFlowPlayer()
{
    if (FlowPlayer) return;

    UWorld* World = GetGameInstance()->GetWorld();
    if (!ensureMsgf(World, TEXT("UNarrativeFlowManager::EnsureFlowPlayer — no valid world")))
    {
        return;
    }

    // UArticyFlowPlayer is a UActorComponent and must live on an AActor.
    // Spawn a minimal transient host actor so GetOwner() is valid — articy's
    // GetMethodsProvider() asserts GetOwner() is non-null at runtime.
    FActorSpawnParameters Params;
    Params.Name        = TEXT("NarrativeFlowPlayerHost");
    Params.ObjectFlags = RF_Transient;
    AActor* HostActor  = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    check(HostActor);

    HostActor->SetActorHiddenInGame(true);
    HostActor->SetActorEnableCollision(false);

    FlowPlayer = NewObject<UArticyFlowPlayer>(HostActor, TEXT("ArticyFlowPlayer"));
    HostActor->AddInstanceComponent(FlowPlayer);
    FlowPlayer->RegisterComponent();

    // Pause on DialogueFragment, Dialogue, and FlowFragment by default.
    // Matches the plugin's own default bitmask — adjust with SetPauseOn() if needed.
    FlowPlayer->SetPauseOn(EArticyPausableType::DialogueFragment);

    FlowPlayer->OnPlayerPaused.AddDynamic(this,  &UNarrativeFlowManager::HandlePlayerPaused);
    FlowPlayer->OnBranchesUpdated.AddDynamic(this, &UNarrativeFlowManager::HandleBranchesUpdated);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UNarrativeFlowManager::StartFlow(const FArticyRef& StartNode)
{
    EnsureFlowPlayer();
    if (!FlowPlayer) return;

    CachedBranches.Reset();
    PendingNode = nullptr;
    bFlowActive = true;

    FlowPlayer->SetStartNode(StartNode);
    FlowPlayer->Play();
}

void UNarrativeFlowManager::ChooseBranch(int32 BranchIndex)
{
    if (!bFlowActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("UNarrativeFlowManager::ChooseBranch — no active flow"));
        return;
    }

    if (!CachedBranches.IsValidIndex(BranchIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("UNarrativeFlowManager::ChooseBranch — index %d out of range (%d branches)"),
               BranchIndex, CachedBranches.Num());
        return;
    }

    FlowPlayer->Play(BranchIndex);
    CachedBranches.Reset();
    PendingNode = nullptr;
}

void UNarrativeFlowManager::ContinueFlow()
{
    ChooseBranch(0);
}

void UNarrativeFlowManager::StopFlow()
{
    if (!bFlowActive) return;

    bFlowActive = false;
    CachedBranches.Reset();
    PendingNode = nullptr;

    OnFlowFinished.Broadcast();
}

// ---------------------------------------------------------------------------
// Articy delegate handlers
// ---------------------------------------------------------------------------

void UNarrativeFlowManager::HandlePlayerPaused(TScriptInterface<IArticyFlowObject> PausedOn)
{
    // Store the node — we pair it with branches in HandleBranchesUpdated,
    // which articy fires immediately after this on the same frame.
    PendingNode = Cast<UArticyObject>(PausedOn.GetObject());
}

void UNarrativeFlowManager::HandleBranchesUpdated(const TArray<FArticyBranch>& AvailableBranches)
{
    CachedBranches = AvailableBranches;

    if (CachedBranches.Num() == 0)
    {
        bFlowActive = false;
        OnFlowFinished.Broadcast();
        return;
    }

    FDialogueNodePayload Payload;
    Payload.Node     = PendingNode;
    Payload.Branches = CachedBranches;

    OnNodeReached.Broadcast(Payload);
}