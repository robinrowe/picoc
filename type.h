/* type.h */
void TypeInit(Picoc *pc);
void TypeCleanup(Picoc *pc);
/* member function support - type.c */
void TypeAddMemberFunction(Picoc *pc, struct ValueType *StructType, const char *MemberName, 
                          const char *MangledName, struct ValueType *FuncType);
int TypeGetMemberFunction(struct ValueType *StructType, const char *MemberName,
                         const char **MangledName, struct ValueType **FuncType);
int TypeSize(struct ValueType *Typ, int ArraySize, int Compact);
int TypeSizeValue(struct Value *Val, int Compact);
int TypeStackSizeValue(struct Value *Val);
int TypeLastAccessibleOffset(Picoc *pc, struct Value *Val);
int TypeParseFront(struct ParseState *Parser, struct ValueType **Typ,
    int *IsStatic);
void TypeParseIdentPart(struct ParseState *Parser,
    struct ValueType *BasicTyp, struct ValueType **Typ, char **Identifier);
void TypeParse(struct ParseState *Parser, struct ValueType **Typ,
    char **Identifier, int *IsStatic);
struct ValueType *TypeGetMatching(Picoc *pc, struct ParseState *Parser,
    struct ValueType *ParentType, enum BaseType Base, int ArraySize, const char *Identifier, int AllowDuplicates);
struct ValueType *TypeCreateOpaqueStruct(Picoc *pc, struct ParseState *Parser,
    const char *StructName, int Size);
int TypeIsForwardDeclared(struct ParseState *Parser, struct ValueType *Typ);
