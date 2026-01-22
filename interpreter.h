/* picoc main header file - this has all the main data structures and
 * function prototypes. If you're just calling picoc you should look at the
 * external interface instead, in picoc.h */
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "platform.h"
#include "parse.h"

#ifndef NULL
#define NULL 0
#endif

/*
#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif
#ifndef min
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
*/
/* Get the name of a type */
// #define typename(x) _Generic((x),   \
//     _Bool: "_Bool", \
//     unsigned char: "unsigned char", \
//     char: "char", \
//     signed char: "signed char", \
//     short int: "short int", \
//     unsigned short int: "unsigned short int",   \
//     int: "int", \
//     unsigned int: "unsigned int", \
//     long int: "long int", \
//     unsigned long int: "unsigned long int", \
//     long long int: "long long int", \
//     unsigned long long int: "unsigned long long int", \
//     float: "float", \
//     double: "double", \
//     long double: "long double", \
//     char *: "pointer to char", \
//     void *: "pointer to void", \
//     int *: "pointer to int", \
//     default: "other") (x)


#define MEM_ALIGN(x) (((x) + sizeof(ALIGN_TYPE)-1) & ~(sizeof(ALIGN_TYPE)-1))

/* for debugging */
#define PRINT_SOURCE_POS() { \
                                PrintSourceTextErrorLine(Parser->pc->CStdOut, \
                                                         Parser->FileName, \
                                                         Parser->SourceText, \
                                                         Parser->Line, \
                                                         Parser->CharacterPos); \
                                PlatformPrintf(Parser->pc->CStdOut, "\n"); \
                            }

#define PRINT_TYPE(typ) PlatformPrintf(Parser->pc->CStdOut, "%t\n", typ);

/* coercion of numeric types to other numeric types */
#define IS_FP(v) ((v)->Typ->Base == TypeFP)
#define FP_VAL(v) ((v)->Val->FP)

/* ap -> AllowPointerCoercion = true | false */
#define IS_POINTER_COERCIBLE(v, ap) ((ap) ? ((v)->Typ->Base == TypePointer) : 0)
#define POINTER_COERCE(v) ((int)(v)->Val->Pointer)

#define IS_INTEGER_NUMERIC_TYPE(t) ((t)->Base >= TypeInt && (t)->Base <= TypeUnsignedLong)
#define IS_INTEGER_NUMERIC(v) IS_INTEGER_NUMERIC_TYPE((v)->Typ)
#define IS_NUMERIC_COERCIBLE(v) (IS_INTEGER_NUMERIC(v) || IS_FP(v))
#define IS_NUMERIC_COERCIBLE_PLUS_POINTERS(v,ap) (IS_NUMERIC_COERCIBLE(v) || IS_POINTER_COERCIBLE(v,ap))


struct Table;

/* lexical tokens */
enum LexToken {
    /* 0x00 */ TokenNone,
    /* 0x01 */ TokenComma,
    /* 0x02 */ TokenAssign,
               TokenAddAssign,
               TokenSubtractAssign,
               TokenMultiplyAssign,
               TokenDivideAssign,
               TokenModulusAssign,
    /* 0x08 */ TokenShiftLeftAssign,
               TokenShiftRightAssign,
               TokenArithmeticAndAssign,
               TokenArithmeticOrAssign,
               TokenArithmeticExorAssign,
    /* 0x0d */ TokenQuestionMark,
               TokenColon,
    /* 0x0f */ TokenLogicalOr,
    /* 0x10 */ TokenLogicalAnd,
    /* 0x11 */ TokenArithmeticOr,
    /* 0x12 */ TokenArithmeticExor,
    /* 0x13 */ TokenAmpersand,
    /* 0x14 */ TokenEqual,
               TokenNotEqual,
    /* 0x16 */ TokenLessThan,
               TokenGreaterThan,
               TokenLessEqual,
               TokenGreaterEqual,
    /* 0x1a */ TokenShiftLeft,
               TokenShiftRight,
    /* 0x1c */ TokenPlus,
               TokenMinus,
    /* 0x1e */ TokenAsterisk,
               TokenSlash,
               TokenModulus,
    /* 0x21 */ TokenIncrement,
               TokenDecrement,
               TokenUnaryNot,
               TokenUnaryExor,
               TokenSizeof,
               TokenCast,
    /* 0x27 */ TokenLeftSquareBracket,
               TokenRightSquareBracket,
               TokenDot,
               TokenArrow,
  /* 0x2b ( */ TokenOpenParen,
               TokenCloseParen,
    /* 0x2d */ TokenIdentifier,
               TokenIntegerConstant,
               TokenFPConstant,
               TokenStringConstant,
               TokenCharacterConstant,
    /* 0x32 */ TokenSemicolon,
               TokenEllipsis,
    /* 0x34 */ TokenLeftBrace,
               TokenRightBrace,
    /* 0x36 */ TokenIntType,
               TokenCharType,
               TokenFloatType,
               TokenDoubleType,
               TokenVoidType,
               TokenEnumType,
    /* 0x3c */ TokenLongType,
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
    /* 0x46 */ TokenContinue,
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
    /* 0x52 */ TokenHashDefine,
               TokenHashInclude,
               TokenHashIf,
               TokenHashIfdef,
               TokenHashIfndef,
               TokenHashElse,
               TokenHashEndif,
    /* 0x59 */ TokenNew,
               TokenDelete,
    /* 0x5b */ TokenOpenMacroBracket,
    /* 0x5c */ TokenEOF,
               TokenEndOfLine,
               TokenEndOfFunction,
               TokenBackSlash
};

/* used in dynamic memory allocation */
struct AllocNode {
    unsigned int Size;
    struct AllocNode *NextFree;
};

/* whether we're running or skipping code */
enum RunMode {
    RunModeRun,                 /* we're running code as we parse it */
    RunModeSkip,                /* skipping code, not running */
    RunModeReturn,              /* returning from a function */
    RunModeCaseSearch,          /* searching for a case label */
    RunModeBreak,               /* breaking out of a switch/while/do */
    RunModeContinue,            /* as above but repeat the loop */
    RunModeGoto                 /* searching for a goto label */
};
/* values */
enum BaseType {
    TypeVoid,                   /* no type */
    TypeInt,                    /* integer */
    TypeShort,                  /* short integer */
    TypeChar,                   /* a single character (signed) */
    TypeLong,                   /* long integer */
    TypeUnsignedInt,            /* unsigned integer */
    TypeUnsignedShort,          /* unsigned short integer */
    TypeUnsignedChar,           /* unsigned 8-bit number */ /* must be before unsigned long */
    TypeUnsignedLong,           /* unsigned long integer */
    TypeFP,                     /* floating point */
    TypeFunction,               /* a function */
    TypeMacro,                  /* a macro */
    TypePointer,                /* a pointer */
    TypeArray,                  /* an array of a sub-type */
    TypeStruct,                 /* aggregate type */
    TypeUnion,                  /* merged type */
    TypeEnum,                   /* enumerated integer type */
    TypeGotoLabel,              /* a label we can "goto" */
    Type_Type                   /* a type for storing types */
};

/* data type */
struct ValueType {
    enum BaseType Base;             /* what kind of type this is */
    int ArraySize;                  /* the size of an array type */
    int Sizeof;                     /* the storage required */
    int AlignBytes;                 /* the alignment boundary of this type */
    const char *Identifier;         /* the name of a struct or union */
    struct ValueType *FromType;     /* the type we're derived from (or NULL) */
    struct ValueType *DerivedTypeList;  /* first in a list of types derived from this one */
    struct ValueType *Next;         /* next item in the derived type list */
    struct Table *Members;          /* members of a struct or union */
#if 0
    struct Table *MemberFunctions;  /* member functions table for structs (or NULL) */
    char *ThisPtr;
#endif
    int OnHeap;                     /* true if allocated on the heap */
    int StaticQualifier;            /* true if it's a static */
};

/* function definition */
struct FuncDef {
    struct ValueType *ReturnType;   /* the return value type */
    int NumParams;                  /* the number of parameters */
    int VarArgs;                    /* has a variable number of arguments after
                                        the explicitly specified ones */
    struct ValueType **ParamType;   /* array of parameter types */
    char **ParamName;               /* array of parameter names */
    void (*Intrinsic)();            /* intrinsic call address or NULL */
    struct ParseState Body;         /* lexical tokens of the function body if
                                        not intrinsic */
};

/* macro definition */
struct MacroDef {
    int NumParams;              /* the number of parameters */
    char **ParamName;           /* array of parameter names */
    struct ParseState Body;     /* lexical tokens of the function body
                                        if not intrinsic */
};

/* values */
union AnyValue {
    char Character;
    short ShortInteger;
    int Integer;
    long LongInteger;
    unsigned short UnsignedShortInteger;
    unsigned int UnsignedInteger;
    unsigned long UnsignedLongInteger;
    unsigned char UnsignedCharacter;
    char *Identifier;
    char ArrayMem[2];       /* placeholder for where the data starts,
                                doesn't point to it */
    struct ValueType *Typ;
    struct FuncDef FuncDef;
    struct MacroDef MacroDef;
    double FP;
    void *Pointer;      /* unsafe native pointers */
};

struct Value {
    struct ValueType *Typ;      /* the type of this value */
    union AnyValue *Val;        /* pointer to the AnyValue which holds the actual content */
    struct Value *LValueFrom;   /* if an LValue, this is a Value our LValue is contained within (or NULL) */
    char ValOnHeap;             /* this Value is on the heap */
    char ValOnStack;            /* the AnyValue is on the stack along with this Value */
    char AnyValOnHeap;          /* the AnyValue is separately allocated from the Value on the heap */
    char IsLValue;              /* is modifiable and is allocated somewhere we can usefully modify it */
    int ScopeID;                /* to know when it goes out of scope */
    char OutOfScope;
//    struct Value *MemberThisPtr;    /* 'this' pointer for member function calls */
};

/* hash table data structure */
struct TableEntry {
    struct TableEntry *Next;        /* next item in this hash chain */
    const char *DeclFileName;       /* where the variable was declared */
    unsigned short DeclLine;
    unsigned short DeclColumn;

    union TableEntryPayload {
        struct ValueEntry {
            char *Key;              /* points to the shared string table */
            struct Value *Val;      /* the value we're storing */
        } v;                        /* used for tables of values */

        char Key[1];                /* dummy size - used for the shared string table */

        /* defines a breakpoint */
        struct BreakpointEntry {
            const char *FileName;
            short int Line;
            short int CharacterPos;
        } b;

    } p;
};

struct Table {
    short Size;
    short OnHeap;
    struct TableEntry **HashTable;
};

/* stack frame for function calls */
struct StackFrame {
    struct ParseState ReturnParser;         /* how we got here */
    const char *FuncName;                   /* the name of the function we're in */
    struct Value *ReturnValue;              /* copy the return value here */
    struct Value **Parameter;               /* array of parameter values */
    int NumParams;                          /* the number of parameters */
    struct Table LocalTable;                /* the local variables and parameters */
    struct TableEntry *LocalHashTable[LOCAL_TABLE_SIZE];
    struct StackFrame *PreviousStackFrame;  /* the next lower stack frame */
//    struct Value ThisValue;                 /* 'this' pointer Value for member functions */
//    union AnyValue ThisData;                /* data storage for 'this' pointer */
//    int HasThis;                            /* true if this is a member function call */
};

/* lexer state */
enum LexMode {
    LexModeNormal,
    LexModeHashInclude,
    LexModeHashDefine,
    LexModeHashDefineSpace,
    LexModeHashDefineSpaceIdent
};

struct LexState {
    const char *Pos;
    const char *End;
    const char *FileName;
    int Line;
    int CharacterPos;
    const char *SourceText;
    enum LexMode Mode;
    int EmitExtraNewlines;
};

/* library function definition */
struct LibraryFunction {
    void (*Func)(struct ParseState *Parser, struct Value *, struct Value **, int);
    const char *Prototype;
};

/* output stream-type specific state information */
union OutputStreamInfo {
    struct StringOutputStream {
        struct ParseState *Parser;
        char *WritePos;
    } Str;
};

/* stream-specific method for writing characters to the console */
typedef void CharWriter(unsigned char, union OutputStreamInfo *);

/* used when writing output to a string - eg. sprintf() */
struct OutputStream {
    CharWriter *Putch;
    union OutputStreamInfo i;
};

/* possible results of parsing a statement */
enum ParseResult { ParseResultEOF, ParseResultError, ParseResultOk };

/* a chunk of heap-allocated tokens we'll cleanup when we're done */
struct CleanupTokenNode {
    void *Tokens;
    const char *SourceText;
    struct CleanupTokenNode *Next;
};

/* linked list of lexical tokens used in interactive mode */
struct TokenLine {
    struct TokenLine *Next;
    unsigned char *Tokens;
    int NumBytes;
};


/* a list of libraries we can include */
struct IncludeLibrary {
    char *IncludeName;
    void (*SetupFunction)(Picoc *pc);
    struct LibraryFunction *FuncList;
    const char *SetupCSource;
    struct IncludeLibrary *NextLib;
};

#define FREELIST_BUCKETS (8)        /* freelists for 4, 8, 12 ... 32 byte allocs */
#define SPLIT_MEM_THRESHOLD (16)    /* don't split memory which is close in size */
#define BREAKPOINT_TABLE_SIZE (21)

struct TypeNameEntry 
{
    const char *VarName;
    const char *TypeName;
    struct TypeNameEntry *Next;
};

struct TypeNameTable 
{
    struct TypeNameEntry *HashTable[VARIABLE_TYPE_TABLE_SIZE];
};

/* the entire state of the picoc system */
struct Picoc {
    /* parser global data */
    struct Table GlobalTable;
    struct CleanupTokenNode *CleanupTokenList;
    struct TableEntry *GlobalHashTable[GLOBAL_TABLE_SIZE];

    /* lexer global data */
    struct TokenLine *InteractiveHead;
    struct TokenLine *InteractiveTail;
    struct TokenLine *InteractiveCurrentLine;
    int LexUseStatementPrompt;
    union AnyValue LexAnyValue;
    struct Value LexValue;
    struct Table ReservedWordTable;
    struct TableEntry *ReservedWordHashTable[RESERVED_WORD_TABLE_SIZE];

    /* the table of string literal values */
    struct Table StringLiteralTable;
    struct TableEntry *StringLiteralHashTable[STRING_LITERAL_TABLE_SIZE];

    /* the stack */
    struct StackFrame *TopStackFrame;

    /* the value passed to exit() */
    int PicocExitValue;

    /* a list of libraries we can include */
    struct IncludeLibrary *IncludeLibList;

    /* heap memory */
    unsigned char *HeapMemory;  /* stack memory since our heap is malloc()ed */
    void *HeapBottom;           /* the bottom of the (downward-growing) heap */
    void *StackFrame;           /* the current stack frame */
    void *HeapStackTop;         /* the top of the stack */

    struct AllocNode *FreeListBucket[FREELIST_BUCKETS]; /* we keep a pool of freelist buckets to reduce fragmentation */
    struct AllocNode *FreeListBig;    /* free memory which doesn't fit in a bucket */

    /* types */
    struct ValueType UberType;
    struct ValueType IntType;
    struct ValueType ShortType;
    struct ValueType CharType;
    struct ValueType LongType;
    struct ValueType UnsignedIntType;
    struct ValueType UnsignedShortType;
    struct ValueType UnsignedLongType;
    struct ValueType UnsignedCharType;
    struct ValueType FPType;
    struct ValueType VoidType;
    struct ValueType TypeType;
    struct ValueType FunctionType;
    struct ValueType MacroType;
    struct ValueType EnumType;
    struct ValueType GotoLabelType;
    struct ValueType *CharPtrType;
    struct ValueType *CharPtrPtrType;
    struct ValueType *CharArrayType;
    struct ValueType *VoidPtrType;

    /* debugger */
    struct Table BreakpointTable;
    struct TableEntry *BreakpointHashTable[BREAKPOINT_TABLE_SIZE];
    int BreakpointCount;
    int DebugManualBreak;

    /* C library */
    int BigEndian;
    int LittleEndian;

    IOFILE *CStdOut;
    IOFILE CStdOutBase;

    /* the picoc version string */
    const char *VersionString;

    /* exit longjump buffer */
#if defined(UNIX_HOST) || defined(WIN32)
    jmp_buf PicocExitBuf;
#endif
    struct Table StringTable;
    struct TableEntry *StringHashTable[STRING_TABLE_SIZE];
    char *StrEmpty;
    struct ValueType *StructType;
#if 0
    struct Table VariableTypeTable;  /* Maps variable name -> type name */
    struct TableEntry *VariableTypeHashTable[VARIABLE_TYPE_TABLE_SIZE];
#else
    struct TypeNameTable VarTypeMap;
#endif
};

void PrintLexToken(enum LexToken token);
void ShowX(const char* function,const char* table,const char* word,size_t length);
inline
int IsMemberFunction(const char* s)
{   return strchr(s,'.') != 0;
}

#endif /* INTERPRETER_H */
