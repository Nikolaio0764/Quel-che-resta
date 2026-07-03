// QCRZombieController.cpp
#include "QCRZombieController.h"
#include "QCRZombie.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"

AQCRZombieController::AQCRZombieController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	// Il raggio massimo qui e' un tetto: il raggio EFFETTIVO di ogni rumore
	// lo decide chi lo emette (ReportNoiseEvent con MaxRange).
	HearingConfig->HearingRange = 6000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(UAISenseConfig_Hearing::StaticClass());
	Perception->OnTargetPerceptionUpdated.AddDynamic(
		this, &AQCRZombieController::OnPerception);
}

void AQCRZombieController::OnPerception(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed()) return;
	if (AQCRZombie* Z = Cast<AQCRZombie>(GetPawn()))
	{
		// il vagante "sente" e va a investigare il PUNTO del rumore,
		// non chi lo ha fatto: esattamente come nella serie.
		Z->HearNoise(Stimulus.StimulusLocation);
	}
}
