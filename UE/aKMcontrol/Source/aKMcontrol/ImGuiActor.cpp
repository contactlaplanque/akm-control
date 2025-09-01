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

}

void AImGuiActor::ToggleImGuiInput()
{
	FImGuiModule::Get().GetProperties().ToggleInput();
}

