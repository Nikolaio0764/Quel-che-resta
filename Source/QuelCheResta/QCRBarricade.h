// QCRBarricade.h — Barricata costruibile e distruttibile.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QCRBarricade.generated.h"

class UStaticMeshComponent;

UCLASS()
class QUELCHERESTA_API AQCRBarricade : public AActor
{
	GENERATED_BODY()

public:
	AQCRBarricade();

	void ReceiveDamage(float Amount);

protected:
	float HP = 120.f;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
};
