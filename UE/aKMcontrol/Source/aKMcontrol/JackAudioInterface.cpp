// Fill out your copyright notice in the Description page of Project Settings.

#include "JackAudioInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "TimerManager.h"

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
	
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Starting Jack audio system..."));
	
	// Ensure Jack server is running with our custom parameters
	if (bAutoStartServer)
	{
		// Check if a server is already running and kill it
		if (CheckJackServerRunning())
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Found existing Jack server, killing it to restart with custom parameters"));
			KillJackServer();
			FPlatformProcess::Sleep(1.0f); // Give it time to shut down
		}
		
		// Start the server with our custom parameters
		if (StartJackServerWithParameters(JackServerPath, SampleRate, BufferSize, AudioDriver))
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Jack server started with custom parameters"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to start Jack server with custom parameters"));
			return;
				}
	}

	// Connect to the Jack server
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Jack server is ready, connecting client..."));
	
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

		// Start server monitoring if enabled
		if (bMonitorServerStatus)
		{
			GetWorld()->GetTimerManager().SetTimer(ServerStatusTimerHandle, this, &AJackAudioInterface::OnServerStatusCheck, ServerCheckInterval, true);
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Started server monitoring (checking every %.1f seconds)"), ServerCheckInterval);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to connect to Jack server"));
	}
}

// Called when the actor is being destroyed
void AJackAudioInterface::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stop server monitoring
	if (ServerStatusTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ServerStatusTimerHandle);
	}

	// Kill Jack server if requested
	if (bKillServerOnShutdown)
	{
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Shutting down, killing Jack server..."));
		KillJackServer();
	}

	// Disconnect from Jack
	DisconnectFromJack();

	Super::EndPlay(EndPlayReason);
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
		// Activate the client
		if (JackClient.Activate())
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Jack client activated"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackAudioInterface: Failed to activate Jack client"));
		}
		bWasConnected = true;
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
	bWasConnected = false;
}

bool AJackAudioInterface::IsConnectedToJack() const
{
	return JackClient.IsConnected();
}

bool AJackAudioInterface::CheckJackServerRunning()
{
	return JackClient.CheckJackServerRunning();
}

bool AJackAudioInterface::KillJackServer()
{
	return JackClient.KillJackServer();
}

bool AJackAudioInterface::StartJackServer(const FString& JackdPath)
{
	return JackClient.StartJackServer(JackdPath);
}

bool AJackAudioInterface::EnsureJackServerRunning(const FString& JackdPath)
{
	return JackClient.EnsureJackServerRunning(JackdPath);
}

bool AJackAudioInterface::StartJackServerWithParameters(const FString& JackdPath, int32 InSampleRate, int32 InBufferSize, const FString& InDriver)
{
	return JackClient.StartJackServerWithParameters(JackdPath, InSampleRate, InBufferSize, InDriver);
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
		DebugText += FString::Printf(TEXT("Input Ports: %d\n"), GetNumRegisteredInputPorts());
		DebugText += FString::Printf(TEXT("Output Ports: %d\n"), GetNumRegisteredOutputPorts());
	}

	// Display on screen
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, DebugTextColor, DebugText);
}

void AJackAudioInterface::UpdateDebugDisplay()
{
	// Update debug display every frame if enabled
	DisplayJackInfoOnScreen();
}

void AJackAudioInterface::CheckServerStatus()
{
	// Check if Jack server is still running
	bool bServerRunning = CheckJackServerRunning();
	bool bClientConnected = IsConnectedToJack();
	
	// Track connection state changes for logging
	if (bClientConnected != bWasConnected)
	{
		if (bClientConnected)
		{
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Connected to Jack server"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JackAudioInterface: Disconnected from Jack server"));
		}
		bWasConnected = bClientConnected;
	}
	
	// If server is not running but we think we're connected, force disconnect
	if (!bServerRunning && bClientConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("JackAudioInterface: Jack server stopped running, forcing disconnect"));
		DisconnectFromJack();
		bWasConnected = false;
	}
	
	// Additional check: if we think we're connected but can't get basic info, we're probably disconnected
	if (bClientConnected)
	{
		// Try to get sample rate - if this fails, we're disconnected
		int32 CurrentSampleRate = GetJackSampleRate();
		if (CurrentSampleRate == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("JackAudioInterface: Cannot get sample rate, forcing disconnect"));
			DisconnectFromJack();
			bWasConnected = false;
		}
	}
}

void AJackAudioInterface::OnServerStatusCheck()
{
	CheckServerStatus();
}

void AJackAudioInterface::TestJackConnection()
{
	UE_LOG(LogTemp, Log, TEXT("=== Jack Connection Test ==="));
	
	bool bServerRunning = CheckJackServerRunning();
	UE_LOG(LogTemp, Log, TEXT("Server Running: %s"), bServerRunning ? TEXT("YES") : TEXT("NO"));
	
	bool bClientConnected = IsConnectedToJack();
	UE_LOG(LogTemp, Log, TEXT("Client Connected: %s"), bClientConnected ? TEXT("YES") : TEXT("NO"));
	
	if (bClientConnected)
	{
		FString ClientName = GetJackClientName();
		int32 CurrentSampleRate = GetJackSampleRate();
		int32 CurrentBufferSize = GetJackBufferSize();
		float CPUUsage = GetJackCPUUsage();
		
		UE_LOG(LogTemp, Log, TEXT("Client Name: %s"), *ClientName);
		UE_LOG(LogTemp, Log, TEXT("Sample Rate: %d"), CurrentSampleRate);
		UE_LOG(LogTemp, Log, TEXT("Buffer Size: %d"), CurrentBufferSize);
		UE_LOG(LogTemp, Log, TEXT("CPU Usage: %.2f%%"), CPUUsage);
		
		// Test if we can get ports
		int32 NumInputs = GetNumRegisteredInputPorts();
		int32 NumOutputs = GetNumRegisteredOutputPorts();
		UE_LOG(LogTemp, Log, TEXT("Input Ports: %d, Output Ports: %d"), NumInputs, NumOutputs);
	}
	
	UE_LOG(LogTemp, Log, TEXT("=== End Test ==="));
}

