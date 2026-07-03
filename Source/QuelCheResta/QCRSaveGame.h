// QCRSaveGame.h — Persistenza del record (giorni sopravvissuti).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "QCRSaveGame.generated.h"

UCLASS()
class QUELCHERESTA_API UQCRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 BestDays = 0;
};
