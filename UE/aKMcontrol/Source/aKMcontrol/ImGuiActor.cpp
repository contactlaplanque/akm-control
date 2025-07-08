// Fill out your copyright notice in the Description page of Project Settings.


#include "ImGuiActor.h"
#include "SourceActor.h"
#include "Kismet/GameplayStatics.h"
#include "ImGuiModule.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

// Sets default values
AImGuiActor::AImGuiActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void AImGuiActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Find the JackAudioInterface actor in the world once
	JackInterface = FindJackAudioInterface();
	
	// Ensure ImGui input is disabled by default so camera controls work
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	
	// Scale ImGui style by 2x (this should be done once)
	ImGui::GetStyle().ScaleAllSizes(2.0f);
}

// Called every frame
void AImGuiActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

	// --- Update Level Meter State ---
	if (JackInterface)
	{
		// Get the raw RMS level for channel 0 using our public getter
		const float RawRms = JackInterface->GetInputChannelLevel(0);

		// Apply smoothing (exponential moving average)
		const float SmoothingFactor = 0.6f;
		SmoothedRmsLevel = (RawRms * (1.0f - SmoothingFactor)) + (SmoothedRmsLevel * SmoothingFactor);

		// Update peak level
		if (SmoothedRmsLevel > PeakLevel)
		{
			PeakLevel = SmoothedRmsLevel;
			TimeOfLastPeak = GetWorld()->GetTimeSeconds();
		}

		// Let peak level fall off after a short time
		const float PeakHoldTime = 1.5f;
		if (GetWorld()->GetTimeSeconds() - TimeOfLastPeak > PeakHoldTime)
		{
			// Decay peak level smoothly
			PeakLevel = FMath::Max(0.0f, PeakLevel - (DeltaTime * 0.5f));
		}
	}
	// --- End Update ---
	
    // Scale ImGui font by 2x (apply every frame to ensure it takes effect)
    ImGui::GetIO().FontGlobalScale = 2.0f;

    ImGui::Begin("Main Window");

    // Show ImGui input capture status
    bool bImGuiInputEnabled = FImGuiModule::Get().GetProperties().IsInputEnabled();
    ImGui::TextColored(bImGuiInputEnabled ? ImVec4(1,1,0,1) : ImVec4(0.5f,0.5f,0.5f,1),
        bImGuiInputEnabled ? "ImGui Input: ENABLED" : "ImGui Input: DISABLED");
    ImGui::Separator();

    // FPS Monitor
    static float FPS = 0.0f;
    static float FrameTime = 0.0f;
    static float FPSUpdateTimer = 0.0f;
    
    FPSUpdateTimer += DeltaTime;
    if (FPSUpdateTimer >= 0.5f) // Update every 0.5 seconds
    {
        FPS = 1.0f / DeltaTime;
        FrameTime = DeltaTime * 1000.0f; // Convert to milliseconds
        FPSUpdateTimer = 0.0f;
    }
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "FPS Monitor");
    ImGui::Text("FPS: %.1f", FPS);
    ImGui::Text("Frame Time: %.2f ms", FrameTime);
    
    ImGui::Separator();

    // Render Jack Audio Interface section
    RenderJackAudioSection();
    
    ImGui::Separator();
    
    // Render existing Actor Picking section
    RenderActorPickingSection();

    ImGui::End();

	// Render the level meter in its own window if the interface is active
	if (JackInterface && JackInterface->IsConnectedToJack())
	{
		// Set a default position and size for the meter window so it doesn't overlap the main one
		ImGui::SetNextWindowPos(ImVec2(600, 50), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(100, 300), ImGuiCond_FirstUseEver);

		ImGui::Begin("Input Level Meter");
		RenderLevelMeter();
		ImGui::End();
	}
}

void AImGuiActor::RenderJackAudioSection()
{
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Jack Audio Interface");
    
    if (!JackInterface)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No JackAudioInterface found in scene");
        return;
    }
    
    // Connection Status
    EJackConnectionStatus Status = JackInterface->GetJackConnectionStatus();
    ImVec4 StatusColor;
    const char* StatusText;
    
    switch (Status)
    {
    case EJackConnectionStatus::Connected:
        StatusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        StatusText = "CONNECTED";
        break;
    case EJackConnectionStatus::Connecting:
        StatusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        StatusText = "CONNECTING";
        break;
    case EJackConnectionStatus::Disconnected:
        StatusColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
        StatusText = "DISCONNECTED";
        break;
    case EJackConnectionStatus::Failed:
        StatusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        StatusText = "FAILED";
        break;
    default:
        StatusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        StatusText = "UNKNOWN";
        break;
    }
    
    ImGui::Text("Status: ");
    ImGui::SameLine();
    ImGui::TextColored(StatusColor, "%s", StatusText);
    
    if (JackInterface->IsConnectedToJack())
    {
        // Client Information - get values safely
        FString ClientName = JackInterface->GetJackClientName();
        int32 SampleRate = JackInterface->GetJackSampleRate();
        int32 BufferSize = JackInterface->GetJackBufferSize();
        float CPUUsage = JackInterface->GetJackCPUUsage();
        int32 NumInputs = JackInterface->GetNumRegisteredInputPorts();
        int32 NumOutputs = JackInterface->GetNumRegisteredOutputPorts();
        
        // Only display if we got valid values (prevents displaying zeros when server is dead)
        if (!ClientName.IsEmpty() && SampleRate > 0 && BufferSize > 0)
        {
            ImGui::Text("Client: %s", TCHAR_TO_UTF8(*ClientName));
            ImGui::Text("Sample Rate: %d Hz", SampleRate);
            ImGui::Text("Buffer Size: %d frames", BufferSize);
            ImGui::Text("CPU Usage: %.2f%%", CPUUsage);
            ImGui::Text("Ports: %d in, %d out", NumInputs, NumOutputs);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Connection appears to be dead");
        }
        
        // Connection info displayed above
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Not connected to Jack server");
    }

	// Render the new level meter - MOVED to Tick() in its own window
	// if (JackInterface->IsConnectedToJack())
	// {
	// 	ImGui::Separator();
	// 	RenderLevelMeter();
	// }
}

void AImGuiActor::RenderLevelMeter()
{
	// --- Constants ---
	const float MinDB = -60.0f;
	const float MaxDB = 6.0f;

	// --- Calculations ---
	const float Epsilon = 1e-9f;
	const float RmsDB = 20.0f * FMath::LogX(10.0f, FMath::Max(SmoothedRmsLevel, Epsilon));
	const float PeakDB = 20.0f * FMath::LogX(10.0f, FMath::Max(PeakLevel, Epsilon));
	const float NormalizedRms = (FMath::Clamp(RmsDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);
	const float NormalizedPeak = (FMath::Clamp(PeakDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);
	
	// --- Drawing ---
	const ImVec2 MeterSize(20.0f, 200.0f);
	// ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - MeterSize.x) * 0.5f); // No longer centering the meter
	const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	const ImVec2 BarStart = ImVec2(CursorPos.x, CursorPos.y + MeterSize.y);

	// 1. Draw Background
	DrawList->AddRectFilled(CursorPos, ImVec2(CursorPos.x + MeterSize.x, CursorPos.y + MeterSize.y), IM_COL32(30, 30, 30, 255), 4.0f);

	// 2. Draw RMS Level Bar (from bottom to top)
	const float RmsBarHeight = MeterSize.y * NormalizedRms;
	if (RmsBarHeight > 0)
	{
		const ImU32 GreenColor = IM_COL32(0, 200, 0, 255);
		const ImU32 YellowColor = IM_COL32(255, 255, 0, 255);
		const ImU32 RedColor = IM_COL32(255, 0, 0, 255);
		const float YellowThreshold = 0.75f;
		const float RedThreshold = 0.90f;

		float GreenHeight = FMath::Min(RmsBarHeight, MeterSize.y * YellowThreshold);
		DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - GreenHeight), ImVec2(BarStart.x + MeterSize.x, BarStart.y), GreenColor, 4.0f, ImDrawFlags_RoundCornersBottom);
		if (NormalizedRms > YellowThreshold) {
			float YellowHeight = FMath::Min(RmsBarHeight, MeterSize.y * RedThreshold) - GreenHeight;
			DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - GreenHeight - YellowHeight), ImVec2(BarStart.x + MeterSize.x, BarStart.y - GreenHeight), YellowColor);
		}
		if (NormalizedRms > RedThreshold) {
			float RedHeight = RmsBarHeight - (MeterSize.y * RedThreshold);
			DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - (MeterSize.y * RedThreshold) - RedHeight), ImVec2(BarStart.x + MeterSize.x, BarStart.y - (MeterSize.y * RedThreshold)), RedColor);
		}
	}

	// 3. Draw Peak-Hold Indicator
	if (NormalizedPeak > 0) {
		const float PeakLineY = BarStart.y - (MeterSize.y * NormalizedPeak);
		DrawList->AddLine(ImVec2(CursorPos.x, PeakLineY), ImVec2(CursorPos.x + MeterSize.x, PeakLineY), IM_COL32(255, 255, 255, 180), 2.0f);
	}
	
	// 4. Draw dBFS value text - REMOVED

	// Advance cursor past our custom widget
	ImGui::Dummy(MeterSize);
	
	// 5. Draw Channel Number centered below the meter
	const FString ChannelLabel = FString::Printf(TEXT("%d"), 1);
	const ImVec2 ChannelTextSize = ImGui::CalcTextSize(TCHAR_TO_UTF8(*ChannelLabel));
	
	// Get the starting X position and add half the difference between meter width and text width
	const float StartX = ImGui::GetCursorPosX();
	ImGui::SetCursorPosX(StartX + (MeterSize.x - ChannelTextSize.x) * 0.5f);
	ImGui::Text("%s", TCHAR_TO_UTF8(*ChannelLabel));
}

void AImGuiActor::RenderActorPickingSection()
{
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Actor Picking");

    if (ImGui::Button("Toggle ImGui Input"))
    {
        ToggleImGuiInput();
    }

    // Actor Picking
    const char* ButtonTitle = bIsPickingActor ? "Stop Picking" : "Start Picking";
    if (ImGui::Button(ButtonTitle))
    {
        bIsPickingActor = !bIsPickingActor;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
    ULocalPlayer* LP = World ? World->GetFirstLocalPlayerFromController() : nullptr;
    if (bIsPickingActor)
    {
        if (LP && LP->ViewportClient)
        {
            // Get the projection data from world to viewport
            FSceneViewProjectionData ProjectionData;
            if (LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData))
            {
                ImVec2 ScreenPosImGui = ImGui::GetMousePos();
                FVector2D ScreenPos = { ScreenPosImGui.x, ScreenPosImGui.y };

                FMatrix const InvViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix().InverseFast();

                FVector WorldPosition, WorldDirection;
                FSceneView::DeprojectScreenToWorld(ScreenPos, ProjectionData.GetConstrainedViewRect(), InvViewProjectionMatrix, WorldPosition, WorldDirection);

                FCollisionQueryParams Params("DevGuiActorPickerTrace", SCENE_QUERY_STAT_ONLY(UDevGuiSubsystem), true);

                FCollisionObjectQueryParams ObjectParams(
                    ECC_TO_BITFIELD(ECC_WorldStatic)
                    | ECC_TO_BITFIELD(ECC_WorldDynamic)
                    | ECC_TO_BITFIELD(ECC_Pawn)
                    | ECC_TO_BITFIELD(ECC_PhysicsBody)
                );

                FHitResult OutHit;
                if (World->LineTraceSingleByObjectType(
                    OutHit,
                    WorldPosition + WorldDirection * 100.0,
                    WorldPosition + WorldDirection * 10000.0,
                    ObjectParams,
                    Params))
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
                    {
                        PickedActor = OutHit.GetActor()->GetClass()->GetName() == "BP_Source_C" ? OutHit.GetActor() : nullptr;
                        bIsPickingActor = false;
                    }
                }
                else
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse)
                    {
                        PickedActor = nullptr;
                        bIsPickingActor = false;
                    }
                }
            }
        }

        // Exiting Actor Picking mode if we click the right button !
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            bIsPickingActor = false;
        }
    }

    if (AActor* Actor = PickedActor.Get())
    {
        if (ASourceActor* SourceActor = Cast<ASourceActor>(Actor))
        {
            ImGui::Text("Picked source:");
            ImGui::SameLine(); 
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), " %ls", *SourceActor->GetName());

            const FVector actorLocation = SourceActor->GetActorLocation();

            location[0] = actorLocation.X;
            location[1] = actorLocation.Y;
            location[2] = actorLocation.Z;

            ImGui::InputFloat3("Location", location);
        }
    }

    // Update actor location if changed
    if (AActor* Actor = PickedActor.Get())
    {
        if (ASourceActor* SourceActor = Cast<ASourceActor>(Actor))
        {
            if (location[0] != locationPrevious[0] || location[1] != locationPrevious[1] || location[2] != locationPrevious[2])
            {
                locationPrevious[0] = location[0];
                locationPrevious[1] = location[1];
                locationPrevious[2] = location[2];

                SourceActor->SetActorLocation(FVector(location[0], location[1], location[2]));
            }
        }
    }
}

void AImGuiActor::ToggleImGuiInput()
{
	FImGuiModule::Get().GetProperties().ToggleInput();
}

AJackAudioInterface* AImGuiActor::FindJackAudioInterface()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    // Find the first JackAudioInterface in the scene
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AJackAudioInterface::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        return Cast<AJackAudioInterface>(FoundActors[0]);
    }
    
    return nullptr;
}

