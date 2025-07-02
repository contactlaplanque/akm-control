// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Jack/JackClient.h"
#include "Jack/JackTypes.h"
#include "JackAudioInterface.generated.h"

UCLASS()
class AKMCONTROL_API AJackAudioInterface : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AJackAudioInterface();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Jack connection management
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool ConnectToJack(const FString& ClientName = TEXT("UnrealJackClient"));

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	void DisconnectFromJack();

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool IsConnectedToJack() const;

	// Jack server management
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool CheckJackServerRunning();

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool KillJackServer();

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool StartJackServer(const FString& JackdPath = TEXT("C:/Program Files/JACK2/jackd.exe"));

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool EnsureJackServerRunning(const FString& JackdPath = TEXT("C:/Program Files/JACK2/jackd.exe"));

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool StartJackServerWithParameters(const FString& JackdPath, int32 InSampleRate, int32 InBufferSize, const FString& InDriver = TEXT("portaudio"));

	// Jack information
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	FString GetJackClientName() const;

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	int32 GetJackSampleRate() const;

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	int32 GetJackBufferSize() const;

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	float GetJackCPUUsage() const;

	// Port management
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	TArray<FString> GetAvailableJackPorts();

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool ConnectJackPorts(const FString& SourcePort, const FString& DestinationPort);

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool DisconnectJackPorts(const FString& SourcePort, const FString& DestinationPort);

	// Audio port registration
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool RegisterAudioPorts(int32 NumInputs = 64, int32 NumOutputs = 64, const FString& BaseName = TEXT("unreal"));

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	void UnregisterAllAudioPorts();

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	int32 GetNumRegisteredInputPorts() const;

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	int32 GetNumRegisteredOutputPorts() const;

	// Performance metrics
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	FJackPerformanceMetrics GetJackPerformanceMetrics() const;

	// Connection status
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	EJackConnectionStatus GetJackConnectionStatus() const;

	// Server information
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	bool WasJackServerStarted() const;

	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	FString GetJackServerInfo() const;

	// Debug display
	UFUNCTION(BlueprintCallable, Category = "Jack Audio")
	void DisplayJackInfoOnScreen();

protected:
	// Jack client instance (regular C++ class, not a UObject)
	FJackClient JackClient;

	// Jack server configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio|Server Configuration")
	FString JackServerPath = TEXT("C:/Program Files/JACK2/jackd.exe");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio|Server Configuration")
	int32 SampleRate = 48000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio|Server Configuration")
	int32 BufferSize = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio|Server Configuration")
	FString AudioDriver = TEXT("portaudio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio|Server Configuration")
	bool bAutoStartServer = true;

	// Debug display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio")
	bool bShowDebugInfo = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio")
	FVector2D DebugTextLocation = FVector2D(50.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jack Audio")
	FColor DebugTextColor = FColor::Green;

private:
	// Internal helper functions
	void UpdateDebugDisplay();
};
