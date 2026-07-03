// QCRAudio.cpp — sintesi PCM 16-bit mono 22050 Hz, stessa "ricetta" della
// versione Godot. I buffer sono calcolati una volta e riusati.
#include "QCRAudio.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 SR = 22050;

	// --- utilita' ---
	float FRandN() // rumore bianco [-1,1]
	{
		return FMath::FRandRange(-1.f, 1.f);
	}

	void ToPCM(const TArray<float>& In, TArray<uint8>& Out)
	{
		Out.SetNumUninitialized(In.Num() * 2);
		int16* P = reinterpret_cast<int16*>(Out.GetData());
		for (int32 i = 0; i < In.Num(); ++i)
			P[i] = (int16)(FMath::Clamp(In[i], -1.f, 1.f) * 32767.f);
	}

	// --- le "ricette": traduzione 1:1 della sintesi Python del progetto Godot ---
	void SynthGunshot(TArray<float>& S)
	{
		const int32 N = int32(SR * 0.5f);
		S.SetNumZeroed(N);
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			const float noise = FRandN() * FMath::Exp(-t * 22.f);
			const float thump = 0.9f * FMath::Sin(2.f * PI * 70.f * t) * FMath::Exp(-t * 12.f);
			const float crack = FRandN() * FMath::Exp(-t * 90.f) * 1.4f;
			S[i] = (noise * 0.6f + thump + crack * 0.5f) * 0.9f;
		}
	}

	void SynthGroan(TArray<float>& S)
	{
		const int32 N = int32(SR * 1.5f);
		S.SetNumZeroed(N);
		float Phase = 0.f;
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			const float f = 85.f + 25.f * FMath::Sin(2.f * PI * 1.7f * t)
				+ 10.f * FMath::Sin(2.f * PI * 5.3f * t);
			Phase += 2.f * PI * f / SR;
			const float growl = FMath::Sign(FMath::Sin(Phase)) * 0.35f
				+ FMath::Sin(Phase) * 0.4f;
			const float rasp = FRandN() * 0.12f;
			const float env = FMath::Min(t / 0.15f, 1.f)
				* FMath::Exp(-FMath::Max(t - 0.9f, 0.f) * 4.f);
			S[i] = (growl + rasp) * env * 0.8f;
		}
	}

	void SynthHit(TArray<float>& S)
	{
		const int32 N = int32(SR * 0.18f);
		S.SetNumZeroed(N);
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			S[i] = (FMath::Sin(2.f * PI * 95.f * t) * FMath::Exp(-t * 30.f)
				+ FRandN() * FMath::Exp(-t * 60.f) * 0.5f) * 0.9f;
		}
	}

	void SynthSwing(TArray<float>& S)
	{
		const int32 N = int32(SR * 0.3f);
		S.SetNumZeroed(N);
		float A = 0.f;
		for (int32 i = 1; i < N; ++i)
		{
			const float t = float(i) / SR;
			const float c = 0.02f + 0.25f * (float(i) / N); // LPF mobile
			A += c * (FRandN() - A);
			S[i] = A * FMath::Exp(-FMath::Square(t - 0.15f) / 0.004f) * 2.2f;
		}
	}

	void SynthEat(TArray<float>& S)
	{
		const int32 N = int32(SR * 0.6f);
		S.SetNumZeroed(N);
		const float Starts[] = { 0.02f, 0.18f, 0.35f, 0.48f };
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			for (float St : Starts)
				if (t > St && t < St + 0.07f)
					S[i] += FRandN() * FMath::Exp(-(t - St) * 50.f);
			S[i] *= 0.7f;
		}
	}

	void SynthKnock(TArray<float>& S)
	{
		const int32 N = int32(SR * 0.5f);
		S.SetNumZeroed(N);
		const float Starts[] = { 0.02f, 0.22f };
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			for (float St : Starts)
				if (t > St)
				{
					S[i] += FMath::Sin(2.f * PI * 180.f * (t - St))
						* FMath::Exp(-(t - St) * 35.f);
					S[i] += FRandN() * FMath::Exp(-(t - St) * 80.f) * 0.4f;
				}
			S[i] *= 0.8f;
		}
	}

	void SynthHeartbeat(TArray<float>& S)
	{
		const int32 N = int32(SR * 1.0f);
		S.SetNumZeroed(N);
		const float Beats[][2] = { { 0.f, 1.f }, { 0.18f, 0.7f } };
		for (int32 i = 0; i < N; ++i)
		{
			const float t = float(i) / SR;
			for (auto& B : Beats)
				if (t >= B[0])
					S[i] += B[1] * FMath::Sin(2.f * PI * 55.f * (t - B[0]))
						* FMath::Exp(-(t - B[0]) * 22.f);
			S[i] *= 0.9f;
		}
	}

	// cache dei buffer PCM (calcolati alla prima richiesta)
	const TArray<uint8>& GetPCM(QCRAudio::ESound Sound)
	{
		static TArray<uint8> Cache[7];
		const int32 Idx = int32(Sound);
		if (Cache[Idx].Num() == 0)
		{
			TArray<float> S;
			switch (Sound)
			{
			case QCRAudio::ESound::Gunshot:   SynthGunshot(S); break;
			case QCRAudio::ESound::Groan:     SynthGroan(S); break;
			case QCRAudio::ESound::Hit:       SynthHit(S); break;
			case QCRAudio::ESound::Swing:     SynthSwing(S); break;
			case QCRAudio::ESound::Eat:       SynthEat(S); break;
			case QCRAudio::ESound::Knock:     SynthKnock(S); break;
			case QCRAudio::ESound::Heartbeat: SynthHeartbeat(S); break;
			}
			ToPCM(S, Cache[Idx]);
		}
		return Cache[Idx];
	}

	USoundWaveProcedural* MakeWave(QCRAudio::ESound Sound)
	{
		const TArray<uint8>& PCM = GetPCM(Sound);
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
		Wave->SetSampleRate(SR);
		Wave->NumChannels = 1;
		Wave->Duration = float(PCM.Num() / 2) / SR;
		Wave->SoundGroup = SOUNDGROUP_Default;
		Wave->bLooping = false;
		Wave->QueueAudio(PCM.GetData(), PCM.Num());
		return Wave;
	}
}

namespace QCRAudio
{
	void Play2D(UObject* WorldContext, ESound Sound, float Volume)
	{
		if (USoundWaveProcedural* W = MakeWave(Sound))
			UGameplayStatics::PlaySound2D(WorldContext, W, Volume);
	}

	void PlayAt(UObject* WorldContext, ESound Sound, const FVector& Location,
		float Volume)
	{
		if (USoundWaveProcedural* W = MakeWave(Sound))
			UGameplayStatics::PlaySoundAtLocation(WorldContext, W, Location, Volume);
	}
}
