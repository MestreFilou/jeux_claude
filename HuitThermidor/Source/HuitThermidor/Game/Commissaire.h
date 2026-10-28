#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Commissaire.generated.h"

class UCameraComponent;
class AInteractif;

/** Le joueur : Désiré Lacombe, vu à la première personne. */
UCLASS()
class HUITTHERMIDOR_API ACommissaire : public ACharacter
{
	GENERATED_BODY()

public:
	ACommissaire();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	AInteractif* GetFocused() const { return Focused.Get(); }

	UPROPERTY(VisibleAnywhere, Category = "Thermidor")
	TObjectPtr<UCameraComponent> Camera;

	/** Distance maximale d'interaction, en centimètres. */
	UPROPERTY(EditAnywhere, Category = "Thermidor")
	float InteractDistance = 260.f;

private:
	void Avancer(float Value);
	void Lateral(float Value);
	void Tourner(float Value);
	void Regarder(float Value);
	void Interagir();
	void Explorer();
	void Choisir(int32 Number);

	TWeakObjectPtr<AInteractif> Focused;
};
