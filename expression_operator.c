/* expression_operator.c - Expression operator evaluation */
#include <stdint.h>
#include "expression_operator.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "type.h"
#include "expression_call.h"
#include "expression_stack.h"

/* evaluate the first half of a ternary operator x ? y : z */
void ExpressionQuestionMarkOperator(ParseState *Parser,
    ExpressionStack **StackTop, Value *BottomValue, Value *TopValue)
{
    if (!IS_NUMERIC_COERCIBLE(TopValue))
        ProgramFail(Parser, "first argument to '?' should be a number");

    if (ExpressionCoerceInteger(TopValue)) {
        /* the condition's true, return the BottomValue */
        ExpressionStackPushValue(Parser, StackTop, BottomValue);
    } else {
        /* the condition's false, return void */
        ExpressionStackPushValueByType(Parser, StackTop, &Parser->pc->VoidType);
    }
}

/* evaluate the second half of a ternary operator x ? y : z */
void ExpressionColonOperator(ParseState *Parser,
    ExpressionStack **StackTop, Value *BottomValue, Value *TopValue)
{
    if (TopValue->Typ->Base == TypeVoid) {
        /* invoke the "else" part - return the BottomValue */
        ExpressionStackPushValue(Parser, StackTop, BottomValue);
    } else {
        /* it was a "then" - return the TopValue */
        ExpressionStackPushValue(Parser, StackTop, TopValue);
    }
}

/* evaluate a prefix operator */
void ExpressionPrefixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op, Value *TopValue)
{
    Value *Result;
    union AnyValue *ValPtr;
    ValueType *Typ;

#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionPrefixOperator()\n");
#endif

    switch (Op) {
    case TokenAmpersand:
        if (!TopValue->IsLValue)
            ProgramFail(Parser, "can't get the address of this");

        ValPtr = TopValue->Val;
        Result = VariableAllocValueFromType(Parser->pc, Parser,
                    TypeGetMatching(Parser->pc, Parser, TopValue->Typ,
                        TypePointer, 0, Parser->pc->StrEmpty, true),
                    false, NULL, false);
        Result->Val->Pointer = (void*)ValPtr;
        ExpressionStackPushValueNode(Parser, StackTop, Result);
        break;
    case TokenAsterisk:
        if (StackTop != NULL && (*StackTop) != NULL && (*StackTop)->Op == TokenSizeof)
            /* ignored */
            ExpressionStackPushValueByType(Parser, StackTop, TopValue->Typ);
        else
            ExpressionStackPushDereference(Parser, StackTop, TopValue);
        break;
    case TokenSizeof:
        /* return the size of the argument */
        if (TopValue->Typ == &Parser->pc->TypeType)
            Typ = TopValue->Val->Typ;
        else
            Typ = TopValue->Typ;
        if (Typ->FromType != NULL && Typ->FromType->Base == TypeStruct)
            Typ = Typ->FromType;
        ExpressionPushInt(Parser, StackTop, TypeSize(Typ, Typ->ArraySize, true));
        break;
    default:
        /* an arithmetic operator */
        if (TopValue->Typ == &Parser->pc->FPType) {
            /* floating point prefix arithmetic */
            double ResultFP = 0.0;
            switch (Op) {
            case TokenPlus:
                ResultFP = TopValue->Val->FP;
                break;
            case TokenMinus:
                ResultFP = -TopValue->Val->FP;
                break;
            case TokenIncrement:
                ResultFP = ExpressionAssignFP(Parser, TopValue,
                    TopValue->Val->FP+1);
                break;
            case TokenDecrement:
                ResultFP = ExpressionAssignFP(Parser, TopValue,
                    TopValue->Val->FP-1);
                break;
            case TokenUnaryNot:
                ResultFP = !TopValue->Val->FP;
                break;
            default:
                ProgramFail(Parser, "invalid operation");
                break;
            }
            ExpressionPushFP(Parser, StackTop, ResultFP);
        } else if (IS_NUMERIC_COERCIBLE(TopValue)) {
            /* integer prefix arithmetic */
            long ResultInt = 0;
            long TopInt = 0;
            if (TopValue->Typ->Base == TypeLong)
                TopInt = TopValue->Val->LongInteger;
            else
                TopInt = ExpressionCoerceInteger(TopValue);
            switch (Op) {
            case TokenPlus:
                ResultInt = TopInt;
                break;
            case TokenMinus:
                ResultInt = -TopInt;
                break;
            case TokenIncrement:
                ResultInt = ExpressionAssignInt(Parser, TopValue,
                    TopInt+1, false);
                break;
            case TokenDecrement:
                ResultInt = ExpressionAssignInt(Parser, TopValue,
                    TopInt-1, false);
                break;
            case TokenUnaryNot:
                ResultInt = !TopInt;
                break;
            case TokenUnaryExor:
                ResultInt = ~TopInt;
                break;
            default:
                ProgramFail(Parser, "invalid operation");
                break;
            }
            ExpressionPushInt(Parser, StackTop, ResultInt);
        } else if (TopValue->Typ->Base == TypePointer) {
            /* pointer prefix arithmetic */
            int Size = TypeSize(TopValue->Typ->FromType, 0, true);
            Value *StackValue;
            void *ResultPtr = 0;
            if (Op != TokenUnaryNot && TopValue->Val->Pointer == NULL)
                ProgramFail(Parser, "a. invalid use of a NULL pointer");
            if (!TopValue->IsLValue)
                ProgramFail(Parser, "can't assign to this");
            switch (Op) {
            case TokenIncrement:
                ResultPtr = TopValue->Val->Pointer =
                    (void*)((char*)TopValue->Val->Pointer+Size);
                break;
            case TokenDecrement:
                ResultPtr = TopValue->Val->Pointer =
                    (void*)((char*)TopValue->Val->Pointer-Size);
                break;
            case TokenUnaryNot:
                /* conditionally checking a pointer's value */
                TopValue->Val->Pointer =
                    (void*)((char*)TopValue->Val->Pointer);
                    ResultPtr = TopValue->Val->Pointer ? NULL : (void*)0x01;
                break;
            default:
                ProgramFail(Parser, "invalid operation");
                break;
            }
            StackValue = ExpressionStackPushValueByType(Parser, StackTop,
                TopValue->Typ);
            StackValue->Val->Pointer = ResultPtr;
        } else {
            ProgramFail(Parser, "invalid operation");
        }
        break;
    }
}

/* evaluate a postfix operator */
void ExpressionPostfixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op, Value *TopValue)
{
#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionPostfixOperator()\n");
#endif

    if (TopValue->Typ == &Parser->pc->FPType) {
        /* floating point prefix arithmetic */
        double ResultFP = 0.0;

        switch (Op) {
        case TokenIncrement:
            ResultFP = ExpressionAssignFP(Parser, TopValue, TopValue->Val->FP+1);
            break;
        case TokenDecrement:
            ResultFP = ExpressionAssignFP(Parser, TopValue, TopValue->Val->FP-1);
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }
        ExpressionPushFP(Parser, StackTop, ResultFP);
    } else if (IS_NUMERIC_COERCIBLE(TopValue)) {
        long ResultInt = 0;
        long TopInt = ExpressionCoerceInteger(TopValue);
        switch (Op) {
        case TokenIncrement:
            ResultInt = ExpressionAssignInt(Parser, TopValue, TopInt+1, true);
            break;
        case TokenDecrement:
            ResultInt = ExpressionAssignInt(Parser, TopValue, TopInt-1, true);
            break;
        case TokenRightSquareBracket:
            ProgramFail(Parser, "not supported");
            break;
        case TokenCloseParen:
            ProgramFail(Parser, "not supported");
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }
        ExpressionPushInt(Parser, StackTop, ResultInt);
    } else if (TopValue->Typ->Base == TypePointer) {
        /* pointer postfix arithmetic */
        int Size = TypeSize(TopValue->Typ->FromType, 0, true);
        Value *StackValue;
        void *OrigPointer = TopValue->Val->Pointer;

        if (TopValue->Val->Pointer == NULL)
            ProgramFail(Parser, "b. invalid use of a NULL pointer");

        if (!TopValue->IsLValue)
            ProgramFail(Parser, "can't assign to this");

        switch (Op) {
        case TokenIncrement:
            TopValue->Val->Pointer = (void*)((char*)TopValue->Val->Pointer+Size);
            break;
        case TokenDecrement:
            TopValue->Val->Pointer = (void*)((char*)TopValue->Val->Pointer-Size);
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }
        StackValue = ExpressionStackPushValueByType(Parser, StackTop,
            TopValue->Typ);
        StackValue->Val->Pointer = OrigPointer;
    } else {
        ProgramFail(Parser, "invalid operation");
    }
}

/* evaluate an infix operator */
void ExpressionInfixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op,
    Value *BottomValue, Value *TopValue)
{
    long ResultInt = 0;
    Value *StackValue;
    void *Pointer;

#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionInfixOperator()\n");
#endif

    if (BottomValue == NULL || TopValue == NULL)
        ProgramFail(Parser, "invalid expression");

    if (Op == TokenLeftSquareBracket) {
        /* array index */
        int ArrayIndex;
        Value *Result = NULL;

        if (!IS_NUMERIC_COERCIBLE(TopValue))
            ProgramFail(Parser, "array index must be an integer");

        ArrayIndex = ExpressionCoerceInteger(TopValue);

        /* make the array element result */
        switch (BottomValue->Typ->Base) {
        case TypeArray:
            Result = VariableAllocValueFromExistingData(Parser,
            BottomValue->Typ->FromType,
            (union AnyValue*)(&BottomValue->Val->ArrayMem[0] +
                TypeSize(BottomValue->Typ, ArrayIndex, true)),
            BottomValue->IsLValue, BottomValue->LValueFrom);
            break;
        case TypePointer: 
            Result = VariableAllocValueFromExistingData(Parser,
            BottomValue->Typ->FromType,
            (union AnyValue*)((char*)BottomValue->Val->Pointer +
                TypeSize(BottomValue->Typ->FromType, 0, true) * ArrayIndex),
            BottomValue->IsLValue, BottomValue->LValueFrom);
            break;
        default:
            ProgramFail(Parser, "this %t is not an array", BottomValue->Typ);
            break;
        }

        ExpressionStackPushValueNode(Parser, StackTop, Result);
    } else if (Op == TokenQuestionMark)
        ExpressionQuestionMarkOperator(Parser, StackTop, TopValue, BottomValue);
    else if (Op == TokenColon)
        ExpressionColonOperator(Parser, StackTop, TopValue, BottomValue);
    else if ((TopValue->Typ == &Parser->pc->FPType &&
                 BottomValue->Typ == &Parser->pc->FPType) ||
              (TopValue->Typ == &Parser->pc->FPType
                    && IS_NUMERIC_COERCIBLE(BottomValue)) ||
              (IS_NUMERIC_COERCIBLE(TopValue)
                && BottomValue->Typ == &Parser->pc->FPType) ) {
        /* floating point infix arithmetic */
        int ResultIsInt = false;
        double ResultFP = 0.0;
        double TopFP = (TopValue->Typ == &Parser->pc->FPType) ?
            TopValue->Val->FP : (double)ExpressionCoerceInteger(TopValue);
        double BottomFP = (BottomValue->Typ == &Parser->pc->FPType) ?
            BottomValue->Val->FP : (double)ExpressionCoerceInteger(BottomValue);

        switch (Op) {
        case TokenAssign:
            if (IS_FP(BottomValue)) {
                ResultFP = ExpressionAssignFP(Parser, BottomValue, TopFP);
            } else {
                ResultInt = ExpressionAssignInt(Parser, BottomValue,  
                    (long)(TopFP), false);
                ResultIsInt = true;
            }
            break;
        case TokenAddAssign:
            if (IS_FP(BottomValue)) {
                ResultFP = ExpressionAssignFP(Parser, BottomValue, BottomFP + TopFP);
            } else {
                ResultInt = ExpressionAssignInt(Parser, BottomValue,  
                    (long)(BottomFP + TopFP), false);
                ResultIsInt = true;
            }
            break;
        case TokenSubtractAssign:
            if (IS_FP(BottomValue)) {
                ResultFP = ExpressionAssignFP(Parser, BottomValue, BottomFP - TopFP);
            } else {
                ResultInt = ExpressionAssignInt(Parser, BottomValue,  
                    (long)(BottomFP - TopFP), false);
                ResultIsInt = true;
            }
            break;
        case TokenMultiplyAssign:
            if (IS_FP(BottomValue)) {
                ResultFP = ExpressionAssignFP(Parser, BottomValue, BottomFP * TopFP);
            } else {
                ResultInt = ExpressionAssignInt(Parser, BottomValue,  
                    (long)(BottomFP * TopFP), false);
                ResultIsInt = true;
            }
            break;
        case TokenDivideAssign:
            if (IS_FP(BottomValue)) {
                ResultFP = ExpressionAssignFP(Parser, BottomValue, BottomFP / TopFP);
            } else {
                ResultInt = ExpressionAssignInt(Parser, BottomValue,  
                    (long)(BottomFP / TopFP), false);
                ResultIsInt = true;
            }
            break;
        case TokenEqual:
            ResultInt = BottomFP == TopFP;
            ResultIsInt = true;
            break;
        case TokenNotEqual:
            ResultInt = BottomFP != TopFP;
            ResultIsInt = true;
            break;
        case TokenLessThan:
            ResultInt = BottomFP < TopFP;
            ResultIsInt = true;
            break;
        case TokenGreaterThan:
            ResultInt = BottomFP > TopFP;
            ResultIsInt = true;
            break;
        case TokenLessEqual:
            ResultInt = BottomFP <= TopFP;
            ResultIsInt = true;
            break;
        case TokenGreaterEqual:
            ResultInt = BottomFP >= TopFP;
            ResultIsInt = true;
            break;
        case TokenPlus:
            ResultFP = BottomFP + TopFP;
            break;
        case TokenMinus:
            ResultFP = BottomFP - TopFP;
            break;
        case TokenAsterisk:
            ResultFP = BottomFP * TopFP;
            break;
        case TokenSlash:
            ResultFP = BottomFP / TopFP;
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }

        if (ResultIsInt)
            ExpressionPushInt(Parser, StackTop, ResultInt);
        else
            ExpressionPushFP(Parser, StackTop, ResultFP);
    } else if (IS_NUMERIC_COERCIBLE(TopValue) && IS_NUMERIC_COERCIBLE(BottomValue)) {
        /* integer operation */
        long TopInt = ExpressionCoerceInteger(TopValue);
        long BottomInt = ExpressionCoerceInteger(BottomValue);
        switch (Op) {
        case TokenAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue, TopInt, false);
            break;
        case TokenAddAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt + TopInt, false);
            break;
        case TokenSubtractAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt-TopInt, false);
            break;
        case TokenMultiplyAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt*TopInt, false);
            break;
        case TokenDivideAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt/TopInt, false);
            break;
        case TokenModulusAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt%TopInt, false);
            break;
        case TokenShiftLeftAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt<<TopInt, false);
            break;
        case TokenShiftRightAssign:
            if (BottomValue->Typ->Base == TypeUnsignedInt || 
                BottomValue->Typ->Base == TypeUnsignedLong)
                ResultInt = ExpressionAssignInt(Parser, BottomValue, 
                    (uint64_t) BottomInt >> TopInt, false);
            else
                ResultInt = ExpressionAssignInt(Parser, BottomValue, 
                    BottomInt >> TopInt, false);
            break;
        case TokenArithmeticAndAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt&TopInt, false);
            break;
        case TokenArithmeticOrAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt|TopInt, false);
            break;
        case TokenArithmeticExorAssign:
            ResultInt = ExpressionAssignInt(Parser, BottomValue,
                BottomInt^TopInt, false);
            break;
        case TokenLogicalOr:
            ResultInt = BottomInt || TopInt;
            break;
        case TokenLogicalAnd:
            ResultInt = BottomInt && TopInt;
            break;
        case TokenArithmeticOr:
            ResultInt = BottomInt | TopInt;
            break;
        case TokenArithmeticExor:
            ResultInt = BottomInt ^ TopInt;
            break;
        case TokenAmpersand:
            ResultInt = BottomInt & TopInt;
            break;
        case TokenEqual:
            ResultInt = BottomInt == TopInt;
            break;
        case TokenNotEqual:
            ResultInt = BottomInt != TopInt;
            break;
        case TokenLessThan:
            ResultInt = BottomInt < TopInt;
            break;
        case TokenGreaterThan:
            ResultInt = BottomInt > TopInt;
            break;
        case TokenLessEqual:
            ResultInt = BottomInt <= TopInt;
            break;
        case TokenGreaterEqual:
            ResultInt = BottomInt >= TopInt;
            break;
        case TokenShiftLeft:
            ResultInt = BottomInt << TopInt;
            break;
        case TokenShiftRight:
            ResultInt = BottomInt >> TopInt;
            break;
        case TokenPlus:
            ResultInt = BottomInt + TopInt;
            break;
        case TokenMinus:
            ResultInt = BottomInt - TopInt;
            break;
        case TokenAsterisk:
            ResultInt = BottomInt * TopInt;
            break;
        case TokenSlash:
            ResultInt = BottomInt / TopInt;
            break;
        case TokenModulus:
            ResultInt = BottomInt % TopInt;
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }
        ExpressionPushInt(Parser, StackTop, ResultInt);
    } else if (BottomValue->Typ->Base == TypePointer &&
            IS_NUMERIC_COERCIBLE(TopValue)) {
        /* pointer/integer infix arithmetic */
        long TopInt = ExpressionCoerceInteger(TopValue);

        if (Op == TokenEqual || Op == TokenNotEqual) {
            /* comparison to a NULL pointer */
            if (TopInt != 0)
                ProgramFail(Parser, "invalid operation");

            if (Op == TokenEqual)
                ExpressionPushInt(Parser, StackTop,
                    BottomValue->Val->Pointer == NULL);
            else
                ExpressionPushInt(Parser, StackTop,
                    BottomValue->Val->Pointer != NULL);
        } else if (Op == TokenPlus || Op == TokenMinus) {
            /* pointer arithmetic */
            int Size = TypeSize(BottomValue->Typ->FromType, 0, true);

            Pointer = BottomValue->Val->Pointer;
            if (Pointer == NULL)
                ProgramFail(Parser, "c. invalid use of a NULL pointer");

            if (Op == TokenPlus)
                Pointer = (void*)((char*)Pointer + TopInt * Size);
            else
                Pointer = (void*)((char*)Pointer - TopInt * Size);

            StackValue = ExpressionStackPushValueByType(Parser, StackTop,
                BottomValue->Typ);
            StackValue->Val->Pointer = Pointer;
        } else if (Op == TokenAssign && TopInt == 0) {
            /* assign a NULL pointer */
            HeapUnpopStack(Parser->pc, sizeof(Value));
            ExpressionAssign(Parser, BottomValue, TopValue, false, NULL, 0, false);
            ExpressionStackPushValueNode(Parser, StackTop, BottomValue);
        } else if (Op == TokenAddAssign || Op == TokenSubtractAssign) {
            /* pointer arithmetic */
            int Size = TypeSize(BottomValue->Typ->FromType, 0, true);

            Pointer = BottomValue->Val->Pointer;
            if (Pointer == NULL)
                ProgramFail(Parser, "d. invalid use of a NULL pointer");

            if (Op == TokenAddAssign)
                Pointer = (void*)((char*)Pointer + TopInt * Size);
            else
                Pointer = (void*)((char*)Pointer - TopInt * Size);

            HeapUnpopStack(Parser->pc, sizeof(Value));
            BottomValue->Val->Pointer = Pointer;
            ExpressionStackPushValueNode(Parser, StackTop, BottomValue);
        } else
            ProgramFail(Parser, "invalid operation");
    } else if (BottomValue->Typ->Base == TypePointer &&
            TopValue->Typ->Base == TypePointer && Op != TokenAssign) {
        /* pointer/pointer operations */
        char *TopLoc = (char*)TopValue->Val->Pointer;
        char *BottomLoc = (char*)BottomValue->Val->Pointer;

        switch (Op) {
        case TokenEqual:
            ExpressionPushInt(Parser, StackTop, BottomLoc == TopLoc);
            break;
        case TokenNotEqual:
            ExpressionPushInt(Parser, StackTop, BottomLoc != TopLoc);
            break;
        case TokenMinus:
            ExpressionPushInt(Parser, StackTop, BottomLoc - TopLoc);
            break;
        default:
            ProgramFail(Parser, "invalid operation");
            break;
        }
    } else if (Op == TokenAssign) {
        /* assign a non-numeric type */
        HeapUnpopStack(Parser->pc, sizeof(Value));
        ExpressionAssign(Parser, BottomValue, TopValue, false, NULL, 0, false);
        ExpressionStackPushValueNode(Parser, StackTop, BottomValue);
    } else if (Op == TokenCast) {
        /* cast a value to a different type */
        Value *ValueLoc = ExpressionStackPushValueByType(Parser, StackTop,
            BottomValue->Val->Typ);
        ExpressionAssign(Parser, ValueLoc, TopValue, true, NULL, 0, true);
    } else {
        ProgramFail(Parser, "invalid operation");
    }
}

/* do the '.' and '->' operators */
void ExpressionGetStructElement(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Token)
{
    Value *Ident = 0;

    /* get the identifier following the '.' or '->' */
    if (LexGetToken(Parser, &Ident, true) != TokenIdentifier)
        ProgramFail(Parser, "need an structure or union member after '%s'",
            (Token == TokenDot) ? "." : "->");

    if (Parser->Mode == RunModeRun) {
        /* look up the struct element */
        Value *ParamVal = (*StackTop)->Val;
        Value *StructVal = ParamVal;
        ValueType *StructType = ParamVal->Typ;
        char *DerefDataLoc = (char *)ParamVal->Val;
        Value *MemberValue = NULL;
        Value *Result;

        /* if we're doing '->' dereference the struct pointer first */
        if (Token == TokenArrow)
            DerefDataLoc = VariableDereferencePointer(ParamVal, &StructVal,
                NULL, &StructType, NULL);

        if (StructType->Base != TypeStruct && StructType->Base != TypeUnion)
            ProgramFail(Parser,
                "can't use '%s' on something that's not a struct or union %s : it's a %t",
                (Token == TokenDot) ? "." : "->",
                (Token == TokenArrow) ? "pointer" : "", ParamVal->Typ);
        
        ShowX(">TableGet", "Members", Ident->Val->Identifier, 0);
        if (!TableGet(StructType->Members, Ident->Val->Identifier,
                &MemberValue, NULL, NULL, NULL))
            ProgramFail(Parser, "doesn't have a member called '%s'",
                Ident->Val->Identifier);

        /* pop the value */
        HeapPopStack(Parser->pc, ParamVal,
            sizeof(ExpressionStack) +
            sizeof(Value) +
            TypeStackSizeValue(StructVal));
        *StackTop = (*StackTop)->Next;

        /* make the result value for this member only */
        Result = VariableAllocValueFromExistingData(Parser, MemberValue->Typ,
            (void*)(DerefDataLoc + MemberValue->Val->Integer), true,
            (StructVal != NULL) ? StructVal->LValueFrom : NULL);
        ExpressionStackPushValueNode(Parser, StackTop, Result);
    }
}

/* handle member function calls: obj.method() or ptr->method() */
void ExpressionMemberFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Token, const char *MemberName)
{
    Value *ParamVal = (*StackTop)->Val;
    Value *StructVal = ParamVal;
    ValueType *StructType = ParamVal->Typ;
    Value *Ident;
    
    if (LexGetToken(Parser, &Ident, true) != TokenIdentifier) {
        ProgramFail(Parser, "identifier expected");
    }
    
    if (LexGetToken(Parser, &Ident, false) != TokenOpenParen) {
        ProgramFail(Parser, "OpenParen expected");
    }
    
    /* if we're doing '->' dereference the struct pointer first */
    if (Token == TokenArrow) {
        if (StructType->Base != TypePointer)
            ProgramFail(Parser, "-> can only be used on pointers");
        StructType = StructType->FromType;  // Dereference pointer to get struct type
    }
    
    if (StructType->Base != TypeStruct)
        ProgramFail(Parser, "member functions can only be called on structs (got %t)", StructType);
    
    /* Get the struct type name */
    const char *TypeName = StructType->Identifier;
    printf("DEBUG: StructType=%p, Base=%d, Identifier='%s'\n", 
           (void*)StructType, StructType->Base, TypeName ? TypeName : "(null)");
    
    /* consume the identifier token (should match MemberName) */
    if (LexGetToken(Parser, &Ident, true) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");
    
    /* Build the mangled function name: StructType.MemberName */
    char MangledName[256];
    snprintf(MangledName, sizeof(MangledName), "%s.%s", TypeName, MemberName);
    
    /* Intern the mangled name */
    char *FunctionName = TableStrRegister(Parser->pc, MangledName, strlen(MangledName));
    printf("DEBUG: Mangled to '%s'\n", FunctionName);
    
    /* Pop the struct from expression stack AND heap together */
    HeapPopStack(Parser->pc, ParamVal,
        sizeof(ExpressionStack) +
        sizeof(Value) +
        TypeStackSizeValue(StructVal));
    *StackTop = (*StackTop)->Next;
    
    /* Call the member function with the mangled name */
    ExpressionParseFunctionCall(Parser, StackTop, FunctionName, 
        Parser->Mode == RunModeRun);
}

const char* GetObjectType(ParseState *Parser, const char* object)
{
    /* Look up the variable to get its type */
    Value *ObjValue = NULL;
    VariableGet(Parser->pc, Parser, object, &ObjValue);
    
    if (ObjValue->Typ->Base != TypeStruct)
        ProgramFail(Parser, "member function call on non-struct");
    
    return ObjValue->Typ->Identifier;
}

/* Look it up in GetTypeName */
const char* GetTypeName(ParseState *Parser, ExpressionStack **StackTop, 
    const char *struct_name)
{
    if (struct_name == NULL)
        ProgramFail(Parser, "GetTypeName requires variable name");
    
    /* Look up in VariableTypeTable - works in all modes */
    const char *type_name = LookupVarType(Parser->pc, struct_name);
    if (type_name) {
        return type_name;
    }
    
    ProgramFail(Parser, "Cannot determine type of '%s'", struct_name);
    return NULL;
}

// Check for pattern .method(
const char* GetMethodName(ParseState *Parser)
{
    ParseState PeekState;
    Value *MemberIdent = NULL;
    
    ParserCopy(&PeekState, Parser);
    enum LexToken token = LexGetToken(&PeekState, NULL, true);
    if (token != TokenDot)
        return NULL;
    
    token = LexGetToken(&PeekState, &MemberIdent, true);
    if (token != TokenIdentifier)
        return NULL;
    
    token = LexGetToken(&PeekState, NULL, false);
    if (token != TokenOpenParen)
        return NULL;
    
    return MemberIdent->Val->Identifier;
}

char* GetMangleName(ParseState *Parser, ExpressionStack **StackTop, 
    const char *struct_name)
{
    const char* method_name = GetMethodName(Parser);
    ShowX("GetMangleName", struct_name, method_name, 0);
    
    if (!method_name) {
        return 0;
    }
    
    const char *type_name = GetTypeName(Parser, StackTop, struct_name);
    
    /* Build and intern mangled name */
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s.%s", type_name, method_name);
    char *mangle_name = TableStrRegister(Parser->pc, buffer, strlen(buffer));
    
    /* Consume tokens from real parser */
    enum LexToken token = LexGetToken(Parser, NULL, true);   /* consume . */
    if (token != TokenDot)
        ProgramFail(Parser, "Dot expected here");
    
    token = LexGetToken(Parser, NULL, true);   /* consume member name */
    if (token != TokenIdentifier)
        ProgramFail(Parser, "OpenParen expected here");
    
    return mangle_name;
}

void ParseTokenIdentifier(ParseState *Parser, ExpressionStack **StackTop, 
    Value *LexValue, int* PrefixState, int* Precedence, int* IgnorePrecedence)
{
    const char *TokenName = LexValue->Val->Identifier;
    ShowX("ParseTokenIdentifier", "", TokenName, 0);
    
    if (!*PrefixState)
        ProgramFail(Parser, "identifier not expected here");

    /* Peek ahead to check for member function call pattern: var.method() */
    char* mangle_name = GetMangleName(Parser, StackTop, TokenName);
    
    if (mangle_name) {
        /* It's a member function call like foo.fooMethod() */
        ShowX("ParseTokenIdentifier", "push this", mangle_name, 0);
        
        /* Push the struct instance onto the stack first (for 'this' parameter) */
        if (Parser->Mode == RunModeRun) {
            Value *StructValue = NULL;
            VariableGet(Parser->pc, Parser, TokenName, &StructValue);
            ExpressionStackPushLValue(Parser, StackTop, StructValue, 0);
        }
        
        /* Call the function with the mangled name */
        ExpressionParseFunctionCall(Parser, StackTop, mangle_name,
            Parser->Mode == RunModeRun && *Precedence < *IgnorePrecedence);
        
        *PrefixState = false;
        if (*Precedence <= *IgnorePrecedence)
            *IgnorePrecedence = DEEP_PRECEDENCE;
        return;  // Done handling member function call
    }

    /* Not a member function, check if it's a regular function call */
    enum LexToken token = LexGetToken(Parser, NULL, false);
    if (token == TokenOpenParen) {
        /* Regular function call */
        ExpressionParseFunctionCall(Parser, StackTop, TokenName,
            Parser->Mode == RunModeRun && *Precedence < *IgnorePrecedence);
    } else {
        /* Variable reference */
        if (Parser->Mode == RunModeRun) {
            Value *VariableValue = NULL;
            VariableGet(Parser->pc, Parser, TokenName, &VariableValue);
            
            if (VariableValue->Typ->Base == TypeMacro) {
                /* ... handle macro ... */
            } else if (VariableValue->Typ == &Parser->pc->VoidType)
                ProgramFail(Parser, "a void value isn't much use here");
            else
                ExpressionStackPushLValue(Parser, StackTop,
                    VariableValue, 0); /* it's a value variable */
        } else {
            /* push a dummy value */
            ExpressionPushInt(Parser, StackTop, 0);
        }
    }
    
    *PrefixState = false;
    if (*Precedence <= *IgnorePrecedence)
        *IgnorePrecedence = DEEP_PRECEDENCE;
}
