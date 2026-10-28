#include "Game/Interactif.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Camera/PlayerCameraManager.h"

AInteractif::AInteractif()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(11.f);
	Label->SetTextRenderColor(FColor(226, 216, 198));
	Label->SetVisibility(false);
}

UStaticMeshComponent* AInteractif::AddShape(AActor* Owner, USceneComponent* Parent, const TCHAR* Shape, const FVector& RelativeLocation, const FVector& SizeCm, const FLinearColor& Color)
{
	const FString MeshPath = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(Parent);
	C->SetRelativeLocation(RelativeLocation);
	C->SetRelativeScale3D(SizeCm / 100.f);
	C->SetCollisionProfileName(TEXT("BlockAll"));
	if (BaseMat)
	{
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMat, C);
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		C->SetMaterial(0, Mid);
	}
	C->RegisterComponent();
	Owner->AddInstanceComponent(C);
	return C;
}

void AInteractif::SetupAsPerson(const FString& InName, const FLinearColor& Coat, float Height, bool bSeated)
{
	DisplayName = InName;
	const float BodyH = bSeated ? Height * 0.45f : Height * 0.82f;
	const float BaseZ = bSeated ? 45.f : 0.f;
	AddShape(this, Root, TEXT("Cylinder"), FVector(0.f, 0.f, BaseZ + BodyH * 0.5f), FVector(48.f, 48.f, BodyH), Coat);
	const float HeadZ = BaseZ + BodyH + 13.f;
	AddShape(this, Root, TEXT("Sphere"), FVector(0.f, 0.f, HeadZ), FVector(26.f, 26.f, 28.f), FLinearColor(0.80f, 0.66f, 0.56f));
	Label->SetText(FText::FromString(InName));
	Label->SetRelativeLocation(FVector(0.f, 0.f, HeadZ + 30.f));
}

void AInteractif::SetupAsObject(const FString& InName, const TCHAR* Shape, const FVector& SizeCm, const FLinearColor& Color)
{
	DisplayName = InName;
	AddShape(this, Root, Shape, FVector::ZeroVector, SizeCm, Color);
	Label->SetText(FText::FromString(InName));
	Label->SetRelativeLocation(FVector(0.f, 0.f, SizeCm.Z * 0.5f + 18.f));
}

void AInteractif::AddProp(const TCHAR* Shape, const FVector& RelativeLocation, const FVector& SizeCm, const FLinearColor& Color)
{
	UStaticMeshComponent* C = AddShape(this, Root, Shape, RelativeLocation, SizeCm, Color);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AInteractif::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// L'étiquette regarde toujours la caméra.
	if (Label && Label->IsVisible())
	{
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
			Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
		}
	}
}
