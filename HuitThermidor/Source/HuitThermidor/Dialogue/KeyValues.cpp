#include "Dialogue/KeyValues.h"

FString FKVPair::Get(const FString& InKey) const
{
	for (const FKVPair& Child : Children)
	{
		if (!Child.bIsBlock && Child.Key == InKey)
		{
			return Child.Value;
		}
	}
	return FString();
}

const FKVPair* FKVPair::GetBlock(const FString& InKey) const
{
	for (const FKVPair& Child : Children)
	{
		if (Child.bIsBlock && Child.Key == InKey)
		{
			return &Child;
		}
	}
	return nullptr;
}

TArray<FString> FKVPair::GetAll(const FString& InKey) const
{
	TArray<FString> Out;
	for (const FKVPair& Child : Children)
	{
		if (!Child.bIsBlock && Child.Key == InKey)
		{
			Out.Add(Child.Value);
		}
	}
	return Out;
}

TArray<const FKVPair*> FKVPair::GetAllBlocks(const FString& InKey) const
{
	TArray<const FKVPair*> Out;
	for (const FKVPair& Child : Children)
	{
		if (Child.bIsBlock && Child.Key == InKey)
		{
			Out.Add(&Child);
		}
	}
	return Out;
}

bool FKVPair::Has(const FString& InKey) const
{
	for (const FKVPair& Child : Children)
	{
		if (Child.Key == InKey)
		{
			return true;
		}
	}
	return false;
}

namespace
{
	enum class ETokenType : uint8 { String, Open, Close };

	struct FToken
	{
		ETokenType Type;
		FString Text;
		int32 Line;
	};

	bool Tokenize(const FString& Src, TArray<FToken>& Out, FString& OutError)
	{
		int32 Line = 1;
		int32 i = 0;
		const int32 Len = Src.Len();
		while (i < Len)
		{
			const TCHAR C = Src[i];
			if (C == TEXT('\n')) { ++Line; ++i; continue; }
			if (FChar::IsWhitespace(C)) { ++i; continue; }
			if (C == TEXT('/') && i + 1 < Len && Src[i + 1] == TEXT('/'))
			{
				while (i < Len && Src[i] != TEXT('\n')) { ++i; }
				continue;
			}
			if (C == TEXT('{')) { Out.Add({ ETokenType::Open, FString(), Line }); ++i; continue; }
			if (C == TEXT('}')) { Out.Add({ ETokenType::Close, FString(), Line }); ++i; continue; }
			if (C == TEXT('"'))
			{
				const int32 StartLine = Line;
				FString S;
				++i;
				while (i < Len && Src[i] != TEXT('"'))
				{
					if (Src[i] == TEXT('\\') && i + 1 < Len)
					{
						++i;
						S.AppendChar(Src[i] == TEXT('n') ? TEXT('\n') : Src[i]);
					}
					else
					{
						if (Src[i] == TEXT('\n')) { ++Line; }
						S.AppendChar(Src[i]);
					}
					++i;
				}
				if (i >= Len)
				{
					OutError = FString::Printf(TEXT("guillemet non fermé (ligne %d)"), StartLine);
					return false;
				}
				++i; // guillemet fermant
				Out.Add({ ETokenType::String, MoveTemp(S), StartLine });
				continue;
			}
			// Mot sans guillemets
			FString S;
			while (i < Len && !FChar::IsWhitespace(Src[i]) && Src[i] != TEXT('{') && Src[i] != TEXT('}') && Src[i] != TEXT('"'))
			{
				S.AppendChar(Src[i]);
				++i;
			}
			Out.Add({ ETokenType::String, MoveTemp(S), Line });
		}
		return true;
	}

	bool ParseBlock(const TArray<FToken>& Tokens, int32& Pos, TArray<FKVPair>& Out, bool bTopLevel, FString& OutError)
	{
		while (Pos < Tokens.Num())
		{
			const FToken& T = Tokens[Pos];
			if (T.Type == ETokenType::Close)
			{
				if (bTopLevel)
				{
					OutError = FString::Printf(TEXT("accolade fermante en trop (ligne %d)"), T.Line);
					return false;
				}
				++Pos;
				return true;
			}
			if (T.Type != ETokenType::String)
			{
				OutError = FString::Printf(TEXT("clé attendue (ligne %d)"), T.Line);
				return false;
			}
			FKVPair Pair;
			Pair.Key = T.Text;
			++Pos;
			if (Pos >= Tokens.Num())
			{
				OutError = FString::Printf(TEXT("valeur manquante pour \"%s\" (ligne %d)"), *Pair.Key, T.Line);
				return false;
			}
			const FToken& V = Tokens[Pos];
			if (V.Type == ETokenType::Open)
			{
				++Pos;
				Pair.bIsBlock = true;
				if (!ParseBlock(Tokens, Pos, Pair.Children, false, OutError))
				{
					return false;
				}
			}
			else if (V.Type == ETokenType::String)
			{
				Pair.Value = V.Text;
				++Pos;
			}
			else
			{
				OutError = FString::Printf(TEXT("valeur attendue après \"%s\" (ligne %d)"), *Pair.Key, V.Line);
				return false;
			}
			Out.Add(MoveTemp(Pair));
		}
		if (!bTopLevel)
		{
			OutError = TEXT("accolade non fermée en fin de fichier");
			return false;
		}
		return true;
	}
}

bool KeyValues::Parse(const FString& Text, TArray<FKVPair>& OutRoot, FString& OutError)
{
	TArray<FToken> Tokens;
	if (!Tokenize(Text, Tokens, OutError))
	{
		return false;
	}
	int32 Pos = 0;
	return ParseBlock(Tokens, Pos, OutRoot, true, OutError);
}
