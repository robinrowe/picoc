/* table.h */

#ifndef table_h
#define table_h

void TableInit(Engine *pc);
char *TableStrRegister(Engine *pc, const char *Str, size_t Len);
char *TableMemberFunctionRegister(Engine *pc, const char *Str);
void TableInitTable(struct Table *Tbl, struct TableEntry **HashTable,
    int Size, int OnHeap);
int TableSet(Engine *pc, struct Table *Tbl, char *Key, struct Value *Val,
    const char *DeclFileName, int DeclLine, int DeclColumn);
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn);
struct Value *TableDelete(Engine *pc, struct Table *Tbl, const char *Key);
char *TableSetIdentifier(Engine *pc, struct Table *Tbl, const char *Ident,
    int IdentLen);
void TableStrFree(Engine *pc);
unsigned int TableHash(const char *Key, int Len);
struct TableEntry *TableSearch(struct Table *Tbl, const char *Key,int *AddAt);
struct TableEntry *TableSearchIdentifier(struct Table *Tbl,const char *Key, int Len, int *AddAt);
/* Store: variable name -> type name */
void StoreVarType(Engine *pc, const char *VarName, const char *TypeName);
/* Lookup: variable name -> type name */
const char* LookupVarType(Engine *pc, const char *VarName);

#endif