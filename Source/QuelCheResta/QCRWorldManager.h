// QCRWorldManager.h — Genera il mondo greybox, gestisce sole/luna, spawn e orde.
// Piazzane UNO nel livello (o lascialo spawnare dal GameMode).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QCRWorldManager.generated.h"

class UDirectionalLightComponent;
class AExponentialHeightFog;
class AQCRGameMode;

UCLASS()
class QUELCHERESTA_API AQCRWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AQCRWorldManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Mondo")
	float WorldHalfSize = 12000.f; // cm (120 m per lato dal centro)

	UPROPERTY(EditAnywhere, Category = "Mondo")
	int32 BuildingCount = 22;

	UPROPERTY(EditAnywhere, Category = "Mondo")
	int32 InitialZombies = 14;

protected:
	UPROPERTY(VisibleAnywhere)
	UDirectionalLightComponent* Sun;

	UPROPERTY()
	AExponentialHeightFog* Fog = nullptr;

	float SpawnTimer = 0.f;

	void BuildWorld();
	void BuildRoadsAndCars();
	void BuildTrees();
	void BuildCamps();
	void SpawnBox(const FVector& Pos, const FVector& Scale, const FLinearColor& Color);
	void SpawnShape(const TCHAR* MeshPath, const FVector& Pos, const FVector& Scale,
		const FLinearColor& Color, bool bCollide);
	void SpawnCrate(const FVector& Pos);
	void SpawnZombie(bool bAnywhere, const FVector& NearPos = FVector::ZeroVector,
		bool bUseNearPos = false);
	void OnDayChanged(int32 Day, bool bNight);
	void UpdateSun(float DeltaTime);

	AQCRGameMode* GM() const;
};
