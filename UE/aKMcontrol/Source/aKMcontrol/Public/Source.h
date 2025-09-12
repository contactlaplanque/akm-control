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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UStaticMeshComponent* SourceOuterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInterface* SourceOuterMeshMaterial;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* SourceOuterMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UStaticMeshComponent* SourceInnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UMaterialInterface* SourceInnerMeshMaterial;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* SourceInnerMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|Components")
	UTextRenderComponent* SourceLabel;

	// Initialization and controls
	void Initialize(int32 InID);
	void SetActive(bool bInActive);
	void SetPosition(const FVector& InPosition);
	void SetRadius(float InRadius);
	void SetColor(const FColor& InColor);

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
