
/* expression.h */

#ifndef expression_h
#define expression_h

/* local prototypes */
enum OperatorOrder {
    OrderNone,
    OrderPrefix,
    OrderInfix,
    OrderPostfix
};

/* a stack of expressions we use in evaluation */
struct ExpressionStack {
    struct ExpressionStack *Next;  /* the next lower item on the stack */
    struct Value *Val;  /* the value for this stack node */
    enum LexToken Op;  /* the operator */
    unsigned short Precedence;  /* the operator precedence of this node */
    unsigned char Order;  /* the evaluation order of this operator */
};

/* operator precedence definitions */
struct OpPrecedence {
    unsigned int PrefixPrecedence:4;
    unsigned int PostfixPrecedence:4;
    unsigned int InfixPrecedence:4;
    char *Name;
};

/* NOTE: the order of this array must correspond exactly to the order of
    these tokens in enum LexToken */
struct OpPrecedence OperatorPrecedence[] = {
    /* TokenNone, */ {0, 0, 0, "none"},
    /* TokenComma, */ {0, 0, 0, ","},
    /* TokenAssign, */ {0, 0, 2, "="},
    /* TokenAddAssign, */ {0, 0, 2, "+="},
    /* TokenSubtractAssign, */ {0, 0, 2, "-="},
    /* TokenMultiplyAssign, */ {0, 0, 2, "*="},
    /* TokenDivideAssign, */ { 0, 0, 2, "/=" },
    /* TokenModulusAssign, */ { 0, 0, 2, "%=" },
    /* TokenShiftLeftAssign, */ {0, 0, 2, "<<="},
    /* TokenShiftRightAssign, */ { 0, 0, 2, ">>=" },
    /* TokenArithmeticAndAssign, */ { 0, 0, 2, "&=" },
    /* TokenArithmeticOrAssign, */ {0, 0, 2, "|="},
    /* TokenArithmeticExorAssign, */ { 0, 0, 2, "^=" },
    /* TokenQuestionMark, */ {0, 0, 3, "?"},
    /* TokenColon, */ {0, 0, 3, ":" },
    /* TokenLogicalOr, */ {0, 0, 4, "||"},
    /* TokenLogicalAnd, */ {0, 0, 5, "&&"},
    /* TokenArithmeticOr, */ {0, 0, 6, "|"},
    /* TokenArithmeticExor, */ {0, 0, 7, "^"},
    /* TokenAmpersand, */ {14, 0, 8, "&"},
    /* TokenEqual, */  {0, 0, 9, "=="},
    /* TokenNotEqual, */ {0, 0, 9, "!="},
    /* TokenLessThan, */ {0, 0, 10, "<"},
    /* TokenGreaterThan, */ {0, 0, 10, ">"},
    /* TokenLessEqual, */ {0, 0, 10, "<="},
    /* TokenGreaterEqual, */ {0, 0, 10, ">="},
    /* TokenShiftLeft, */ {0, 0, 11, "<<"},
    /* TokenShiftRight, */ {0, 0, 11, ">>"},
    /* TokenPlus, */ {14, 0, 12, "+"},
    /* TokenMinus, */ {14, 0, 12, "-"},
    /* TokenAsterisk, */ {14, 0, 13, "*"},
    /* TokenSlash, */ {0, 0, 13, "/"},
    /* TokenModulus, */ {0, 0, 13, "%"},
    /* TokenIncrement, */ {14, 15, 0, "++"},
    /* TokenDecrement, */ {14, 15, 0, "--"},
    /* TokenUnaryNot, */ {14, 0, 0, "!"},
    /* TokenUnaryExor, */ {14, 0, 0, "~"},
    /* TokenSizeof, */ {14, 0, 0, "sizeof"},
    /* TokenCast, */ {14, 0, 0, "cast"},
    /* TokenLeftSquareBracket, */ {0, 0, 15, "["},
    /* TokenRightSquareBracket, */ {0, 15, 0, "]"},
    /* TokenDot, */ {0, 0, 15, "."},
    /* TokenArrow, */ {0, 0, 15, "->"},
    /* TokenOpenParen, */ {15, 0, 0, "("},
    /* TokenCloseParen, */ {0, 15, 0, ")"}
};

//int ExpressionParse(struct ParseState *Parser, struct Value **Result);
long ExpressionParseInt(struct ParseState *Parser);
void ExpressionAssign(struct ParseState *Parser, struct Value *DestValue,
    struct Value *SourceValue, int Force, const char *FuncName, int ParamNo, int AllowPointerCoercion);
intptr_t ExpressionCoerceInteger(struct Value *Val);
uintptr_t ExpressionCoerceUnsignedInteger(struct Value *Val);
double ExpressionCoerceFP(struct Value *Val);
void ExpressionParseFunctionCall(struct ParseState *Parser,
    struct ExpressionStack **StackTop, const char *FuncName, int RunIt);
int IsTypeToken(struct ParseState * Parser, enum LexToken t, struct Value * LexValue);
long ExpressionAssignInt(struct ParseState *Parser, struct Value *DestValue, long FromInt, int After);
double ExpressionAssignFP(struct ParseState *Parser, struct Value *DestValue, double FromFP);
void ExpressionStackPushValueNode(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *ValueLoc);
struct Value *ExpressionStackPushValueByType(struct ParseState *Parser, struct ExpressionStack **StackTop, struct ValueType *PushType);
void ExpressionStackPushValue(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *PushValue);
void ExpressionStackPushLValue(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *PushValue, int Offset);
void ExpressionStackPushDereference(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *DereferenceValue);
void ExpressionPushInt(struct ParseState *Parser, struct ExpressionStack **StackTop, long IntValue);
void ExpressionPushFP(struct ParseState *Parser, struct ExpressionStack **StackTop, double FPValue);
void ExpressionAssignToPointer(struct ParseState *Parser, struct Value *ToValue, struct Value *FromValue, const char *FuncName, int ParamNo, int AllowPointerCoercion);
void ExpressionQuestionMarkOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *BottomValue, struct Value *TopValue);
void ExpressionColonOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, struct Value *BottomValue, struct Value *TopValue);
void ExpressionPrefixOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, enum LexToken Op, struct Value *TopValue);
void ExpressionPostfixOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, enum LexToken Op, struct Value *TopValue);
void ExpressionInfixOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, enum LexToken Op, struct Value *BottomValue, struct Value *TopValue);
void ExpressionStackCollapse(struct ParseState *Parser, struct ExpressionStack **StackTop, int Precedence, int *IgnorePrecedence);
void ExpressionStackPushOperator(struct ParseState *Parser, struct ExpressionStack **StackTop, enum OperatorOrder Order, enum LexToken Token, int Precedence);
void ExpressionParseMacroCall(struct ParseState *Parser, struct ExpressionStack **StackTop, const char *MacroName, struct MacroDef *MDef);

#endif
