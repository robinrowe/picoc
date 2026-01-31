/* itrapc external interface. This should be the only header you need to use if
 * you're using itrapc as a library. Internal details are in interpreter.h */
#ifndef engine_h
#define engine_h

/* itrapc version number */
//#define PROGRAM_VERSION "v2.3.2"
#define PROGRAM_VERSION "v1.0.0"
#define PROGRAM_NAME "itrapc"

#include "interpreter.h"

/* this has to be a macro, otherwise errors will occur due to
	the stack being corrupt */
#define EnginePlatformSetExitPoint(pc) setjmp((pc)->EngineExitBuf)


/* parse.c */
extern void EngineParse(Engine *pc, const char *FileName, const char *Source,
	int SourceLen, int RunIt, int CleanupNow, int CleanupSource, int EnableDebugger);
extern void EngineParseInteractive(Engine *pc);

/* platform.c */
extern void EngineCallMain(Engine *pc, int argc, char **argv);
extern void EngineInitialize(Engine *pc, int StackSize);
extern void EngineCleanup(Engine *pc);
extern void EnginePlatformScanFile(Engine *pc, const char *FileName);

/* include.c */
extern void EngineIncludeAllSystemHeaders(Engine *pc);

#endif /* engine_h */
