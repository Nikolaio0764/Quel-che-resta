// QCRLootCrate.cpp
#include "QCRLootCrate.h"
#include "QCRGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AQCRLootCrate::AQCRLootCrate()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetRelativeScale3D(FVector(0.9f));
	if (CubeMesh.Succeeded()) Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AQCRLootCrate::Loot(AQCRGameMode* Mode)
{
	if (bLooted || !Mode) return;
	bLooted = true;
	SetActorScale3D(FVector(0.6f)); // feedback visivo: cassa "svuotata"

	const float R = FMath::FRand();
	if (R < 0.30f)
	{
		++Mode->Food;
		Mode->PushMessage(TEXT("Trovata una scatoletta di cibo."));
	}
	else if (R < 0.50f)
	{
		Mode->Wood += 2;
		Mode->PushMessage(TEXT("Trovata legna (x2)."));
	}
	else if (R < 0.68f)
	{
		++Mode->Bandages;
		Mode->PushMessage(TEXT("Trovate delle bende."));
	}
	else if (R < 0.86f)
	{
		const int32 N = 1 + FMath::RandRange(0, 2);
		Mode->Ammo += N;
		Mode->PushMessage(FString::Printf(TEXT("Trovate munizioni (x%d)."), N));
	}
	else
	{
		Mode->PushMessage(TEXT("Vuoto. Qualcuno e' passato prima di te."));
	}
}
