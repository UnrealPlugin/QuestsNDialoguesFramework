// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArticyRef.h"
#include "GameFramework/Actor.h"
#include "NPC_Base.generated.h"

struct FDialogueNodePayload;
class UNarrativeFlowManager;
struct FArticyRef;

UCLASS()
class QUESTSNDIALOGUESFRAMEWORK_API ANPC_Base : public AActor
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FArticyRef ArticyRef;
	
	// Sets default values for this actor's properties
	ANPC_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	
	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void StartDialogue();

private:
	UFUNCTION()
	void HandleNodeReached(const FDialogueNodePayload& Payload);

	UFUNCTION()
	void HandleFlowFinished();

	UNarrativeFlowManager* GetNarrativeManager() const;

	/** Point this at the articy dialogue node in the editor */
	UPROPERTY(EditAnywhere, Category = "Narrative")
	FArticyRef DialogueRef;
};