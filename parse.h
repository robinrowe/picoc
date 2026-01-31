/* parse.h */
/* the following are defined in engine.h:
 * void PicocParse(const char *FileName, const char *Source, int SourceLen, int RunIt, int CleanupNow, int CleanupSource);
 * void PicocParseInteractive(); */

 #ifndef parse_h
 #define parse_h

typedef struct Picoc Picoc;
typedef FILE IOFILE;
 
/* parser state - has all this detail so we can parse nested files */
struct ParseState {
    struct Picoc *pc;                  /* the itrapc instance this parser is a part of */
    const unsigned char *Pos;   /* the character position in the source text */
    char *FileName;             /* what file we're executing (registered string) */
    short int Line;             /* line number we're executing */
    short int CharacterPos;     /* character/column in the line we're executing */
    enum RunMode Mode;          /* whether to skip or run code */
    int SearchLabel;            /* what case label we're searching for */
    const char *SearchGotoLabel;/* what goto label we're searching for */
    const char *SourceText;     /* the entire source text */
    short int HashIfLevel;      /* how many "if"s we're nested down */
    short int HashIfEvaluateToLevel;    /* if we're not evaluating an if branch,
                                          what the last evaluated level was */
    char DebugMode;             /* debugging mode */
    int ScopeID;   /* for keeping track of local variables (free them after t
                      hey go out of scope) */
};

void PicocParseInteractiveNoStartPrompt(Picoc *pc, int EnableDebugger);
enum ParseResult ParseStatement(struct ParseState *Parser,
    int CheckTrailingSemicolon);
struct Value *ParseFunctionDefinition(struct ParseState *Parser,
    struct ValueType *ReturnType, char *Identifier,struct ValueType *this_type);
void ParseCleanup(Picoc *pc);
void ParserCopyPos(struct ParseState *To, struct ParseState *From);
void ParserCopy(struct ParseState *To, struct ParseState *From);
struct Value *ParseMemberFunctionDefinition(struct ParseState *Parser,
    struct ValueType *StructType, struct ValueType *ReturnType, char *Identifier);

#endif