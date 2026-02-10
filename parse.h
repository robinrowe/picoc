/* parse.h - Main parser interface */
#ifndef PARSE_H
#define PARSE_H

/* Forward declarations */
typedef struct Engine Engine;
typedef struct ParseState ParseState;
typedef struct Value Value;
typedef struct ValueType ValueType;

/* Parser state */
struct ParseState {
    Engine *pc;                         /* the itrapc instance */
    const unsigned char *Pos;           /* character position in source */
    char *FileName;                     /* file being executed */
    short int Line;                     /* line number */
    short int CharacterPos;             /* character/column in line */
    enum RunMode Mode;                  /* whether to skip or run code */
    int SearchLabel;                    /* case label searching for */
    const char *SearchGotoLabel;        /* goto label searching for */
    const char *SourceText;             /* entire source text */
    short int HashIfLevel;              /* nested #if level */
    short int HashIfEvaluateToLevel;    /* last evaluated #if level */
    char DebugMode;                     /* debugging mode */
    int ScopeID;                        /* local variable scope tracking */
    int is_global;
};

/* Result codes */
enum ParseResult {
    ParseResultOk,
    ParseResultError,
    ParseResultEOF
};

/* Public API functions */
void EngineParse(Engine *pc, const char *FileName, const char *Source,
    int SourceLen, int RunIt, int CleanupNow, int CleanupSource,
    int EnableDebugger);
void EngineParseInteractive(Engine *pc);
void EngineParseInteractiveNoStartPrompt(Engine *pc, int EnableDebugger);
void ParseCleanup(Engine *pc);

/* Core parsing functions */
enum ParseResult ParseStatement(ParseState *Parser, int CheckTrailingSemicolon);
enum ParseResult ParseStatementMaybeRun(ParseState *Parser,
    int Condition, int CheckTrailingSemicolon);
enum RunMode ParseBlock(ParseState *Parser, int AbsorbOpenBrace, int Condition);

/* Function parsing */
Value *ParseFunctionDefinition(ParseState *Parser, ValueType *ReturnType,
    char *Identifier, ValueType *this_type);
Value *ParseMemberFunctionDefinition(ParseState *Parser, ValueType *StructType,
    ValueType *ReturnType, char *function_name);
int ParseCountParams(ParseState *Parser);

/* Utility functions */
void ParserCopy(ParseState *To, ParseState *From);
void ParserCopyPos(ParseState *To, ParseState *From);

#endif /* PARSE_H */
