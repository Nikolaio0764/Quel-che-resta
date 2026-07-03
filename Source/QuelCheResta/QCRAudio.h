// QCRAudio.h — Audio sintetizzato a RUNTIME in C++: niente asset .uasset.
// Porta in UE5 i suoni procedurali della versione Godot (generati con Python):
// stessa sintesi, stesso carattere, ma calcolata al volo dal gioco.
#pragma once

#include "CoreMinimal.h"

class USoundWaveProcedural;
class UObject;

namespace QCRAudio
{
	enum class ESound : uint8
	{
		Gunshot, Groan, Hit, Swing, Eat, Knock, Heartbeat
	};

	// Riproduce un suono 2D (interfaccia/giocatore)
	void Play2D(UObject* WorldContext, ESound Sound, float Volume = 1.f);

	// Riproduce un suono posizionale nel mondo (es. versi dei vaganti)
	void PlayAt(UObject* WorldContext, ESound Sound, const FVector& Location,
		float Volume = 1.f);
}
