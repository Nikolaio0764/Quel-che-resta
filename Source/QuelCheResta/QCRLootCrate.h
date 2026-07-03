// QCRLootCrate.h — Cassa perquisibile con loot casuale.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QCRLootCrate.generated.h"

class UStaticMeshComponent;
class AQCRGameMode;

UCLASS()
class QUELCHERESTA_API AQCRLootCrate : public AActor
{
	GENERATED_BODY()

public:
	AQCRLootCrate();

	bool bLooted = false;

	void Loot(AQCRGameMode* Mode);

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
};
