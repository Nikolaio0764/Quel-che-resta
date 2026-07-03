// QCRHUD.cpp
#include "QCRHUD.h"
#include "QCRCharacter.h"
#include "QCRGameMode.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AQCRHUD::DrawBar(float X, float Y, float W, float H,
	float Fraction, const FLinearColor& Color, const FString& Label)
{
	DrawRect(FLinearColor(0.1f, 0.1f, 0.09f, 0.8f), X, Y, W, H);
	DrawRect(Color, X, Y, W * FMath::Clamp(Fraction, 0.f, 1.f), H);
	DrawText(Label, FLinearColor(0.85f, 0.83f, 0.77f), X, Y - 16.f,
		nullptr, 0.9f);
}

void AQCRHUD::DrawHUD()
{
	Super::DrawHUD();

	AQCRGameMode* Mode = GetWorld()->GetAuthGameMode<AQCRGameMode>();
	AQCRCharacter* P = Cast<AQCRCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Mode || !P) return;

	// --- flash rosso quando vieni colpito ---
	if (Mode->HurtFlash > 0.f)
		DrawRect(FLinearColor(0.6f, 0.05f, 0.02f, Mode->HurtFlash),
			0, 0, Canvas->SizeX, Canvas->SizeY);

	// --- schermata di morte ---
	if (Mode->bGameOver)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.75f), 0, 0, Canvas->SizeX, Canvas->SizeY);
		const FString RecordText = (Mode->Day >= Mode->BestDays)
			? TEXT("NUOVO RECORD!")
			: FString::Printf(TEXT("Record: %d giorni"), Mode->BestDays);
		const FString Death = FString::Printf(
			TEXT("SEI UNO DI LORO\n\nSei sopravvissuto %d %s (%d min).\nVaganti abbattuti: %d\n%s\n\nPremi INVIO per ricominciare."),
			Mode->Day,
			Mode->Day == 1 ? TEXT("giorno") : TEXT("giorni"),
			int32(Mode->Elapsed / 60.f), Mode->Kills, *RecordText);
		DrawText(Death, FLinearColor(0.85f, 0.3f, 0.2f),
			Canvas->SizeX * 0.5f - 180.f, Canvas->SizeY * 0.42f, nullptr, 1.4f);
		return;
	}

	// --- menu di pausa ---
	if (Mode->bPaused)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0, 0, Canvas->SizeX, Canvas->SizeY);
		DrawText(TEXT("IN PAUSA\n\nP riprendi | INVIO ricomincia"),
			FLinearColor(0.85f, 0.83f, 0.77f),
			Canvas->SizeX * 0.5f - 130.f, Canvas->SizeY * 0.45f, nullptr, 1.3f);
		return;
	}

	// --- hint comandi nei primi 14 secondi ---
	if (Mode->Elapsed < 14.f)
	{
		const float A = FMath::Clamp((14.f - Mode->Elapsed) / 2.f, 0.f, 0.75f);
		DrawText(TEXT("WASD muovi | SHIFT corri | CTRL accovacciati | 1 mazza / 2 pistola\nclick attacca | E cerca/mangia/cura | B barricata | T torcia | P pausa"),
			FLinearColor(0.8f, 0.78f, 0.7f, A),
			Canvas->SizeX * 0.5f - 300.f, Canvas->SizeY - 120.f, nullptr, 1.f);
	}

	// --- barre vitali (alto a sinistra) ---
	DrawBar(20.f, 40.f, 220.f, 14.f, P->HP / 100.f,
		FLinearColor(0.71f, 0.23f, 0.14f), TEXT("SALUTE"));
	DrawBar(20.f, 90.f, 220.f, 14.f, P->Hunger / 100.f,
		FLinearColor(0.85f, 0.64f, 0.25f), TEXT("FAME"));
	DrawBar(20.f, 140.f, 220.f, 14.f, P->Stamina / 100.f,
		FLinearColor(0.29f, 0.36f, 0.29f), TEXT("FIATO"));

	// --- giorno + fase (alto a destra) ---
	const FString DayText = FString::Printf(TEXT("GIORNO %d (%s) | uccisi: %d"),
		Mode->Day, Mode->bIsNight ? TEXT("NOTTE") : TEXT("giorno"), Mode->Kills);
	DrawText(DayText, FLinearColor::White, Canvas->SizeX - 320.f, 30.f, nullptr, 1.1f);
	DrawBar(Canvas->SizeX - 320.f, 60.f, 280.f, 6.f, Mode->GetPhaseFraction(),
		FLinearColor(0.85f, 0.64f, 0.25f), TEXT(""));

	// --- inventario (alto al centro) ---
	const FString Inv = FString::Printf(
		TEXT("Munizioni %d | Cibo %d | Bende %d | Legna %d"),
		Mode->Ammo, Mode->Food, Mode->Bandages, Mode->Wood);
	DrawText(Inv, FLinearColor(0.85f, 0.83f, 0.77f),
		Canvas->SizeX * 0.5f - 160.f, 30.f, nullptr, 1.f);

	// --- arma corrente (basso a destra) ---
	const FString WeaponText = (P->Weapon == EQCRWeapon::Bat)
		? TEXT("MAZZA (silenziosa)")
		: FString::Printf(TEXT("PISTOLA - %d colpi (RUMOROSA)"), Mode->Ammo);
	DrawText(WeaponText, FLinearColor(0.85f, 0.83f, 0.77f),
		Canvas->SizeX - 320.f, Canvas->SizeY - 60.f, nullptr, 1.1f);

	// --- mirino ---
	DrawText(TEXT("+"), FLinearColor(0.85f, 0.83f, 0.77f, 0.8f),
		Canvas->SizeX * 0.5f - 5.f, Canvas->SizeY * 0.5f - 8.f, nullptr, 1.2f);

	// --- log messaggi (basso a sinistra) ---
	float Y = Canvas->SizeY - 180.f;
	for (const FQCRMessage& M : Mode->Messages)
	{
		DrawText(M.Text, FLinearColor(0.85f, 0.83f, 0.77f,
			FMath::Clamp(M.TimeLeft, 0.f, 1.f)), 20.f, Y, nullptr, 1.f);
		Y += 22.f;
	}
}
