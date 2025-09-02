// ImGuiActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImGuiModule.h"
#include "akMSpatServerManager.h"

#include "ImGuiActor.generated.h"

UCLASS()
class AKMCONTROL_API AImGuiActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AImGuiActor();

	UPROPERTY(EditAnywhere, Category="Object References")
	AakMSpatServerManager* SpatServerManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	static void ToggleImGuiInput();

	// Actor Picking Variables
	TWeakObjectPtr<AActor> PickedActor;
	bool bIsPickingActor = false;

private:
	

};
