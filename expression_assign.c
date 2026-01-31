/* expression_assign.c - Assignment operations */
#include "expression_assign.h"
#include "interpreter.h"
#include "variable.h"
#include "type.h"
#include "heap.h"

/* assign an integer value */
long ExpressionAssignInt(ParseState *Parser, Value *DestValue,
    long FromInt, int After)
{
    long Result;

    if (!DestValue->IsLValue)
        ProgramFail(Parser, "can't assign to this");

    if (After)
        Result = ExpressionCoerceInteger(DestValue);
    else
        Result = FromInt;

    switch (DestValue->Typ->Base) {
    case TypeInt:
        DestValue->Val->Integer = (int)FromInt;
        break;
    case TypeShort:
        DestValue->Val->ShortInteger = (short)FromInt;
        break;
    case TypeChar:
        DestValue->Val->Character = (char)FromInt;
        break;
    case TypeLong:
        DestValue->Val->LongInteger = (long)FromInt;
        break;
    case TypeUnsignedInt:
        DestValue->Val->UnsignedInteger = (unsigned int)FromInt;
        break;
    case TypeUnsignedShort:
        DestValue->Val->UnsignedShortInteger = (unsigned short)FromInt;
        break;
    case TypeUnsignedLong:
        DestValue->Val->UnsignedLongInteger = (unsigned long)FromInt;
        break;
    case TypeUnsignedChar:
        DestValue->Val->UnsignedCharacter = (unsigned char)FromInt;
        break;
    default:
        break;
    }
    return Result;
}

/* assign a floating point value */
double ExpressionAssignFP(ParseState *Parser, Value *DestValue,
    double FromFP)
{
    if (!DestValue->IsLValue)
        ProgramFail(Parser, "can't assign to this");

    DestValue->Val->FP = FromFP;
    return FromFP;
}

/* assign to a pointer */
void ExpressionAssignToPointer(ParseState *Parser, Value *ToValue,
    Value *FromValue, const char *FuncName, int ParamNo,
    int AllowPointerCoercion)
{
    ValueType *PointedToType = ToValue->Typ->FromType;

    if (FromValue->Typ == ToValue->Typ ||
            FromValue->Typ == Parser->pc->VoidPtrType ||
            (ToValue->Typ == Parser->pc->VoidPtrType &&
            FromValue->Typ->Base == TypePointer)) {
        ToValue->Val->Pointer = FromValue->Val->Pointer; /* plain pointer assignment */
    } else if (FromValue->Typ->Base == TypeArray &&
            (PointedToType == FromValue->Typ->FromType ||
            ToValue->Typ == Parser->pc->VoidPtrType)) {
        /* the form is: blah *x = array of blah */
        ToValue->Val->Pointer = (void *)&FromValue->Val->ArrayMem[0];
    } else if (FromValue->Typ->Base == TypePointer &&
                FromValue->Typ->FromType->Base == TypeArray &&
               (PointedToType == FromValue->Typ->FromType->FromType ||
                ToValue->Typ == Parser->pc->VoidPtrType) ) {
        /* the form is: blah *x = pointer to array of blah */
        ToValue->Val->Pointer = VariableDereferencePointer(FromValue, NULL,
            NULL, NULL, NULL);
    } else if (IS_NUMERIC_COERCIBLE(FromValue) &&
            ExpressionCoerceInteger(FromValue) == 0) {
        /* null pointer assignment */
        ToValue->Val->Pointer = NULL;
    } else if (AllowPointerCoercion && IS_NUMERIC_COERCIBLE(FromValue)) {
        /* assign integer to native pointer */
        ToValue->Val->Pointer =
            (void*)(uintptr_t)ExpressionCoerceUnsignedInteger(FromValue);
    } else if (AllowPointerCoercion && FromValue->Typ->Base == TypePointer) {
        /* assign a pointer to a pointer to a different type */
        ToValue->Val->Pointer = FromValue->Val->Pointer;
    } else {
        AssignFail(Parser, "%t from %t", ToValue->Typ, FromValue->Typ, 0, 0,
            FuncName, ParamNo);
    }
}

/* assign any kind of value */
void ExpressionAssign(ParseState *Parser, Value *DestValue,
    Value *SourceValue, int Force, const char *FuncName, int ParamNo,
    int AllowPointerCoercion)
{
    if (!DestValue->IsLValue && !Force)
        AssignFail(Parser, "not an lvalue", NULL, NULL, 0, 0, FuncName, ParamNo);

    if (IS_NUMERIC_COERCIBLE(DestValue) &&
            !IS_NUMERIC_COERCIBLE_PLUS_POINTERS(SourceValue, AllowPointerCoercion))
        AssignFail(Parser, "%t from %t", DestValue->Typ, SourceValue->Typ, 0, 0,
            FuncName, ParamNo);

    switch (DestValue->Typ->Base) {
    case TypeInt:
        DestValue->Val->Integer = (int)ExpressionCoerceInteger(SourceValue);
        break;
    case TypeShort:
        DestValue->Val->ShortInteger = (short)ExpressionCoerceInteger(SourceValue);
        break;
    case TypeChar:
        DestValue->Val->Character = (char)ExpressionCoerceInteger(SourceValue);
        break;
    case TypeLong:
        DestValue->Val->LongInteger = SourceValue->Val->LongInteger;
        break;
    case TypeUnsignedInt:
        DestValue->Val->UnsignedInteger =
            (unsigned int)ExpressionCoerceUnsignedInteger(SourceValue);
        break;
    case TypeUnsignedShort:
        DestValue->Val->UnsignedShortInteger =
            (unsigned short)ExpressionCoerceUnsignedInteger(SourceValue);
        break;
    case TypeUnsignedLong:
        DestValue->Val->UnsignedLongInteger = SourceValue->Val->UnsignedLongInteger;
        break;
    case TypeUnsignedChar:
        DestValue->Val->UnsignedCharacter =
            (unsigned char)ExpressionCoerceUnsignedInteger(SourceValue);
        break;
    case TypeFP:
        if (!IS_NUMERIC_COERCIBLE_PLUS_POINTERS(SourceValue, AllowPointerCoercion))
            AssignFail(Parser, "%t from %t", DestValue->Typ, SourceValue->Typ,
                0, 0, FuncName, ParamNo);
        DestValue->Val->FP = (double)ExpressionCoerceFP(SourceValue);
        break;
    case TypePointer:
        ExpressionAssignToPointer(Parser, DestValue, SourceValue, FuncName,
            ParamNo, AllowPointerCoercion);
        break;
    case TypeArray:
        if (SourceValue->Typ->Base == TypeArray && DestValue->Typ->ArraySize == 0) {
            /* destination array is unsized - need to resize */
            DestValue->Typ = SourceValue->Typ;
            VariableRealloc(Parser, DestValue, TypeSizeValue(DestValue, false));

            if (DestValue->LValueFrom != NULL) {
                /* copy the resized value back to the LValue */
                DestValue->LValueFrom->Val = DestValue->Val;
                DestValue->LValueFrom->AnyValOnHeap = DestValue->AnyValOnHeap;
            }
        }
        /* char array = "abcd" */
        if (DestValue->Typ->FromType->Base == TypeChar &&
                SourceValue->Typ->Base == TypePointer &&
                SourceValue->Typ->FromType->Base == TypeChar) {
            if (DestValue->Typ->ArraySize == 0) { /* char x[] = "abcd" */
                int Size = strlen(SourceValue->Val->Pointer) + 1;
#ifdef DEBUG_ARRAY_INITIALIZER
                PRINT_SOURCE_POS();
                fprintf(stderr, "str size: %d\n", Size);
#endif
                DestValue->Typ = TypeGetMatching(Parser->pc, Parser,
                            DestValue->Typ->FromType, DestValue->Typ->Base,
                            Size, DestValue->Typ->Identifier, true);
                VariableRealloc(Parser, DestValue, TypeSizeValue(DestValue,
                    false));
            }
#ifdef DEBUG_ARRAY_INITIALIZER
            PRINT_SOURCE_POS();
            fprintf(stderr, "char[%d] from char* (len=%d)\n",
                    DestValue->Typ->ArraySize,
                    strlen(SourceValue->Val->Pointer));
#endif
            memcpy((void*)DestValue->Val, SourceValue->Val->Pointer,
                TypeSizeValue(DestValue, false));
            break;
        }

        if (DestValue->Typ != SourceValue->Typ)
            AssignFail(Parser, "%t from %t", DestValue->Typ, SourceValue->Typ,
                0, 0, FuncName, ParamNo);

        if (DestValue->Typ->ArraySize != SourceValue->Typ->ArraySize)
            AssignFail(Parser, "from an array of size %d to one of size %d",
                NULL, NULL, DestValue->Typ->ArraySize,
                SourceValue->Typ->ArraySize, FuncName, ParamNo);

        memcpy((void*)DestValue->Val, (void*)SourceValue->Val,
                TypeSizeValue(DestValue, false));
        break;
    case TypeStruct:
    case TypeUnion:
        if (DestValue->Typ != SourceValue->Typ)
            AssignFail(Parser, "%t from %t", DestValue->Typ, SourceValue->Typ,
                        0, 0, FuncName, ParamNo);
        memcpy((void*)DestValue->Val, (void*)SourceValue->Val,
                TypeSizeValue(SourceValue, false));
        break;
    default:
        AssignFail(Parser, "%t", DestValue->Typ, NULL, 0, 0, FuncName, ParamNo);
        break;
    }
}
