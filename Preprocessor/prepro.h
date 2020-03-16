#ifndef PREPRO_H
#define PREPRO_H

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <Windows.h>

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

using namespace std;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int b32;

enum token_number_type {
	TokenNumber_None,
	TokenNumber_Integer,
	TokenNumber_FloatingPoint,
};

enum token_bracket_type {
	TokenBracket_Angular,
	TokenBracket_Square,
	TokenBracket_Bracket,
	TokenBracket_CurlyBrace,
};

enum token_bracket_open_type {
	TokenBracketOpen_Close,
	TokenBracketOpen_Open,
};

enum token_type {
	Token_Identifier,
	Token_String,
	Token_Number,
	Token_Operator,
	Token_Bracket,
};

struct token {
	char* Text;
	int TextLen;
	uint32_t Type;
	std::string TokenString;

	union{
		u32 NumberType;

		struct {
			u16 BracketType;
			u16 IsOpenBracket;
		};
	};
};

inline token CreateToken(char* Text, int TextLen, u32 Type) {
	token Result = {};
	Result.Text = Text;
	Result.TextLen = TextLen;
	Result.Type = Type;
	Result.TokenString = string(Text, TextLen);
	return(Result);
}

inline void InitTokenBracket(token& Token, u16 BracketType, u16 IsOpen) {
	Token.IsOpenBracket = IsOpen;
	Token.BracketType = BracketType;
}

struct tokenizer {
	char* At;

	vector<token> Tokens;
};

inline void PushToken(tokenizer* Tokenizer, const token& Token) {
	Tokenizer->Tokens.push_back(Token);
}


inline b32 IsAlpha(char C) {
	b32 Result = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z');

	return(Result);
}

inline b32 IsDigit(char C) {
	b32 Result = (C >= '0' && C <= '9');

	return(Result);
}

inline b32 IsOctDigit(char C) {
	b32 Result = (C >= '0' && C <= '7');

	return(Result);
}

inline b32 IsHexDigit(char C) {
	b32 Result = (IsDigit(C) ||
		((C >= 'a') && (C <= 'f')) ||
		((C >= 'A') && (C <= 'F')));

	return(Result);
}

inline b32 IsBinDigit(char C) {
	b32 Result = (C == '0' || C == '1');

	return(Result);
}


inline b32 IsSkippable(char C) {
	b32 Result = (C == ' ' || C == '\t' || C == '\r' || C == '\n');

	return(Result);
}

static char GlobalOpSymbols[] = {
	'!', '^', '#', '~','?', ';', ':', ',', '\\'
};

inline b32 IsOperatorSymbol(char C) {
	b32 Result = 0;

	for (int SymbIndex = 0;
		SymbIndex < ARRAY_COUNT(GlobalOpSymbols);
		SymbIndex++)
	{
		if (C == GlobalOpSymbols[SymbIndex]) {
			Result = 1;
			break;
		}
	}

	return(Result);
}

inline b32 NotEnd(char C) {
	b32 Result = C != 0;

	return(Result);
}

#endif