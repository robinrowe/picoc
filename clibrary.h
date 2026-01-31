
/* clibrary.h */
void BasicIOInit(Engine *pc);
void LibraryInit(Engine *pc);
void LibraryAdd(Engine *pc, struct LibraryFunction *FuncList);
#if 0
void CLibraryInit(Engine *pc);
void PrintCh(char OutCh, IOFILE *Stream);
void PrintSimpleInt(long Num, IOFILE *Stream);
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify,
  IOFILE *Stream);
void PrintStr(const char *Str, IOFILE *Stream);
void PrintFP(double Num, IOFILE *Stream);
void PrintType(struct ValueType *Typ, IOFILE *Stream);
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue,
  struct Value **Param, int NumArgs);
#endif
