// Fill out your copyright notice in the Description page of Project Settings.


#include "Source.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// Sets default values
ASource::ASource()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	RootComponent = SphereMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		SphereMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereMesh->SetGenerateOverlapEvents(false);
	SphereMesh->SetCastShadow(false);
}

// Called when the game starts or when spawned
void ASource::BeginPlay()
{
	Super::BeginPlay();
	RefreshVisual();
}

// Called every frame
void ASource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASource::Initialize(int32 InID)
{
	ID = InID;
	SetActive(false);
	SetPosition(Position);
	SetRadius(Radius);
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
	Radius = FMath::Max(0.001f, InRadius);
	RefreshVisual();
}

void ASource::RefreshVisual()
{
	if (SphereMesh)
	{
		const float DefaultSphereRadius = 50.0f; // UE BasicShapes Sphere radius (cm)
		const float UniformScale = Radius / DefaultSphereRadius;
		SphereMesh->SetWorldScale3D(FVector(UniformScale));
	}
	SetActorHiddenInGame(!Active);
}

