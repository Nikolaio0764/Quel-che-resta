// QCRZombie.cpp
#include "QCRZombie.h"
#include "QCRCharacter.h"
#include "QCRBarricade.h"
#include "QCRGameMode.h"
#include "QCRZombieController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "QCRAudio.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

AQCRZombie::AQCRZombie()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	AIControllerClass = AQCRZombieController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// corpo placeholder: capsula-cilindro
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterial> BaseMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CylMesh.Succeeded()) BodyMesh->SetStaticMesh(CylMesh.Object);
	if (BaseMat.Succeeded()) BodyMesh->SetMaterial(0, BaseMat.Object);

	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
}

void AQCRZombie::BeginPlay()
{
	Super::BeginPlay();
	ApplyKind();
	BodyMID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
	SetTint(BaseColor);
	WanderTimer = FMath::FRandRange(1.f, 4.f);
	GroanTimer = FMath::FRandRange(3.f, 12.f);
}

void AQCRZombie::ApplyKind()
{
	switch (Kind)
	{
	case 1: // CORRIDORE
		HP = 60.f;
		BaseSpeed = FMath::FRandRange(300.f, 360.f);
		Damage = 10.f;
		BaseColor = FLinearColor(0.48f, 0.30f, 0.22f);
		break;
	case 2: // BRUTO
		HP = 250.f;
		BaseSpeed = FMath::FRandRange(100.f, 130.f);
		Damage = 25.f;
		BaseColor = FLinearColor(0.22f, 0.26f, 0.20f);
		SetActorScale3D(FVector(1.35f));
		break;
	default: // VAGANTE
		HP = 100.f;
		BaseSpeed = FMath::FRandRange(130.f, 210.f);
		Damage = 12.f;
		break;
	}
}

void AQCRZombie::SetTint(const FLinearColor& C)
{
	if (BodyMID)
		BodyMID->SetVectorParameterValue(TEXT("Color"), C);
}

void AQCRZombie::HearNoise(const FVector& Location)
{
	if (State == EZombieState::Chase) return;
	State = EZombieState::Investigate;
	NoiseTarget = Location;
}

void AQCRZombie::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AQCRGameMode* Mode = GetWorld()->GetAuthGameMode<AQCRGameMode>();
	if (!Mode || Mode->bGameOver) return;

	AttackTimer = FMath::Max(0.f, AttackTimer - DeltaTime);

	// --- verso posizionale ogni tanto: li SENTI prima di vederli ---
	GroanTimer -= DeltaTime;
	if (GroanTimer <= 0.f)
	{
		GroanTimer = FMath::FRandRange(4.f, 14.f);
		QCRAudio::PlayAt(this, QCRAudio::ESound::Groan, GetActorLocation(),
			Kind == 2 ? 1.0f : 0.7f);
	}

	AQCRCharacter* Player = Cast<AQCRCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	// --- "vista" per prossimita' (semplice), modulata dallo stealth ---
	float Detect = Mode->bIsNight ? 2000.f : 1200.f; // cm
	if (Player)
	{
		Detect *= Player->GetStealthFactor();
		if (Mode->bIsNight && Player->bFlashlightOn)
			Detect *= 1.7f;

		const float D = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
		if (D < Detect)
			State = EZombieState::Chase;
		else if (State == EZombieState::Chase && D > Detect * 2.5f + 1000.f)
			State = EZombieState::Wander;
	}

	FVector Target = GetActorLocation();
	float Speed = BaseSpeed;

	switch (State)
	{
	case EZombieState::Chase:
		if (Player)
		{
			Target = Player->GetActorLocation();
			Speed = BaseSpeed * 1.7f;
			SetTint(FLinearColor::LerpUsingHSV(BaseColor, FLinearColor(0.6f, 0.2f, 0.1f), 0.5f));
			if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) < 170.f
				&& AttackTimer <= 0.f)
			{
				AttackTimer = 1.1f;
				Player->ReceiveDamage(Damage);
			}
		}
		break;

	case EZombieState::Investigate:
		Target = NoiseTarget;
		SetTint(FLinearColor::LerpUsingHSV(BaseColor, FLinearColor(0.6f, 0.6f, 0.2f), 0.35f));
		if (FVector::Dist2D(GetActorLocation(), NoiseTarget) < 250.f)
			State = EZombieState::Wander;
		break;

	case EZombieState::Wander:
		SetTint(BaseColor);
		WanderTimer -= DeltaTime;
		if (WanderTimer <= 0.f)
		{
			WanderTimer = FMath::FRandRange(2.f, 5.f);
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			WanderDir = FVector(FMath::Cos(A), FMath::Sin(A), 0.f);
		}
		Target = GetActorLocation() + WanderDir * 400.f;
		Speed = BaseSpeed * 0.5f;
		break;
	}

	// --- movimento: PATHFINDING se c'e' un NavMeshBoundsVolume nel livello,
	//     altrimenti fallback in linea retta (funziona comunque) ---
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	bool bPathing = false;
	RepathTimer -= DeltaTime;
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (RepathTimer <= 0.f)
		{
			RepathTimer = 0.25f;
			const EPathFollowingRequestResult::Type R =
				AIC->MoveToLocation(Target, 60.f, true, true);
			bPathing = (R != EPathFollowingRequestResult::Failed);
		}
		else
		{
			bPathing = (AIC->GetMoveStatus() == EPathFollowingStatus::Moving);
		}
	}
	if (!bPathing)
	{
		FVector Dir = Target - GetActorLocation();
		Dir.Z = 0.f;
		if (Dir.SizeSquared() > 900.f)
		{
			Dir.Normalize();
			AddMovementInput(Dir, 1.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(),
				Dir.Rotation(), DeltaTime, 6.f));
		}
	}

	// --- se sbatte contro una barricata la attacca ---
	if (AttackTimer <= 0.f)
	{
		FHitResult Hit;
		FCollisionQueryParams P;
		P.AddIgnoredActor(this);
		const FVector S = GetActorLocation();
		const FVector E = S + GetActorForwardVector() * 130.f;
		if (GetWorld()->LineTraceSingleByChannel(Hit, S, E, ECC_WorldDynamic, P))
		{
			if (AQCRBarricade* B = Cast<AQCRBarricade>(Hit.GetActor()))
			{
				AttackTimer = 1.f;
				B->ReceiveDamage(25.f);
			}
		}
	}
}

void AQCRZombie::ReceiveDamage(float Amount)
{
	HP -= Amount;
	if (HP <= 0.f)
	{
		if (AQCRGameMode* Mode = GetWorld()->GetAuthGameMode<AQCRGameMode>())
			Mode->AddKill();
		Destroy();
	}
	else
	{
		SetTint(FLinearColor(0.9f, 0.9f, 0.85f)); // flash: il tick lo riporta
	}
}
