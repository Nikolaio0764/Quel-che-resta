// QCRZombieController.h — Controller IA con UDITO nativo di Unreal (AIPerception).
// Ogni ReportNoiseEvent nel mondo arriva qui: il "sistema del rumore" e' il motore stesso.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "QCRZombieController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;

UCLASS()
class QUELCHERESTA_API AQCRZombieController : public AAIController
{
	GENERATED_BODY()

public:
	AQCRZombieController();

protected:
	UPROPERTY(VisibleAnywhere)
	UAIPerceptionComponent* Perception;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	UFUNCTION()
	void OnPerception(AActor* Actor, FAIStimulus Stimulus);
};
