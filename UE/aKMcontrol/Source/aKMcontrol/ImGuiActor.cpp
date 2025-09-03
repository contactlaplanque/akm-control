// ImGuiActor.cpp

#include "ImGuiActor.h"
#include "Kismet/GameplayStatics.h"
#include "ImGuiModule.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

AImGuiActor::AImGuiActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AImGuiActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure ImGui input is disabled by default so camera controls work
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	
	// Scale ImGui style by 2x (this should be done once)
	ImGui::GetStyle().ScaleAllSizes(2.0f);
	
}

void AImGuiActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Scale ImGui font by 2x (apply every frame to ensure it takes effect)
    ImGui::GetIO().FontGlobalScale = 2.0f;

	ImGui::SetNextWindowSize(ImVec2(400, 250));
	ImGui::SetNextWindowPos(ImVec2(25, 25));
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
    
    ImGui::End();

	// akM Server Window
	RenderAkMServerWindow();

	// Audio Monitoring Window
	RenderAudioMonitoringWindow();
	
}


void AImGuiActor::ToggleImGuiInput()
{
	FImGuiModule::Get().GetProperties().ToggleInput();
}

void AImGuiActor::RenderAkMServerWindow() const
{
	if (SpatServerManager != nullptr)
	{
		ImGui::SetNextWindowSize(ImVec2(1000, 500));
		ImGui::SetNextWindowPos(ImVec2(25, 300));
		ImGui::Begin("akM Server");

		bool bServerAlive = SpatServerManager->bIsServerAlive;
		ImVec4 ServerStatusColor = bServerAlive ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1);
		ImGui::TextColored(ServerStatusColor, bServerAlive ? "ONLINE" : "OFFLINE");

		if (bServerAlive) 
		{
			if (ImGui::Button("STOP"))
			{
				UFunction* StopServerBlueprintFunction = SpatServerManager->FindFunction(TEXT("StopSpatServer"));
				if (StopServerBlueprintFunction != nullptr)
				{
					
					SpatServerManager->ProcessEvent(StopServerBlueprintFunction, nullptr);
					SpatServerManager->StopSpatServerProcess();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("StopServerBlueprintFunction not found."));
				}
			}
		}
		else
		{
			if (ImGui::Button("START")){
			
				SpatServerManager->StartSpatServer();
			}
		}

		// Scrollable child region
		ImGui::BeginChild("ConsoleRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (const FString& Line : SpatServerManager->ImGuiConsoleBuffer)
		{
			ImGui::TextUnformatted(TCHAR_TO_ANSI(*Line));
		}

		// Auto-scroll to bottom
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		

		ImGui::EndChild();
		
		ImGui::End();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("SpatServerManager not found, akM Server window not rendered"));
	}
}

void AImGuiActor::RenderAudioMonitoringWindow() const
{
	if (AudioManager != nullptr) {
		
		ImGui::SetNextWindowSize(ImVec2(800, 650));
		ImGui::SetNextWindowPos(ImVec2(1050, 300));
		ImGui::Begin("Audio Levels");
		
		if (AudioManager->SmoothedRmsLevels.IsEmpty())
		{
			ImGui::Text("No input channels to display.");
			return;
		}

		// --- Adaptive Layout ---
		const int32 ChannelsPerGroup = 8;
		const float MeterWidth = 12.0f;
		const float MeterSpacing = 4.0f; // Spacing between individual meters for clarity
		const float MeterHeight = 240.0f; // Increased height for better scale readability
		const float ScaleWidth = 30.0f;
		const float GroupSpacing = 20.0f; // Increased spacing between groups
		// Account for the spacing between meters in the total group width
		const float GroupWidth = ScaleWidth + (MeterWidth * ChannelsPerGroup) + (MeterSpacing * (ChannelsPerGroup - 1)) + GroupSpacing;

		int numColumns = FMath::FloorToInt(ImGui::GetContentRegionAvail().x / GroupWidth);
		numColumns = FMath::Max(1, numColumns);

		for (int32 groupIdx = 0; groupIdx * ChannelsPerGroup < AudioManager->SmoothedRmsLevels.Num(); ++groupIdx)
		{
			ImGui::BeginGroup(); // Group the scale, meters, and label together

			// --- Get base positions and draw list ---
			const ImVec2 GroupCursorStart = ImGui::GetCursorScreenPos();
			ImDrawList* DrawList = ImGui::GetWindowDrawList();

			// --- 1. Draw the dB Scale on the left of the group ---
			{
				const ImVec2 ScalePos = GroupCursorStart;
				const float MinDB = -60.0f;
				const float TickLevels[] = {6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f};

				for (float TickDB : TickLevels)
				{
					float NormalizedTick = (FMath::Clamp(TickDB, -60.f, 6.f) - MinDB) / (6.f - MinDB);
					float TickY = ScalePos.y + MeterHeight - (MeterHeight * NormalizedTick);
					
					// Draw tick mark
					DrawList->AddLine(ImVec2(ScalePos.x + ScaleWidth - 5, TickY), ImVec2(ScalePos.x + ScaleWidth, TickY), IM_COL32(180, 180, 180, 255));

					// Draw text
					FString TickLabel = FString::Printf(TEXT("%.0f"), TickDB);
					ImVec2 TextSize = ImGui::CalcTextSize(TCHAR_TO_UTF8(*TickLabel));
					DrawList->AddText(ImVec2(ScalePos.x + ScaleWidth - 7 - TextSize.x, TickY - TextSize.y / 2), IM_COL32(180, 180, 180, 255), TCHAR_TO_UTF8(*TickLabel));
				}
			}

			// --- 2. Draw the block of 8 meters ---
			ImGui::SetCursorScreenPos(ImVec2(GroupCursorStart.x + ScaleWidth, GroupCursorStart.y)); // Position cursor for meters
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(MeterSpacing, 0.0f));
			for (int32 i = 0; i < ChannelsPerGroup; ++i)
			{
				int32 channelIdx = groupIdx * ChannelsPerGroup + i;
				if (channelIdx >= AudioManager->SmoothedRmsLevels.Num()) break;

				const float MinDB = -60.0f, MaxDB = 6.0f;
				const float RmsDB = 20.0f * FMath::LogX(10.0f, FMath::Max(AudioManager->SmoothedRmsLevels[channelIdx], 1e-9f));
				const float PeakDB = 20.0f * FMath::LogX(10.0f, FMath::Max(AudioManager->PeakLevels[channelIdx], 1e-9f));
				const float NormalizedRms = (FMath::Clamp(RmsDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);
				const float NormalizedPeak = (FMath::Clamp(PeakDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);
				
				const ImVec2 MeterSize(MeterWidth, MeterHeight);
				const ImVec2 CursorPos = ImGui::GetCursorScreenPos();
				const ImVec2 BarStart = ImVec2(CursorPos.x, CursorPos.y + MeterSize.y);

				DrawList->AddRectFilled(CursorPos, ImVec2(CursorPos.x + MeterSize.x, CursorPos.y + MeterSize.y), IM_COL32(30, 30, 30, 255), 2.0f);
				
				if (NormalizedRms > 0) {
					const ImU32 GreenColor=IM_COL32(0,200,0,255), YellowColor=IM_COL32(255,255,0,255), RedColor=IM_COL32(255,0,0,255);
					const float YellowT=0.75f, RedT=0.90f;
					float GreenHeight = FMath::Min(MeterSize.y * NormalizedRms, MeterSize.y * YellowT);
					DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y-GreenHeight), ImVec2(BarStart.x+MeterSize.x, BarStart.y), GreenColor, 2.0f, ImDrawFlags_RoundCornersBottom);
					if(NormalizedRms > YellowT){
						float YellowHeight = FMath::Min(MeterSize.y*NormalizedRms, MeterSize.y*RedT) - GreenHeight;
						DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y-GreenHeight-YellowHeight), ImVec2(BarStart.x+MeterSize.x, BarStart.y-GreenHeight), YellowColor);
					}
					if(NormalizedRms > RedT){
						float RedHeight = (MeterSize.y*NormalizedRms) - (MeterSize.y*RedT);
						DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y-(MeterSize.y*RedT)-RedHeight), ImVec2(BarStart.x+MeterSize.x, BarStart.y-(MeterSize.y*RedT)), RedColor);
					}
				}

				if (NormalizedPeak > 0) {
					const float PeakLineY = BarStart.y - (MeterSize.y * NormalizedPeak);
					DrawList->AddLine(ImVec2(CursorPos.x, PeakLineY), ImVec2(CursorPos.x + MeterSize.x, PeakLineY), IM_COL32(255, 255, 255, 180), 1.0f);
				}
				
				ImGui::Dummy(MeterSize);
				ImGui::SameLine();
			}
			ImGui::PopStyleVar();
			
			// --- 3. Draw the group label, properly centered under the meters ---
			ImGui::SetCursorScreenPos(ImVec2(GroupCursorStart.x + ScaleWidth, GroupCursorStart.y + MeterHeight + 5));
			const int32 StartChannel = groupIdx * ChannelsPerGroup + 1;
			const int32 EndChannel = FMath::Min(StartChannel + ChannelsPerGroup - 1, AudioManager->SmoothedRmsLevels.Num());
			const FString ChannelLabel = FString::Printf(TEXT("%d-%d"), StartChannel, EndChannel);
			const ImVec2 TextSize = ImGui::CalcTextSize(TCHAR_TO_UTF8(*ChannelLabel));
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((MeterWidth * ChannelsPerGroup) - TextSize.x) * 0.5f);
			ImGui::Text("%s", TCHAR_TO_UTF8(*ChannelLabel));

			ImGui::EndGroup(); // End the main group for scale, meters, and label

			// --- Handle wrapping for the next group ---
			if ((groupIdx + 1) % numColumns != 0)
			{
				ImGui::SameLine(0, GroupSpacing);
			}
		}
		ImGui::End();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("AudioManager not found, Audio Monitoring window not rendered"));
	}
}

