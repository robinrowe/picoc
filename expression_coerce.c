/* expression_coerce.c - Type coercion functions */
#include "expression_coerce.h"
#include "interpreter.h"

/* Coerce any numeric value to a signed integer */
intptr_t ExpressionCoerceInteger(Value *Val)
{
    switch (Val->Typ->Base) {
    case TypeInt:
        return (intptr_t)Val->Val->Integer;
    case TypeChar:
        return (intptr_t)Val->Val->Character;
    case TypeShort:
        return (intptr_t)Val->Val->ShortInteger;
    case TypeLong:
        return (intptr_t)Val->Val->LongInteger;
    case TypeUnsignedInt:
        return (intptr_t)Val->Val->UnsignedInteger;
    case TypeUnsignedShort:
        return (intptr_t)Val->Val->UnsignedShortInteger;
    case TypeUnsignedLong:
        return (intptr_t)Val->Val->UnsignedLongInteger;
    case TypeUnsignedChar:
        return (intptr_t)Val->Val->UnsignedCharacter;
    case TypePointer:
        return (intptr_t)Val->Val->Pointer;
    case TypeFP:
        return (intptr_t)Val->Val->FP;
    default:
        return 0;
    }
}

/* Coerce any numeric value to an unsigned integer */
uintptr_t ExpressionCoerceUnsignedInteger(Value *Val)
{
    switch (Val->Typ->Base) {
    case TypeInt:
        return (uintptr_t)Val->Val->Integer;
    case TypeChar:
        return (uintptr_t)Val->Val->Character;
    case TypeShort:
        return (uintptr_t)Val->Val->ShortInteger;
    case TypeLong:
        return (uintptr_t)Val->Val->LongInteger;
    case TypeUnsignedInt:
        return (uintptr_t)Val->Val->UnsignedInteger;
    case TypeUnsignedShort:
        return (uintptr_t)Val->Val->UnsignedShortInteger;
    case TypeUnsignedLong:
        return (uintptr_t)Val->Val->UnsignedLongInteger;
    case TypeUnsignedChar:
        return (uintptr_t)Val->Val->UnsignedCharacter;
    case TypePointer:
        return (uintptr_t)Val->Val->Pointer;
    case TypeFP:
        return (uintptr_t)Val->Val->FP;
    default:
        return 0;
    }
}

/* Coerce any numeric value to a floating point */
double ExpressionCoerceFP(Value *Val)
{
    switch (Val->Typ->Base) {
    case TypeInt:
        return (double)Val->Val->Integer;
    case TypeChar:
        return (double)Val->Val->Character;
    case TypeShort:
        return (double)Val->Val->ShortInteger;
    case TypeLong:
        return (double)Val->Val->LongInteger;
    case TypeUnsignedInt:
        return (double)Val->Val->UnsignedInteger;
    case TypeUnsignedShort:
        return (double)Val->Val->UnsignedShortInteger;
    case TypeUnsignedLong:
        return (double)Val->Val->UnsignedLongInteger;
    case TypeUnsignedChar:
        return (double)Val->Val->UnsignedCharacter;
    case TypeFP:
        return Val->Val->FP;
    default:
        return 0.0;
    }
}
