/* expression_stack.h - Expression stack operations interface */
#ifndef EXPRESSION_STACK_H
#define EXPRESSION_STACK_H

#include "expression.h"

/* Stack push operations */
void ExpressionStackPushValueNode(ParseState *Parser,
    ExpressionStack **StackTop, Value *ValueLoc);
Value *ExpressionStackPushValueByType(ParseState *Parser,
    ExpressionStack **StackTop, ValueType *PushType);
void ExpressionStackPushValue(ParseState *Parser,
    ExpressionStack **StackTop, Value *PushValue);
void ExpressionStackPushLValue(ParseState *Parser,
    ExpressionStack **StackTop, Value *PushValue, int Offset);
void ExpressionStackPushDereference(ParseState *Parser,
    ExpressionStack **StackTop, Value *DereferenceValue);
void ExpressionPushInt(ParseState *Parser,
    ExpressionStack **StackTop, long IntValue);
void ExpressionPushFP(ParseState *Parser,
    ExpressionStack **StackTop, double FPValue);

/* Operator stack operations */
void ExpressionStackPushOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum OperatorOrder Order,
    enum LexToken Token, int Precedence);

/* Stack evaluation */
void ExpressionStackCollapse(ParseState *Parser,
    ExpressionStack **StackTop, int Precedence, int *IgnorePrecedence);

/* Debug */
#ifdef DEBUG_EXPRESSIONS
void ExpressionStackShow(Engine *pc, ExpressionStack *StackTop);
#endif

#endif /* EXPRESSION_STACK_H */
