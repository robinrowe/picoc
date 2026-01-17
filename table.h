/* table.h */
void TableInit(Picoc *pc);
char *TableStrRegister(Picoc *pc, const char *Str, size_t Len);
char *TableMemberFunctionRegister(Picoc *pc, const char *Str);
void TableInitTable(struct Table *Tbl, struct TableEntry **HashTable,
    int Size, int OnHeap);
int TableSet(Picoc *pc, struct Table *Tbl, char *Key, struct Value *Val,
    const char *DeclFileName, int DeclLine, int DeclColumn);
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn);
struct Value *TableDelete(Picoc *pc, struct Table *Tbl, const char *Key);
char *TableSetIdentifier(Picoc *pc, struct Table *Tbl, const char *Ident,
    int IdentLen);
void TableStrFree(Picoc *pc);
unsigned int TableHash(const char *Key, int Len);
struct TableEntry *TableSearch(struct Table *Tbl, const char *Key,int *AddAt);
struct TableEntry *TableSearchIdentifier(struct Table *Tbl,const char *Key, int Len, int *AddAt);