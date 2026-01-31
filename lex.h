
/* lex.h */
void LexInit(Engine *pc);
void LexCleanup(Engine *pc);
void *LexAnalyse(Engine *pc, const char *FileName, const char *Source,
    int SourceLen, int *TokenLen);
void LexInitParser(struct ParseState *Parser, Engine *pc,
    const char *SourceText, void *TokenSource, char *FileName, int RunIt, int SetDebugMode);
enum LexToken LexGetToken(struct ParseState *Parser, struct Value **Value,
    int IncPos);
enum LexToken LexRawPeekToken(struct ParseState *Parser);
void LexToEndOfMacro(struct ParseState *Parser);
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser);
void LexInteractiveClear(Engine *pc, struct ParseState *Parser);
void LexInteractiveCompleted(Engine *pc, struct ParseState *Parser);
void LexInteractiveStatementPrompt(Engine *pc);

