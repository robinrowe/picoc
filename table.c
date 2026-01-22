/* picoc hash table module. This hash table code is used for both symbol tables
 * and the shared string table. */

#include "interpreter.h"
#include "table.h"
#include "variable.h"
#include "heap.h"

#define MAGIC
#define VERBOSE

/* initialize the shared string system */
void TableInit(Picoc *pc)
{
    TableInitTable(&pc->StringTable, &pc->StringHashTable[0],
            STRING_TABLE_SIZE, true);
    pc->StrEmpty = TableStrRegister(pc, "",0);
    /* Initialize VarTypeMap hash table to NULL, not really necessary as Picoc memset everything to zero */
    memset(pc->VarTypeMap.HashTable, 0, sizeof(pc->VarTypeMap.HashTable));
}

/* hash function for strings */
unsigned int TableHash(const char *Key, int Len)
{
    unsigned int Hash = Len;
    int Offset;
    int Count;

    for (Count = 0, Offset = 8; Count < Len; Count++, Offset+=7) {
        if (Offset > sizeof(unsigned int) * 8 - 7)
            Offset -= sizeof(unsigned int) * 8 - 6;

        Hash ^= *Key++ << Offset;
    }

    return Hash;
}

/* initialize a table */
void TableInitTable(struct Table *Tbl, struct TableEntry **HashTable, int Size,
    int OnHeap)
{
    Tbl->Size = Size;
    Tbl->OnHeap = OnHeap;
    Tbl->HashTable = HashTable;
    memset((void*)HashTable, '\0', sizeof(struct TableEntry*) * Size);
}

/* check a hash table entry for a key */
struct TableEntry *TableSearch(struct Table *Tbl, const char *Key,
    int *AddAt)
{
    /* Shared strings have unique addresses so we don't need to hash them. 
       The Key parameter is not hashed by its string content - it's hashed by its memory address. */
    uintptr_t HashValue = ((uintptr_t)Key) % Tbl->Size;
    struct TableEntry *Entry;
    for (Entry = Tbl->HashTable[HashValue]; Entry != NULL; Entry = Entry->Next) {
        if (Entry->p.v.Key == Key)
            return Entry;   /* found */
    }
    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set an identifier to a value. returns FALSE if it already exists.
 * Key must be a shared string from TableStrRegister() */
int TableSet(Picoc *pc, struct Table *Tbl, char *Key, struct Value *Val,
    const char *DeclFileName, int DeclLine, int DeclColumn)
{
    int AddAt;
    struct TableEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);

    if (FoundEntry == NULL) {   /* add it to the table */
        struct TableEntry *NewEntry = VariableAlloc(pc, NULL,
            sizeof(struct TableEntry), Tbl->OnHeap);
        NewEntry->DeclFileName = DeclFileName;
        NewEntry->DeclLine = DeclLine;
        NewEntry->DeclColumn = DeclColumn;
        NewEntry->p.v.Key = Key;
        NewEntry->p.v.Val = Val;
        NewEntry->Next = Tbl->HashTable[AddAt];
        Tbl->HashTable[AddAt] = NewEntry;
        return true;
    }

    return false;
}

/* find a value in a table. returns FALSE if not found.
 * Key must be a shared string from TableStrRegister() */
 #if 1
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn)
{
#ifdef MAGIC2
    if(strstr(Key,"Foo") || strstr(Key,"foo") || strstr(Key,"Global"))
    {    printf("MAGIC: table=%p %p TableGet: %p \"%s\"\n",Tbl,Tbl->HashTable,&Key, Key);
    }
#endif
    int AddAt;
    struct TableEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);
    if (FoundEntry == NULL)
    {   ShowX("<TableSearch","NOT found",Key,0);
        return false;
    }
    *Val = FoundEntry->p.v.Val;
    if (DeclFileName != NULL) {
        *DeclFileName = FoundEntry->DeclFileName;
        *DeclLine = FoundEntry->DeclLine;
        *DeclColumn = FoundEntry->DeclColumn;
    }
    ShowX("<TableGet","found",Key,0);
    return true;
}
#else
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn)
{
    int AddAt;
    struct TableEntry *FoundEntry;
    
    /* Use content-based search for identifier lookups */
    FoundEntry = TableSearchIdentifier(Tbl, Key, strlen(Key), &AddAt);
    
    if (FoundEntry == NULL)
        return false;
        
    *Val = FoundEntry->p.v.Val;
    if (DeclFileName != NULL) {
        *DeclFileName = FoundEntry->DeclFileName;
        *DeclLine = FoundEntry->DeclLine;
        *DeclColumn = FoundEntry->DeclColumn;
    }
    return true;
}
#endif
/* remove an entry from the table */
struct Value *TableDelete(Picoc *pc, struct Table *Tbl, const char *Key)
{
    /* shared strings have unique addresses so we don't need to hash them */
    uintptr_t HashValue = ((uintptr_t)Key) % Tbl->Size;
    struct TableEntry **EntryPtr;

    for (EntryPtr = &Tbl->HashTable[HashValue];
            *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next) {
        if ((*EntryPtr)->p.v.Key == Key) {
            struct TableEntry *DeleteEntry = *EntryPtr;
            struct Value *Val = DeleteEntry->p.v.Val;
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);

            return Val;
        }
    }

    return NULL;
}

/* check a hash table entry for an identifier */
struct TableEntry *TableSearchIdentifier(struct Table *Tbl,
    const char *Key, int Len, int *AddAt)
{   int HashValue = TableHash(Key, Len) % Tbl->Size;
    struct TableEntry *Entry;
//    ShowX("TableSearchIdentifier","looking",Key,Len);
#ifdef MAGIC2
    if (strstr(Key, "Foo.fooMethod")) {
        printf("MAGIC: TableSearchIdentifier for '%.*s' len=%d hash=%d\n", 
               Len, Key, Len, HashValue);
    }
#endif
#ifdef VERBOSE2
     printf("MAGIC: TableSearchIdentifier: searching for \"%.*s\" (len=%d, hash=%d)\n", 
           Len, Key, Len, HashValue);
#endif
    for (Entry = Tbl->HashTable[HashValue]; Entry != NULL; Entry = Entry->Next) 
    {   if (strncmp(&Entry->p.Key[0], (char*)Key, Len) == 0 && Entry->p.Key[Len] == '\0')
        {   ShowX("<TableEntry","found",Key,Len);   
            return Entry;   /* found */
    }   }
    *AddAt = HashValue;    /* didn't find it in the chain */
    ShowX("<TableEntry","NOT found",Key,Len);
    return NULL;
}

/* set an identifier and return the identifier. share if possible */
char *TableSetIdentifier(Picoc *pc, struct Table *Tbl, const char *Ident,
    int IdentLen)
{   int AddAt;
    struct TableEntry *FoundEntry = TableSearchIdentifier(Tbl, Ident, IdentLen,&AddAt);
    if (FoundEntry != NULL)
    {   ShowX("<TableSetIdentifier","found",Ident,IdentLen);
        return &FoundEntry->p.Key[0];
    }
    /* add it to the table - we economise by not allocating
        the whole structure here */
    struct TableEntry *NewEntry = HeapAllocMem(pc,
        sizeof(struct TableEntry) -
        sizeof(union TableEntryPayload) + IdentLen + 1);
    if (NewEntry == NULL)
        ProgramFailNoParser(pc, "(TableSetIdentifier) out of memory");
    strncpy((char *)&NewEntry->p.Key[0], (char *)Ident, IdentLen);
    NewEntry->p.Key[IdentLen] = '\0';
    NewEntry->Next = Tbl->HashTable[AddAt];
    Tbl->HashTable[AddAt] = NewEntry;
#ifdef MAGIC2
    if(strstr(NewEntry->p.Key,"Foo") || strstr(NewEntry->p.Key,"foo"))
    {    printf("MAGIC: TableSetIdentifier: NewEntry table=%p %p TableSetIdentifier: %p \"%s\"\n",Tbl, Tbl->HashTable,&NewEntry->p.Key,NewEntry->p.Key);
    }
#endif
    ShowX("<Added: TableSetIdentifier","NewEntry",Ident,IdentLen);
    return &NewEntry->p.Key[0];
}

/* register a string in the shared string store */
char *TableStrRegister(Picoc *pc, const char *Str, size_t Len)
{ 
#ifdef MAGIC2
    if(!memcmp(Str,"Foo.fooMethod",13) || !memcmp(Str,"fooFunction",11))
    {   printf("DEBUG: TableStrRegister in StringTable: %.*s\n", (int) Len, Str);
    }
#endif
    ShowX(">Search: TableStrRegister","StringTable",Str,Len);
    return TableSetIdentifier(pc, &pc->StringTable, Str, Len);
}

char *TableMemberFunctionRegister(Picoc *pc, const char *Str)
{    size_t Len = strlen(Str);
    assert(0);
#ifdef MAGIC
    if(!memcmp(Str,"Foo.fooMethod",13) || !memcmp(Str,"fooFunction",11))
    {    printf("DEBUG: TableMemberFunctionRegister in GlobalTable: %.*s\n", (int) Len, Str);
    }
#endif
    return TableSetIdentifier(pc, &pc->GlobalTable, Str, Len);
}

/* free all the strings */
void TableStrFree(Picoc *pc)
{
    int Count;
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;

    for (Count = 0; Count < pc->StringTable.Size; Count++) {
        for (Entry = pc->StringTable.HashTable[Count];
                Entry != NULL; Entry = NextEntry) {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}

/* Store: variable name -> type name */
void StoreVarType(Picoc *pc, const char *VarName, const char *TypeName)
{
    int hash = TableHash(VarName, strlen(VarName)) % VARIABLE_TYPE_TABLE_SIZE;
    struct TypeNameEntry *entry = HeapAllocMem(pc, sizeof(struct TypeNameEntry));
    if (entry == NULL)
        ProgramFail(pc,"StoreVarType out of memory");
    entry->VarName = VarName;
    entry->TypeName = TypeName;
    entry->Next = pc->VarTypeMap.HashTable[hash];
    pc->VarTypeMap.HashTable[hash] = entry;
}

/* Lookup: variable name -> type name */
const char* LookupVarType(Picoc *pc, const char *VarName)
{
    int hash = TableHash(VarName, strlen(VarName)) % VARIABLE_TYPE_TABLE_SIZE;
    for (struct TypeNameEntry *e = pc->VarTypeMap.HashTable[hash]; e != NULL; e = e->Next) 
    {
        if (e->VarName == VarName)  /* Pointer comparison - strings are interned */
            return e->TypeName;
    }
    return NULL;
}

void VarTypeMapCleanup(Picoc *pc)
{
    for (int i = 0; i < VARIABLE_TYPE_TABLE_SIZE; i++) 
    {
        struct TypeNameEntry *entry = pc->VarTypeMap.HashTable[i];
        while (entry != NULL) 
        {
            struct TypeNameEntry *next = entry->Next;
            HeapFreeMem(pc, entry);
            entry = next;
        }
        pc->VarTypeMap.HashTable[i] = NULL;
    }
}