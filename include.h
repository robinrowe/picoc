


/* include.h */
void IncludeInit(Picoc *pc);
void IncludeCleanup(Picoc *pc);
void IncludeRegister(Picoc *pc, const char *IncludeName,
    void (*SetupFunction)(Picoc *pc), struct LibraryFunction *FuncList,
    const char *SetupCSource);
void IncludeFile(Picoc *pc, char *Filename);
/* the following is defined in engine.h:
 * void PicocIncludeAllSystemHeaders(); */

#ifdef DEBUGGER
/* debug.c */
void DebugInit(Picoc *pc);
void DebugCleanup(Picoc *pc);
void DebugCheckStatement(struct ParseState *Parser);
void DebugSetBreakpoint(struct ParseState *Parser);
int DebugClearBreakpoint(struct ParseState *Parser);
void DebugStep(void)
#endif

/* stdio.c */
const char StdioDefs[];
struct LibraryFunction StdioFunctions[];
void StdioSetupFunc(Picoc *pc);

/* math.c */
struct LibraryFunction MathFunctions[];
void MathSetupFunc(Picoc *pc);

/* string.c */
struct LibraryFunction StringFunctions[];
void StringSetupFunc(Picoc *pc);

/* stdlib.c */
struct LibraryFunction StdlibFunctions[];
void StdlibSetupFunc(Picoc *pc);

/* time.c */
const char StdTimeDefs[];
struct LibraryFunction StdTimeFunctions[];
void StdTimeSetupFunc(Picoc *pc);

/* errno.c */
void StdErrnoSetupFunc(Picoc *pc);

/* ctype.c */
struct LibraryFunction StdCtypeFunctions[];

/* stdbool.c */
const char StdboolDefs[];
void StdboolSetupFunc(Picoc *pc);

/* unistd.c */
const char UnistdDefs[];
struct LibraryFunction UnistdFunctions[];
void UnistdSetupFunc(Picoc *pc);
