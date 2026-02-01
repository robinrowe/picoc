/* expression_operator.h - Expression operator evaluation interface */
#ifndef EXPRESSION_OPERATOR_H
#define EXPRESSION_OPERATOR_H

#include "expression.h"

/* Operator evaluation functions */
void ExpressionPrefixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op, Value *TopValue);
void ExpressionPostfixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op, Value *TopValue);
void ExpressionInfixOperator(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Op,
    Value *BottomValue, Value *TopValue);
void ExpressionQuestionMarkOperator(ParseState *Parser,
    ExpressionStack **StackTop, Value *BottomValue, Value *TopValue);
void ExpressionColonOperator(ParseState *Parser,
    ExpressionStack **StackTop, Value *BottomValue, Value *TopValue);
void ExpressionGetStructElement(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Token);

/* Utility functions for member function calls */
void ExpressionMemberFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, enum LexToken Token, const char *MemberName);
const char* GetObjectType(ParseState *Parser, const char* object);
const char* GetTypeName(ParseState *Parser, ExpressionStack **StackTop, 
    const char *struct_name);
const char* GetMethodName(ParseState *Parser);
char* GetMangleName(ParseState *Parser, ExpressionStack **StackTop, 
    const char *struct_name);
void ParseTokenIdentifier(ParseState *Parser, ExpressionStack **StackTop, 
    Value *LexValue, int* PrefixState, int* Precedence, int* IgnorePrecedence);

/* Operators for TrapC extensions */
void ExpressionDotThisOperator(ParseState *Parser,
    ExpressionStack **StackTop, const char *MemberName);
void ExpressionScopeResolution(ParseState *Parser,
    ExpressionStack **StackTop, const char *Identifier, int IsDoubleDot);
void ExpressionThisArrow(ParseState *Parser,
    ExpressionStack **StackTop, const char *MemberName);

#endif /* EXPRESSION_OPERATOR_H */
