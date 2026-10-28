#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactif.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Un personnage ou un objet du décor avec lequel on interagit (touche E).
 * L'interaction choisit, dans le noeud de dialogue courant, le premier choix
 * qui mène à l'un des noeuds de TargetNodes.
 */
UCLASS()
class HUITTHERMIDOR_API AInteractif : public AActor
{
	GENERATED_BODY()

public:
	AInteractif();
	virtual void Tick(float DeltaSeconds) override;

	/** Silhouette simple : un corps en cylindre et une tête. */
	void SetupAsPerson(const FString& InName, const FLinearColor& Coat, float Height = 175.f, bool bSeated = false);
	/** Objet : une forme de base (Cube, Cylinder, Sphere) mise à l'échelle, en centimètres. */
	void SetupAsObject(const FString& InName, const TCHAR* Shape, const FVector& SizeCm, const FLinearColor& Color);
	/** Ajoute un accessoire coloré (cocarde, bonnet, écharpe...). */
	void AddProp(const TCHAR* Shape, const FVector& RelativeLocation, const FVector& SizeCm, const FLinearColor& Color);

	UPROPERTY(EditAnywhere, Category = "Thermidor")
	FString DisplayName;

	/** Verbe affiché dans l'aide : « Parler à », « Examiner », « Ouvrir »... */
	UPROPERTY(EditAnywhere, Category = "Thermidor")
	FString Verb = TEXT("Parler à");

	/** Noeuds de dialogue que cet objet peut déclencher, par ordre de préférence. */
	UPROPERTY(EditAnywhere, Category = "Thermidor")
	TArray<FString> TargetNodes;

	/** Message si aucun choix ne correspond (déjà fait, ou pas maintenant). */
	UPROPERTY(EditAnywhere, Category = "Thermidor")
	FString IdleText = TEXT("Rien de plus à faire ici pour l'instant.");

	UPROPERTY(VisibleAnywhere, Category = "Thermidor")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Thermidor")
	TObjectPtr<UTextRenderComponent> Label;

	static UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, const TCHAR* Shape, const FVector& RelativeLocation, const FVector& SizeCm, const FLinearColor& Color);
};
