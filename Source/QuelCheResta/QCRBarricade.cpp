// QCRBarricade.cpp
#include "QCRBarricade.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AQCRBarricade::AQCRBarricade()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetRelativeScale3D(FVector(2.4f, 0.4f, 1.2f));
	if (CubeMesh.Succeeded()) Mesh->SetStaticMesh(CubeMesh.Object);
	// canale WorldDynamic: e' quello che il line trace del vagante cerca
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AQCRBarricade::ReceiveDamage(float Amount)
{
	HP -= Amount;
	if (HP <= 0.f)
		Destroy();
}
