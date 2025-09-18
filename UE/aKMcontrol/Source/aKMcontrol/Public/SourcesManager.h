// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Source.h"
#include "GameFramework/Actor.h"
#include "SourcesManager.generated.h"

class AakMSpatServerManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResetSourcesDemo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSourcesDemo1);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSourcesDemo2);

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

	UPROPERTY(BlueprintAssignable, Category="akM|Events")
	FOnResetSourcesDemo OnResetSourcesDemo;

	UPROPERTY(BlueprintAssignable, Category="akM|Events")
	FOnSourcesDemo1 OnSourcesDemo1;

	UPROPERTY(BlueprintAssignable, Category="akM|Events")
	FOnSourcesDemo2 OnSourcesDemo2;
	
	// Initialization and lifecycle
	UFUNCTION(BlueprintCallable)
	void InitializeSources();

	UFUNCTION(BlueprintCallable)
	void DespawnAllSources();

	// Activation control
	UFUNCTION(BlueprintCallable)
	ASource* ActivateNextInactiveSource();

	UFUNCTION(BlueprintCallable)
	bool ActivateSourceByID(int32 ID);

	UFUNCTION(BlueprintCallable)
	bool DeactivateSourceByID(int32 ID);

	// Query helpers
	UFUNCTION(BlueprintCallable)
	ASource* GetSourceByID(int32 ID) const;

	UFUNCTION(BlueprintCallable)
	int32 GetNumActive() const;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
