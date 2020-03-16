#include "prepro.h"
#include "small_timing_utility.h"

static b32 TokenIsEqual(const token& Token, const char* Name) {
	b32 Result = strcmp(Token.TokenString.c_str(), Name) == 0;

	return(Result);
}

static char* ReadFileAndNullTerminate(char* Path) {
	FILE* fp = fopen(Path, "rb");

	char* Result = 0;

	if (fp) {

		fseek(fp, 0, SEEK_END);
		unsigned FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		Result = (char*)malloc(FileSize + 1);
		size_t ReadElemCount = fread(Result, 1, FileSize, fp);

		assert(ReadElemCount == FileSize);

		Result[FileSize] = 0;

		fclose(fp);
	}

	return(Result);
}

static void FreeReadFile(char* Begin) {
	if (Begin) {
		free(Begin);
	}
}

static void ProcessFile(char* FileName) {
	char* FileContent = ReadFileAndNullTerminate(FileName);

	if (FileContent) {
		timing_ctx FileParseTiming = {};
		BeginTiming(&FileParseTiming, FileName);

		tokenizer Tokenizer = {};
		Tokenizer.At = (char*)FileContent;

		b32 ShouldBreakScan = false;
		while (*Tokenizer.At) {
			//NOTE(dima): Skipping to next valid symbol
			while (IsSkippable(*Tokenizer.At) && NotEnd(*Tokenizer.At)) {
				++Tokenizer.At;
			}

			int ToSkipCount = 0;
			token NewToken = {};

			if (IsAlpha(*Tokenizer.At) || (*Tokenizer.At == '_')) {
				int IdNameLen = 0;
				char* IdName = Tokenizer.At;
				while (IsAlpha(*Tokenizer.At) || IsDigit(*Tokenizer.At) || (*Tokenizer.At == '_')) {
					++IdNameLen;
					++Tokenizer.At;
				}
				NewToken = CreateToken(IdName, IdNameLen, Token_Identifier);
			}
			else if (IsOperatorSymbol(*Tokenizer.At)) {
				int OpNameLen = 0;
				char* OpName = Tokenizer.At;
				while (IsOperatorSymbol(*Tokenizer.At)) {
					++OpNameLen;
					++Tokenizer.At;
				}
				NewToken = CreateToken(OpName, OpNameLen, Token_Operator);
			}
			else if (IsDigit(*Tokenizer.At) && (*Tokenizer.At != '0')) {
				/*
					I do not process 0 here because i process bin, oct and
					hex number representations further.
				*/
				b32 MetPoint = 0;
				u32 TokenNumberType = TokenNumber_None;
				char* NumName = Tokenizer.At;
				int NumNameLen = 0;
				while (IsDigit(*Tokenizer.At)) {
					++Tokenizer.At;
					++NumNameLen;
				}
				//NOTE(dima): If number has point than - it is the floating point number
				if (*Tokenizer.At == '.') {
					++Tokenizer.At;
					++NumNameLen;
					MetPoint = true;

					while (IsDigit(*Tokenizer.At)) {
						++Tokenizer.At;
						++NumNameLen;
					}
				}
				if (MetPoint) {
					TokenNumberType = TokenNumber_FloatingPoint;
					
					if (*Tokenizer.At == 'f') {
						++Tokenizer.At;
						++NumNameLen;
					}

				}
				else {
					TokenNumberType = TokenNumber_Integer;

					//NOTE(dima): If the number is integer;
					/* 
						NOTE(dima):
						
						Processing all types of integer suffixes.
						The separation was done for future work if someone 
						will ever want to somehow support different types of
						integer suffixes
					*/
					
					if ((Tokenizer.At[0] == 'u' && Tokenizer.At[1] == 'l' && Tokenizer.At[2] == 'l') ||
						(Tokenizer.At[0] == 'U' && Tokenizer.At[1] == 'L' && Tokenizer.At[2] == 'L'))
					{
						//NOTE(dima): Processing long long type
						NumNameLen += 3;
						Tokenizer.At += 3;
					}
					else if ((Tokenizer.At[0] == 'l' && Tokenizer.At[1] == 'l') ||
						(Tokenizer.At[0] == 'L' && Tokenizer.At[1] == 'L'))
					{
						//NOTE(dima): Processing long long type
						NumNameLen += 2;
						Tokenizer.At += 2;
					}
					else if ((Tokenizer.At[0] == 'u' && Tokenizer.At[1] == 'l') ||
						(Tokenizer.At[0] == 'U' && Tokenizer.At[1] == 'L'))
					{
						//NOTE(dima): Processing unsigned long type
						NumNameLen += 2;
						Tokenizer.At += 2;
					}
					else if (Tokenizer.At[0] == 'L' ||
						Tokenizer.At[0] == 'l') 
					{
						//NOTE(dima): Processing long type
						++NumNameLen;
						++Tokenizer.At;
					}
					else if (Tokenizer.At[0] == 'u' ||
						Tokenizer.At[0] == 'U')
					{
						//NOTE(dima): Processing unsigned integers
						++NumNameLen;
						++Tokenizer.At;
					}
				}

				NewToken = CreateToken(NumName, NumNameLen, Token_Number);
				NewToken.NumberType = TokenNumberType;
			}
			else {

				//NOTE(dima): Parsing current symbol
				switch (*Tokenizer.At) {
					case '{': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_CurlyBrace, TokenBracketOpen_Open);
						ToSkipCount = 1;
					}break;

					case '}': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_CurlyBrace, TokenBracketOpen_Close);
						ToSkipCount = 1;
					}break;

					case '(': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_Bracket, TokenBracketOpen_Open);
						ToSkipCount = 1;
					}break;

					case ')': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_Bracket, TokenBracketOpen_Close);
						ToSkipCount = 1;
					}break;

					case '[': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_Square, TokenBracketOpen_Open);
						ToSkipCount = 1;
					}break;

					case ']': {
						NewToken = CreateToken(Tokenizer.At, 1, Token_Bracket);
						InitTokenBracket(NewToken, TokenBracket_Square, TokenBracketOpen_Close);
						ToSkipCount = 1;
					}break;

					case '-': {
						if (Tokenizer.At[1] == '>' ||
							Tokenizer.At[1] == '-' ||
							Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '+': {
						if (Tokenizer.At[1] == '+' ||
							Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '*': {
						if (Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '/': {
						if (Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else if (Tokenizer.At[1] == '/') {
							++Tokenizer.At;
							++Tokenizer.At;

							while (*Tokenizer.At != 0 &&
								*Tokenizer.At != '\n' &&
								*Tokenizer.At != '\r')
							{
								++Tokenizer.At;
							}
						}
						else if (Tokenizer.At[1] == '*') {
							++Tokenizer.At;
							++Tokenizer.At;

							while (Tokenizer.At[0] != 0 &&
								Tokenizer.At[1] != 0)
							{
								if (Tokenizer.At[0] == '*' &&
									Tokenizer.At[1] == '/')
								{
									++Tokenizer.At;
									++Tokenizer.At;
									break;
								}
								else {
									++Tokenizer.At;
								}
							}
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '%': {
						if (Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '&': {
						if (Tokenizer.At[1] == '=' ||
							Tokenizer.At[1] == '&')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '|': {
						if (Tokenizer.At[1] == '=' ||
							Tokenizer.At[1] == '|')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '=': {
						if (Tokenizer.At[1] == '=')
						{
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, Token_Operator);
					}break;

					case '<': {
						u32 NewTokenType = Token_Bracket;
						if (Tokenizer.At[1] == '=' ||
							Tokenizer.At[1] == '<')
						{
							NewTokenType = Token_Operator;
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, NewTokenType);
						if (NewTokenType == Token_Bracket) {
							InitTokenBracket(NewToken, TokenBracket_Angular, TokenBracketOpen_Open);
						}
					}break;

					case '>': {
						u32 NewTokenType = Token_Bracket;
						if (Tokenizer.At[1] == '=' ||
							Tokenizer.At[1] == '>')
						{
							NewTokenType = Token_Operator;
							ToSkipCount = 2;
						}
						else {
							ToSkipCount = 1;
						}
						NewToken = CreateToken(Tokenizer.At, ToSkipCount, NewTokenType);
						if (NewTokenType == Token_Bracket) {
							InitTokenBracket(NewToken, TokenBracket_Angular, TokenBracketOpen_Close);
						}
					}break;

					case '"': {
						if (NotEnd(Tokenizer.At[1])) {
							++Tokenizer.At;
							int TokenStrLen = 0;
							char* TokenStrName = Tokenizer.At;
							while (*Tokenizer.At != '"' && NotEnd(*Tokenizer.At)) {
								++TokenStrLen;
								++Tokenizer.At;
							}

							if (*Tokenizer.At == '"') {
								++Tokenizer.At;
							}
							else if (*Tokenizer.At == 0) {
								ShouldBreakScan = true;
							}
							else {
								assert(!"Invalide code path");
							}

							NewToken = CreateToken(TokenStrName, TokenStrLen, Token_String);
						}
					}break;

					case '0': {
						//NOTE(dima): Processing bin, oct, and hex representations of the numbers
						if (Tokenizer.At[1] == 'x' ||
							Tokenizer.At[1] == 'b' ||
							IsDigit(Tokenizer.At[1])) 
						{
							char* NumName = Tokenizer.At;
							int NumNameLen = 0;

							++Tokenizer.At;
							++NumNameLen;
							if (Tokenizer.At[0] == 'x')
							{
								++Tokenizer.At;
								++NumNameLen;

								while (IsHexDigit(*Tokenizer.At)) {
									++Tokenizer.At;
									++NumNameLen;
								}
							}
							else if (Tokenizer.At[0] == 'b') {
								++Tokenizer.At;
								++NumNameLen;

								while (IsBinDigit(*Tokenizer.At)) {
									++Tokenizer.At;
									++NumNameLen;
								}
							}
							else {
								while (IsOctDigit(*Tokenizer.At)) {
									++Tokenizer.At;
									++NumNameLen;
								}
							}

							NewToken = CreateToken(NumName, NumNameLen, Token_Number);
						}
						else {
							++Tokenizer.At;
							NewToken = CreateToken(Tokenizer.At, 1, Token_Number);
						}
						NewToken.NumberType = TokenNumber_Integer;
					}break;

					default: {
						ToSkipCount = 1;
					}break;
				}
			}

			if (NewToken.TextLen) {
				PushToken(&Tokenizer, NewToken);
				printf("%s\n", NewToken.TokenString.c_str());
			}

			//NOTE(dima): Exiting if needed
			if (ShouldBreakScan) {
				break;
			}

			if (ToSkipCount) {
				//NOTE(dima): Skipping
				for (int SkipIndex = 0; SkipIndex < ToSkipCount; SkipIndex++) {
					++Tokenizer.At;
				}
			}

		}

		EndTiming(&FileParseTiming);
	}

	FreeReadFile(FileContent);
}

int main(int ArgCount, char** Args) {

	char* FileName = "test.cpp";
	ProcessFile(FileName);

	return(0);
}