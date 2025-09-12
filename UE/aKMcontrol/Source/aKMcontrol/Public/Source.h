// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Source.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTextRenderComponent;
class USceneComponent;

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
	float Radius = 50.0f; //cm

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	FColor Color = FColor(0, 140, 250);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	float Gain = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	int A = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	float DelayMultiplier = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="akM|SourceParameters")
	float Reverb = 0.1f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UStaticMeshComponent* SourceOuterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInterface* SourceOuterMeshMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInstanceDynamic* SourceOuterMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UStaticMeshComponent* SourceInnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInterface* SourceInnerMeshMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInstanceDynamic* SourceInnerMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UTextRenderComponent* SourceLabel;

	// Initialization and controls
	void Initialize(int32 InID);

	UFUNCTION(BlueprintCallable)
	void SetActive(bool bInActive);

	UFUNCTION(BlueprintCallable)
	void SetPosition(const FVector& InPosition);

	UFUNCTION(BlueprintCallable)
	void SetRadius(float InRadius);

	UFUNCTION(BlueprintCallable)
	void SetColor(const FColor& InColor);

	UFUNCTION(BlueprintCallable)
	void SetGain(float InGain);

	UFUNCTION(BlueprintCallable)
	void SetA(int InA);

	UFUNCTION(BlueprintCallable)
	void SetDelayMultiplier(float InDelayMultiplier);

	UFUNCTION(BlueprintCallable)
	void SetReverb(float InReverb);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void RefreshVisual();
	void EnsureDynamicMaterial();
	void ApplyColorToMaterial();
	void UpdateTextFacing();

	UPROPERTY(Transient)
	USceneComponent* LabelFacingTargetComponent;

	float InnerMeshRadius = 10.0f;

};
