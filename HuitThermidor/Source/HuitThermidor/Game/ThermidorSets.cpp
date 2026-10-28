#include "Game/ThermidorSets.h"
#include "Game/Interactif.h"
#include "Dialogue/DialogueSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// Palette : bois sombre, plâtre sale, pierre de Paris, tissus usés.
	const FLinearColor Plancher(0.11f, 0.07f, 0.045f);
	const FLinearColor Platre(0.42f, 0.38f, 0.32f);
	const FLinearColor Bois(0.18f, 0.11f, 0.06f);
	const FLinearColor BoisClair(0.30f, 0.20f, 0.11f);
	const FLinearColor Papier(0.70f, 0.64f, 0.52f);
	const FLinearColor Pierre(0.47f, 0.43f, 0.36f);
	const FLinearColor Pave(0.20f, 0.19f, 0.18f);
	const FLinearColor Bleu(0.10f, 0.14f, 0.36f);
	const FLinearColor Blanc(0.80f, 0.78f, 0.72f);
	const FLinearColor Rouge(0.55f, 0.08f, 0.07f);
	const FLinearColor Fer(0.12f, 0.12f, 0.13f);

	struct FBuilder
	{
		UWorld* World;
		TArray<AActor*>& Out;

		AStaticMeshActor* Shape(const TCHAR* Mesh, const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator)
		{
			AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(Center, Rot);
			if (!A) { return nullptr; }
			A->SetMobility(EComponentMobility::Movable);
			UStaticMeshComponent* C = A->GetStaticMeshComponent();
			const FString Path = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Mesh, Mesh);
			C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path));
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
			{
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, A);
				Mid->SetVectorParameterValue(TEXT("Color"), Color);
				C->SetMaterial(0, Mid);
			}
			A->SetActorScale3D(SizeCm / 100.f);
			Out.Add(A);
			return A;
		}

		AStaticMeshActor* Box(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator)
		{
			return Shape(TEXT("Cube"), Center, SizeCm, Color, Rot);
		}

		APointLight* Light(const FVector& Where, float Candelas, const FLinearColor& Color, float Radius)
		{
			APointLight* L = World->SpawnActor<APointLight>(Where, FRotator::ZeroRotator);
			if (!L) { return nullptr; }
			UPointLightComponent* C = L->PointLightComponent;
			C->SetMobility(EComponentMobility::Movable);
			C->SetIntensityUnits(ELightUnits::Candelas);
			C->SetIntensity(Candelas);
			C->SetLightColor(Color);
			C->SetAttenuationRadius(Radius);
			C->SetSourceRadius(2.f);
			Out.Add(L);
			return L;
		}

		AInteractif* Person(const FVector& Feet, float Yaw, const FString& Name, const FLinearColor& Coat, TArray<FString> Targets, bool bSeated = false)
		{
			AInteractif* P = World->SpawnActor<AInteractif>(Feet, FRotator(0.f, Yaw, 0.f));
			if (!P) { return nullptr; }
			P->SetupAsPerson(Name, Coat, 172.f, bSeated);
			P->TargetNodes = MoveTemp(Targets);
			Out.Add(P);
			return P;
		}

		AInteractif* Thing(const FVector& Center, const FString& Name, const FString& Verb, const TCHAR* Mesh, const FVector& SizeCm, const FLinearColor& Color, TArray<FString> Targets, const FRotator& Rot = FRotator::ZeroRotator)
		{
			AInteractif* T = World->SpawnActor<AInteractif>(Center, Rot);
			if (!T) { return nullptr; }
			T->SetupAsObject(Name, Mesh, SizeCm, Color);
			T->Verb = Verb;
			T->TargetNodes = MoveTemp(Targets);
			Out.Add(T);
			return T;
		}

		void Mood(float ExposureBias, const FLinearColor& Tint)
		{
			APostProcessVolume* V = World->SpawnActor<APostProcessVolume>();
			if (!V) { return; }
			V->bUnbound = true;
			FPostProcessSettings& S = V->Settings;
			S.bOverride_AutoExposureBias = true;           S.AutoExposureBias = ExposureBias;
			S.bOverride_AutoExposureMinBrightness = true;  S.AutoExposureMinBrightness = 0.03f;
			S.bOverride_AutoExposureMaxBrightness = true;  S.AutoExposureMaxBrightness = 2.f;
			S.bOverride_VignetteIntensity = true;          S.VignetteIntensity = 0.6f;
			S.bOverride_FilmGrainIntensity = true;         S.FilmGrainIntensity = 0.25f;
			S.bOverride_ColorSaturation = true;            S.ColorSaturation = FVector4(0.75f, 0.75f, 0.75f, 1.f);
			S.bOverride_SceneColorTint = true;             S.SceneColorTint = Tint;
			Out.Add(V);
		}
	};
}

ThermidorSets::FSetResult ThermidorSets::BuildComite(UWorld* World, TArray<AActor*>& Out)
{
	FBuilder B{ World, Out };
	// Salle de 8 m x 6 m, 3,2 m sous plafond. Origine au centre du plancher.
	const float W = 800.f, D = 600.f, H = 320.f, T = 20.f;
	B.Box(FVector(0, 0, -T * 0.5f), FVector(W, D, T), Plancher);
	B.Box(FVector(0, 0, H + T * 0.5f), FVector(W, D, T), FLinearColor(0.08f, 0.07f, 0.06f));
	B.Box(FVector(0, -D * 0.5f, H * 0.5f), FVector(W, T, H), Platre);			// mur nord
	B.Box(FVector(-W * 0.5f, 0, H * 0.5f), FVector(T, D, H), Platre);			// mur ouest
	B.Box(FVector(W * 0.5f, 0, H * 0.5f), FVector(T, D, H), Platre);			// mur est (fenêtre plus bas)
	// Mur sud avec une porte de 1,2 m
	B.Box(FVector(-W * 0.25f - 30.f, D * 0.5f, H * 0.5f), FVector(W * 0.5f - 60.f, T, H), Platre);
	B.Box(FVector(W * 0.25f + 30.f, D * 0.5f, H * 0.5f), FVector(W * 0.5f - 60.f, T, H), Platre);
	B.Box(FVector(0, D * 0.5f, 260.f + 30.f), FVector(120.f, T, 60.f), Platre);
	B.Thing(FVector(0, D * 0.5f + 5.f, 130.f), TEXT("La porte"), TEXT("Répondre à"), TEXT("Cube"), FVector(118.f, 8.f, 258.f), Bois, { TEXT("messager") });

	// Affiche de la loi des suspects et registres sur des étagères
	B.Box(FVector(-150.f, -D * 0.5f + 12.f, 180.f), FVector(70.f, 2.f, 95.f), Papier);
	for (int32 i = 0; i < 4; ++i)
	{
		B.Box(FVector(-W * 0.5f + 30.f, -150.f + i * 90.f, 120.f), FVector(35.f, 70.f, 6.f), Bois);
		B.Box(FVector(-W * 0.5f + 30.f, -150.f + i * 90.f, 140.f), FVector(28.f, 60.f, 34.f), FLinearColor(0.25f, 0.13f, 0.08f));
	}

	// La table, ses registres, l'encrier renversé, la chandelle
	const FVector Table(40.f, -40.f, 0.f);
	B.Box(Table + FVector(0, 0, 74.f), FVector(200.f, 100.f, 6.f), BoisClair);
	for (const FVector& Leg : { FVector(-90, -42, 36), FVector(90, -42, 36), FVector(-90, 42, 36), FVector(90, 42, 36) })
	{
		B.Box(Table + Leg, FVector(8.f, 8.f, 72.f), Bois);
	}
	B.Box(Table + FVector(-55.f, 10.f, 81.f), FVector(40.f, 30.f, 8.f), FLinearColor(0.30f, 0.12f, 0.07f));
	B.Box(Table + FVector(-50.f, -25.f, 79.f), FVector(32.f, 24.f, 4.f), Papier, FRotator(0, 15, 0));
	B.Shape(TEXT("Cylinder"), Table + FVector(20.f, 25.f, 78.f), FVector(6.f, 6.f, 3.f), FLinearColor(0.02f, 0.02f, 0.03f), FRotator(90, 0, 0));
	B.Shape(TEXT("Cylinder"), Table + FVector(70.f, -20.f, 85.f), FVector(4.f, 4.f, 16.f), Papier);
	B.Light(Table + FVector(70.f, -20.f, 100.f), 6.f, FLinearColor(1.f, 0.62f, 0.32f), 700.f);

	// Le sabre posé sur la table
	B.Thing(Table + FVector(25.f, -15.f, 79.f), TEXT("Le sabre"), TEXT("Examiner"), TEXT("Cube"), FVector(95.f, 5.f, 3.f), Fer,
		{ TEXT("sabre_ok") }, FRotator(0.f, -20.f, 0.f));

	// Sous la table : la bouteille et l'écharpe roulée en boule
	AInteractif* Dessous = B.Thing(Table + FVector(-30.f, 20.f, 6.f), TEXT("Sous la table"), TEXT("Fouiller"), TEXT("Cylinder"),
		FVector(9.f, 9.f, 28.f), FLinearColor(0.10f, 0.16f, 0.08f), { TEXT("carte_ok") }, FRotator(90.f, 0.f, 30.f));
	if (Dessous)
	{
		Dessous->AddProp(TEXT("Cube"), FVector(0.f, 25.f, -2.f), FVector(10.f, 14.f, 8.f), Bleu);
		Dessous->AddProp(TEXT("Cube"), FVector(0.f, 36.f, -2.f), FVector(10.f, 14.f, 8.f), Blanc);
		Dessous->AddProp(TEXT("Cube"), FVector(0.f, 47.f, -2.f), FVector(10.f, 14.f, 8.f), Rouge);
	}

	// Marie-Anne Delorme, ligotée sur sa chaise, face à la table
	const FVector Chaise(40.f, 110.f, 0.f);
	B.Box(Chaise + FVector(0, 0, 45.f), FVector(48.f, 48.f, 5.f), Bois);
	B.Box(Chaise + FVector(0, 24.f, 90.f), FVector(48.f, 5.f, 90.f), Bois);
	B.Person(Chaise, -90.f, TEXT("Marie-Anne Delorme"), FLinearColor(0.36f, 0.34f, 0.31f), { TEXT("marie") }, true);

	// Fenêtre : un peu de jour gris qui entre par le mur est
	B.Box(FVector(W * 0.5f - 11.f, -120.f, 190.f), FVector(2.f, 110.f, 130.f), FLinearColor(0.35f, 0.42f, 0.55f));
	B.Light(FVector(W * 0.5f - 80.f, -120.f, 190.f), 3.f, FLinearColor(0.55f, 0.65f, 0.85f), 500.f);

	B.Mood(-0.5f, FLinearColor(1.f, 0.93f, 0.82f));

	FSetResult R;
	R.PlayerStart = FVector(-250.f, 150.f, 100.f);
	R.PlayerRotation = FRotator(-5.f, -30.f, 0.f);
	return R;
}

ThermidorSets::FSetResult ThermidorSets::BuildPlace(UWorld* World, TArray<AActor*>& Out, const UDialogueSubsystem* Dialogue)
{
	FBuilder B{ World, Out };
	// Place octogonale d'environ 120 m, simplifiée ici en carré de 60 m.
	const float S = 6000.f;
	B.Box(FVector(0, 0, -10.f), FVector(S, S, 20.f), Pave);
	// Façades à arcades tout autour
	const float FacadeH = 1800.f;
	B.Box(FVector(0, -S * 0.5f, FacadeH * 0.5f), FVector(S, 200.f, FacadeH), Pierre);
	B.Box(FVector(0, S * 0.5f, FacadeH * 0.5f), FVector(S, 200.f, FacadeH), Pierre);
	B.Box(FVector(-S * 0.5f, 0, FacadeH * 0.5f), FVector(200.f, S, FacadeH), Pierre);
	B.Box(FVector(S * 0.5f, 0, FacadeH * 0.5f), FVector(200.f, S, FacadeH), Pierre);
	for (int32 i = -6; i <= 6; ++i)
	{
		// Fenêtres sombres sur la façade nord
		B.Box(FVector(i * 420.f, -S * 0.5f + 101.f, 700.f), FVector(140.f, 4.f, 260.f), FLinearColor(0.04f, 0.04f, 0.05f));
		B.Box(FVector(i * 420.f, -S * 0.5f + 101.f, 1200.f), FVector(140.f, 4.f, 220.f), FLinearColor(0.04f, 0.04f, 0.05f));
	}

	// Le socle vide de la statue royale, couvert d'affiches, et le crieur dessus
	B.Box(FVector(0, 0, 160.f), FVector(500.f, 300.f, 320.f), Pierre);
	for (int32 i = 0; i < 5; ++i)
	{
		B.Box(FVector(-180.f + i * 90.f, 151.f, 160.f + (i % 2) * 30.f), FVector(60.f, 2.f, 80.f), Papier);
	}
	B.Person(FVector(0, 60.f, 320.f), 90.f, TEXT("Le crieur public"), FLinearColor(0.20f, 0.17f, 0.14f), { TEXT("crieur") });

	// La fontaine et le porteur d'eau
	const FVector Fontaine(1100.f, 700.f, 0.f);
	B.Shape(TEXT("Cylinder"), Fontaine + FVector(0, 0, 40.f), FVector(260.f, 260.f, 80.f), Pierre);
	B.Shape(TEXT("Cylinder"), Fontaine + FVector(0, 0, 150.f), FVector(40.f, 40.f, 220.f), Pierre);
	if (AInteractif* Porteur = B.Person(Fontaine + FVector(-200.f, -60.f, 0.f), 200.f, TEXT("Le porteur d'eau"), FLinearColor(0.28f, 0.20f, 0.13f), { TEXT("porteur") }))
	{
		Porteur->AddProp(TEXT("Sphere"), FVector(0.f, 0.f, 185.f), FVector(30.f, 30.f, 18.f), Rouge);	// bonnet rouge
		Porteur->AddProp(TEXT("Cylinder"), FVector(0.f, 45.f, 35.f), FVector(30.f, 30.f, 40.f), Bois);	// seau
	}

	// Brutus Mercier, devant une porte cochère, bandage à la mâchoire et écharpe tricolore
	const bool bVerification = Dialogue && Dialogue->HasFlag(TEXT("verification"));
	const FVector Porte(-1500.f, -S * 0.5f + 100.f, 0.f);
	B.Box(Porte + FVector(0, 2.f, 250.f), FVector(360.f, 6.f, 500.f), Bois);
	const FVector MercierPos = bVerification ? FVector(-1200.f, -1500.f, 0.f) : Porte + FVector(0, 120.f, 0.f);
	if (AInteractif* Mercier = B.Person(MercierPos, 90.f, TEXT("Brutus Mercier"), FLinearColor(0.08f, 0.10f, 0.22f), { TEXT("agent") }))
	{
		Mercier->AddProp(TEXT("Cylinder"), FVector(0.f, 0.f, 95.f), FVector(52.f, 52.f, 10.f), Rouge);	// écharpe
		Mercier->AddProp(TEXT("Cube"), FVector(12.f, 0.f, 150.f), FVector(8.f, 22.f, 8.f), Blanc);		// bandage
	}
	if (bVerification)
	{
		// Deux gardes nationaux et la charrette
		B.Person(MercierPos + FVector(-80.f, 90.f, 0.f), 90.f, TEXT("Garde national"), Bleu, {});
		B.Person(MercierPos + FVector(-80.f, -90.f, 0.f), 90.f, TEXT("Garde national"), Bleu, {});
		B.Box(MercierPos + FVector(-350.f, 0.f, 90.f), FVector(300.f, 160.f, 100.f), Bois);
	}

	// Lumière de l'aube : soleil bas, ciel, brume
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0, 0, 3000.f), FRotator(-8.f, 35.f, 0.f)))
	{
		UDirectionalLightComponent* C = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		C->SetMobility(EComponentMobility::Movable);
		C->SetIntensity(3.f);
		C->SetLightColor(FLinearColor(1.f, 0.72f, 0.50f));
		C->SetAtmosphereSunLight(true);
		Out.Add(Sun);
	}
	if (AActor* Sky = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity))
	{
		USkyAtmosphereComponent* Atmo = NewObject<USkyAtmosphereComponent>(Sky);
		Sky->SetRootComponent(Atmo);
		Atmo->RegisterComponent();
		Sky->AddInstanceComponent(Atmo);
		Out.Add(Sky);
	}
	if (ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector(0, 0, 500.f), FRotator::ZeroRotator))
	{
		USkyLightComponent* C = SkyLight->GetLightComponent();
		C->SetMobility(EComponentMobility::Movable);
		C->SetIntensity(0.8f);
		C->RecaptureSky();
		Out.Add(SkyLight);
	}
	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, 0.f), FRotator::ZeroRotator))
	{
		UExponentialHeightFogComponent* C = Fog->GetComponent();
		C->SetFogDensity(0.03f);
		C->SetFogInscatteringColor(FLinearColor(0.35f, 0.33f, 0.32f));
		C->SetVolumetricFog(true);
		Out.Add(Fog);
	}
	B.Mood(0.f, FLinearColor(0.95f, 0.95f, 1.f));

	FSetResult R;
	R.PlayerStart = FVector(1900.f, -900.f, 100.f);
	R.PlayerRotation = FRotator(0.f, 155.f, 0.f);
	return R;
}
