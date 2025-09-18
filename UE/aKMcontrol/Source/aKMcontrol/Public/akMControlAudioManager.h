// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEJackAudioLinkSubsystem.h"

#include "akMControlAudioManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAkMControl, Log, All);

UCLASS()
class AKMCONTROL_API AakMControlAudioManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AakMControlAudioManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	UUEJackAudioLinkSubsystem* JackAudioLinkSubsystem;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Level meter state for all channels
	TArray<float> SmoothedRmsLevels;
	TArray<float> PeakLevels;
	TArray<float> TimesOfLastPeak;

};
