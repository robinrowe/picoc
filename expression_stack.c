/* expression_stack.c - Expression stack operations */
#include "expression_stack.h"
#include "interpreter.h"
#include "variable.h"
#include "type.h"
#include "heap.h"

#ifdef DEBUG_EXPRESSIONS
/* show the contents of the expression stack */
void ExpressionStackShow(Engine *pc, ExpressionStack *StackTop)
{
    printf("Expression stack [0x%llx,0x%llx]: ", 
           (intptr_t)pc->HeapStackTop, (intptr_t)StackTop);

    while (StackTop != NULL) {
        if (StackTop->Order == OrderNone) {
            /* it's a value */
            if (StackTop->Val->IsLValue)
                printf("lvalue=");
            else
                printf("value=");

            switch (StackTop->Val->Typ->Base) {
            case TypeVoid:
                printf("void");
                break;
            case TypeInt:
                printf("%d:int", StackTop->Val->Val->Integer);
                break;
            case TypeShort:
                printf("%d:short", StackTop->Val->Val->ShortInteger);
                break;
            case TypeChar:
                printf("%d:char", StackTop->Val->Val->Character);
                break;
            case TypeLong:
                printf("%ld:long", StackTop->Val->Val->LongInteger);
                break;
            case TypeUnsignedShort:
                printf("%d:unsigned short", 
                       StackTop->Val->Val->UnsignedShortInteger);
                break;
            case TypeUnsignedInt:
                printf("%d:unsigned int", 
                       StackTop->Val->Val->UnsignedInteger);
                break;
            case TypeUnsignedLong:
                printf("%ld:unsigned long", 
                       StackTop->Val->Val->UnsignedLongInteger);
                break;
            case TypeFP:
                printf("%f:fp", StackTop->Val->Val->FP);
                break;
            case TypeFunction:
                printf("%s:function", StackTop->Val->Val->Identifier);
                break;
            case TypeMacro:
                printf("%s:macro", StackTop->Val->Val->Identifier);
                break;
            case TypePointer:
                if (StackTop->Val->Val->Pointer == NULL)
                    printf("ptr(NULL)");
                else if (StackTop->Val->Typ->FromType->Base == TypeChar)
                    printf("\"%s\":string", (char *)StackTop->Val->Val->Pointer);
                else
                    printf("ptr(0x%llx)", (intptr_t)StackTop->Val->Val->Pointer);
                break;
            case TypeArray:
                printf("array");
                break;
            case TypeStruct:
                printf("%s:struct", StackTop->Val->Val->Identifier);
                break;
            case TypeUnion:
                printf("%s:union", StackTop->Val->Val->Identifier);
                break;
            case TypeEnum:
                printf("%s:enum", StackTop->Val->Val->Identifier);
                break;
            case Type_Type:
                PrintType(StackTop->Val->Val->Typ, pc->CStdOut);
                printf(":type");
                break;
            default:
                printf("unknown");
                break;
            }
            printf("[0x%llx,0x%llx]", (intptr_t)StackTop, (intptr_t)StackTop->Val);
        } else {
            /* it's an operator */
            printf("op='%s' %s %d", 
                   OperatorPrecedence[(int)StackTop->Op].Name,
                   (StackTop->Order == OrderPrefix) ? "prefix" : 
                   ((StackTop->Order == OrderPostfix) ? "postfix" : "infix"),
                   StackTop->Precedence);
            printf("[0x%llx]", (intptr_t)StackTop);
        }

        StackTop = StackTop->Next;
        if (StackTop != NULL)
            printf(", ");
    }

    printf("\n");
}
#endif

/* push a node on to the expression stack */
void ExpressionStackPushValueNode(ParseState *Parser,
    ExpressionStack **StackTop, Value *ValueLoc)
{
    ExpressionStack *StackNode = VariableAlloc(Parser->pc, Parser,
                                        sizeof(*StackNode), false);
    StackNode->Next = *StackTop;
    StackNode->Val = ValueLoc;
    *StackTop = StackNode;
    
#ifdef FANCY_ERROR_MESSAGES
    StackNode->Line = Parser->Line;
    StackNode->CharacterPos = Parser->CharacterPos;
#endif
    
#ifdef DEBUG_EXPRESSIONS
    ExpressionStackShow(Parser->pc, *StackTop);
#endif
}

/* push a blank value on to the expression stack by type */
Value *ExpressionStackPushValueByType(ParseState *Parser,
    ExpressionStack **StackTop, ValueType *PushType)
{
    Value *ValueLoc = VariableAllocValueFromType(Parser->pc, Parser,
            PushType, false, NULL, false);
    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
    return ValueLoc;
}

/* push a value on to the expression stack */
void ExpressionStackPushValue(ParseState *Parser,
    ExpressionStack **StackTop, Value *PushValue)
{
    Value *ValueLoc = VariableAllocValueAndCopy(Parser->pc, Parser,
        PushValue, false);
    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
}

/* push an L-value on to the expression stack */
void ExpressionStackPushLValue(ParseState *Parser,
    ExpressionStack **StackTop, Value *PushValue, int Offset)
{
    Value *ValueLoc = VariableAllocValueShared(Parser, PushValue);
    ValueLoc->Val = (void *)((char *)ValueLoc->Val + Offset);
    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
}

/* push a dereferenced value on to the expression stack */
void ExpressionStackPushDereference(ParseState *Parser,
    ExpressionStack **StackTop, Value *DereferenceValue)
{
    int Offset;
    int DerefIsLValue;
    Value *DerefVal;
    Value *ValueLoc;
    ValueType *DerefType;
    void *DerefDataLoc = VariableDereferencePointer(DereferenceValue, &DerefVal,
        &Offset, &DerefType, &DerefIsLValue);
        
    if (DerefDataLoc == NULL)
        ProgramFail(Parser, "NULL pointer dereference");

    ValueLoc = VariableAllocValueFromExistingData(Parser, DerefType,
                    (union AnyValue*)DerefDataLoc, DerefIsLValue, DerefVal);
    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
}

/* push an integer value on to the expression stack */
void ExpressionPushInt(ParseState *Parser,
    ExpressionStack **StackTop, long IntValue)
{
    Value *ValueLoc = VariableAllocValueFromType(Parser->pc, Parser,
                            &Parser->pc->IntType, false, NULL, false);
    
    /* assign value to all integer fields for proper representation */
    ValueLoc->Val->UnsignedLongInteger = (unsigned long)IntValue;
    ValueLoc->Val->LongInteger = (long)IntValue;
    ValueLoc->Val->Integer = (int)IntValue;
    ValueLoc->Val->ShortInteger = (short)IntValue;
    ValueLoc->Val->UnsignedShortInteger = (unsigned short)IntValue;
    ValueLoc->Val->UnsignedInteger = (unsigned int)IntValue;
    ValueLoc->Val->UnsignedCharacter = (unsigned char)IntValue;
    ValueLoc->Val->Character = (char)IntValue;

    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
}

/* push a floating point value on to the expression stack */
void ExpressionPushFP(ParseState *Parser,
    ExpressionStack **StackTop, double FPValue)
{
    Value *ValueLoc = VariableAllocValueFromType(Parser->pc, Parser,
                                 &Parser->pc->FPType, false, NULL, false);
    ValueLoc->Val->FP = FPValue;
    ExpressionStackPushValueNode(Parser, StackTop, ValueLoc);
}

/* push an operator on to the expression stack */
void ExpressionStackPushOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum OperatorOrder Order,
    enum LexToken Token, int Precedence)
{
    ExpressionStack *StackNode = VariableAlloc(Parser->pc, Parser,
        sizeof(*StackNode), false);
    StackNode->Next = *StackTop;
    StackNode->Order = Order;
    StackNode->Op = Token;
    StackNode->Precedence = Precedence;
    *StackTop = StackNode;
    assert(Order);
#ifdef FANCY_ERROR_MESSAGES
    StackNode->Line = Parser->Line;
    StackNode->CharacterPos = Parser->CharacterPos;
#endif
    
#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionStackPushOperator()\n");
    ExpressionStackShow(Parser->pc, *StackTop);
#endif
}

/* take the contents of the expression stack and compute the top */
void ExpressionStackCollapse(ParseState *Parser,
    ExpressionStack **StackTop, int Precedence, int *IgnorePrecedence)
{
    int FoundPrecedence = Precedence;
    Value *TopValue;
    Value *BottomValue;
    ExpressionStack *TopStackNode = *StackTop;
    ExpressionStack *TopOperatorNode;
    assert(StackTop);
#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionStackCollapse(%d):\n", Precedence);
    ExpressionStackShow(Parser->pc, *StackTop);
#endif
    
    while (TopStackNode != NULL && TopStackNode->Next != NULL &&
            FoundPrecedence >= Precedence) {
        /* find the top operator on the stack */
        if (TopStackNode->Order == OrderNone)
            TopOperatorNode = TopStackNode->Next;
        else
            TopOperatorNode = TopStackNode;

        FoundPrecedence = TopOperatorNode->Precedence;

        /* does it have a high enough precedence? */
        if (FoundPrecedence >= Precedence && TopOperatorNode != NULL) {
            /* execute this operator */
            switch (TopOperatorNode->Order) {
            case OrderPrefix:
                /* prefix evaluation */
                TopValue = TopStackNode->Val;

                /* pop the value and then the prefix operator */
                HeapPopStack(Parser->pc, NULL,
                    sizeof(ExpressionStack) +
                    sizeof(Value) +
                    TypeStackSizeValue(TopValue));
                HeapPopStack(Parser->pc, TopOperatorNode,
                    sizeof(ExpressionStack));
                *StackTop = TopOperatorNode->Next;

                /* do the prefix operation */
                if (Parser->Mode == RunModeRun) {
                    ExpressionPrefixOperator(Parser, StackTop,
                        TopOperatorNode->Op, TopValue);
                } else {
                    ExpressionPushInt(Parser, StackTop, 0);
                }
                break;
                
            case OrderPostfix:
                /* postfix evaluation */
                TopValue = TopStackNode->Next->Val;

                /* pop the postfix operator and then the value */
                HeapPopStack(Parser->pc, NULL, sizeof(ExpressionStack));
                HeapPopStack(Parser->pc, TopValue,
                    sizeof(ExpressionStack) +
                    sizeof(Value) +
                    TypeStackSizeValue(TopValue));
                *StackTop = TopStackNode->Next->Next;

                /* do the postfix operation */
                if (Parser->Mode == RunModeRun) {
                    ExpressionPostfixOperator(Parser, StackTop,
                        TopOperatorNode->Op, TopValue);
                } else {
                    ExpressionPushInt(Parser, StackTop, 0);
                }
                break;
                
            case OrderInfix:
                /* infix evaluation */
                TopValue = TopStackNode->Val;
                if (TopValue != NULL) {
                    if (!TopOperatorNode->Next) {
                        puts("Error in TopStackNode, this shouldn't happen");
                        TopStackNode->Val = 0;
                        break;
                    }
                    BottomValue = TopOperatorNode->Next->Val;

                    /* pop a value, the operator and another value */
                    HeapPopStack(Parser->pc, NULL,
                        sizeof(ExpressionStack) +
                        sizeof(Value) +
                        TypeStackSizeValue(TopValue));
                    HeapPopStack(Parser->pc, NULL,
                        sizeof(ExpressionStack));
                    HeapPopStack(Parser->pc, BottomValue,
                        sizeof(ExpressionStack) +
                        sizeof(Value) +
                        TypeStackSizeValue(BottomValue));
                    *StackTop = TopOperatorNode->Next->Next;

                    /* do the infix operation */
                    if (Parser->Mode == RunModeRun) {
                        ExpressionInfixOperator(Parser, StackTop,
                            TopOperatorNode->Op, BottomValue, TopValue);
                    } else {
                        ExpressionPushInt(Parser, StackTop, 0);
                    }
                } else {
                    FoundPrecedence = -1;
                }
                break;
                
            case OrderNone:
            default:
                /* this should never happen */
                if(TopOperatorNode->Order == OrderNone)
                {   ProgramFail(Parser,"Parser logic error: TopOperatorNode->Order == OrderNone");
                }
                break;
            }

            /* if we've returned above the ignored precedence level turn ignoring off */
            if (FoundPrecedence <= *IgnorePrecedence)
                *IgnorePrecedence = DEEP_PRECEDENCE;
        }
        
#ifdef DEBUG_EXPRESSIONS
        ExpressionStackShow(Parser->pc, *StackTop);
#endif
        TopStackNode = *StackTop;
    }
    
#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionStackCollapse() finished\n");
    ExpressionStackShow(Parser->pc, *StackTop);
#endif
}
