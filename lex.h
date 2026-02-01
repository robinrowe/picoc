/* lex.h */


#ifndef lex_h
#define lex_h

#include "itrapc.h"

/* lexical tokens */
enum LexToken {
    TokenNone,
    TokenComma,
    TokenAssign,
    TokenAddAssign,
    TokenSubtractAssign,
    TokenMultiplyAssign,
    TokenDivideAssign,
    TokenModulusAssign,
    TokenShiftLeftAssign,
    TokenShiftRightAssign,
    TokenArithmeticAndAssign,
    TokenArithmeticOrAssign,
    TokenArithmeticExorAssign,
    TokenQuestionMark,
    TokenColon,
    TokenLogicalOr,
    TokenLogicalAnd,
    TokenArithmeticOr,
    TokenArithmeticExor,
    TokenAmpersand,
    TokenEqual,
    TokenNotEqual,
    TokenLessThan,
    TokenGreaterThan,
    TokenLessEqual,
    TokenGreaterEqual,
    TokenShiftLeft,
    TokenShiftRight,
    TokenPlus,
    TokenMinus,
    TokenAsterisk,
    TokenSlash,
    TokenModulus,
    TokenIncrement,
    TokenDecrement,
    TokenUnaryNot,
    TokenUnaryExor,
    TokenSizeof,
    TokenCast,
    TokenLeftSquareBracket,
    TokenRightSquareBracket,
    TokenDot,
    TokenArrow,
    TokenOpenParen,
    TokenCloseParen,
    TokenIdentifier,
    TokenIntegerConstant,
    TokenFPConstant,
    TokenStringConstant,
    TokenCharacterConstant,
    TokenSemicolon,
    TokenEllipsis,
    TokenLeftBrace,
    TokenRightBrace,
    TokenDoubleDot,      /* .. operator (for global calls) */
    TokenScopeRes,       /* :: operator (C++ scope resolution) */
    TokenIntType,
    TokenCharType,
    TokenFloatType,
    TokenDoubleType,
    TokenVoidType,
    TokenEnumType,
    TokenLongType,
    TokenSignedType,
    TokenShortType,
    TokenStaticType,
    TokenAutoType,
    TokenRegisterType,
    TokenExternType,
    TokenStructType,
    TokenUnionType,
    TokenUnsignedType,
    TokenTypedef,
    TokenContinue,
    TokenDo,
    TokenElse,
    TokenFor,
    TokenGoto,
    TokenIf,
    TokenWhile,
    TokenBreak,
    TokenSwitch,
    TokenCase,
    TokenDefault,
    TokenReturn,
    TokenHashDefine,
    TokenHashInclude,
    TokenHashIf,
    TokenHashIfdef,
    TokenHashIfndef,
    TokenHashElse,
    TokenHashEndif,
    TokenNew,
    TokenDelete,
    TokenOpenMacroBracket,
    TokenEOF,
    TokenEndOfLine,
    TokenEndOfFunction,
    TokenBackSlash
};
void LexInit(Engine *pc);
void LexCleanup(Engine *pc);
void *LexAnalyse(Engine *pc, const char *FileName, const char *Source,
    int SourceLen, int *TokenLen);
void LexInitParser(struct ParseState *Parser, Engine *pc,
    const char *SourceText, void *TokenSource, char *FileName, int RunIt, int SetDebugMode);
enum LexToken LexGetToken(struct ParseState *Parser, struct Value **Value,
    int IncPos);
void LexToEndOfMacro(struct ParseState *Parser);
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser);
void LexInteractiveClear(Engine *pc, struct ParseState *Parser);
void LexInteractiveCompleted(Engine *pc, struct ParseState *Parser);
void LexInteractiveStatementPrompt(Engine *pc);
void PrintLexToken(enum LexToken token);
enum LexToken LexRawPeekToken(struct ParseState *Parser);

#endif
