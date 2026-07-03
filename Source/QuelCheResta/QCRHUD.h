// QCRHUD.h — HUD disegnato interamente in C++ (Canvas): niente asset UMG.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "QCRHUD.generated.h"

UCLASS()
class QUELCHERESTA_API AQCRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawBar(float X, float Y, float W, float H,
		float Fraction, const FLinearColor& Color, const FString& Label);
};
