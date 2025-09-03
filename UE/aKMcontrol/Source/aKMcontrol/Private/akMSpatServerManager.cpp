// Fill out your copyright notice in the Description page of Project Settings.


#include "akMSpatServerManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY(LogSpatServer);

// Sets default values
AakMSpatServerManager::AakMSpatServerManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize runtime state
	bIsServerRunning = false;
	ReadPipe = nullptr;
	WritePipe = nullptr;
}

// Called when the game starts or when spawned
void AakMSpatServerManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AakMSpatServerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PumpSpatServerOutput();
}

void AakMSpatServerManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpatServerProcess();
	Super::EndPlay(EndPlayReason);
}

bool AakMSpatServerManager::StartSpatServer()
{
	if (bIsServerRunning)
	{
		UE_LOG(LogSpatServer, Warning, TEXT("Spat server already running."));
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

