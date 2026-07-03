// QCRCharacter.cpp
#include "QCRCharacter.h"
#include "QCRGameMode.h"
#include "QCRZombie.h"
#include "QCRLootCrate.h"
#include "QCRBarricade.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"   // <-- il sistema del rumore NATIVO di Unreal
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "QCRAudio.h"
#include "Components/PointLightComponent.h"

AQCRCharacter::AQCRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // altezza occhi
	Camera->bUsePawnControlRotation = true;

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Camera);
	Flashlight->SetIntensity(0.f);
	Flashlight->SetAttenuationRadius(2200.f);
	Flashlight->SetOuterConeAngle(26.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.95f, 0.8f));

	// --- viewmodel placeholder: cubi ingaggiati alla camera ---
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	BatModel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatModel"));
	BatModel->SetupAttachment(Camera);
	BatModel->SetRelativeLocation(FVector(55.f, 30.f, -25.f));
	BatModel->SetRelativeRotation(FRotator(-10.f, 15.f, 0.f));
	BatModel->SetRelativeScale3D(FVector(0.75f, 0.06f, 0.06f));
	BatModel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded()) BatModel->SetStaticMesh(CubeMesh.Object);

	PistolModel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PistolModel"));
	PistolModel->SetupAttachment(Camera);
	PistolModel->SetRelativeLocation(FVector(50.f, 28.f, -22.f));
	PistolModel->SetRelativeScale3D(FVector(0.28f, 0.05f, 0.09f));
	PistolModel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PistolModel->SetVisibility(false);
	if (CubeMesh.Succeeded()) PistolModel->SetStaticMesh(CubeMesh.Object);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Camera);
	MuzzleLight->SetRelativeLocation(FVector(90.f, 28.f, -18.f));
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetAttenuationRadius(600.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.5f));

	// movimento
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 220.f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void AQCRCharacter::BeginPlay()
{
	Super::BeginPlay();
}

AQCRGameMode* AQCRCharacter::GM() const
{
	return GetWorld()->GetAuthGameMode<AQCRGameMode>();
}

float AQCRCharacter::GetStealthFactor() const
{
	return bIsCrouched ? 0.45f : 1.f;
}

void AQCRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AQCRGameMode* Mode = GM();
	if (!Mode || Mode->bGameOver) return;

	AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaTime);

	// --- stamina + rumore di corsa (usa il sistema udito nativo di UE) ---
	const bool bMoving = GetVelocity().SizeSquared2D() > 100.f;
	if (bSprinting && bMoving && Stamina > 0.f)
	{
		Stamina = FMath::Clamp(Stamina - 20.f * DeltaTime, 0.f, 100.f);
		RunNoiseTimer += DeltaTime;
		if (RunNoiseTimer > 1.f)
		{
			RunNoiseTimer = 0.f;
			UAISense_Hearing::ReportNoiseEvent(
				GetWorld(), GetActorLocation(), 1.f, this, 1100.f); // 11 m
		}
		if (Stamina <= 0.f) StopSprint();
	}
	else
	{
		Stamina = FMath::Clamp(Stamina + 12.f * DeltaTime, 0.f, 100.f);
	}

	// --- animazioni delle armi (interpolazione, zero asset) ---
	SwingAnim = FMath::Max(0.f, SwingAnim - DeltaTime * 4.f);
	RecoilAnim = FMath::Max(0.f, RecoilAnim - DeltaTime * 6.f);
	BatModel->SetRelativeRotation(FRotator(
		-10.f - 60.f * FMath::Sin(SwingAnim * PI), 15.f, 0.f));
	PistolModel->SetRelativeLocation(FVector(
		50.f - 8.f * RecoilAnim, 28.f, -22.f + 3.f * RecoilAnim));
	MuzzleTimer = FMath::Max(0.f, MuzzleTimer - DeltaTime);
	if (MuzzleTimer <= 0.f)
		MuzzleLight->SetIntensity(0.f);

	// --- fame ---
	Hunger = FMath::Clamp(Hunger - 0.4f * DeltaTime, 0.f, 100.f);
	if (Hunger <= 0.f)
		HP -= 2.f * DeltaTime;

	// --- battito cardiaco sotto il 30% ---
	if (HP < 30.f)
	{
		HeartbeatTimer -= DeltaTime;
		if (HeartbeatTimer <= 0.f)
		{
			HeartbeatTimer = 1.0f;
			QCRAudio::Play2D(this, QCRAudio::ESound::Heartbeat, 0.6f);
		}
	}

	if (HP <= 0.f)
		Die();
}

// ==================================================================
//  INPUT
// ==================================================================
void AQCRCharacter::SetupPlayerInputComponent(UInputComponent* PIC)
{
	Super::SetupPlayerInputComponent(PIC);
	PIC->BindAxis("MoveForward", this, &AQCRCharacter::MoveForward);
	PIC->BindAxis("MoveRight", this, &AQCRCharacter::MoveRight);
	PIC->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PIC->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PIC->BindAction("Sprint", IE_Pressed, this, &AQCRCharacter::StartSprint);
	PIC->BindAction("Sprint", IE_Released, this, &AQCRCharacter::StopSprint);
	PIC->BindAction("CrouchAction", IE_Pressed, this, &AQCRCharacter::StartCrouching);
	PIC->BindAction("CrouchAction", IE_Released, this, &AQCRCharacter::StopCrouching);
	PIC->BindAction("Attack", IE_Pressed, this, &AQCRCharacter::Attack);
	PIC->BindAction("Interact", IE_Pressed, this, &AQCRCharacter::Interact);
	PIC->BindAction("Barricade", IE_Pressed, this, &AQCRCharacter::PlaceBarricade);
	PIC->BindAction("Flashlight", IE_Pressed, this, &AQCRCharacter::ToggleFlashlight);
	PIC->BindAction("SlotBat", IE_Pressed, this, &AQCRCharacter::EquipBat);
	PIC->BindAction("SlotPistol", IE_Pressed, this, &AQCRCharacter::EquipPistol);

	FInputActionBinding& PauseBind = PIC->BindAction(
		"PauseGame", IE_Pressed, this, &AQCRCharacter::PauseGame);
	PauseBind.bExecuteWhenPaused = true;
	FInputActionBinding& RestartBind = PIC->BindAction(
		"Restart", IE_Pressed, this, &AQCRCharacter::RestartGame);
	RestartBind.bExecuteWhenPaused = true;
}

void AQCRCharacter::PauseGame()
{
	if (AQCRGameMode* Mode = GM())
		Mode->TogglePause();
}

void AQCRCharacter::RestartGame()
{
	AQCRGameMode* Mode = GM();
	if (Mode && (Mode->bGameOver || Mode->bPaused))
		Mode->RestartGame();
}

void AQCRCharacter::MoveForward(float Value)
{
	if (Value != 0.f) AddMovementInput(GetActorForwardVector(), Value);
}

void AQCRCharacter::MoveRight(float Value)
{
	if (Value != 0.f) AddMovementInput(GetActorRightVector(), Value);
}

void AQCRCharacter::StartSprint()
{
	if (bIsCrouched) return;
	bSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AQCRCharacter::StopSprint()
{
	bSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AQCRCharacter::StartCrouching() { Crouch(); }
void AQCRCharacter::StopCrouching()  { UnCrouch(); }

void AQCRCharacter::ToggleFlashlight()
{
	bFlashlightOn = !bFlashlightOn;
	Flashlight->SetIntensity(bFlashlightOn ? 8000.f : 0.f);
}

void AQCRCharacter::EquipBat()
{
	Weapon = EQCRWeapon::Bat;
	BatModel->SetVisibility(true);
	PistolModel->SetVisibility(false);
}

void AQCRCharacter::EquipPistol()
{
	Weapon = EQCRWeapon::Pistol;
	BatModel->SetVisibility(false);
	PistolModel->SetVisibility(true);
}

void AQCRCharacter::Attack()
{
	if (Weapon == EQCRWeapon::Bat) MeleeAttack();
	else Shoot();
}

// ==================================================================
//  COMBATTIMENTO
// ==================================================================
void AQCRCharacter::MeleeAttack()
{
	if (AttackCooldown > 0.f) return;
	AttackCooldown = 0.55f;
	SwingAnim = 1.f;
	QCRAudio::Play2D(this, QCRAudio::ESound::Swing, 0.7f);

	// rumore lieve (6 m)
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this, 600.f);

	// colpisce i vaganti in un cono frontale corto
	const FVector Fwd = Camera->GetForwardVector().GetSafeNormal2D();
	for (TActorIterator<AQCRZombie> It(GetWorld()); It; ++It)
	{
		AQCRZombie* Z = *It;
		FVector ToZ = Z->GetActorLocation() - GetActorLocation();
		const float Dist = ToZ.Size2D();
		ToZ.Normalize();
		if (Dist < 240.f && FVector::DotProduct(ToZ, Fwd) > 0.35f)
		{
			Z->ReceiveDamage(40.f);
			QCRAudio::Play2D(this, QCRAudio::ESound::Hit, 0.8f);
		}
	}
}

void AQCRCharacter::Shoot()
{
	if (AttackCooldown > 0.f) return;
	AQCRGameMode* Mode = GM();
	if (!Mode) return;
	if (Mode->Ammo <= 0)
	{
		Mode->PushMessage(TEXT("Click. Niente munizioni."));
		return;
	}
	AttackCooldown = 0.35f;
	--Mode->Ammo;
	RecoilAnim = 1.f;
	MuzzleTimer = 0.06f;
	MuzzleLight->SetIntensity(15000.f);
	QCRAudio::Play2D(this, QCRAudio::ESound::Gunshot, 0.9f);

	// IL RUMORE: 48 metri. Ogni vagante che "sente" viene a investigare.
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this, 4800.f);
	Mode->PushMessage(TEXT("BANG - il rumore li attirera'."));

	// line trace dal centro dello schermo
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * 7000.f;
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		if (AQCRZombie* Z = Cast<AQCRZombie>(Hit.GetActor()))
		{
			Z->ReceiveDamage(100.f);
			QCRAudio::Play2D(this, QCRAudio::ESound::Hit, 0.8f);
		}
	}
}

// ==================================================================
//  INTERAZIONI
// ==================================================================
void AQCRCharacter::Interact()
{
	AQCRGameMode* Mode = GM();
	if (!Mode) return;

	// 1) cassa vicina?
	for (TActorIterator<AQCRLootCrate> It(GetWorld()); It; ++It)
	{
		AQCRLootCrate* C = *It;
		if (!C->bLooted &&
			FVector::Dist(C->GetActorLocation(), GetActorLocation()) < 260.f)
		{
			C->Loot(Mode);
			return;
		}
	}
	// 2) mangia / medica
	if (Hunger < 70.f && Mode->Food > 0)
	{
		--Mode->Food;
		Hunger = FMath::Clamp(Hunger + 38.f, 0.f, 100.f);
		QCRAudio::Play2D(this, QCRAudio::ESound::Eat, 0.7f);
		Mode->PushMessage(TEXT("Hai mangiato. Fame +38."));
		return;
	}
	if (HP < 75.f && Mode->Bandages > 0)
	{
		--Mode->Bandages;
		HP = FMath::Clamp(HP + 30.f, 0.f, 100.f);
		Mode->PushMessage(TEXT("Ferita medicata. Salute +30."));
		return;
	}
	Mode->PushMessage(TEXT("Niente da fare qui."));
}

void AQCRCharacter::PlaceBarricade()
{
	AQCRGameMode* Mode = GM();
	if (!Mode) return;
	if (Mode->Wood < 2)
	{
		Mode->PushMessage(TEXT("Serve legna (x2)."));
		return;
	}
	Mode->Wood -= 2;
	FVector Fwd = Camera->GetForwardVector().GetSafeNormal2D();
	FVector Pos = GetActorLocation() + Fwd * 220.f;
	Pos.Z = 60.f;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<AQCRBarricade>(
		AQCRBarricade::StaticClass(), Pos, FRotator(0.f, GetActorRotation().Yaw, 0.f), SP);
	QCRAudio::Play2D(this, QCRAudio::ESound::Knock, 0.7f);
	Mode->PushMessage(TEXT("Barricata piazzata."));
}

void AQCRCharacter::ReceiveDamage(float Amount)
{
	HP -= Amount;
	if (AQCRGameMode* Mode = GM())
		Mode->HurtFlash = 0.45f;   // flash rosso sull'HUD
	if (HP <= 0.f) Die();
}

void AQCRCharacter::Die()
{
	if (AQCRGameMode* Mode = GM())
		Mode->TriggerGameOver();
}
