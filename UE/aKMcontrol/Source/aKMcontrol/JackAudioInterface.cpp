// Fill out your copyright notice in the Description page of Project Settings.

#include "JackAudioInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"

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

		// Always start server monitoring for automatic reconnection attempts
		StartServerMonitoring();

		// Start audio level print timer if enabled
		if (bPrintChannelLevel)
		{
			GetWorld()->GetTimerManager().SetTimer(AudioLevelPrintTimerHandle, this, &AJackAudioInterface::PrintChannelLevel, AudioLevelPrintInterval, true);
		}

		// Start client connection monitoring if enabled
		if (bMonitorNewClientConnections)
		{
			// Initial check
			MonitorNewClients();
			
			// Set up timer for subsequent checks
			GetWorld()->GetTimerManager().SetTimer(ClientMonitorTimerHandle, this, &AJackAudioInterface::MonitorNewClients, ClientMonitorInterval, true);
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

    // Stop audio level timer
    if (AudioLevelPrintTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(AudioLevelPrintTimerHandle);
    }

	// Stop client monitoring timer
	if (ClientMonitorTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ClientMonitorTimerHandle);
	}

	// Disconnect from Jack
	DisconnectFromJack();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AJackAudioInterface::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug display is now handled by ImGui
	// if (bShowDebugInfo)
	// {
	// 	UpdateDebugDisplay();
	// }
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
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Disconnecting from Jack server..."));
	
	// Stop server monitoring first
	StopServerMonitoring();
	
	// Disconnect the Jack client
	JackClient.Disconnect();
	bWasConnected = false;
	
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Disconnected from Jack server"));
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
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Killing Jack server..."));
	
	// Completely disable server monitoring to prevent hanging
	StopServerMonitoring();
	
	// Force disconnect first to prevent hanging
	// This is crucial - we need to close the Jack client before killing the server
	DisconnectFromJack();
	
	// Give the disconnect a moment to complete
	FPlatformProcess::Sleep(0.1f);
	
	bool bResult = JackClient.KillJackServer();
	
	UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Jack server kill result: %s"), bResult ? TEXT("SUCCESS") : TEXT("FAILED"));
	
	// Don't re-enable monitoring automatically - let user manually restart if needed
	// This prevents the system from getting stuck in a hanging loop
	
	return bResult;
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
	// Get current status
	EJackConnectionStatus CurrentStatus = JackClient.GetConnectionStatus();
	bool bClientConnected = (CurrentStatus == EJackConnectionStatus::Connected);
	
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
	
	// If we're not connected, try to reconnect automatically
	if (!bClientConnected && bAutoStartServer)
	{
		// Check if server is running
		bool bServerRunning = CheckJackServerRunning();
		
		if (bServerRunning)
		{
			// Server is running but we're not connected - try to connect
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Server is running, attempting to reconnect..."));
			if (ConnectToJack())
			{
				// Re-register ports after successful reconnection
				if (RegisterAudioPorts(64, 64, TEXT("unreal")))
				{
					UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Successfully re-registered audio ports"));
				}
			}
		}
		else
		{
			// Server is not running - restart it automatically
			UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Server not running, restarting automatically..."));
			if (StartJackServerWithParameters(JackServerPath, SampleRate, BufferSize, AudioDriver))
			{
				UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Server restarted, attempting to connect..."));
				// Give the server a moment to start
				FPlatformProcess::Sleep(2.0f);
				if (ConnectToJack())
				{
					// Register ports after successful connection
					if (RegisterAudioPorts(64, 64, TEXT("unreal")))
					{
						UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Successfully registered audio ports after restart"));
					}
				}
			}
		}
	}
}

void AJackAudioInterface::OnServerStatusCheck()
{
	CheckServerStatus();
}

void AJackAudioInterface::StartServerMonitoring()
{
	if (!bMonitorServerStatus)
	{
		bMonitorServerStatus = true;
		
		// Use a longer interval to reduce the chance of hanging
		float SafeCheckInterval = FMath::Max(ServerCheckInterval, 2.0f);
		GetWorld()->GetTimerManager().SetTimer(ServerStatusTimerHandle, this, &AJackAudioInterface::OnServerStatusCheck, SafeCheckInterval, true);
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Started server monitoring (checking every %.1f seconds)"), SafeCheckInterval);
	}
}

void AJackAudioInterface::StopServerMonitoring()
{
	if (bMonitorServerStatus)
	{
		bMonitorServerStatus = false;
		if (ServerStatusTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(ServerStatusTimerHandle);
		}
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Stopped server monitoring"));
	}
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

void AJackAudioInterface::PrintChannelLevel()
{
    if (IsConnectedToJack())
    {
        float Level = JackClient.GetInputLevel(ChannelToMonitor);

        // Convert to dBFS for readability (add epsilon to avoid log(0))
        const float Epsilon = 1e-9f;
        float dB = 20.0f * FMath::LogX(10.0f, FMath::Max(Level, Epsilon));

        UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Input Channel %d Level -> RMS: %.6f (%.2f dBFS)"), ChannelToMonitor + 1, Level, dB);
    }
}

void AJackAudioInterface::MonitorNewClients()
{
	if (!IsConnectedToJack())
	{
		return;
	}

	// Get the current list of clients from the server
	TArray<FString> CurrentClientArray = JackClient.GetAllClients();
	TSet<FString> CurrentClients(CurrentClientArray);

	// Check for newly connected clients
	TSet<FString> NewClients = CurrentClients.Difference(KnownClients);
	for (const FString& ClientName : NewClients)
	{
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Client connected -> %s"), *ClientName);
	}

	// Check for disconnected clients
	TSet<FString> DisconnectedClients = KnownClients.Difference(CurrentClients);
	for (const FString& ClientName : DisconnectedClients)
	{
		UE_LOG(LogTemp, Log, TEXT("JackAudioInterface: Client disconnected -> %s"), *ClientName);
	}
	
	// Update the set of known clients for the next check
	KnownClients = CurrentClients;
}

