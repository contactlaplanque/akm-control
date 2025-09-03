// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "akMSpatServerManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpatServer, Log, All);

UCLASS()
class AKMCONTROL_API AakMSpatServerManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AakMSpatServerManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the game ends or actor is destroyed
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Blueprint-callable entry point to initialize and start the spat server
	UFUNCTION(BlueprintCallable, Category="akM|SpatServer")
	bool StartSpatServer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ServerStatus")
	bool bIsServerAlive = false;

	bool StartSpatServerProcess();
	void StopSpatServerProcess();

	// Holds console log lines for ImGui rendering
	TArray<FString> ImGuiConsoleBuffer;
	int32 MaxConsoleLines = 500;
	

private:
	// Child process handle and pipes for stdout/stderr
	FProcHandle SpatServerProcessHandle;
	void* ReadPipe;
	void* WritePipe;

	// Paths to executables and scripts
	FString SuperColliderInstallDir;
	FString SpatServerScriptPath;

	// Runtime state
	bool bIsServerRunning;

	// Helpers
	void PumpSpatServerOutput();
	bool ValidateRequiredPaths() const;

};
