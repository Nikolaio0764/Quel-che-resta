// QCRZombie.h — Vagante: IA a stati con udito nativo (AIPerception).
// Varianti: 0 Vagante, 1 Corridore, 2 Bruto.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "QCRZombie.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM()
enum class EZombieState : uint8 { Wander, Investigate, Chase };

UCLASS()
class QUELCHERESTA_API AQCRZombie : public ACharacter
{
	GENERATED_BODY()

public:
	AQCRZombie();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// impostata dal WorldManager prima dello spawn finito
	int32 Kind = 0;

	void ReceiveDamage(float Amount);

	// chiamata dal controller quando l'udito percepisce qualcosa
	void HearNoise(const FVector& Location);

protected:
	EZombieState State = EZombieState::Wander;
	float HP = 100.f;
	float BaseSpeed = 160.f;   // cm/s
	float Damage = 12.f;

	FVector NoiseTarget;
	FVector WanderDir;
	float WanderTimer = 0.f;
	float AttackTimer = 0.f;
	float GroanTimer = 0.f;
	float RepathTimer = 0.f;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BodyMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* BodyMID;

	FLinearColor BaseColor = FLinearColor(0.32f, 0.40f, 0.28f);

	void ApplyKind();
	void SetTint(const FLinearColor& C);
};
