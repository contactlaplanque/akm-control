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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="akM|ServerStatus")
	bool bIsServerAlive = false;

	UPROPERTY(BlueprintReadWrite, Category="akM|ServerStatus")
	bool bServerIsStarting = false;

	bool StartSpatServerProcess();
	void StopSpatServerProcess();

	// Holds console log lines for ImGui rendering
	TArray<FString> ImGuiConsoleBuffer;
	int32 MaxConsoleLines = 500;
	
	// JACK auto-connection state
	UFUNCTION()
	void HandleNewJackClientConnected(const FString& ClientName, int32 NumInputPorts, int32 NumOutputPorts);

	UFUNCTION()
	void HandleJackClientDisconnected(const FString& ClientName);

	// Returns 1-based indices of Unreal JACK client's input ports that we have not reserved yet
	TArray<int32> GetAvailableUnrealInputPortIndices() const;

	// Name of our Unreal JACK client
	FString UnrealJackClientName;

	// Tracks which Unreal input port indices (1-based) are connected to scsynth outputs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="akM|ServerStatus")
	TArray<int32> ConnectedUnrealInputIndicesFromScsynth;

	// Whether scsynth outputs are connected to our Unreal inputs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="akM|ServerStatus")
	bool bAKMserverAudioOutputPortsConnected = false;

	// Map new external clients to Unreal input indices we connected for them (not exposed to UHT)
	TMap<FString, TArray<int32>> ConnectedUnrealInputIndicesByClient;

	// Accept a newly detected external JACK client and wire it to scsynth and Unreal
	UFUNCTION(BlueprintCallable, Category="akM|SpatServer")
	void AcceptExternalClient(const FString& ClientName, int32 NumInputPorts, int32 NumOutputPorts);

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

	// Disconnect all connections to/from our Unreal JACK client
	void DisconnectAllConnectionsToUnreal();



};
