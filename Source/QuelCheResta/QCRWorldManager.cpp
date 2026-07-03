// QCRWorldManager.cpp
#include "QCRWorldManager.h"
#include "QCRGameMode.h"
#include "QCRZombie.h"
#include "QCRLootCrate.h"
#include "QCRCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

AQCRWorldManager::AQCRWorldManager()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetIntensity(6.f);
	Sun->SetRelativeRotation(FRotator(-45.f, -30.f, 0.f));
	Sun->SetCastShadows(true);
	// "atmosphere sun light" da' cielo dinamico se c'e' una SkyAtmosphere nel livello
	Sun->SetAtmosphereSunLight(true);
}

AQCRGameMode* AQCRWorldManager::GM() const
{
	return GetWorld()->GetAuthGameMode<AQCRGameMode>();
}

void AQCRWorldManager::BeginPlay()
{
	Super::BeginPlay();
	BuildWorld();

	// nebbia (si infittisce di notte)
	Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(
		AExponentialHeightFog::StaticClass(),
		FVector::ZeroVector, FRotator::ZeroRotator);
	if (Fog)
		Fog->GetComponent()->SetFogDensity(0.005f);

	for (int32 i = 0; i < InitialZombies; ++i)
		SpawnZombie(true);
	if (AQCRGameMode* Mode = GM())
		Mode->OnDayChanged.AddUObject(this, &AQCRWorldManager::OnDayChanged);
}

void AQCRWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AQCRGameMode* Mode = GM();
	if (!Mode || Mode->bGameOver) return;

	UpdateSun(DeltaTime);

	// --- spawner con cap crescente ---
	SpawnTimer += DeltaTime;
	const float Rate = Mode->bIsNight ? 4.f : 8.f;
	if (SpawnTimer > Rate)
	{
		SpawnTimer = 0.f;
		TArray<AActor*> Zombies;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(),
			AQCRZombie::StaticClass(), Zombies);
		const int32 Cap = 14 + Mode->Day * 4 + (Mode->bIsNight ? 10 : 0);
		if (Zombies.Num() < Cap)
			SpawnZombie(false);
	}
}

void AQCRWorldManager::UpdateSun(float DeltaTime)
{
	AQCRGameMode* Mode = GM();
	if (!Mode) return;
	const float F = Mode->GetPhaseFraction();
	if (!Mode->bIsNight)
	{
		// arco del sole: alba -> tramonto
		const float Pitch = FMath::Lerp(-155.f, -25.f, F);
		Sun->SetWorldRotation(FRotator(Pitch, -30.f, 0.f));
		const float Target = (F > 0.85f) ? FMath::Lerp(6.f, 0.6f, (F - 0.85f) / 0.15f) : 6.f;
		Sun->SetIntensity(FMath::FInterpTo(Sun->Intensity, Target, DeltaTime, 1.5f));
	}
	else
	{
		// "luna": luce fioca e fredda quasi orizzontale
		Sun->SetWorldRotation(FRotator(-40.f, 140.f, 0.f));
		Sun->SetIntensity(FMath::FInterpTo(Sun->Intensity, 0.35f, DeltaTime, 0.8f));
		Sun->SetLightColor(FLinearColor(0.55f, 0.62f, 0.85f));
	}

	if (Fog)
	{
		const float TargetFog = Mode->bIsNight ? 0.035f : 0.005f;
		UExponentialHeightFogComponent* FC = Fog->GetComponent();
		FC->SetFogDensity(FMath::FInterpTo(FC->FogDensity, TargetFog, DeltaTime, 0.5f));
	}
}

// ==================================================================
//  GENERAZIONE DEL MONDO
// ==================================================================
void AQCRWorldManager::BuildWorld()
{
	// pavimento
	SpawnBox(FVector(0.f, 0.f, -50.f),
		FVector(WorldHalfSize / 50.f, WorldHalfSize / 50.f, 1.f),
		FLinearColor(0.16f, 0.19f, 0.14f));

	// edifici + cassa accanto a ciascuno
	for (int32 i = 0; i < BuildingCount; ++i)
	{
		const float W = FMath::FRandRange(600.f, 1500.f);
		const float D = FMath::FRandRange(600.f, 1500.f);
		const float H = FMath::FRandRange(400.f, 1000.f);
		const float X = FMath::FRandRange(-WorldHalfSize + 1400.f, WorldHalfSize - 1400.f);
		const float Y = FMath::FRandRange(-WorldHalfSize + 1400.f, WorldHalfSize - 1400.f);
		if (FVector2D(X, Y).Size() < 2000.f) continue; // spawn libero al centro

		SpawnBox(FVector(X, Y, H / 2.f),
			FVector(W / 100.f, D / 100.f, H / 100.f),
			FLinearColor(0.28f, 0.28f, 0.25f));
		SpawnCrate(FVector(X + W / 2.f + 150.f, Y, 50.f));
	}

	BuildRoadsAndCars();
	BuildTrees();
	BuildCamps();

	// casse sparse
	for (int32 i = 0; i < 10; ++i)
	{
		SpawnCrate(FVector(
			FMath::FRandRange(-WorldHalfSize + 800.f, WorldHalfSize - 800.f),
			FMath::FRandRange(-WorldHalfSize + 800.f, WorldHalfSize - 800.f),
			50.f));
	}
}

void AQCRWorldManager::SpawnShape(const TCHAR* MeshPath, const FVector& Pos,
	const FVector& Scale, const FLinearColor& Color, bool bCollide)
{
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
	if (!A) return;
	A->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* MC = A->GetStaticMeshComponent();
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	UMaterial* Mat = LoadObject<UMaterial>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Mesh) MC->SetStaticMesh(Mesh);
	if (Mat)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, MC);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MC->SetMaterial(0, MID);
	}
	A->SetActorScale3D(Scale);
	MC->SetCollisionEnabled(bCollide
		? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AQCRWorldManager::BuildRoadsAndCars()
{
	// due strade che si incrociano (piatte, senza collisione)
	SpawnShape(TEXT("/Engine/BasicShapes/Cube.Cube"),
		FVector(0.f, 0.f, 2.f),
		FVector(WorldHalfSize / 50.f, 7.f, 0.05f),
		FLinearColor(0.13f, 0.13f, 0.14f), false);
	SpawnShape(TEXT("/Engine/BasicShapes/Cube.Cube"),
		FVector(0.f, 0.f, 2.f),
		FVector(7.f, WorldHalfSize / 50.f, 0.05f),
		FLinearColor(0.13f, 0.13f, 0.14f), false);

	// auto abbandonate lungo le strade
	const FLinearColor Tones[] = {
		FLinearColor(0.35f, 0.15f, 0.12f),
		FLinearColor(0.20f, 0.25f, 0.30f),
		FLinearColor(0.30f, 0.30f, 0.28f) };
	for (int32 i = 0; i < 9; ++i)
	{
		const bool bAlongX = FMath::FRand() < 0.5f;
		const float S = FMath::FRandRange(-WorldHalfSize + 1500.f, WorldHalfSize - 1500.f);
		const float Off = FMath::FRandRange(-220.f, 220.f);
		const FVector Pos = bAlongX ? FVector(S, Off, 55.f) : FVector(Off, S, 55.f);
		const FVector CarScale = bAlongX
			? FVector(4.2f, 1.9f, 1.1f) : FVector(1.9f, 4.2f, 1.1f);
		SpawnBox(Pos, CarScale, Tones[FMath::RandRange(0, 2)]);
		// abitacolo
		SpawnShape(TEXT("/Engine/BasicShapes/Cube.Cube"),
			Pos + FVector(0.f, 0.f, 95.f),
			bAlongX ? FVector(2.0f, 1.7f, 0.8f) : FVector(1.7f, 2.0f, 0.8f),
			FLinearColor(0.12f, 0.14f, 0.16f), false);
		if (FMath::FRand() < 0.6f)
			SpawnCrate(Pos + (bAlongX ? FVector(0.f, 260.f, -5.f)
				: FVector(260.f, 0.f, -5.f)));
	}
}

void AQCRWorldManager::BuildTrees()
{
	for (int32 i = 0; i < 60; ++i)
	{
		const float X = FMath::FRandRange(-WorldHalfSize + 500.f, WorldHalfSize - 500.f);
		const float Y = FMath::FRandRange(-WorldHalfSize + 500.f, WorldHalfSize - 500.f);
		if (FMath::Abs(X) < 600.f || FMath::Abs(Y) < 600.f
			|| FVector2D(X, Y).Size() < 1600.f)
			continue;
		// tronco (con collisione) + chioma (senza)
		SpawnShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
			FVector(X, Y, 175.f), FVector(0.6f, 0.6f, 3.5f),
			FLinearColor(0.25f, 0.19f, 0.13f), true);
		const float R = FMath::FRandRange(1.6f, 2.6f);
		SpawnShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"),
			FVector(X, Y, 350.f + R * 50.f), FVector(R * 1.4f, R * 1.4f, R * 1.6f),
			FLinearColor(0.14f, 0.22f, 0.11f), false);
	}
}

void AQCRWorldManager::BuildCamps()
{
	// accampamenti abbandonati: cerchio di pietre + casse (loot concentrato)
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = FMath::FRandRange(-WorldHalfSize + 2000.f, WorldHalfSize - 2000.f);
		const float Y = FMath::FRandRange(-WorldHalfSize + 2000.f, WorldHalfSize - 2000.f);
		if (FVector2D(X, Y).Size() < 3000.f) continue;
		for (int32 j = 0; j < 6; ++j)
		{
			const float A = 2.f * PI * j / 6.f;
			SpawnBox(FVector(X + FMath::Cos(A) * 120.f,
				Y + FMath::Sin(A) * 120.f, 20.f),
				FVector(0.5f, 0.5f, 0.4f), FLinearColor(0.32f, 0.32f, 0.32f));
		}
		const int32 NCrates = 2 + FMath::RandRange(0, 1);
		for (int32 j = 0; j < NCrates; ++j)
			SpawnCrate(FVector(X + FMath::FRandRange(-300.f, 300.f),
				Y + FMath::FRandRange(-300.f, 300.f), 50.f));
	}
}

void AQCRWorldManager::SpawnBox(const FVector& Pos, const FVector& Scale,
	const FLinearColor& Color)
{
	AStaticMeshActor* Box = GetWorld()->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
	if (!Box) return;
	Box->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* MC = Box->GetStaticMeshComponent();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterial* Mat = LoadObject<UMaterial>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cube) MC->SetStaticMesh(Cube);
	if (Mat)
	{
		UMaterialInstanceDynamic* MID =
			UMaterialInstanceDynamic::Create(Mat, MC);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MC->SetMaterial(0, MID);
	}
	Box->SetActorScale3D(Scale);
	MC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AQCRWorldManager::SpawnCrate(const FVector& Pos)
{
	GetWorld()->SpawnActor<AQCRLootCrate>(
		AQCRLootCrate::StaticClass(), Pos, FRotator::ZeroRotator);
}

void AQCRWorldManager::SpawnZombie(bool bAnywhere, const FVector& NearPos,
	bool bUseNearPos)
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	FVector Pos;
	if (bUseNearPos)
	{
		Pos = NearPos + FVector(FMath::FRandRange(-600.f, 600.f),
			FMath::FRandRange(-600.f, 600.f), 100.f);
	}
	else
	{
		int32 Tries = 0;
		do
		{
			Pos = FVector(
				FMath::FRandRange(-WorldHalfSize + 600.f, WorldHalfSize - 600.f),
				FMath::FRandRange(-WorldHalfSize + 600.f, WorldHalfSize - 600.f),
				100.f);
			++Tries;
		} while (!bAnywhere && Player && Tries < 30 &&
			FVector::Dist2D(Pos, Player->GetActorLocation()) < 4000.f);
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AQCRZombie* Z = GetWorld()->SpawnActorDeferred<AQCRZombie>(
		AQCRZombie::StaticClass(),
		FTransform(FRotator::ZeroRotator, Pos));
	if (Z)
	{
		// varianti: 8% bruto, 15% corridore
		const float R = FMath::FRand();
		if (R < 0.08f) Z->Kind = 2;
		else if (R < 0.23f) Z->Kind = 1;
		Z->FinishSpawning(FTransform(FRotator::ZeroRotator, Pos));
	}
}

void AQCRWorldManager::OnDayChanged(int32 Day, bool bNight)
{
	if (!bNight) return;

	// ORDA NOTTURNA dal bordo mappa
	const float Edge = WorldHalfSize - 800.f;
	FVector Origin;
	switch (FMath::RandRange(0, 3))
	{
	case 0: Origin = FVector(FMath::FRandRange(-Edge, Edge), -Edge, 100.f); break;
	case 1: Origin = FVector(FMath::FRandRange(-Edge, Edge), Edge, 100.f); break;
	case 2: Origin = FVector(-Edge, FMath::FRandRange(-Edge, Edge), 100.f); break;
	default: Origin = FVector(Edge, FMath::FRandRange(-Edge, Edge), 100.f); break;
	}
	const int32 Count = 5 + Day * 2;
	for (int32 i = 0; i < Count; ++i)
		SpawnZombie(true, Origin, true);

	// mezzo secondo dopo, l'orda "sente" la posizione del giocatore
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, [this]()
	{
		if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
		{
			UAISense_Hearing::ReportNoiseEvent(GetWorld(),
				P->GetActorLocation(), 1.f, P, 50000.f);
			if (AQCRGameMode* Mode = GM())
				Mode->PushMessage(TEXT("Senti un'orda in lontananza. Si sta muovendo."));
		}
	}, 0.5f, false);
}
