// Fill out your copyright notice in the Description page of Project Settings.


#include "SourcesManager.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ASourcesManager::ASourcesManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASourcesManager::BeginPlay()
{
	Super::BeginPlay();
	InitializeSources();
}

void ASourcesManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DespawnAllSources();
	Super::EndPlay(EndPlayReason);
}

void ASourcesManager::InitializeSources()
{
	if (!SourceClass)
	{
		SourceClass = ASource::StaticClass();
	}

	Sources.SetNum(0);
	Sources.Reserve(NumSources);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 Index = 0; Index < NumSources; ++Index)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASource* NewSource = World->SpawnActor<ASource>(SourceClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (NewSource)
		{
			NewSource->Initialize(Index + 1);
			NewSource->SetActive(false);
			Sources.Add(NewSource);
		}
	}
}

void ASourcesManager::DespawnAllSources()
{
	for (ASource* Src : Sources)
	{
		if (IsValid(Src))
		{
			Src->Destroy();
		}
	}
	Sources.Reset();
}

ASource* ASourcesManager::GetSourceByID(int32 ID) const
{
	if (ID < 1 || ID > Sources.Num())
	{
		return nullptr;
	}
	return Sources[ID - 1];
}

int32 ASourcesManager::GetNumActive() const
{
	int32 Count = 0;
	for (ASource* Src : Sources)
	{
		if (IsValid(Src) && Src->Active)
		{
			++Count;
		}
	}
	return Count;
}

ASource* ASourcesManager::ActivateNextInactiveSource()
{
	for (ASource* Src : Sources)
	{
		if (IsValid(Src) && !Src->Active)
		{
			Src->SetActive(true);
			return Src;
		}
	}
	return nullptr;
}

bool ASourcesManager::ActivateSourceByID(int32 ID)
{
	ASource* Src = GetSourceByID(ID);
	if (!IsValid(Src)) { return false; }
	Src->SetActive(true);
	return true;
}

bool ASourcesManager::DeactivateSourceByID(int32 ID)
{
	ASource* Src = GetSourceByID(ID);
	if (!IsValid(Src)) { return false; }
	Src->SetActive(false);
	return true;
}

// Called every frame
void ASourcesManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

