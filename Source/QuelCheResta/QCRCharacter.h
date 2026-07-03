// QCRCharacter.h — Personaggio in prima persona: statistiche, armi, stealth.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "QCRCharacter.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class AQCRGameMode;

UENUM()
enum class EQCRWeapon : uint8 { Bat, Pistol };

UCLASS()
class QUELCHERESTA_API AQCRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AQCRCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- statistiche di sopravvivenza ---
	float HP = 100.f;
	float Hunger = 100.f;
	float Stamina = 100.f;

	// --- stato ---
	EQCRWeapon Weapon = EQCRWeapon::Bat;
	bool bSprinting = false;
	bool bFlashlightOn = false;

	// Fattore di individuabilita' letto dai vaganti (accovacciato = 0.45)
	float GetStealthFactor() const;

	void ReceiveDamage(float Amount);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	USpotLightComponent* Flashlight;

	// modelli-placeholder delle armi in vista
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BatModel;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PistolModel;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* MuzzleLight;

	// --- input ---
	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartSprint();
	void StopSprint();
	void StartCrouching();
	void StopCrouching();
	void Attack();
	void Interact();
	void PlaceBarricade();
	void ToggleFlashlight();
	void EquipBat();
	void EquipPistol();

	// --- azioni ---
	void MeleeAttack();
	void Shoot();
	void Die();
	void PauseGame();
	void RestartGame();

	AQCRGameMode* GM() const;

	float AttackCooldown = 0.f;
	float RunNoiseTimer = 0.f;
	float SwingAnim = 0.f;      // 1 -> 0: animazione dello swing della mazza
	float RecoilAnim = 0.f;     // 1 -> 0: rinculo della pistola
	float MuzzleTimer = 0.f;
	float HeartbeatTimer = 0.f;

	static constexpr float WalkSpeed = 450.f;
	static constexpr float SprintSpeed = 750.f;
};
