// Fill out your copyright notice in the Description page of Project Settings.


#include "akMSpatServerManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"
#include "Engine/Engine.h"
#include "UEJackAudioLinkSubsystem.h"

DEFINE_LOG_CATEGORY(LogSpatServer);

// Sets default values
AakMSpatServerManager::AakMSpatServerManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize runtime state
	bIsServerRunning = false;
	bServerIsStarting = false;
	ReadPipe = nullptr;
	WritePipe = nullptr;
}

// Called when the game starts or when spawned
void AakMSpatServerManager::BeginPlay()
{
	Super::BeginPlay();

	// Bind to UEJackAudioLink events
	if (GEngine)
	{
		if (UUEJackAudioLinkSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>())
		{
			Subsystem->OnNewJackClientConnected.AddDynamic(this, &AakMSpatServerManager::HandleNewJackClientConnected);
			Subsystem->OnJackClientDisconnected.AddDynamic(this, &AakMSpatServerManager::HandleJackClientDisconnected);
			UnrealJackClientName = Subsystem->GetJackClientName();
		}
	}
}

// Called every frame
void AakMSpatServerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PumpSpatServerOutput();
}

void AakMSpatServerManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind UEJackAudioLink events
	if (GEngine)
	{
		if (UUEJackAudioLinkSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>())
		{
			Subsystem->OnNewJackClientConnected.RemoveDynamic(this, &AakMSpatServerManager::HandleNewJackClientConnected);
			Subsystem->OnJackClientDisconnected.RemoveDynamic(this, &AakMSpatServerManager::HandleJackClientDisconnected);
		}
	}

	// Disconnect all connections involving our Unreal JACK client so next run starts clean
	DisconnectAllConnectionsToUnreal();

	StopSpatServerProcess();
	Super::EndPlay(EndPlayReason);
}
void AakMSpatServerManager::DisconnectAllConnectionsToUnreal()
{
	UUEJackAudioLinkSubsystem* Subsystem = (GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr);
	if (!Subsystem)
	{
		return;
	}
	const FString UnrealClient = Subsystem->GetJackClientName();
	if (UnrealClient.IsEmpty())
	{
		return;
	}
	TArray<FString> UnrealInputs, UnrealOutputs;
	Subsystem->GetClientPorts(UnrealClient, UnrealInputs, UnrealOutputs);

	// To discover connections, iterate all other clients and try disconnect both directions
	TArray<FString> Clients = Subsystem->GetConnectedClients();
	for (const FString& Client : Clients)
	{
		if (Client.Equals(UnrealClient, ESearchCase::IgnoreCase)) { continue; }
		TArray<FString> OtherInputs, OtherOutputs;
		Subsystem->GetClientPorts(Client, OtherInputs, OtherOutputs);
		for (const FString& UnrealOut : UnrealOutputs)
		{
			for (const FString& OtherIn : OtherInputs)
			{
				Subsystem->DisconnectPorts(UnrealOut, OtherIn);
			}
		}
		for (const FString& OtherOut : OtherOutputs)
		{
			for (const FString& UnrealIn : UnrealInputs)
			{
				Subsystem->DisconnectPorts(OtherOut, UnrealIn);
			}
		}
	}

	// Clear our internal mappings
	ConnectedUnrealInputIndicesByClient.Reset();
	ConnectedUnrealInputIndicesFromScsynth.Reset();
}

bool AakMSpatServerManager::StartSpatServer()
{
	if (bIsServerRunning)
	{
		UE_LOG(LogSpatServer, Warning, TEXT("Spat server already running."));
		return false;
	}

	if (bServerIsStarting)
	{
		UE_LOG(LogSpatServer, Warning, TEXT("Spat server is starting."));
		return false;
	}

	// Configure expected paths (Windows)
	SuperColliderInstallDir = TEXT("C:/Program Files/SuperCollider-3.13.0/");
	const FString UserDir = FPaths::ConvertRelativePathToFull(FPlatformProcess::UserDir());
	SpatServerScriptPath = FPaths::Combine(UserDir, TEXT("akm-server"), TEXT("akM_spatServer.scd"));

	if (!ValidateRequiredPaths())
	{
		UE_LOG(LogSpatServer, Error, TEXT("Required files not found. sclang: %s, script: %s"), *SuperColliderInstallDir, *SpatServerScriptPath);
		return false;
	}

	return StartSpatServerProcess();
}

bool AakMSpatServerManager::ValidateRequiredPaths() const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const bool bExeExists = PlatformFile.DirectoryExists(*SuperColliderInstallDir);
	const bool bScriptExists = PlatformFile.FileExists(*SpatServerScriptPath);

	if (!bExeExists)
	{
		UE_LOG(LogSpatServer, Error, TEXT("SuperCollider install directory not found at: %s"), *SuperColliderInstallDir);
	}

	if (!bScriptExists)
	{
		UE_LOG(LogSpatServer, Error, TEXT("akM_spatServer.scd not found at: %s"), *SpatServerScriptPath);
	}

	return bExeExists && bScriptExists;
}

bool AakMSpatServerManager::StartSpatServerProcess()
{
	if (bIsServerRunning)
	{
		return false;
	}

	if (bServerIsStarting)
	{
		return false;
	}

	bServerIsStarting = true;

	// Create pipes for capturing stdout
	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	UE_LOG(LogSpatServer, Log, TEXT("Starting spat server. SC Dir: %s Script: %s"), *SuperColliderInstallDir, *SpatServerScriptPath);

	const FString ScDir = SuperColliderInstallDir;
	const FString ScExe = FPaths::Combine(ScDir, TEXT("sclang.exe"));
	const FString Script = SpatServerScriptPath;
	const FString Args = FString::Printf(TEXT("\"%s\""), *Script);

	// Spawn process
	SpatServerProcessHandle = FPlatformProcess::CreateProc(
	*ScExe, *Args,
		true,
		false,
		false,
		nullptr,
		0,
		*SuperColliderInstallDir,
		WritePipe,
		ReadPipe
	);

	if (!SpatServerProcessHandle.IsValid())
	{
		UE_LOG(LogSpatServer, Error, TEXT("Failed to start sclang process."));
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		ReadPipe = nullptr;
		WritePipe = nullptr;
		bServerIsStarting = false;
		return false;
	}

	bIsServerRunning = true;
	return true;
}

void AakMSpatServerManager::StopSpatServerProcess()
{
	if (!bIsServerRunning)
	{
		return;
	}

	UE_LOG(LogSpatServer, Log, TEXT("Stopping spat server."));

	// Try to terminate gracefully
	if (SpatServerProcessHandle.IsValid())
	{
		FPlatformProcess::TerminateProc(SpatServerProcessHandle, true);
		FPlatformProcess::CloseProc(SpatServerProcessHandle);
	}
	SpatServerProcessHandle.Reset();

	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
	ReadPipe = nullptr;
	WritePipe = nullptr;

	bIsServerRunning = false;
}

void AakMSpatServerManager::PumpSpatServerOutput()
{
	if (!bIsServerRunning || ReadPipe == nullptr)
	{
		return;
	}

	FString Output = FPlatformProcess::ReadPipe(ReadPipe);
	if (!Output.IsEmpty())
	{
		TArray<FString> Lines;
		Output.ParseIntoArray(Lines, TEXT("\n"), false);
		for (const FString& Line : Lines)
		{
			if (!Line.IsEmpty())
			{
				//UE_LOG(LogSpatServer, Log, TEXT("sclang: %s"), *Line);

				// Write to ImGui buffer
				ImGuiConsoleBuffer.Add(Line);

				// Limit size
				if (ImGuiConsoleBuffer.Num() > MaxConsoleLines)
				{
					ImGuiConsoleBuffer.RemoveAt(0, ImGuiConsoleBuffer.Num() - MaxConsoleLines);
				}
			}
		}
	}

	// Detect if process has exited
	if (SpatServerProcessHandle.IsValid() && !FPlatformProcess::IsProcRunning(SpatServerProcessHandle))
	{
		UE_LOG(LogSpatServer, Warning, TEXT("sclang process exited."));
		ImGuiConsoleBuffer.Add(TEXT("sclang process exited."));
		StopSpatServerProcess();
	}
}

void AakMSpatServerManager::HandleNewJackClientConnected(const FString& ClientName, int32 NumInputPorts, int32 NumOutputPorts)
{
	// Only act when scsynth connects and we haven't connected yet
	if (bAKMserverAudioOutputPortsConnected)
	{
		return;
	}

	if (!ClientName.Equals(TEXT("scsynth"), ESearchCase::IgnoreCase))
	{
		// Ignore our own Unreal JACK client
		if (!UnrealJackClientName.IsEmpty() && ClientName.Equals(UnrealJackClientName, ESearchCase::IgnoreCase))
		{
			return;
		}
		// UI handled in AImGuiActor
		return;
	}

	UUEJackAudioLinkSubsystem* Subsystem = (GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr);
	if (!Subsystem)
	{
		UE_LOG(LogSpatServer, Error, TEXT("UEJackAudioLinkSubsystem not available; cannot auto-connect scsynth."));
		return;
	}

	// Refresh our Unreal client name and input list
	UnrealJackClientName = Subsystem->GetJackClientName();
	if (UnrealJackClientName.IsEmpty())
	{
		UE_LOG(LogSpatServer, Warning, TEXT("Unreal JACK client name is empty; is the UE JACK client connected?"));
		return;
	}

	// Proactively disconnect scsynth <-> system default connections (both directions)
	{
		TArray<FString> ScIn, ScOut;
		Subsystem->GetClientPorts(TEXT("scsynth"), ScIn, ScOut);
		TArray<FString> SysIn, SysOut;
		Subsystem->GetClientPorts(TEXT("system"), SysIn, SysOut);
		int32 DisconnectAttempts = 0;
		for (const FString& ScOutPort : ScOut)
		{
			for (const FString& SysInPort : SysIn)
			{
				if (!ScOutPort.IsEmpty() && !SysInPort.IsEmpty())
				{
					Subsystem->DisconnectPorts(ScOutPort, SysInPort);
					++DisconnectAttempts;
				}
			}
		}
		for (const FString& SysOutPort : SysOut)
		{
			for (const FString& ScInPort : ScIn)
			{
				if (!SysOutPort.IsEmpty() && !ScInPort.IsEmpty())
				{
					Subsystem->DisconnectPorts(SysOutPort, ScInPort);
					++DisconnectAttempts;
				}
			}
		}
		if (DisconnectAttempts > 0)
		{
			UE_LOG(LogSpatServer, Log, TEXT("Attempted to disconnect %d scsynth<->system port pairs."), DisconnectAttempts);
		}
	}

	// Compute available input indices on our Unreal client (1-based)
	TArray<int32> AvailableInputIndices = GetAvailableUnrealInputPortIndices();
	const int32 NumAvailable = AvailableInputIndices.Num();

	if (NumAvailable < NumOutputPorts)
	{
		UE_LOG(LogSpatServer, Error, TEXT("Not enough available Unreal JACK input ports (%d) to connect scsynth outputs (%d). Increase input port count in UEJackAudioLink plugin settings."), NumAvailable, NumOutputPorts);
		return;
	}

	// Connect scsynth outputs [1..NumOutputPorts] to next available Unreal inputs ascending
	bool bAllConnected = true;
	ConnectedUnrealInputIndicesFromScsynth.Reset();
	for (int32 OutIndex1Based = 1; OutIndex1Based <= NumOutputPorts; ++OutIndex1Based)
	{
		const int32 DestInputIndex1Based = AvailableInputIndices[OutIndex1Based - 1];
		const bool bOK = Subsystem->ConnectPortsByIndex(EJackPortDirection::Output, TEXT("scsynth"), OutIndex1Based,
			EJackPortDirection::Input, UnrealJackClientName, DestInputIndex1Based);
		if (!bOK)
		{
			UE_LOG(LogSpatServer, Error, TEXT("Failed to connect scsynth output #%d to Unreal input #%d."), OutIndex1Based, DestInputIndex1Based);
			bAllConnected = false;
		}
		else
		{
			ConnectedUnrealInputIndicesFromScsynth.Add(DestInputIndex1Based);
		}
	}

	bAKMserverAudioOutputPortsConnected = bAllConnected && ConnectedUnrealInputIndicesFromScsynth.Num() == NumOutputPorts;
	if (bAKMserverAudioOutputPortsConnected)
	{
		UE_LOG(LogSpatServer, Log, TEXT("Connected %d scsynth outputs to Unreal JACK inputs."), NumOutputPorts);
	}
}

void AakMSpatServerManager::HandleJackClientDisconnected(const FString& ClientName)
{
	if (ClientName.Equals(TEXT("scsynth"), ESearchCase::IgnoreCase))
	{
		ConnectedUnrealInputIndicesFromScsynth.Reset();
		bAKMserverAudioOutputPortsConnected = false;
		UE_LOG(LogSpatServer, Log, TEXT("scsynth disconnected; cleared port mapping state."));
	}
	else
	{
		if (ConnectedUnrealInputIndicesByClient.Remove(ClientName) > 0)
		{
			UE_LOG(LogSpatServer, Log, TEXT("Client '%s' disconnected; cleared its Unreal input mapping."), *ClientName);
		}
	}
}

TArray<int32> AakMSpatServerManager::GetAvailableUnrealInputPortIndices() const
{
	TArray<int32> Result;
	UUEJackAudioLinkSubsystem* Subsystem = (GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr);
	if (!Subsystem)
	{
		return Result;
	}
	TArray<FString> UnrealInputs, UnrealOutputs;
	Subsystem->GetClientPorts(UnrealJackClientName, UnrealInputs, UnrealOutputs);
	const int32 NumInputs = UnrealInputs.Num();
	if (NumInputs <= 0)
	{
		return Result;
	}
	// Build set of reserved/used indices from all known client mappings (1-based)
	TSet<int32> Used;
	for (const auto& Pair : ConnectedUnrealInputIndicesByClient)
	{
		for (int32 Index1Based : Pair.Value)
		{
			Used.Add(Index1Based);
		}
	}
	for (int32 Index1Based : ConnectedUnrealInputIndicesFromScsynth)
	{
		Used.Add(Index1Based);
	}
	for (int32 Index1Based = 1; Index1Based <= NumInputs; ++Index1Based)
	{
		if (!Used.Contains(Index1Based))
		{
			Result.Add(Index1Based);
		}
	}
	return Result;
}

void AakMSpatServerManager::AcceptExternalClient(const FString& ClientName, int32 NumInputPorts, int32 NumOutputPorts)
{
	UUEJackAudioLinkSubsystem* Subsystem = (GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr);
	if (!Subsystem)
	{
		UE_LOG(LogSpatServer, Error, TEXT("UEJackAudioLinkSubsystem not available; cannot accept client '%s'."), *ClientName);
		return;
	}

	// Ensure scsynth inputs count
	TArray<FString> ScIn, ScOut;
	Subsystem->GetClientPorts(TEXT("scsynth"), ScIn, ScOut);
	const int32 NumScInputs = ScIn.Num();
	int32 NumToConnect = FMath::Min(NumScInputs, NumOutputPorts);
	if (NumOutputPorts > NumScInputs)
	{
		UE_LOG(LogSpatServer, Warning, TEXT("Client '%s' has %d outputs > scsynth inputs %d. Will connect first %d channels."), *ClientName, NumOutputPorts, NumScInputs, NumToConnect);
	}

	// Build mapping of existing connections from this client's outputs to our Unreal inputs (1-based indices)
	TMap<int32, int32> AlreadyConnectedMap;
	TArray<FString> UnrealInputs, UnrealOutputs;
	Subsystem->GetClientPorts(UnrealJackClientName, UnrealInputs, UnrealOutputs);
	TArray<FString> ClientInputs, ClientOutputs;
	Subsystem->GetClientPorts(ClientName, ClientInputs, ClientOutputs);
	for (int32 ch = 1; ch <= NumToConnect && ch <= ClientOutputs.Num(); ++ch)
	{
		const FString& ClientOutName = ClientOutputs[ch - 1];
		for (int32 idx = 0; idx < UnrealInputs.Num(); ++idx)
		{
			const FString& UnrealInName = UnrealInputs[idx];
			// Probe: attempt a disconnect to see if connection exists; if so, reconnect and record mapping
			const bool bWasConnected = Subsystem->DisconnectPorts(ClientOutName, UnrealInName);
			if (bWasConnected)
			{
				Subsystem->ConnectPorts(ClientOutName, UnrealInName);
				AlreadyConnectedMap.Add(ch, idx + 1);
				break;
			}
		}
	}

	const int32 NumExisting = AlreadyConnectedMap.Num();
	const int32 NumNeeded = FMath::Max(0, NumToConnect - NumExisting);

	// Compute available Unreal input indices, excluding ones already used by existing connections for this client
	TArray<int32> AvailableInputIndices = GetAvailableUnrealInputPortIndices();
	if (NumNeeded > 0)
	{
		TSet<int32> UsedExisting;
		for (const auto& Pair : AlreadyConnectedMap)
		{
			UsedExisting.Add(Pair.Value);
		}
		AvailableInputIndices.RemoveAll([&UsedExisting](int32 Idx){ return UsedExisting.Contains(Idx); });
		if (AvailableInputIndices.Num() < NumNeeded)
		{
			UE_LOG(LogSpatServer, Error, TEXT("Not enough available Unreal JACK input ports (%d) for %d new connections from '%s' (reusing %d existing). Increase input ports in UEJackAudioLink settings."), AvailableInputIndices.Num(), NumNeeded, *ClientName, NumExisting);
			return;
		}
	}

	TArray<int32>& UnrealIndicesForClient = ConnectedUnrealInputIndicesByClient.FindOrAdd(ClientName);
	int32 NextAvailIdx = 0;
	for (int32 ch = 1; ch <= NumToConnect; ++ch)
	{
		int32 UnrealIndex = 0;
		if (const int32* FoundIdx = AlreadyConnectedMap.Find(ch))
		{
			UnrealIndex = *FoundIdx;
		}
		else
		{
			UnrealIndex = AvailableInputIndices[NextAvailIdx++];
		}

		const bool b1 = Subsystem->ConnectPortsByIndex(EJackPortDirection::Output, ClientName, ch, EJackPortDirection::Input, TEXT("scsynth"), ch);
		bool b2 = true;
		if (!AlreadyConnectedMap.Contains(ch))
		{
			b2 = Subsystem->ConnectPortsByIndex(EJackPortDirection::Output, ClientName, ch, EJackPortDirection::Input, UnrealJackClientName, UnrealIndex);
		}
		if (b1 && b2)
		{
			UnrealIndicesForClient.Add(UnrealIndex);
		}
		else
		{
			UE_LOG(LogSpatServer, Error, TEXT("Failed to connect '%s' output #%d."), *ClientName, ch);
		}
	}

	UE_LOG(LogSpatServer, Log, TEXT("Accepted client '%s' and connected %d channels to scsynth and Unreal inputs."), *ClientName, NumToConnect);
}

void AakMSpatServerManager::TestValueChangeCallback(float value)
{
	UE_LOG(LogSpatServer, Log, TEXT("TestValueChangeCallback: %f"), value);
}

void AakMSpatServerManager::SendOSCFloat(const FString& OSCAddress, float Value)
{
	OnRequestedOSCSend_Float.Broadcast(OSCAddress, Value);
}



