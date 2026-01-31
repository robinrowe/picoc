
/* heap.h */
#ifdef DEBUG_HEAP
void ShowBigList(Engine *pc);
#endif
void HeapInit(Engine *pc, int StackSize);
void HeapCleanup(Engine *pc);
void *HeapAllocStack(Engine *pc, int Size);
int HeapPopStack(Engine *pc, void *Addr, int Size);
void HeapUnpopStack(Engine *pc, int Size);
void HeapPushStackFrame(Engine *pc);
int HeapPopStackFrame(Engine *pc);
void *HeapAllocMem(Engine *pc, int Size);
void HeapFreeMem(Engine *pc, void *Mem);
