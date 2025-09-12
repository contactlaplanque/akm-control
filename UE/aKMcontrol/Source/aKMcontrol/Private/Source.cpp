// Fill out your copyright notice in the Description page of Project Settings.


#include "Source.h"

#include "akMControlAudioManager.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

// Sets default values
ASource::ASource()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SourceOuterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceOuterMesh"));
	SourceInnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceInnerMesh"));
	RootComponent = SourceInnerMesh;
	SourceOuterMesh->SetupAttachment(RootComponent);

	// Label component
	SourceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SourceLabel"));
	SourceLabel->SetupAttachment(RootComponent);
	SourceLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	SourceLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	SourceLabel->SetTextRenderColor(Color);
	SourceLabel->SetWorldSize(150.0f);
	SourceLabel->SetText(FText::FromString(TEXT("")));
	SourceLabel->SetRelativeLocation(FVector(0.0, 0.0, 140.0));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SourceOuterMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SourceOuterMeshAsset.Succeeded())
	{
		SourceOuterMesh->SetStaticMesh(SourceOuterMeshAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SourceInnerMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SourceInnerMeshAsset.Succeeded())
	{
		SourceInnerMesh->SetStaticMesh(SourceInnerMeshAsset.Object);
	}

	// Load base material asset from content path
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SourceOuterMatAsset(TEXT("/Game/LAPLANQUE_ND/MaterialsLibrary/M_SourceOuter.M_SourceOuter"));
	if (SourceOuterMatAsset.Succeeded())
	{
		SourceOuterMeshMaterial = SourceOuterMatAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SourceInnerMatAsset(TEXT("/Game/LAPLANQUE_ND/MaterialsLibrary/M_SourceInner.M_SourceInner"));
	if (SourceInnerMatAsset.Succeeded())
	{
		SourceInnerMeshMaterial = SourceInnerMatAsset.Object;
	}
	
	SourceOuterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SourceOuterMesh->SetGenerateOverlapEvents(false);
	SourceOuterMesh->SetCastShadow(false);

	SourceInnerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SourceInnerMesh->SetGenerateOverlapEvents(false);
	SourceInnerMesh->SetCastShadow(false);
}

// Called when the game starts or when spawned
void ASource::BeginPlay()
{
	Super::BeginPlay();
	EnsureDynamicMaterial();
	ApplyColorToMaterial();
	RefreshVisual();

	// Initialize label text and color
	SourceLabel->SetText(FText::AsNumber(ID));
	SourceLabel->SetTextRenderColor(Color);

	// Find BP_akMCharacter and its active camera
	TArray<AActor*> FoundCharacters;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("akMCharacter")), FoundCharacters);
	// If tag is not set, try by class path
	if (FoundCharacters.Num() == 0)
	{
		TArray<AActor*> FoundByClass;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundByClass);
		for (AActor* Actor : FoundByClass)
		{
			if (Actor && Actor->GetClass()->GetPathName().Contains(TEXT("/Game/LAPLANQUE_ND/Character/BP_akMCharacter")))
			{
				UActorComponent* CamComp = Actor->GetComponentByClass(UCameraComponent::StaticClass());
				LabelFacingTargetComponent = Cast<USceneComponent>(CamComp);
				break;
			}
		}
	}
	else
	{
		UActorComponent* CamComp = FoundCharacters[0]->GetComponentByClass(UCameraComponent::StaticClass());
		LabelFacingTargetComponent = Cast<USceneComponent>(CamComp);
	}
}

// Called every frame
void ASource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTextFacing();
}

void ASource::Initialize(int32 InID)
{
	ID = InID;
	SetActive(false);
	SetPosition(Position);
	SetRadius(Radius);
	EnsureDynamicMaterial();
	ApplyColorToMaterial();
	SourceLabel->SetText(FText::AsNumber(ID));
	SourceLabel->SetTextRenderColor(Color);
}

void ASource::SetActive(bool bInActive)
{
	Active = bInActive;
	SetActorHiddenInGame(!Active);
	SetActorEnableCollision(false);
	SetActorTickEnabled(Active);
}

void ASource::SetPosition(const FVector& InPosition)
{
	Position = InPosition;
	SetActorLocation(Position);
}

void ASource::SetRadius(float InRadius)
{
	Radius = FMath::Max(1.0f, InRadius);
	InnerMeshRadius = FMath::Min(10.0f,InRadius * 0.2f);
	RefreshVisual();
}

void ASource::SetColor(const FColor& InColor)
{
	Color = InColor;
	ApplyColorToMaterial();
	SourceLabel->SetTextRenderColor(Color);
}

void ASource::RefreshVisual()
{
	if (SourceOuterMesh)
	{
		const float DefaultSphereRadius = 50.0f;
		const float UniformScale = Radius / DefaultSphereRadius;
		SourceOuterMesh->SetWorldScale3D(FVector(UniformScale));
	}
	if (SourceInnerMesh)
	{
		const float DefaultSphereRadius = 50.0f;
		const float UniformScale = InnerMeshRadius / DefaultSphereRadius;
		SourceInnerMesh->SetWorldScale3D(FVector(UniformScale));
	}
	SetActorHiddenInGame(!Active);
}

void ASource::EnsureDynamicMaterial()
{
	if (!IsValid(SourceOuterMesh) || !IsValid(SourceInnerMesh))
	{
		return;
	}
	if (!SourceOuterMID)
	{
		if (SourceOuterMeshMaterial)
		{
			SourceOuterMID = SourceOuterMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, SourceOuterMeshMaterial);
		}
		else
		{
			// Fall back to creating a MID from current material if any
			UMaterialInterface* CurrentMat = SourceOuterMesh->GetMaterial(0);
			if (CurrentMat)
			{
				SourceOuterMID = SourceOuterMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, CurrentMat);
			}
		}
	}
	if (!SourceInnerMID)
	{
		if (SourceInnerMeshMaterial)
		{
			SourceInnerMID = SourceInnerMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, SourceInnerMeshMaterial);
		}
		else
		{
			// Fall back to creating a MID from current material if any
			UMaterialInterface* CurrentMat = SourceInnerMesh->GetMaterial(0);
			if (CurrentMat)
			{
				SourceInnerMID = SourceInnerMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, CurrentMat);
			}
		}
	}
}

void ASource::ApplyColorToMaterial()
{
	const FLinearColor Linear = FLinearColor(Color);
	if (SourceOuterMID)
	{
		SourceOuterMID->SetVectorParameterValue(TEXT("TintColor"), Linear);
	}
	if (SourceInnerMID)
	{
		SourceInnerMID->SetVectorParameterValue(TEXT("Color"), Linear);
	}
	
}

void ASource::UpdateTextFacing()
{
	if (!SourceLabel)
	{
		return;
	}
	if (!LabelFacingTargetComponent)
	{
		return;
	}

	const FVector CameraLocation = LabelFacingTargetComponent->GetComponentLocation();
	const FVector LabelLocation = SourceLabel->GetComponentLocation();
	FVector Forward = (CameraLocation - LabelLocation);
	Forward.Z = 0.0f; // keep upright
	Forward.Normalize();

	const FRotator NewRot = Forward.Rotation();
	SourceLabel->SetWorldRotation(NewRot);
}

