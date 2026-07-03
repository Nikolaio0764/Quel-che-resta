// QCRGameMode.cpp
#include "QCRGameMode.h"
#include "QCRCharacter.h"
#include "QCRHUD.h"
#include "QCRWorldManager.h"
#include "EngineUtils.h"
#include "QCRSaveGame.h"
#include "Kismet/GameplayStatics.h"

AQCRGameMode::AQCRGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AQCRCharacter::StaticClass();
	HUDClass = AQCRHUD::StaticClass();
}

void AQCRGameMode::BeginPlay()
{
	Super::BeginPlay();

	// se il livello non contiene un WorldManager, lo creiamo noi:
	// cosi' il gioco funziona anche in una mappa completamente vuota.
	bool bFound = false;
	for (TActorIterator<AQCRWorldManager> It(GetWorld()); It; ++It)
	{
		bFound = true;
		break;
	}
	if (!bFound)
	{
		GetWorld()->SpawnActor<AQCRWorldManager>(
			AQCRWorldManager::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator);
	}

	// carica il record da disco
	if (UQCRSaveGame* Save = Cast<UQCRSaveGame>(
		UGameplayStatics::LoadGameFromSlot(TEXT("record"), 0)))
	{
		BestDays = Save->BestDays;
	}

	PushMessage(TEXT("Giorno 1. Cerca provviste prima che faccia buio."));
}

void AQCRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGameOver) return;

	Elapsed += DeltaSeconds;
	PhaseTime += DeltaSeconds;
	HurtFlash = FMath::Max(0.f, HurtFlash - DeltaSeconds * 1.2f);

	// scadenza dei messaggi
	for (int32 i = Messages.Num() - 1; i >= 0; --i)
	{
		Messages[i].TimeLeft -= DeltaSeconds;
		if (Messages[i].TimeLeft <= 0.f)
			Messages.RemoveAt(i);
	}

	const float Limit = bIsNight ? NightLength : DayLength;
	if (PhaseTime >= Limit)
	{
		PhaseTime = 0.f;
		if (bIsNight)
		{
			bIsNight = false;
			++Day;
			PushMessage(FString::Printf(TEXT("Giorno %d. Sei ancora vivo."), Day));
		}
		else
		{
			bIsNight = true;
			PushMessage(TEXT("Cala la notte. Sono piu' numerosi."));
		}
		OnDayChanged.Broadcast(Day, bIsNight);
	}
}

float AQCRGameMode::GetPhaseFraction() const
{
	const float Limit = bIsNight ? NightLength : DayLength;
	return PhaseTime / Limit;
}

void AQCRGameMode::PushMessage(const FString& Text)
{
	FQCRMessage M;
	M.Text = Text;
	Messages.Add(M);
	if (Messages.Num() > 5)
		Messages.RemoveAt(0);
}

void AQCRGameMode::AddKill()
{
	++Kills;
}

void AQCRGameMode::TriggerGameOver()
{
	if (bGameOver) return;
	bGameOver = true;

	// salva il record su disco
	if (Day > BestDays)
	{
		BestDays = Day;
		UQCRSaveGame* Save = Cast<UQCRSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UQCRSaveGame::StaticClass()));
		Save->BestDays = BestDays;
		UGameplayStatics::SaveGameToSlot(Save, TEXT("record"), 0);
	}

	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void AQCRGameMode::TogglePause()
{
	if (bGameOver) return;
	bPaused = !bPaused;
	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
}

void AQCRGameMode::RestartGame()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()), false);
}
