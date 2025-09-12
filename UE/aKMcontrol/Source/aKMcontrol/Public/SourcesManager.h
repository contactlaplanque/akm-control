// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Source.h"
#include "GameFramework/Actor.h"
#include "SourcesManager.generated.h"

class AakMSpatServerManager;


UCLASS()
class AKMCONTROL_API ASourcesManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASourcesManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="akM|SourcesParameters")
	int32 NumSources = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="akM|SourcesParameters")
	TArray<ASource*> Sources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="akM|SourcesParameters")
	TSubclassOf<ASource> SourceClass;

	// Reference to akM Spat Server Manager to propagate to sources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Object References")
	AakMSpatServerManager* SpatServerManager = nullptr;

	// Initialization and lifecycle
	void InitializeSources();
	void DespawnAllSources();

	// Activation control
	ASource* ActivateNextInactiveSource();
	bool ActivateSourceByID(int32 ID);
	bool DeactivateSourceByID(int32 ID);

	// Query helpers
	ASource* GetSourceByID(int32 ID) const;
	int32 GetNumActive() const;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
