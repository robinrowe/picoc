
/* variable.h */
void VariableInit(Picoc *pc);
void VariableCleanup(Picoc *pc);
void VariableFree(Picoc *pc, struct Value *Val);
void VariableTableCleanup(Picoc *pc, struct Table *HashTable);
void *VariableAlloc(Picoc *pc, struct ParseState *Parser, int Size, int OnHeap);
void VariableStackPop(struct ParseState *Parser, struct Value *Var);
struct Value *VariableAllocValueAndData(Picoc *pc, struct ParseState *Parser,
    int DataSize, int IsLValue, struct Value *LValueFrom, int OnHeap);
struct Value *VariableAllocValueAndCopy(Picoc *pc, struct ParseState *Parser,
    struct Value *FromValue, int OnHeap);
struct Value *VariableAllocValueFromType(Picoc *pc, struct ParseState *Parser,
    struct ValueType *Typ, int IsLValue, struct Value *LValueFrom, int OnHeap);
struct Value *VariableAllocValueFromExistingData(struct ParseState *Parser,
    struct ValueType *Typ, union AnyValue *FromValue, int IsLValue,
    struct Value *LValueFrom);
struct Value *VariableAllocValueShared(struct ParseState *Parser,
    struct Value *FromValue);
struct Value *VariableDefine(Picoc *pc, struct ParseState *Parser,
    char *Ident, struct Value *InitValue, struct ValueType *Typ, int MakeWritable);
struct Value *VariableDefineButIgnoreIdentical(struct ParseState *Parser,
    char *Ident, struct ValueType *Typ, int IsStatic, int *FirstVisit);
int VariableDefined(Picoc *pc, const char *Ident);
int VariableDefinedAndOutOfScope(Picoc *pc, const char *Ident);
void VariableRealloc(struct ParseState *Parser, struct Value *FromValue,
    int NewSize);
void VariableGet(Picoc *pc, struct ParseState *Parser, const char *Ident,
    struct Value **LVal);
void VariableDefinePlatformVar(Picoc *pc, struct ParseState *Parser,
    char *Ident, struct ValueType *Typ, union AnyValue *FromValue, int IsWritable);
void VariableStackFrameAdd(struct ParseState *Parser, const char *FuncName,
    int NumParams);
void VariableStackFramePop(struct ParseState *Parser);
struct Value *VariableStringLiteralGet(Picoc *pc, char *Ident);
void VariableStringLiteralDefine(Picoc *pc, char *Ident, struct Value *Val);
void *VariableDereferencePointer(struct Value *PointerValue,
    struct Value **DerefVal, int *DerefOffset, struct ValueType **DerefType,
    int *DerefIsLValue);
int VariableScopeBegin(struct ParseState *Parser, int *PrevScopeID);
void VariableScopeEnd(struct ParseState *Parser, int ScopeID, int PrevScopeID);
