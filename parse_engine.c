/* parse_engine.c - Top-level parsing orchestration */
#include "parse.h"
#include "interpreter.h"
#include "lex.h"
#include "heap.h"
#include "platform.h"

/* quick scan a source file for definitions */
void EngineParse(Engine *pc, const char *FileName, const char *Source,
    int SourceLen, int RunIt, int CleanupNow, int CleanupSource,
    int EnableDebugger)
{
    char *RegFileName = TableStrRegister(pc, FileName, strlen(FileName));
    enum ParseResult Ok;
    ParseState Parser;
    struct CleanupTokenNode *NewCleanupNode = 0;

    void *Tokens = LexAnalyse(pc, RegFileName, Source, SourceLen, NULL);

    /* allocate a cleanup node so we can clean up the tokens later */
    if (!CleanupNow) {
        NewCleanupNode = HeapAllocMem(pc, sizeof(struct CleanupTokenNode));
        if (NewCleanupNode == NULL)
            ProgramFailNoParser(pc, "(EngineParse) out of memory");

        NewCleanupNode->Tokens = Tokens;
        if (CleanupSource)
            NewCleanupNode->SourceText = Source;
        else
            NewCleanupNode->SourceText = NULL;

        NewCleanupNode->Next = pc->CleanupTokenList;
        pc->CleanupTokenList = NewCleanupNode;
    }

    /* do the parsing */
    LexInitParser(&Parser, pc, Source, Tokens, RegFileName, RunIt,
        EnableDebugger);

    do {
        Ok = ParseStatement(&Parser, true);
    } while (Ok == ParseResultOk);

    if (Ok == ParseResultError)
        ProgramFail(&Parser, "parse error");

    /* clean up */
    if (CleanupNow)
        HeapFreeMem(pc, Tokens);
}

/* parse interactively */
void EngineParseInteractiveNoStartPrompt(Engine *pc, int EnableDebugger)
{
    enum ParseResult Ok;
    ParseState Parser;

    LexInitParser(&Parser, pc, NULL, NULL, pc->StrEmpty, true, EnableDebugger);
    EnginePlatformSetExitPoint(pc);
    LexInteractiveClear(pc, &Parser);

    do {
        LexInteractiveStatementPrompt(pc);
        Ok = ParseStatement(&Parser, true);
        LexInteractiveCompleted(pc, &Parser);
    } while (Ok == ParseResultOk);

    if (Ok == ParseResultError)
        ProgramFail(&Parser, "parse error");

    PlatformPrintf(pc->CStdOut, "\n");
}

/* parse interactively, showing a startup message */
void EngineParseInteractive(Engine *pc)
{
    PlatformPrintf(pc->CStdOut, INTERACTIVE_PROMPT_START);
    EngineParseInteractiveNoStartPrompt(pc, gEnableDebugger);
}

/* deallocate any memory */
void ParseCleanup(Engine *pc)
{
    while (pc->CleanupTokenList != NULL) {
        struct CleanupTokenNode *Next = pc->CleanupTokenList->Next;

        HeapFreeMem(pc, pc->CleanupTokenList->Tokens);
        if (pc->CleanupTokenList->SourceText != NULL)
            HeapFreeMem(pc, (void *)pc->CleanupTokenList->SourceText);

        HeapFreeMem(pc, pc->CleanupTokenList);
        pc->CleanupTokenList = Next;
    }
}

/* copy the entire parser state */
void ParserCopy(ParseState *To, ParseState *From)
{
    memcpy((void*)To, (void*)From, sizeof(*To));
}

/* copy where we're at in the parsing */
void ParserCopyPos(ParseState *To, ParseState *From)
{
    To->Pos = From->Pos;
    To->Line = From->Line;
    To->HashIfLevel = From->HashIfLevel;
    To->HashIfEvaluateToLevel = From->HashIfEvaluateToLevel;
    To->CharacterPos = From->CharacterPos;
}
