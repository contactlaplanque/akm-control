// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Source.generated.h"

class UStaticMeshComponent;

UCLASS()
class AKMCONTROL_API ASource : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASource();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	int32 ID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	bool Active = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	FVector Position = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	float Radius = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UStaticMeshComponent* SphereMesh;

	// Initialization and controls
	void Initialize(int32 InID);
	void SetActive(bool bInActive);
	void SetPosition(const FVector& InPosition);
	void SetRadius(float InRadius);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void RefreshVisual();

};
