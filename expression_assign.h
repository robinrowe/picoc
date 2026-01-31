/* expression_assign.h - Assignment operations interface */
#ifndef EXPRESSION_ASSIGN_H
#define EXPRESSION_ASSIGN_H

#include "expression.h"

/* Assignment operations */
void ExpressionAssign(ParseState *Parser, Value *DestValue,
    Value *SourceValue, int Force, const char *FuncName, int ParamNo,
    int AllowPointerCoercion);
long ExpressionAssignInt(ParseState *Parser, Value *DestValue,
    long FromInt, int After);
double ExpressionAssignFP(ParseState *Parser, Value *DestValue,
    double FromFP);
void ExpressionAssignToPointer(ParseState *Parser, Value *ToValue,
    Value *FromValue, const char *FuncName, int ParamNo,
    int AllowPointerCoercion);

#endif /* EXPRESSION_ASSIGN_H */
