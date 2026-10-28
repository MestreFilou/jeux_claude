#pragma once

#include "CoreMinimal.h"

/**
 * Une paire du format texte à accolades utilisé par les fichiers de dialogue :
 *   "cle" "valeur"        -> paire simple
 *   "cle" { ... }         -> bloc contenant d'autres paires
 * L'ordre est conservé et les clés peuvent se répéter (plusieurs "line", "choice"...).
 */
struct HUITTHERMIDOR_API FKVPair
{
	FString Key;
	FString Value;
	TArray<FKVPair> Children;
	bool bIsBlock = false;

	/** Première valeur simple pour cette clé, ou chaîne vide. */
	FString Get(const FString& InKey) const;

	/** Premier bloc pour cette clé, ou nullptr. */
	const FKVPair* GetBlock(const FString& InKey) const;

	/** Toutes les valeurs simples pour cette clé, dans l'ordre. */
	TArray<FString> GetAll(const FString& InKey) const;

	/** Tous les blocs pour cette clé, dans l'ordre. */
	TArray<const FKVPair*> GetAllBlocks(const FString& InKey) const;

	bool Has(const FString& InKey) const;
};

namespace KeyValues
{
	/** Lit un texte complet. Renvoie false et remplit OutError si le texte est mal formé. */
	HUITTHERMIDOR_API bool Parse(const FString& Text, TArray<FKVPair>& OutRoot, FString& OutError);
}
