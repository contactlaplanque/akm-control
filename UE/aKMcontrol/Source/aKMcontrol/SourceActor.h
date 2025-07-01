// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SourceActor.generated.h"

UCLASS()
class AKMCONTROL_API ASourceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASourceActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Variables

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FString Name;

	// Getter Functions
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FString GetName();
	
};
