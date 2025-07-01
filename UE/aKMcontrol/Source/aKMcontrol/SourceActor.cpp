// Fill out your copyright notice in the Description page of Project Settings.


#include "SourceActor.h"

// Sets default values
ASourceActor::ASourceActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASourceActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASourceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FString ASourceActor::GetName()
{
	return Name;
}


