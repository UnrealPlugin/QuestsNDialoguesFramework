// Fill out your copyright notice in the Description page of Project Settings.


#include "NPC_Base.h"

#include "NarrativeFlowManager.h"
#include "ArticyFlowClasses.h"
#include "Interfaces/ArticyObjectWithText.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ANPC_Base::ANPC_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
}

// Called when the game starts or when spawned
void ANPC_Base::BeginPlay()
{
    Super::BeginPlay();

    UNarrativeFlowManager* NM = GetNarrativeManager();
    if (!NM) return;

    NM->OnNodeReached.AddDynamic(this,  &ANPC_Base::HandleNodeReached);
    NM->OnFlowFinished.AddDynamic(this, &ANPC_Base::HandleFlowFinished);

    StartDialogue();
}

void ANPC_Base::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Always unbind — the subsystem outlives individual actors
    if (UNarrativeFlowManager* NM = GetNarrativeManager())
    {
        NM->OnNodeReached.RemoveDynamic(this,  &ANPC_Base::HandleNodeReached);
        NM->OnFlowFinished.RemoveDynamic(this, &ANPC_Base::HandleFlowFinished);
    }

    Super::EndPlay(EndPlayReason);
}

void ANPC_Base::StartDialogue()
{
    UNarrativeFlowManager* NM = GetNarrativeManager();
    if (!NM) return;

    NM->StartFlow(DialogueRef);
}

void ANPC_Base::HandleNodeReached(const FDialogueNodePayload& Payload)
{
    if (!Payload.Node) return;

    // Cast to UArticyDialogueFragment to access speaker + text.
    // If your articy project uses a custom template, cast to your generated
    // type instead (e.g. UArticyObject_DialogueFragment_MyProject).
    // UArticyDialogueFragment is the safe base class that always exists.
    const UArticyDialogueFragment* Fragment = Cast<UArticyDialogueFragment>(Payload.Node);
    if (!Fragment)
    {
        // Node is a hub, dialogue root, or flow fragment — no text to display.
        // Auto-continue through structural nodes.
        GetNarrativeManager()->ContinueFlow();
        return;
    }

    // FArticyText wraps FText — call ToString() or use it directly with Slate/UMG.
    //const FString SpeakerText  = Fragment->ToString();

    // Speaker is a reference to another articy entity (e.g. a Character object).
    // GetDisplayName() gives you the name set in articy:draft.
    FString SpeakerText = TEXT("Unknown");
    if (const IArticyObjectWithText* Speaker = Cast<const IArticyObjectWithText>(Fragment))
    {
        SpeakerText = Speaker->GetText().ToString();
    }

    //UE_LOG(LogTemp, Log, TEXT("[%s]: %s"), *SpeakerName, *SpeakerText);
    UE_LOG(LogTemp, Log, TEXT("%s"), *SpeakerText);

    // Decide what to do next based on branch count:
    const int32 NumBranches = Payload.Branches.Num();

    if (NumBranches == 1)
    {
        // Linear flow — advance automatically.
        // In a real game you'd wait for player input or a "Next" button press.
        GetNarrativeManager()->ContinueFlow();
    }
    else if (NumBranches > 1)
    {
        // Multiple choices — log them for now.
        // In a real game you'd populate a choice UI widget here.
        for (int32 i = 0; i < NumBranches; ++i)
        {
            const FArticyBranch& Branch = Payload.Branches[i];
            // The branch target's display name is the choice text.
            // if (UArticyObject* Target = Cast<UArticyObject>(Branch.GetTarget().GetObject(GetWorld())))
            // {
            //     UE_LOG(LogTemp, Log, TEXT("  [%d] %s"), i, *Target->GetDisplayName().ToString());
            // }
        }

        // For testing, just pick the first branch automatically.
        // Replace with: GetNarrativeManager()->ChooseBranch(PlayerChoiceIndex);
        GetNarrativeManager()->ChooseBranch(0);
    }
}

void ANPC_Base::HandleFlowFinished()
{
    UE_LOG(LogTemp, Log, TEXT("ANPC_Base: dialogue finished"));
    // Re-enable player movement, hide UI, etc.
}

UNarrativeFlowManager* ANPC_Base::GetNarrativeManager() const
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    return GI ? GI->GetSubsystem<UNarrativeFlowManager>() : nullptr;
}