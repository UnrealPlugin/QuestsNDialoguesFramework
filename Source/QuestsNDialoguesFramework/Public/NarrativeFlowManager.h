#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArticyFlowPlayer.h"
#include "ArticyRef.h"

#include "NarrativeFlowManager.generated.h"

class UArticyObject;

// ---------------------------------------------------------------------------
// Payload broadcast to UI / gameplay when a dialogue node is reached
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FDialogueNodePayload
{
    GENERATED_BODY()

    /** The articy object the flow paused on */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UArticyObject> Node = nullptr;

    /**
     * Available branches from the current node.
     * Num() == 0  -> flow ended, OnFlowFinished will also fire.
     * Num() == 1  -> single path, call ContinueFlow().
     * Num()  > 1  -> player choice, call ChooseBranch(index).
     */
    UPROPERTY(BlueprintReadOnly)
    TArray<FArticyBranch> Branches;
};

// ---------------------------------------------------------------------------
// Delegate signatures
// ---------------------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeReached,   const FDialogueNodePayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlowFinished);

// ---------------------------------------------------------------------------
// UNarrativeFlowManager
// ---------------------------------------------------------------------------
UCLASS()
class QUESTSNDIALOGUESFRAMEWORK_API UNarrativeFlowManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /** Start flow from any articy object (dialogue, hub, jump target) */
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void StartFlow(const FArticyRef& StartNode);

    /** Choose a branch by index after OnNodeReached fires with Num(Branches) > 1. */
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void ChooseBranch(int32 BranchIndex);

    /** Shorthand for ChooseBranch(0) -- use when Num(Branches) == 1. */
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void ContinueFlow();

    /** Hard-stop the current flow. */
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void StopFlow();

    UFUNCTION(BlueprintPure, Category = "Narrative")
    bool IsFlowActive() const { return bFlowActive; }

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    /** Fired each time the flow pauses on a node. Check Payload.Branches.Num(). */
    UPROPERTY(BlueprintAssignable, Category = "Narrative")
    FOnNodeReached OnNodeReached;

    /** Fired when flow reaches a terminal node or StopFlow() is called. */
    UPROPERTY(BlueprintAssignable, Category = "Narrative")
    FOnFlowFinished OnFlowFinished;

private:
    /** Spawns the host actor + component on first StartFlow() call. */
    void EnsureFlowPlayer();

    // OnPlayerPaused: void(TScriptInterface<IArticyFlowObject>)
    UFUNCTION()
    void HandlePlayerPaused(TScriptInterface<IArticyFlowObject> PausedOn);

    // OnBranchesUpdated: void(const TArray<FArticyBranch>&)
    // Fires after the flow player explores outgoing branches from the paused node.
    UFUNCTION()
    void HandleBranchesUpdated(const TArray<FArticyBranch>& AvailableBranches);

    UPROPERTY()
    TObjectPtr<UArticyFlowPlayer> FlowPlayer;

    // Populated by HandleBranchesUpdated; consumed by ChooseBranch.
    TArray<FArticyBranch> CachedBranches;

    // Pending node from HandlePlayerPaused; paired with branches in HandleBranchesUpdated.
    TObjectPtr<UArticyObject> PendingNode;

    bool bFlowActive = false;
};