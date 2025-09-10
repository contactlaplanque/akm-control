// ImGuiActor.cpp

#include "ImGuiActor.h"

#include <string>

#include "Kismet/GameplayStatics.h"
#include "ImGuiModule.h"
#include "Engine/Engine.h"
#include "UEJackAudioLinkSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"
#include "HAL/CriticalSection.h"
#include "akMInternalLogCapture.h"

// (duplicate internal logs capture removed; single definition lives at file top)

AImGuiActor::AImGuiActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AImGuiActor::BeginPlay()
{
	Super::BeginPlay();

	MainViewLocalSize = FVector2D(0.0f, 0.0f);
	MainViewLocalTopLeft = FVector2D(0.0f, 0.0f);
	
	// Ensure ImGui input is disabled by default so camera controls work
	FImGuiModule::Get().GetProperties().SetInputEnabled(false);
	
	// Scale ImGui style by 2x (this should be done once)
	// ImGui::GetStyle().ScaleAllSizes(2.0f);

	// Bind UEJackAudioLink events to drive UI prompts
	if (GEngine)
	{
		if (UUEJackAudioLinkSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>())
		{
			Subsystem->OnNewJackClientConnected.AddDynamic(this, &AImGuiActor::OnNewJackClient);
			Subsystem->OnJackClientDisconnected.AddDynamic(this, &AImGuiActor::OnJackClientDisconnected);
			// Defer initial scan until akM server is ready (handled in Tick)
		}
	}

	// Attach internal logs capture to UE log
	if (!InternalLogCapture.IsValid())
	{
		InternalLogCapture = MakeUnique<FAkMInternalLogCapture>();
		if (GLog)
		{
			GLog->AddOutputDevice(InternalLogCapture.Get());
		}
	}
}

void AImGuiActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GLog && InternalLogCapture.IsValid())
	{
		GLog->RemoveOutputDevice(InternalLogCapture.Get());
	}
	InternalLogCapture.Reset();
	Super::EndPlay(EndPlayReason);
}

void AImGuiActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
	
	FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10.0f);
	
	// LEFT PANEL

	bool LeftPanelOpen = true;

	float LeftPanelWidth = ViewportSize.X - MainViewAbsoluteSize.X - 1;
	float LeftPanelHeight = ViewportSize.Y - BottomBarHeight;

	ImGuiWindowFlags LeftPanelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse ;
	
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(LeftPanelWidth, LeftPanelHeight));
    ImGui::Begin("Left Panel",&LeftPanelOpen,LeftPanelFlags);

	ImGui::Text("akM Server Status");
	// akM Server Window
	RenderAkMServerWindow();

	ImGui::Text("akM Server General Params");
	RenderGeneralServerParametersWindow();

	ImGui::Text("akM Speaker Params");
	RenderSpeakersParametersWindow();
    
    ImGui::End();

	// BOTTOM PANEL

	bool BottomPanelOpen = true;

	float BottomPanelHeight = ViewportSize.Y - MainViewAbsoluteSize.Y - 1 - BottomBarHeight;

	ImGuiWindowFlags BottomPanelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar ;

	ImGui::SetNextWindowPos(ImVec2(LeftPanelWidth, MainViewAbsoluteSize.Y + 1));
	ImGui::SetNextWindowSize(ImVec2( MainViewAbsoluteSize.X, BottomPanelHeight));
	ImGui::Begin("Bottom Panel", &BottomPanelOpen, BottomPanelFlags);

	ImVec2 AvailableSize = ImGui::GetContentRegionAvail();

	ImGui::BeginGroup();
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::BeginChild("LevelsMonitoringContainer", ImVec2(AvailableSize.x * 0.5f - 10, 0),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar);

	ImGui::PopStyleVar();
	ImGui::Text("Audio Levels Monitoring");
	// Audio Monitoring Window
	RenderAudioMonitoringWindow();

	ImGui::EndChild();
	ImGui::SameLine(0,10);

	ImGui::BeginChild("InternalLogsContainer", ImVec2(AvailableSize.x * 0.5f - 10, 0),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar);
	ImGui::Text("akM Control Internal Logs");
	RenderInternalLogsWindow();
	
	ImGui::EndChild();

	ImGui::EndGroup();
	
	ImGui::End();

	// BOTTOM BAR

	bool BottomBarOpen = true;

	ImGuiWindowFlags BottomBarFlags = ImGuiWindowFlags_NoTitleBar 
	                                | ImGuiWindowFlags_NoResize 
	                                | ImGuiWindowFlags_NoMove 
	                                | ImGuiWindowFlags_NoCollapse;

	ImGui::SetNextWindowPos(ImVec2(0, ViewportSize.Y - BottomBarHeight));
	ImGui::SetNextWindowSize(ImVec2(ViewportSize.X, BottomBarHeight));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.85f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Bottom Bar", &BottomBarOpen, BottomBarFlags))
	{
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

	    // Style adjustments
	    ImGuiTableFlags TableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingFixedFit;
	    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 2));
	    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	    ImGui::SetWindowFontScale(0.9f);

	    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,  ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

	    // Table
	    if (ImGui::BeginTable("App_info", 4, TableFlags, ImGui::GetContentRegionAvail()))
	    {
	        ImGui::TableNextRow();

	        ImGui::TableNextColumn();
	        ImGui::Text("akM Control PROTO");

	        ImGui::TableNextColumn();
	        bool bImGuiInputEnabled = FImGuiModule::Get().GetProperties().IsInputEnabled();
	        ImGui::TextColored(
	            bImGuiInputEnabled ? ImVec4(1, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1),
	            bImGuiInputEnabled ? "ImGui Input: ENABLED" : "ImGui Input: DISABLED");

	        ImGui::TableNextColumn();
	        ImGui::Text("FPS: %.1f", FPS);

	        ImGui::TableNextColumn();
	        ImGui::Text("Frame Time: %.2f ms", FrameTime);

	        ImGui::EndTable();
	    }

	    // Restore styles
	    ImGui::PopStyleColor(2);
	    ImGui::SetWindowFontScale(1.0f);
	    ImGui::PopStyleVar(3);
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);


    // Scale ImGui font by 2x (apply every frame to ensure it takes effect)
    // ImGui::GetIO().FontGlobalScale = 2.0f;

	DrawNewClientPopup();

	// After akM server is ready (scsynth wired to Unreal), run one-time scan for already connected clients
	if (!bInitialClientScanDone && SpatServerManager && SpatServerManager->bAKMserverAudioOutputPortsConnected)
	{
		if (GEngine)
		{
			if (UUEJackAudioLinkSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>())
			{
				const FString UnrealClientName = Subsystem->GetJackClientName();
				TArray<FString> Clients = Subsystem->GetConnectedClients();
				for (const FString& Client : Clients)
				{
					if (Client.Equals(TEXT("scsynth"), ESearchCase::IgnoreCase)) { continue; }
					if (!UnrealClientName.IsEmpty() && Client.Equals(UnrealClientName, ESearchCase::IgnoreCase)) { continue; }
					TArray<FString> Inputs, Outputs;
					Subsystem->GetClientPorts(Client, Inputs, Outputs);
					if (Inputs.Num() > 0 || Outputs.Num() > 0)
					{
						OnNewJackClient(Client, Inputs.Num(), Outputs.Num());
					}
				}
				bInitialClientScanDone = true;
			}
		}
	}
	
	
	
	

	// Disable ImGui input if mouse is over main view
	if (ImGui::IsMousePosValid())
	{
		bool isMouseXinRange = MainViewLocalTopLeft.X <= ImGui::GetMousePos().x && ImGui::GetMousePos().x <= MainViewLocalTopLeft.X + MainViewLocalSize.X;
		bool isMouseYinRange = MainViewLocalTopLeft.Y <= ImGui::GetMousePos().y && ImGui::GetMousePos().y <= MainViewLocalTopLeft.Y + MainViewLocalSize.Y;
		if (isMouseXinRange || isMouseYinRange)
		{
			SetImGuiInput(false);
		}
	}
	
}


void AImGuiActor::SetImGuiInput(bool NewState)
{
	//FImGuiModule::Get().GetProperties().ToggleInput();
	FImGuiModule::Get().GetProperties().SetInputEnabled(NewState);
}

void AImGuiActor::RenderAkMServerWindow() const
{
	if (SpatServerManager != nullptr)
	{

		ImGui::BeginChild("akM_Server",ImVec2(0,400),true,ImGuiChildFlags_Borders);

		bool bServerAlive = SpatServerManager->bIsServerAlive;
		bool bServerIsStarting = SpatServerManager->bServerIsStarting;
		ImVec4 ServerStatusColor = bServerIsStarting ? ImVec4(1, 1, 0, 1) : bServerAlive ? ImVec4(0,1,0,1) : ImVec4(1, 0, 0, 1);
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(ServerStatusColor, bServerIsStarting ? "SERVER STARTING..." : bServerAlive ? "ONLINE" : "OFFLINE");

		if (bServerIsStarting)
			ImGui::BeginDisabled();

		ImGui::SameLine();

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
		
		if (bServerIsStarting)
			ImGui::EndDisabled();

		// Info row: JACK CPU load and scsynth port counts
		{
			float CpuLoad = -1.0f;
			int32 NumScInputs = -1;
			int32 NumScOutputs = -1;
			if (GEngine)
			{
				if (UUEJackAudioLinkSubsystem* Subsystem = GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>())
				{
					CpuLoad = Subsystem->GetCpuLoad(); // 0..100
					TArray<FString> ScIn, ScOut;
					Subsystem->GetClientPorts(TEXT("scsynth"), ScIn, ScOut);
					NumScInputs = ScIn.Num();
					NumScOutputs = ScOut.Num();
				}
			}

			if (CpuLoad >= 0.0f)
			{
				ImGui::Text("CPU: %.1f%%", CpuLoad);
			}
			else
			{
				ImGui::Text("CPU: N/A");
			}
			ImGui::SameLine();
			if (NumScInputs >= 0 && NumScOutputs >= 0)
			{
				ImGui::Text("scsynth: %d in / %d out", NumScInputs, NumScOutputs);
			}
			else
			{
				ImGui::Text("scsynth: N/A");
			}
		}

		ImGui::Separator();

		// Scrollable child region
		ImGui::SetWindowFontScale(0.7f);
		ImGui::BeginChild("ConsoleRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (const FString& Line : SpatServerManager->ImGuiConsoleBuffer)
		{
			ImGui::TextUnformatted(TCHAR_TO_ANSI(*Line));
		}

		// Auto-scroll to bottom
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		

		ImGui::EndChild();
		ImGui::SetWindowFontScale(1.0f);
		
		ImGui::EndChild();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("SpatServerManager not found, akM Server window not rendered"));
	}
}

void AImGuiActor::RenderAudioMonitoringWindow() const
{
	if (AudioManager == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager not found, Audio Monitoring window not rendered"));
		return;
	}

	if (SpatServerManager == nullptr)
	{
		ImGui::Text("SpatServerManager not found; cannot determine connected input ports.");
		return;
	}

	// Layout constants
	const float MeterWidth = 14.0f;
	const float MeterSpacing = 10.0f;
	const float GroupPadding = 4.0f;

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float sectionHeight = FMath::Max(0, avail.y * 0.5f);

	// Helpers

	auto DrawSingleMeter = [&](ImDrawList* DrawList, const ImVec2& topLeft, float meterHeight, float rms, float peak)
	{
		const float MinDB = -60.0f;
		const float MaxDB = 6.0f;
		const float RmsDB = 20.0f * FMath::LogX(10.0f, FMath::Max(rms, 1e-9f));
		const float PeakDB = 20.0f * FMath::LogX(10.0f, FMath::Max(peak, 1e-9f));
		const float NormalizedRms = (FMath::Clamp(RmsDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);
		const float NormalizedPeak = (FMath::Clamp(PeakDB, MinDB, MaxDB) - MinDB) / (MaxDB - MinDB);

		const ImVec2 BarStart = ImVec2(topLeft.x, topLeft.y + meterHeight);
		// Background
		DrawList->AddRectFilled(topLeft, ImVec2(topLeft.x + MeterWidth, topLeft.y + meterHeight), IM_COL32(30, 30, 30, 255), 2.0f);
		// RMS
		if (NormalizedRms > 0.0f)
		{
			const ImU32 GreenColor = IM_COL32(0, 200, 0, 255);
			const ImU32 YellowColor = IM_COL32(255, 255, 0, 255);
			const ImU32 RedColor = IM_COL32(255, 0, 0, 255);
			const float YellowT = 0.75f;
			const float RedT = 0.90f;
			float GreenHeight = FMath::Min(meterHeight * NormalizedRms, meterHeight * YellowT);
			DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - GreenHeight), ImVec2(BarStart.x + MeterWidth, BarStart.y), GreenColor, 2.0f, ImDrawFlags_RoundCornersBottom);
			if (NormalizedRms > YellowT)
			{
				float YellowHeight = FMath::Min(meterHeight * NormalizedRms, meterHeight * RedT) - GreenHeight;
				DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - GreenHeight - YellowHeight), ImVec2(BarStart.x + MeterWidth, BarStart.y - GreenHeight), YellowColor);
			}
			if (NormalizedRms > RedT)
			{
				float RedHeight = (meterHeight * NormalizedRms) - (meterHeight * RedT);
				DrawList->AddRectFilled(ImVec2(BarStart.x, BarStart.y - (meterHeight * RedT) - RedHeight), ImVec2(BarStart.x + MeterWidth, BarStart.y - (meterHeight * RedT)), RedColor);
			}
		}
		// Peak
		if (NormalizedPeak > 0.0f)
		{
			const float PeakLineY = BarStart.y - (meterHeight * NormalizedPeak);
			DrawList->AddLine(ImVec2(topLeft.x, PeakLineY), ImVec2(topLeft.x + MeterWidth, PeakLineY), IM_COL32(255, 255, 255, 180), 1.0f);
		}
	};

	const bool bHaveScsynth = SpatServerManager->ConnectedUnrealInputIndicesFromScsynth.Num() > 0;
	const bool bHaveSources = SpatServerManager->ConnectedUnrealInputIndicesByClient.Num() > 0;

	if (!bHaveScsynth && !bHaveSources)
	{
		ImGui::BeginChild("EmptyMeters", ImVec2(0, sectionHeight), true);
		ImGui::Text("No connected audio input ports on UE Jack Client");
		ImGui::EndChild();
		ImGui::BeginChild("EmptyMeters2", ImVec2(0, sectionHeight), true);
		ImGui::EndChild();
			return;
		}

	// SPEAKERS OUTPUT (scsynth)
	if (bHaveScsynth)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::BeginChild("SpeakersOutputChild", ImVec2(0, sectionHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::SetWindowFontScale(0.7f);

		const TArray<int32>& UnrealInputsFromSc = SpatServerManager->ConnectedUnrealInputIndicesFromScsynth;
		const int32 totalPorts = UnrealInputsFromSc.Num();
		const int32 numGroups = FMath::DivideAndRoundUp(totalPorts, 3);
		const float meterHeight = 120.0f;

		ImGui::Text("SPEAKERS OUTPUT");

		ImGuiTableFlags TableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollX;
		const int32 tableCols = numGroups; //
		if (ImGui::BeginTable("SpeakersTable", tableCols, TableFlags, ImVec2(0, 0)))
		{
			for (int32 g = 0; g < numGroups; ++g)
			{
				const int32 countInGroup = FMath::Min(3, totalPorts - g * 3);
				const float groupWidth = countInGroup * MeterWidth + (countInGroup - 1) * MeterSpacing + GroupPadding * 2.0f;
				FString ColName = FString::Printf(TEXT("G%d"), g + 1);
				ImGui::TableSetupColumn(TCHAR_TO_UTF8(*ColName), ImGuiTableColumnFlags_WidthFixed, groupWidth);
			}
			ImGui::TableNextRow();
			for (int32 g = 0; g < numGroups; ++g)
			{
				ImGui::TableNextColumn();
				if (g == numGroups - 1) { ImGui::Text("SUBS"); }
				else { ImGui::Text("COL. %1.d", g + 1); }
			}

			ImGui::TableNextRow();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			for (int32 g = 0; g < numGroups; ++g)
			{
				ImGui::TableNextColumn();
				const ImVec2 cellPos = ImGui::GetCursorScreenPos();
				float x = cellPos.x + GroupPadding;
				float y = cellPos.y;
				const int32 start = g * 3;
				const int32 count = FMath::Min(3, totalPorts - start);
				
				for (int32 i = 0; i < count; ++i)
				{
					const int32 unrealIdx1 = UnrealInputsFromSc[start + i];
					const int32 ch = unrealIdx1 - 1;
					if (AudioManager->SmoothedRmsLevels.IsValidIndex(ch) && AudioManager->PeakLevels.IsValidIndex(ch))
					{
						DrawSingleMeter(DrawList, ImVec2(x, y), meterHeight, AudioManager->SmoothedRmsLevels[ch], AudioManager->PeakLevels[ch]);
					}
					x += MeterWidth + MeterSpacing;
				}
				// Space for meters area
				ImGui::Dummy(ImVec2(GroupPadding * 2.0f + count * MeterWidth + (count - 1) * MeterSpacing, meterHeight));
			}
			ImGui::TableNextRow();
			for (int32 g = 0; g < numGroups; ++g)
			{
				ImGui::TableNextColumn();
				const int32 start = g * 3;
				const int32 count = FMath::Min(3, totalPorts - start);
				
				for (int32 i = 0; i < count; ++i)
				{
					const int32 unrealIdx1 = UnrealInputsFromSc[start + i];
					const int32 ch = unrealIdx1;
					ImGui::Text("%.2d",ch);ImGui::SameLine(0,4);
				}
			}

			ImGui::EndTable();
		}

		ImGui::SetWindowFontScale(1.0f);
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	else
	{
		ImGui::BeginChild("SpeakersOutputChildEmpty", ImVec2(0, sectionHeight), true);
		ImGui::EndChild();
	}

	// SOURCES INPUT (external clients)
	if (bHaveSources)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
		ImGui::BeginChild("SourcesInputChild", ImVec2(0, sectionHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::SetWindowFontScale(0.7f);

		// Prepare groups (max 4 meters per group) ordered by client name
		TArray<FString> ClientNames;
		SpatServerManager->ConnectedUnrealInputIndicesByClient.GetKeys(ClientNames);
		ClientNames.Sort();

		struct FGroup { FString Name; TArray<int32> Indices; TArray<int32> Labels; };
		TArray<FGroup> Groups; Groups.Reserve(16);
		for (const FString& Client : ClientNames)
		{
			const TArray<int32>* UnrealInputsPtr = SpatServerManager->ConnectedUnrealInputIndicesByClient.Find(Client);
			if (!UnrealInputsPtr) { continue; }
			const TArray<int32>& UnrealInputs = *UnrealInputsPtr;
			for (int32 start = 0; start < UnrealInputs.Num(); start += 4)
			{
				FGroup G;
				const int32 count = FMath::Min(4, UnrealInputs.Num() - start);
				G.Indices.Reserve(count);
				G.Labels.Reserve(count);

				for (int32 i = 0; i < count; ++i)
				{
					G.Indices.Add(UnrealInputs[start + i]);
					G.Labels.Add(start + i + 1);
				}
				G.Name = FString::Printf(TEXT("%s"), *Client);
				Groups.Add(MoveTemp(G));
			}
		}
		
		const float meterHeight = 120.0f;

		ImGui::Text("AUDIO SOURCES INPUTS");

		ImGuiTableFlags TableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollX;
		const int32 tableCols = 1 + Groups.Num();
		if (ImGui::BeginTable("SourcesTable", tableCols, TableFlags, ImVec2(0, 0)))
		{
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				const int32 count = Groups[g].Indices.Num();
				const float groupWidth = count * MeterWidth + (count - 1) * MeterSpacing + GroupPadding * 2.0f;
				FString ColName = FString::Printf(TEXT("C%d"), g + 1);
				ImGui::TableSetupColumn(TCHAR_TO_UTF8(*ColName), ImGuiTableColumnFlags_WidthFixed, groupWidth);
			}
			ImGui::TableNextRow();
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				ImGui::TableNextColumn();
				ImGui::Text(TCHAR_TO_UTF8(*Groups[g].Name));
			}
			ImGui::TableNextRow();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				ImGui::TableNextColumn();
				const ImVec2 cellPos = ImGui::GetCursorScreenPos();
				float x = cellPos.x + GroupPadding;
				float y = cellPos.y;
				const TArray<int32>& Indices = Groups[g].Indices;
				for (int32 i = 0; i < Indices.Num(); ++i)
				{
					const int32 ch = Indices[i] - 1;
					if (AudioManager->SmoothedRmsLevels.IsValidIndex(ch) && AudioManager->PeakLevels.IsValidIndex(ch))
					{
						DrawSingleMeter(DrawList, ImVec2(x, y), meterHeight, AudioManager->SmoothedRmsLevels[ch], AudioManager->PeakLevels[ch]);
					}
					x += MeterWidth + MeterSpacing;
				}
				// Space for meters
				ImGui::Dummy(ImVec2(GroupPadding * 2.0f + Indices.Num() * MeterWidth + (Indices.Num() - 1) * MeterSpacing, meterHeight));
			}
			
			ImGui::TableNextRow();
			
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				ImGui::TableNextColumn();
				const TArray<int32>& Labels = Groups[g].Labels;
				for (int32 i = 0; i < Labels.Num(); ++i)
				{
					const int32 ch = Labels[i];
					ImGui::Text("%.2d",ch);ImGui::SameLine(0,4);
				}
			}

			ImGui::EndTable();
		}

		ImGui::SetWindowFontScale(1.0f);
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	else
	{
		ImGui::BeginChild("SourcesInputChildEmpty", ImVec2(0, sectionHeight), true);
		ImGui::EndChild();
	}
}

void AImGuiActor::RenderInternalLogsWindow() const
{
    if (ImGui::BeginChild("akM Control Internal Logs",	ImVec2(0,0)))
    {
        // Snapshot lines under lock
        TArray<FString> Copy;
        if (InternalLogCapture.IsValid())
        {
            InternalLogCapture->GetSnapshot(Copy);
        }

    	ImGui::SetWindowFontScale(0.7f);
        ImGui::BeginChild("InternalLogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const FString& Line : Copy)
        {
            ImGui::TextUnformatted(TCHAR_TO_ANSI(*Line));
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    	ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::EndChild();
}

void AImGuiActor::RenderGeneralServerParametersWindow() const
{

	ImGui::SetWindowFontScale(0.8f);
	
	ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("ServerGainAndReverb",ImVec2(AvailableSize.x * 0.5f - 10,200 ));

	ImGui::Text("Main Gain");
	ImGui::SliderFloat("Gain", &SpatServerManager->systemGain, -90.0f, 30.0f, "%.1f dB");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/gain";
		SpatServerManager->SendOSCFloat(address,SpatServerManager->systemGain);
	};

	ImGui::Separator();
	ImGui::Text("Main Reverb");
	
	ImGui::SliderFloat("Decay", &SpatServerManager->reverbDecay, 0.0f, 20.0f, "%.2f s");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/reverb";
		const TArray<float> values = { SpatServerManager->reverbDecay, SpatServerManager->reverbFeedback };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	
	ImGui::SliderFloat("Feedback", &SpatServerManager->reverbFeedback, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/reverb";
		const TArray<float> values = { SpatServerManager->reverbDecay, SpatServerManager->reverbFeedback };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	ImGui::Separator();
	
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginChild("ServerSatsAndSubsFilters",ImVec2(AvailableSize.x * 0.5f - 10,200));
	
	ImGui::Text("Satellites Filter");
	ImGui::PushID("SatsFilterFrequency");
	ImGui::SliderFloat("Frequency", &SpatServerManager->satsFilterFrequency, 0.0f, 22000.0f, "%.1f Hz");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/filter/sats";
		const TArray<float> values = { SpatServerManager->satsFilterFrequency, SpatServerManager->satsFilterRq };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	ImGui::PopID();
	
	ImGui::PushID("SatsFilterRq");
	ImGui::SliderFloat("Rq", &SpatServerManager->satsFilterRq, 0.0f, 10.0f, "%.2f");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/filter/sats";
		const TArray<float> values = { SpatServerManager->satsFilterFrequency, SpatServerManager->satsFilterRq };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	ImGui::PopID();

	ImGui::Separator();
	ImGui::Text("Subwoofers Filter");
	ImGui::PushID("SubsFilterFrequency");
	ImGui::SliderFloat("Frequency", &SpatServerManager->subsFilterFrequency, 0.0f, 22000.0f, "%.1f Hz");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/filter/subs";
		const TArray<float> values = { SpatServerManager->subsFilterFrequency, SpatServerManager->subsFilterRq };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	ImGui::PopID();
	
	ImGui::PushID("SubsFilterRq");
	ImGui::SliderFloat("Rq", &SpatServerManager->subsFilterRq, 0.0f, 10.0f, "%.2f");
	if (ImGui::IsItemEdited())
	{
		const FString address = "/system/filter/subs";
		const TArray<float> values = { SpatServerManager->subsFilterFrequency, SpatServerManager->subsFilterRq };
		SpatServerManager->SendOSCFloatArray(address,values);
	};
	ImGui::PopID();
	
	ImGui::Separator();
	
	ImGui::EndChild();

	ImGui::SetWindowFontScale(1.0f);
}

void AImGuiActor::RenderSpeakersParametersWindow() const
{
	ImGui::SetWindowFontScale(0.8f);
	
	// Ensure we start on a new line after any previous SameLine() usage
	ImGui::NewLine();

	ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("SpeakersGain", ImVec2(AvailableSize.x, 0.0f));
	ImGui::Text("Satellites Gain");

	for (int i=0;i<12;i++)
	{
		ImGui::Text("SAT %d",i+1);
		ImGui::PushID(i);
		ImGui::SliderFloat("Gain", &SpatServerManager->satsGains[i], -90.0f, 30.0f, "%.1f dB");
		if (ImGui::IsItemEdited())
		{
			const FString address = "/sat"+FString::FromInt(i+1)+"/gain";
			SpatServerManager->SendOSCFloat(address,SpatServerManager->satsGains[i]);
		};
		ImGui::PopID();
	}
	
	ImGui::EndChild();

	ImGui::SetWindowFontScale(1.0f);
}


void AImGuiActor::OnNewJackClient(const FString& ClientName, int32 NumInputs, int32 NumOutputs)
{
	// Ignore scsynth (handled by manager) and our own Unreal client
	if (ClientName.Equals(TEXT("scsynth"), ESearchCase::IgnoreCase))
	{
		return;
	}
	// Ignore JACK system device
	if (ClientName.Equals(TEXT("system"), ESearchCase::IgnoreCase))
	{
		return;
	}
	UUEJackAudioLinkSubsystem* Subsystem = (GEngine ? GEngine->GetEngineSubsystem<UUEJackAudioLinkSubsystem>() : nullptr);
	if (Subsystem)
	{
		const FString UnrealClient = Subsystem->GetJackClientName();
		if (!UnrealClient.IsEmpty() && ClientName.Equals(UnrealClient, ESearchCase::IgnoreCase))
		{
			return;
		}
	}

	FPendingClientPrompt P; P.ClientName = ClientName; P.NumInputs = NumInputs; P.NumOutputs = NumOutputs;
	PendingClientPrompts.Add(MoveTemp(P));
	bImGuiOpenNextPopup = true;
}

void AImGuiActor::OnJackClientDisconnected(const FString& ClientName)
{
	// Remove pending prompt for this client if present
	for (int32 i = PendingClientPrompts.Num() - 1; i >= 0; --i)
	{
		if (PendingClientPrompts[i].ClientName.Equals(ClientName, ESearchCase::IgnoreCase))
		{
			PendingClientPrompts.RemoveAt(i);
		}
	}
}

void AImGuiActor::DrawNewClientPopup()
{
	if (PendingClientPrompts.Num() == 0)
	{
		return;
	}
	if (bImGuiOpenNextPopup)
	{
		ImGui::OpenPopup("NewJackClientPopup");
		bImGuiOpenNextPopup = false;
	}
	
	if (ImGui::BeginPopupModal("NewJackClientPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const FPendingClientPrompt& Prompt = PendingClientPrompts[0];
		ImGui::Text("New client connected on Jack server: %s - %d inputs, %d outputs", TCHAR_TO_ANSI(*Prompt.ClientName), Prompt.NumInputs, Prompt.NumOutputs);
		ImGui::Separator();
		ImGui::TextWrapped("Would you like to consider this as an audio input for sources in akM server and connect the outputs of this client to akM server?");

		bool bDoConnect = false;
		if (ImGui::Button("Yes", ImVec2(120, 0)))
		{
			bDoConnect = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			PendingClientPrompts.RemoveAt(0);
			if (PendingClientPrompts.Num() > 0)
			{
				bImGuiOpenNextPopup = true;
			}
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		if (bDoConnect)
		{
			// Ask the manager to perform actual connects and validation
			if (SpatServerManager)
			{
				SpatServerManager->AcceptExternalClient(Prompt.ClientName, Prompt.NumInputs, Prompt.NumOutputs);
			}
			PendingClientPrompts.RemoveAt(0);
			if (PendingClientPrompts.Num() > 0)
			{
				bImGuiOpenNextPopup = true;
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

