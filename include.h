


/* include.h */
void IncludeInit(Engine *pc);
void IncludeCleanup(Engine *pc);
void IncludeRegister(Engine *pc, const char *IncludeName,
    void (*SetupFunction)(Engine *pc), struct LibraryFunction *FuncList,
    const char *SetupCSource);
void IncludeFile(Engine *pc, char *Filename);
/* the following is defined in engine.h:
 * void EngineIncludeAllSystemHeaders(); */

#ifdef DEBUGGER
/* debug.c */
void DebugInit(Engine *pc);
void DebugCleanup(Engine *pc);
void DebugCheckStatement(struct ParseState *Parser);
void DebugSetBreakpoint(struct ParseState *Parser);
int DebugClearBreakpoint(struct ParseState *Parser);
void DebugStep(void)
#endif

/* stdio.c */
const char StdioDefs[];
struct LibraryFunction StdioFunctions[];
void StdioSetupFunc(Engine *pc);

/* math.c */
struct LibraryFunction MathFunctions[];
void MathSetupFunc(Engine *pc);

/* string.c */
struct LibraryFunction StringFunctions[];
void StringSetupFunc(Engine *pc);

/* stdlib.c */
struct LibraryFunction StdlibFunctions[];
void StdlibSetupFunc(Engine *pc);

/* time.c */
const char StdTimeDefs[];
struct LibraryFunction StdTimeFunctions[];
void StdTimeSetupFunc(Engine *pc);

/* errno.c */
void StdErrnoSetupFunc(Engine *pc);

/* ctype.c */
struct LibraryFunction StdCtypeFunctions[];

/* stdbool.c */
const char StdboolDefs[];
void StdboolSetupFunc(Engine *pc);

/* unistd.c */
const char UnistdDefs[];
struct LibraryFunction UnistdFunctions[];
void UnistdSetupFunc(Engine *pc);
