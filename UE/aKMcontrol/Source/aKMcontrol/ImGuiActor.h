// ImGuiActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImGuiModule.h"
#include "akMSpatServerManager.h"
#include "akMControlAudioManager.h"
#include "akMInternalLogCapture.h"

#include "ImGuiActor.generated.h"

class ASourcesManager;

UCLASS()
class AKMCONTROL_API AImGuiActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AImGuiActor();

	UPROPERTY(EditAnywhere, Category="Object References")
	AakMSpatServerManager* SpatServerManager;

	UPROPERTY(EditAnywhere, Category="Object References")
	AakMControlAudioManager* AudioManager;

	UPROPERTY(EditAnywhere, Category="Object References")
	ASourcesManager* SourcesManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called when the game ends or actor is destroyed
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	static void SetImGuiInput(bool NewState);

	UFUNCTION(BlueprintCallable)
	bool GetImGuiInput() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ImGui")
	FVector2D MainViewLocalTopLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ImGui")
	FVector2D MainViewLocalSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ImGui")
	FVector2D MainViewAbsoluteSize;

	// Actor Picking Variables
	TWeakObjectPtr<AActor> PickedActor;
	bool bIsPickingActor = false;

private:

	void RenderAkMServerWindow() const;
	void RenderBottomBar(float DeltaTime) const;
	void RenderAudioMonitoringWindow() const;
	void RenderInternalLogsWindow() const;
	void RenderGeneralServerParametersWindow() const;
	void RenderSpeakersParametersWindow() const;

	// New client prompt state
	struct FPendingClientPrompt
	{
		FString ClientName;
		int32 NumInputs = 0;
		int32 NumOutputs = 0;
	};
	TArray<FPendingClientPrompt> PendingClientPrompts;
	bool bImGuiOpenNextPopup = false;
	bool bInitialClientScanDone = false;

	// Event bindings
	UFUNCTION()
	void OnNewJackClient(const FString& ClientName, int32 NumInputs, int32 NumOutputs);

	UFUNCTION()
	void OnJackClientDisconnected(const FString& ClientName);

	// Helper to draw popup
	void DrawNewClientPopup();

	int BottomBarHeight = 20;
	
	// Internal logs capture device
	TUniquePtr<FAkMInternalLogCapture> InternalLogCapture;
	

};
