/* itrapc interactive debugger */
#include "interpreter.h"

#define SHOWX

#define BREAKPOINT_HASH(p) (((unsigned long)(p)->FileName) ^ (((p)->Line << 16) | ((p)->CharacterPos << 16)))

#ifdef DEBUGGER
/* initialize the debugger by clearing the breakpoint table */
void DebugInit(Engine *pc)
{
    TableInitTable(&pc->BreakpointTable, &pc->BreakpointHashTable[0],
        BREAKPOINT_TABLE_SIZE, true);
    pc->BreakpointCount = 0;
}

/* free the contents of the breakpoint table */
void DebugCleanup(Engine *pc)
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
    Engine *pc = Parser->pc;
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
    Engine *pc = Parser->pc;

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
    Engine *pc = Parser->pc;
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
    Engine *pc = Parser->pc;

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
        EngineParseInteractiveNoStartPrompt(pc, false);
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

