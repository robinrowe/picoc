/* expression.h - Expression parsing and evaluation interface */
#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "interpreter.h"

/* Forward declarations */
typedef struct Engine Engine;
typedef struct ParseState ParseState;
typedef struct Value Value;
typedef struct ValueType ValueType;
typedef struct ExpressionStack ExpressionStack;

/* Operator evaluation order */
enum OperatorOrder {
    OrderNone,
    OrderPrefix,
    OrderInfix,
    OrderPostfix
};

/* Expression stack node */
struct ExpressionStack {
    ExpressionStack *Next;      /* next lower item on stack */
    Value *Val;                 /* value for this stack node */
    enum LexToken Op;           /* operator */
    unsigned short Precedence;  /* operator precedence */
    unsigned char Order;        /* evaluation order */
};

/* Operator precedence definitions */
struct OpPrecedence {
    unsigned int PrefixPrecedence:4;
    unsigned int PostfixPrecedence:4;
    unsigned int InfixPrecedence:4;
    char *Name;
};

/* Global operator precedence table */
extern struct OpPrecedence OperatorPrecedence[];

/* Macros */
#define IS_LEFT_TO_RIGHT(p) ((p) != 2 && (p) != 14)
#define BRACKET_PRECEDENCE (20)
#define DEEP_PRECEDENCE (BRACKET_PRECEDENCE * 1000)

/* Core expression parsing */
int ExpressionParse(ParseState *Parser, Value **Result);
long ExpressionParseInt(ParseState *Parser);

/* Type coercion */
intptr_t ExpressionCoerceInteger(Value *Val);
uintptr_t ExpressionCoerceUnsignedInteger(Value *Val);
double ExpressionCoerceFP(Value *Val);

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

/* Function and macro calls */
void ExpressionParseFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt);
void ExpressionParseMacroCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *MacroName,
    struct MacroDef *MDef);

/* Utility functions */
int IsTypeToken(ParseState *Parser, enum LexToken t, Value *LexValue);

#endif /* EXPRESSION_H */
