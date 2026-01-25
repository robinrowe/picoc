/* picoc interactive debugger */
#include "interpreter.h"

#define SHOWX
//#define PRINT_TOKEN

#define BREAKPOINT_HASH(p) (((unsigned long)(p)->FileName) ^ (((p)->Line << 16) | ((p)->CharacterPos << 16)))

#ifdef DEBUGGER
/* initialize the debugger by clearing the breakpoint table */
void DebugInit(Picoc *pc)
{
    TableInitTable(&pc->BreakpointTable, &pc->BreakpointHashTable[0],
        BREAKPOINT_TABLE_SIZE, true);
    pc->BreakpointCount = 0;
}

/* free the contents of the breakpoint table */
void DebugCleanup(Picoc *pc)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    int Count;

    for (Count = 0; Count < pc->BreakpointTable.Size; Count++) {
        for (Entry = pc->BreakpointHashTable[Count]; Entry != NULL;
                Entry = NextEntry) {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}

/* search the table for a breakpoint */
static struct TableEntry *DebugTableSearchBreakpoint(struct ParseState *Parser,
    int *AddAt)
{
    struct TableEntry *Entry;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (Entry = pc->BreakpointHashTable[HashValue];
            Entry != NULL; Entry = Entry->Next) {
        if (Entry->p.b.FileName == Parser->FileName &&
                Entry->p.b.Line == Parser->Line &&
                Entry->p.b.CharacterPos == Parser->CharacterPos)
            return Entry;   /* found */
    }

    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set a breakpoint in the table */
void DebugSetBreakpoint(struct ParseState *Parser)
{
    int AddAt;
    struct TableEntry *FoundEntry = DebugTableSearchBreakpoint(Parser, &AddAt);
    Picoc *pc = Parser->pc;

    if (FoundEntry == NULL) {
        /* add it to the table */
        struct TableEntry *NewEntry = HeapAllocMem(pc, sizeof(*NewEntry));
        if (NewEntry == NULL)
            ProgramFailNoParser(pc, "(DebugSetBreakpoint) out of memory");

        NewEntry->p.b.FileName = Parser->FileName;
        NewEntry->p.b.Line = Parser->Line;
        NewEntry->p.b.CharacterPos = Parser->CharacterPos;
        NewEntry->Next = pc->BreakpointHashTable[AddAt];
        pc->BreakpointHashTable[AddAt] = NewEntry;
        pc->BreakpointCount++;
    }
}

/* delete a breakpoint from the hash table */
int DebugClearBreakpoint(struct ParseState *Parser)
{
    struct TableEntry **EntryPtr;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (EntryPtr = &pc->BreakpointHashTable[HashValue];
            *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next) {
        struct TableEntry *DeleteEntry = *EntryPtr;
        if (DeleteEntry->p.b.FileName == Parser->FileName &&
                DeleteEntry->p.b.Line == Parser->Line &&
                DeleteEntry->p.b.CharacterPos == Parser->CharacterPos) {
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);
            pc->BreakpointCount--;

            return true;
        }
    }

    return false;
}

/* before we run a statement, check if there's anything we have to
    do with the debugger here */
void DebugCheckStatement(struct ParseState *Parser)
{
    int DoBreak = false;
    int AddAt;
    Picoc *pc = Parser->pc;

    /* has the user manually pressed break? */
    if (pc->DebugManualBreak) {
        PlatformPrintf(pc->CStdOut, "break\n");
        DoBreak = true;
        pc->DebugManualBreak = false;
    }

    /* is this a breakpoint location? */
    if (Parser->pc->BreakpointCount != 0 &&
            DebugTableSearchBreakpoint(Parser, &AddAt) != NULL)
        DoBreak = true;

    /* handle a break */
    if (DoBreak) {
        PlatformPrintf(pc->CStdOut, "Handling a break\n");
        PicocParseInteractiveNoStartPrompt(pc, false);
    }
}

void DebugStep(void)
{
}
#endif /* DEBUGGER */

#ifdef SHOWX

#define CHECK(x) show |= !strcmp(word,x); show<<=1

void ShowX(const char* function,const char* table,const char* word,size_t length)
{	char buffer[100];
	if(!word || length>99)
	{	return;
	}
	if(length)
	{	memcpy(buffer,word,length);
		buffer[length] = 0;
		word = buffer;
	}
	else
	{	length = strlen(word);
	}
#if 1
	unsigned show = 0;
//	CHECK("Foo.fooMethod");
//	CHECK("Foo"); 
	CHECK("foo"); 
	CHECK("__exit_value"); 
	CHECK("main"); 
	if(!show)
	{	return;
	}
#endif
//	printf("SHOWX: %s(%s): '%.*s'\n",function,table,(int) length, word);
	printf("SHOWX: %s(%s): '%s'",function,table,word);
	puts("");
}
#else
void ShowX(const char* function,const char* table,const char* word)
{}
#endif 

#ifdef PRINT_TOKEN

#define SHOW(token) case token: printf("DEBUG: token = %s (%d)\n",#token,token); return

void PrintLexToken(enum LexToken token)
{	switch(token)
	{	SHOW(TokenNone);
		SHOW(TokenComma);
		SHOW(TokenAssign);
		SHOW(TokenAddAssign);
		SHOW(TokenSubtractAssign);
		SHOW(TokenMultiplyAssign);
		SHOW(TokenDivideAssign);
		SHOW(TokenModulusAssign);
		SHOW(TokenShiftLeftAssign);
		SHOW(TokenShiftRightAssign);
		SHOW(TokenArithmeticAndAssign);
		SHOW(TokenArithmeticOrAssign);
		SHOW(TokenArithmeticExorAssign);
		SHOW(TokenQuestionMark);
		SHOW(TokenColon);
		SHOW(TokenLogicalOr);
		SHOW(TokenLogicalAnd);
		SHOW(TokenArithmeticOr);
		SHOW(TokenArithmeticExor);
		SHOW(TokenAmpersand);
		SHOW(TokenEqual);
		SHOW(TokenNotEqual);
		SHOW(TokenLessThan);
		SHOW(TokenGreaterThan);
		SHOW(TokenLessEqual);
		SHOW(TokenGreaterEqual);
		SHOW(TokenShiftLeft);
		SHOW(TokenShiftRight);
		SHOW(TokenPlus);
		SHOW(TokenMinus);
		SHOW(TokenAsterisk);
		SHOW(TokenSlash);
		SHOW(TokenModulus);
		SHOW(TokenIncrement);
		SHOW(TokenDecrement);
		SHOW(TokenUnaryNot);
		SHOW(TokenUnaryExor);
		SHOW(TokenSizeof);
		SHOW(TokenCast);
		SHOW(TokenLeftSquareBracket);
		SHOW(TokenRightSquareBracket);
		SHOW(TokenDot);
		SHOW(TokenArrow);
		SHOW(TokenOpenParen);
		SHOW(TokenCloseParen);
		SHOW(TokenIdentifier);
		SHOW(TokenIntegerConstant);
		SHOW(TokenFPConstant);
		SHOW(TokenStringConstant);
		SHOW(TokenCharacterConstant);
		SHOW(TokenSemicolon);
		SHOW(TokenEllipsis);
		SHOW(TokenLeftBrace);
		SHOW(TokenRightBrace);
		SHOW(TokenIntType);
		SHOW(TokenCharType);
		SHOW(TokenFloatType);
		SHOW(TokenDoubleType);
		SHOW(TokenVoidType);
		SHOW(TokenEnumType);
		SHOW(TokenLongType);
		SHOW(TokenSignedType);
		SHOW(TokenShortType);
		SHOW(TokenStaticType);
		SHOW(TokenAutoType);
		SHOW(TokenRegisterType);
		SHOW(TokenExternType);
		SHOW(TokenStructType);
		SHOW(TokenUnionType);
		SHOW(TokenUnsignedType);
		SHOW(TokenTypedef);
		SHOW(TokenContinue);
		SHOW(TokenDo);
		SHOW(TokenElse);
		SHOW(TokenFor);
		SHOW(TokenGoto);
		SHOW(TokenIf);
		SHOW(TokenWhile);
		SHOW(TokenBreak);
		SHOW(TokenSwitch);
		SHOW(TokenCase);
		SHOW(TokenDefault);
		SHOW(TokenReturn);
		SHOW(TokenHashDefine);
		SHOW(TokenHashInclude);
		SHOW(TokenHashIf);
		SHOW(TokenHashIfdef);
		SHOW(TokenHashIfndef);
		SHOW(TokenHashElse);
		SHOW(TokenHashEndif);
		SHOW(TokenNew);
		SHOW(TokenDelete);
		SHOW(TokenOpenMacroBracket);
		SHOW(TokenEOF);
		SHOW(TokenEndOfLine);
		SHOW(TokenEndOfFunction);
		SHOW(TokenBackSlash);
}	}
#else
void PrintLexToken(enum LexToken token)
{}
#endif
