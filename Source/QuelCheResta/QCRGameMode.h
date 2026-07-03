// QCRGameMode.h — GameMode: stato di partita, inventario, ciclo giorno/notte.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "QCRGameMode.generated.h"

USTRUCT()
struct FQCRMessage
{
	GENERATED_BODY()
	FString Text;
	float TimeLeft = 5.f;
};

UCLASS()
class QUELCHERESTA_API AQCRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AQCRGameMode();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	// --- ciclo giorno/notte ---
	UPROPERTY(EditAnywhere, Category = "Ciclo")
	float DayLength = 200.f;

	UPROPERTY(EditAnywhere, Category = "Ciclo")
	float NightLength = 130.f;

	int32 Day = 1;
	bool bIsNight = false;
	float PhaseTime = 0.f;
	float Elapsed = 0.f;
	bool bGameOver = false;

	// --- inventario (single player: vive qui) ---
	int32 Ammo = 6;
	int32 Food = 1;
	int32 Bandages = 1;
	int32 Wood = 2;
	int32 Kills = 0;
	int32 BestDays = 0;
	bool bPaused = false;

	// timer del flash rosso quando il giocatore viene colpito (letto dall'HUD)
	float HurtFlash = 0.f;

	// --- log messaggi per l'HUD ---
	TArray<FQCRMessage> Messages;

	float GetPhaseFraction() const;
	void PushMessage(const FString& Text);
	void AddKill();
	void TriggerGameOver();
	void RestartGame();
	void TogglePause();

	// Notifica quando cambia fase (il WorldManager la usa per luce e orde)
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDayChanged, int32 /*Day*/, bool /*bNight*/);
	FOnDayChanged OnDayChanged;
};
