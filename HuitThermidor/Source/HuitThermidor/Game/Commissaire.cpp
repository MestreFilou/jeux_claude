#include "Game/Commissaire.h"
#include "Game/Interactif.h"
#include "Game/ThermidorPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

ACommissaire::ACommissaire()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.f;	// on marche, on ne court pas
	GetCharacterMovement()->BrakingDecelerationWalking = 1400.f;
}

void ACommissaire::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAxis(TEXT("Avancer"), this, &ACommissaire::Avancer);
	Input->BindAxis(TEXT("Lateral"), this, &ACommissaire::Lateral);
	Input->BindAxis(TEXT("Tourner"), this, &ACommissaire::Tourner);
	Input->BindAxis(TEXT("Regarder"), this, &ACommissaire::Regarder);
	Input->BindAction(TEXT("Interagir"), IE_Pressed, this, &ACommissaire::Interagir);
	Input->BindAction(TEXT("Explorer"), IE_Pressed, this, &ACommissaire::Explorer);

	// Touches 1 à 9 (rangée du haut et pavé numérique) pour les répliques.
	const FKey Keys[9] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	const FKey Pad[9] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive,
		EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
	for (int32 i = 0; i < 9; ++i)
	{
		const int32 Number = i + 1;
		for (const FKey& Key : { Keys[i], Pad[i] })
		{
			FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
			Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Number]() { Choisir(Number); });
			Input->KeyBindings.Add(Binding);
		}
	}
}

void ACommissaire::Avancer(float Value)
{
	if (Value != 0.f) { AddMovementInput(GetActorForwardVector(), Value); }
}

void ACommissaire::Lateral(float Value)
{
	if (Value != 0.f) { AddMovementInput(GetActorRightVector(), Value); }
}

void ACommissaire::Tourner(float Value)
{
	AddControllerYawInput(Value);
}

void ACommissaire::Regarder(float Value)
{
	AddControllerPitchInput(Value);
}

void ACommissaire::Interagir()
{
	if (AThermidorPlayerController* PC = Cast<AThermidorPlayerController>(GetController()))
	{
		PC->InteractWith(Focused.Get());
	}
}

void ACommissaire::Explorer()
{
	if (AThermidorPlayerController* PC = Cast<AThermidorPlayerController>(GetController()))
	{
		PC->ToggleExplore();
	}
}

void ACommissaire::Choisir(int32 Number)
{
	if (AThermidorPlayerController* PC = Cast<AThermidorPlayerController>(GetController()))
	{
		PC->ChooseNumber(Number);
	}
}

void ACommissaire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Ce que vise le commissaire.
	AInteractif* NewFocus = nullptr;
	if (Controller)
	{
		const FVector Start = Camera->GetComponentLocation();
		const FVector End = Start + Controller->GetControlRotation().Vector() * InteractDistance;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ThermidorInteract), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			NewFocus = Cast<AInteractif>(Hit.GetActor());
		}
	}
	if (NewFocus != Focused.Get())
	{
		if (AInteractif* Old = Focused.Get()) { Old->Label->SetVisibility(false); }
		if (NewFocus) { NewFocus->Label->SetVisibility(true); }
		Focused = NewFocus;
	}
}
