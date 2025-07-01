// Fill out your copyright notice in the Description page of Project Settings.

#include "JackAudioInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// Sets default values
AJackAudioInterface::AJackAudioInterface()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AJackAudioInterface::BeginPlay()
{
	Super::BeginPlay();
	
	// Try to connect to Jack automatically on BeginPlay
	if (ConnectToJack())
	{
		// Register 64 I/O ports after successful connection
		if (RegisterAudioPorts(64, 64, TEXT("unreal")))
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Successfully registered 64 I/O ports"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to register audio ports"));
		}
	}
}

// Called every frame
void AJackAudioInterface::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update debug display if enabled
	if (bShowDebugInfo)
	{
		UpdateDebugDisplay();
	}
}

bool AJackAudioInterface::ConnectToJack(const FString& ClientName)
{
	bool bSuccess = JackClient.Connect(ClientName);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Successfully connected to Jack server"));
		
		// Activate the client
		if (JackClient.Activate())
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Jack client activated"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to activate Jack client"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to connect to Jack server"));
	}
	
	return bSuccess;
}

void AJackAudioInterface::DisconnectFromJack()
{
	JackClient.Disconnect();
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Disconnected from Jack server"));
}

bool AJackAudioInterface::IsConnectedToJack() const
{
	return JackClient.IsConnected();
}

FString AJackAudioInterface::GetJackClientName() const
{
	return JackClient.GetClientName();
}

int32 AJackAudioInterface::GetJackSampleRate() const
{
	return JackClient.GetSampleRate();
}

int32 AJackAudioInterface::GetJackBufferSize() const
{
	return JackClient.GetBufferSize();
}

float AJackAudioInterface::GetJackCPUUsage() const
{
	return JackClient.GetCPUUsage();
}

TArray<FString> AJackAudioInterface::GetAvailableJackPorts()
{
	return JackClient.GetAvailablePorts();
}

bool AJackAudioInterface::ConnectJackPorts(const FString& SourcePort, const FString& DestinationPort)
{
	return JackClient.ConnectPorts(SourcePort, DestinationPort);
}

bool AJackAudioInterface::DisconnectJackPorts(const FString& SourcePort, const FString& DestinationPort)
{
	return JackClient.DisconnectPorts(SourcePort, DestinationPort);
}

bool AJackAudioInterface::RegisterAudioPorts(int32 NumInputs, int32 NumOutputs, const FString& BaseName)
{
	return JackClient.RegisterAudioPorts(NumInputs, NumOutputs, BaseName);
}

void AJackAudioInterface::UnregisterAllAudioPorts()
{
	JackClient.UnregisterAllPorts();
}

int32 AJackAudioInterface::GetNumRegisteredInputPorts() const
{
	return JackClient.GetInputPorts().Num();
}

int32 AJackAudioInterface::GetNumRegisteredOutputPorts() const
{
	return JackClient.GetOutputPorts().Num();
}

FJackPerformanceMetrics AJackAudioInterface::GetJackPerformanceMetrics() const
{
	return JackClient.GetPerformanceMetrics();
}

EJackConnectionStatus AJackAudioInterface::GetJackConnectionStatus() const
{
	return JackClient.GetConnectionStatus();
}

bool AJackAudioInterface::WasJackServerStarted() const
{
	return JackClient.WasServerStarted();
}

FString AJackAudioInterface::GetJackServerInfo() const
{
	return JackClient.GetServerInfo();
}

void AJackAudioInterface::DisplayJackInfoOnScreen()
{
	if (!GEngine)
	{
		return;
	}

	FString DebugText;
	
	// Connection status
	EJackConnectionStatus Status = GetJackConnectionStatus();
	switch (Status)
	{
	case EJackConnectionStatus::Connected:
		DebugText += TEXT("Jack Status: CONNECTED\n");
		break;
	case EJackConnectionStatus::Connecting:
		DebugText += TEXT("Jack Status: CONNECTING\n");
		break;
	case EJackConnectionStatus::Disconnected:
		DebugText += TEXT("Jack Status: DISCONNECTED\n");
		break;
	case EJackConnectionStatus::Failed:
		DebugText += TEXT("Jack Status: FAILED\n");
		break;
	}

	if (IsConnectedToJack())
	{
		// Client information
		DebugText += FString::Printf(TEXT("Client Name: %s\n"), *GetJackClientName());
		DebugText += FString::Printf(TEXT("Sample Rate: %d Hz\n"), GetJackSampleRate());
		DebugText += FString::Printf(TEXT("Buffer Size: %d frames\n"), GetJackBufferSize());
		DebugText += FString::Printf(TEXT("CPU Usage: %.2f%%\n"), GetJackCPUUsage());

		// Server startup information
		if (WasJackServerStarted())
		{
			DebugText += TEXT("Server: Started by this app\n");
		}
		else
		{
			DebugText += TEXT("Server: Connected to existing\n");
		}

		// Performance metrics
		FJackPerformanceMetrics Metrics = GetJackPerformanceMetrics();
		DebugText += FString::Printf(TEXT("XRuns: %d\n"), Metrics.XRuns);
		DebugText += FString::Printf(TEXT("Audio Callback Time: %.2f ms\n"), Metrics.AudioCallbackTime);

		// Registered ports information
		DebugText += FString::Printf(TEXT("Registered Ports: %d in, %d out\n"), 
			GetNumRegisteredInputPorts(), GetNumRegisteredOutputPorts());

		// Available ports
		TArray<FString> Ports = GetAvailableJackPorts();
		DebugText += FString::Printf(TEXT("Available Ports: %d\n"), Ports.Num());
		
		// Show the first few ports as examples
		for (int32 i = 0; i < FMath::Min(Ports.Num(), 5); i++)
		{
			DebugText += FString::Printf(TEXT("  %s\n"), *Ports[i]);
		}
		if (Ports.Num() > 5)
		{
			DebugText += TEXT("  ...\n");
		}
	}

	// Display on screen
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, DebugTextColor, DebugText);
}

void AJackAudioInterface::UpdateDebugDisplay()
{
	// Update debug display every frame if enabled
	DisplayJackInfoOnScreen();
}

