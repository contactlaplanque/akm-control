// Fill out your copyright notice in the Description page of Project Settings.


#include "ImGuiActor.h"

#include "SourceActor.h"

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
}

// Called every frame
void AImGuiActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Actor Picking Variables
    static TWeakObjectPtr<AActor> PickedActor = nullptr;
    static bool bIsPickingActor = false;

    static float location[3] = {0.0f, 0.0f, 0.0f};
    static float locationPrevious[3] = {0.0f, 0.0f, 0.0f};

    ImGui::Begin("Main Window");

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

            ImGui::Text("Picked source:", *SourceActor->GetName());
            ImGui::SameLine(); ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), " %ls", *SourceActor->GetName());

            const FVector actorLocation = SourceActor->GetActorLocation();

            location[0] = actorLocation.X;
            location[1] = actorLocation.Y;
            location[2] = actorLocation.Z;

            ImGui::InputFloat3("Location", location);

        }

    }

    ImGui::End();

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

