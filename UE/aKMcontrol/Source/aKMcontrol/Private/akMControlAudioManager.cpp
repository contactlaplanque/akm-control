// Fill out your copyright notice in the Description page of Project Settings.


#include "akMControlAudioManager.h"

#include "Misc/LowLevelTestAdapter.h"

DEFINE_LOG_CATEGORY(LogAkMControl);

// Sets default values
AakMControlAudioManager::AakMControlAudioManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AakMControlAudioManager::BeginPlay()
{
	Super::BeginPlay();

	JackAudioLinkSubsystem = GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr;
	
}

// Called every frame
void AakMControlAudioManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- Update Level Meter State ---
	if (JackAudioLinkSubsystem != nullptr)
	{
		const FString UEJackClientName = JackAudioLinkSubsystem->GetJackClientName();
		TArray<FString>	InputPorts;
		TArray<FString>	OutputPorts;
		JackAudioLinkSubsystem->GetClientPorts(UEJackClientName,InputPorts,OutputPorts);
		const int32 NumChannels = InputPorts.Num();

		// Ensure our state arrays are the correct size
		if (SmoothedRmsLevels.Num() != NumChannels)
		{
			SmoothedRmsLevels.Init(0.0f, NumChannels);
			PeakLevels.Init(0.0f, NumChannels);
			TimesOfLastPeak.Init(0.0f, NumChannels);
		}
		
		for (int32 i = 0; i < NumChannels; ++i)
		{
			// Get the raw RMS level for the current channel
			const float RawRms = JackAudioLinkSubsystem->GetInputLevel(i);

			// Apply smoothing (exponential moving average)
			const float SmoothingFactor = 0.6f;
			SmoothedRmsLevels[i] = (RawRms * (1.0f - SmoothingFactor)) + (SmoothedRmsLevels[i] * SmoothingFactor);

			// Update peak level
			if (SmoothedRmsLevels[i] > PeakLevels[i])
			{
				PeakLevels[i] = SmoothedRmsLevels[i];
				TimesOfLastPeak[i] = GetWorld()->GetTimeSeconds();
			}

			// Let peak level fall off after a short time
			const float PeakHoldTime = 1.5f;
			if (GetWorld()->GetTimeSeconds() - TimesOfLastPeak[i] > PeakHoldTime)
			{
				// Decay peak level smoothly
				PeakLevels[i] = FMath::Max(0.0f, PeakLevels[i] - (DeltaTime * 0.5f));
			}
		}
	}
	// --- End Update ---

}

