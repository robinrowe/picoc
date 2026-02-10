/* all platform-specific includes and defines go in this file */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <setjmp.h>
#include <math.h>
#include <stdbool.h>
#include "parse.h"
#include "itrapc.h"

/* host platform includes */
#ifdef UNIX_HOST
# include <stdint.h>
# include <unistd.h>
#elif defined(WIN32) /*(predefined on MSVC)*/
#else
# error ***** A platform must be explicitly defined! *****
#endif
typedef FILE IOFILE;
extern int gEnableDebugger;

/* configurable options */
/* select your host type (or do it in the Makefile):
 #define UNIX_HOST
 #define DEBUGGER
 #define USE_READLINE (defined by default for UNIX_HOST)
 */
#define USE_READLINE

#if defined(WIN32) /*(predefined on MSVC)*/
#undef USE_READLINE
#endif

/* undocumented, but probably useful */
#undef DEBUG_HEAP
#undef DEBUG_EXPRESSIONS
#undef FANCY_ERROR_MESSAGES
#undef DEBUG_ARRAY_INITIALIZER
#undef DEBUG_LEXER
#undef DEBUG_VAR_SCOPE

#if defined(__hppa__) || defined(__sparc__)
/* the default data type to use for alignment */
#define ALIGN_TYPE double
#else
/* the default data type to use for alignment */
#define ALIGN_TYPE void*
#endif

/* Common hash prime choices for hash tables:

Small: 53, 97, 193
Medium: 389, 769, 1543
Large: 3079, 6151, 12289 

Why prime numbers for hash table sizes:

Better distribution - Prime numbers reduce clustering because they don't share common factors with typical data patterns
Fewer collisions - When you do hash % 97, you get better spread across buckets than with composite numbers like 100
Mathematical properties - Primes minimize the chance that your hash function and table size interact badly */

#define HASH_PRIME 97

#define GLOBAL_TABLE_SIZE (HASH_PRIME)    /* global variable table */
#define STRING_TABLE_SIZE (HASH_PRIME)    /* shared string table size */
#define VARIABLE_TYPE_TABLE_SIZE (HASH_PRIME) /* varialbe-type table size */
#define STRING_LITERAL_TABLE_SIZE (HASH_PRIME) /* string literal table size */
#define RESERVED_WORD_TABLE_SIZE (HASH_PRIME)  /* reserved word table size */
#define PARAMETER_MAX (16)     /* maximum number of parameters to a function */
#define LINEBUFFER_MAX (256)   /* maximum number of characters on a line */
#define LOCAL_TABLE_SIZE (11)  /* size of local variable table (can expand) */
#define STRUCT_TABLE_SIZE (11) /* size of struct/union member table (can expand) */

#ifdef _WIN32
#define INTERACTIVE_PROMPT_START "starting " PROGRAM_NAME " " PROGRAM_VERSION " (Ctrl+C to quit)\n"
#else
#define INTERACTIVE_PROMPT_START "starting " PROGRAM_NAME " " PROGRAM_VERSION " (Ctrl+D to quit)\n"
#endif
#define INTERACTIVE_PROMPT_STATEMENT "> "
#define INTERACTIVE_PROMPT_LINE "     > "

extern jmp_buf ExitBuf;

/* platform.h */
/* the following are defined in engine.h:
 * void EngineCallMain(int argc, char **argv);
 * int EnginePlatformSetExitPoint();
 * void EngineInitialize(int StackSize);
 * void EngineCleanup();
 * void EnginePlatformScanFile(const char *FileName);
 * int EngineExitValue; */
void PrintSourceLine(struct ParseState *Parser);
void ProgramFail(struct ParseState *Parser, const char *Message, ...);
void ProgramFailNoParser(Engine *pc, const char *Message, ...);
void AssignFail(struct ParseState *Parser, const char *Format,
    struct ValueType *Type1, struct ValueType *Type2, int Num1, int Num2,
    const char *FuncName, int ParamNo);
void LexFail(Engine *pc, struct LexState *Lexer, const char *Message, ...);
void PlatformInit(Engine *pc);
void PlatformCleanup(Engine *pc);
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt);
int PlatformGetCharacter();
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *);
void PlatformPrintf(IOFILE *Stream, const char *Format, ...);
void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args);
void PlatformExit(Engine *pc, int ExitVal);
char *PlatformMakeTempName(Engine *pc, char *TempNameBuffer);
void PlatformLibraryInit(Engine *pc);

#endif /* PLATFORM_H */
