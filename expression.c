/* expression.c - Core expression parsing */
#include "expression.h"
#include "interpreter.h"
#include "table.h"
#include "variable.h"
#include "type.h"
#include "heap.h"
#include "expression_stack.h"
#include "expression_operator.h"
#include "expression_call.h"

/* Operator precedence table - must match enum LexToken order */
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

/* Check if a token represents a type */
int IsTypeToken(ParseState *Parser, enum LexToken t, Value *LexValue)
{
    if (t >= TokenIntType && t <= TokenUnsignedType)
        return 1; /* base type */

    /* typedef'ed type? */
    if (t == TokenIdentifier) {
        /* see TypeParseFront, case TokenIdentifier and ParseTypedef */
        Value *VarValue;
        if (VariableDefined(Parser->pc, LexValue->Val->Pointer)) {
#ifdef MAGIC
            if (strcmp(LexValue->Val->Pointer, "Foo_hello") == 0) {
                printf("DEBUG: VariableGet called for Foo_hello from line %d\n", __LINE__);
            }
#endif
            VariableGet(Parser->pc, Parser, LexValue->Val->Pointer, &VarValue);
            if (VarValue->Typ == &Parser->pc->TypeType)
                return 1;
        }
    }

    return 0;
}

/* parse an expression with operator precedence */
int ExpressionParse(ParseState *Parser, Value **Result)
{
    int PrefixState = true;
    int Done = false;
    int BracketPrecedence = 0;
    int LocalPrecedence;
    int Precedence = 0;
    int IgnorePrecedence = DEEP_PRECEDENCE;
    int TernaryDepth = 0;
    Value *LexValue = 0;
    ExpressionStack *StackTop = NULL;

#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionParse():\n");
#endif

    do {
        ParseState PreState;
        enum LexToken Token;

        ParserCopy(&PreState, Parser);
        Token = LexGetToken(Parser, &LexValue, true);
        
        if ((((int)Token > TokenComma && (int)Token <= (int)TokenOpenParen) ||
               (Token == TokenCloseParen && BracketPrecedence != 0)) &&
               (Token != TokenColon || TernaryDepth > 0)) {
            /* it's an operator with precedence */
            if (PrefixState) {
                /* expect a prefix operator */
                if (OperatorPrecedence[(int)Token].PrefixPrecedence == 0)
                    ProgramFail(Parser, "operator not expected here");

                LocalPrecedence = OperatorPrecedence[(int)Token].PrefixPrecedence;
                Precedence = BracketPrecedence + LocalPrecedence;

                if (Token == TokenOpenParen) {
                    /* it's either a new bracket level or a cast */
                    enum LexToken BracketToken = LexGetToken(Parser, &LexValue, false);
                    if (IsTypeToken(Parser, BracketToken, LexValue) &&
                            (StackTop == NULL || StackTop->Op != TokenSizeof)) {
                        /* it's a cast - get the new type */
                        ValueType *CastType;
                        char *CastIdentifier;
                        Value *CastTypeValue;

                        TypeParse(Parser, &CastType, &CastIdentifier, NULL);
                        if (LexGetToken(Parser, &LexValue, true) != TokenCloseParen)
                            ProgramFail(Parser, "brackets not closed");

                        /* scan and collapse the stack to the precedence of
                            this infix cast operator, then push */
                        Precedence = BracketPrecedence +
                            OperatorPrecedence[(int)TokenCast].PrefixPrecedence;

                        ExpressionStackCollapse(Parser, &StackTop, Precedence+1,
                            &IgnorePrecedence);
                        CastTypeValue = VariableAllocValueFromType(Parser->pc,
                            Parser, &Parser->pc->TypeType, false, NULL, false);
                        CastTypeValue->Val->Typ = CastType;
                        ExpressionStackPushValueNode(Parser, &StackTop, CastTypeValue);
                        ExpressionStackPushOperator(Parser, &StackTop, OrderInfix,
                            TokenCast, Precedence);
                    } else {
                        /* boost the bracket operator precedence */
                        BracketPrecedence += BRACKET_PRECEDENCE;
                    }
                } else {
                    /* scan and collapse the stack to the precedence of
                        this operator, then push */

                    /* take some extra care for double prefix operators,
                        e.g. x = - -5, or x = **y */
                    int NextToken = LexGetToken(Parser, NULL, false);
                    int TempPrecedenceBoost = 0;
                    if (NextToken > TokenComma && NextToken < TokenOpenParen) {
                        int NextPrecedence =
                            OperatorPrecedence[(int)NextToken].PrefixPrecedence;

                        /* two prefix operators with equal precedence? make
                            sure the innermost one runs first */
                        if (LocalPrecedence == NextPrecedence)
                            TempPrecedenceBoost = -1;
                    }

                    ExpressionStackCollapse(Parser, &StackTop, Precedence,
                        &IgnorePrecedence);
                    ExpressionStackPushOperator(Parser, &StackTop, OrderPrefix,
                        Token, Precedence + TempPrecedenceBoost);
                }
            } else {
                /* expect an infix or postfix operator */
                if (OperatorPrecedence[(int)Token].PostfixPrecedence != 0) {
                    switch (Token) {
                    case TokenCloseParen:
                    case TokenRightSquareBracket:
                        if (BracketPrecedence == 0) {
                            /* assume this bracket is after the end of the
                                expression */
                            ParserCopy(Parser, &PreState);
                            Done = true;
                        } else {
                            /* collapse to the bracket precedence */
                            ExpressionStackCollapse(Parser, &StackTop,
                                BracketPrecedence, &IgnorePrecedence);
                            BracketPrecedence -= BRACKET_PRECEDENCE;
                        }
                        break;
                    default:
                        /* scan and collapse the stack to the precedence of
                            this operator, then push */
                        Precedence = BracketPrecedence +
                            OperatorPrecedence[(int)Token].PostfixPrecedence;
                        ExpressionStackCollapse(Parser, &StackTop, Precedence,
                            &IgnorePrecedence);
                        ExpressionStackPushOperator(Parser, &StackTop,
                            OrderPostfix, Token, Precedence);
                        break;
                    }
                } else if (OperatorPrecedence[(int)Token].InfixPrecedence != 0) {
                    /* Handle dot and arrow operators specially - don't collapse the stack */
                    if ((Token == TokenDot || Token == TokenArrow) && Parser->Mode == RunModeRun) {
                        ParseState PeekState;
                        Value *MemberIdent;
                        ParserCopy(&PeekState, Parser);
                        if (LexGetToken(&PeekState, &MemberIdent, true) == TokenIdentifier &&
                            LexGetToken(&PeekState, NULL, false) == TokenOpenParen) {
                            /* It's a member function call */
                            const char* get_from_stack = 0;
                            char *FuncName = GetMangleName(Parser, &StackTop, get_from_stack);
                            if (FuncName) {
                                ExpressionParseFunctionCall(Parser, &StackTop, FuncName, true);
                                PrefixState = false;
                                continue;  // Skip regular dot handling
                            }
                        }
                        /* Regular member access */
                        ExpressionGetStructElement(Parser, &StackTop, Token);
                    } else {
                        /* scan and collapse the stack, then push */
                        Precedence = BracketPrecedence +
                            OperatorPrecedence[(int)Token].InfixPrecedence;

                        /* for right to left order, only go down to the next
                            higher precedence so we evaluate it in reverse order */
                        /* for left to right order, collapse down to this precedence
                            so we evaluate it in forward order */
                        if (IS_LEFT_TO_RIGHT(OperatorPrecedence[(int)Token].InfixPrecedence))
                            ExpressionStackCollapse(Parser, &StackTop, Precedence,
                                &IgnorePrecedence);
                        else
                            ExpressionStackCollapse(Parser, &StackTop, Precedence+1,
                                &IgnorePrecedence);

                        /* if it's a && or || operator we may not need to
                            evaluate the right hand side of the expression */
                        if ((Token == TokenLogicalOr || Token == TokenLogicalAnd) &&
                                IS_NUMERIC_COERCIBLE(StackTop->Val)) {
                            long LHSInt = ExpressionCoerceInteger(StackTop->Val);
                            if (((Token == TokenLogicalOr && LHSInt) ||
                                    (Token == TokenLogicalAnd && !LHSInt)) &&
                                 (IgnorePrecedence > Precedence) )
                                IgnorePrecedence = Precedence;
                        }

                        /* push the operator on the stack */
                        ExpressionStackPushOperator(Parser, &StackTop,
                            OrderInfix, Token, Precedence);
                        PrefixState = true;

                        switch (Token) {
                        case TokenQuestionMark:
                            TernaryDepth++;
                            break;
                        case TokenColon:
                            TernaryDepth--;
                            break;
                        default:
                            break;
                        }
                    }

                    /* treat an open square bracket as an infix array index
                        operator followed by an open bracket */
                    if (Token == TokenLeftSquareBracket) {
                        /* boost the bracket operator precedence, then push */
                        BracketPrecedence += BRACKET_PRECEDENCE;
                    }
                } else {
                    ProgramFail(Parser, "operator not expected here");
                }
            }
        } else if (Token == TokenIdentifier) {
            ParseTokenIdentifier(Parser, &StackTop, LexValue, &PrefixState, 
                                 &Precedence, &IgnorePrecedence);
        } else if ((int)Token > TokenCloseParen &&
                   (int)Token <= TokenCharacterConstant) {
            /* it's a value of some sort, push it */
            if (!PrefixState)
                ProgramFail(Parser, "value not expected here");

            PrefixState = false;
            ExpressionStackPushValue(Parser, &StackTop, LexValue);
        } else if (IsTypeToken(Parser, Token, LexValue)) {
            /* it's a type. push it on the stack like a value.
                this is used in sizeof() */
            ValueType *Typ;
            char *Identifier;
            Value *TypeValue;

            if (!PrefixState)
                ProgramFail(Parser, "type not expected here");

            PrefixState = false;
            ParserCopy(Parser, &PreState);
            TypeParse(Parser, &Typ, &Identifier, NULL);
            TypeValue = VariableAllocValueFromType(Parser->pc, Parser,
                &Parser->pc->TypeType, false, NULL, false);
            TypeValue->Val->Typ = Typ;
            ExpressionStackPushValueNode(Parser, &StackTop, TypeValue);
        } else {
            /* it isn't a token from an expression */
            ParserCopy(Parser, &PreState);
            Done = true;
        }
    } while (!Done);

    /* check that brackets have been closed */
    if (BracketPrecedence > 0)
        ProgramFail(Parser, "brackets not closed");

    /* scan and collapse the stack to precedence 0 */
    ExpressionStackCollapse(Parser, &StackTop, 0, &IgnorePrecedence);

    /* fix up the stack and return the result if we're in run mode */
    if (StackTop != NULL) {
        /* all that should be left is a single value on the stack */
        if (Parser->Mode == RunModeRun) {
            if (StackTop->Order != OrderNone || StackTop->Next != NULL)
                ProgramFail(Parser, "invalid expression");

            *Result = StackTop->Val;
            HeapPopStack(Parser->pc, StackTop, sizeof(ExpressionStack));
        } else {
            HeapPopStack(Parser->pc, StackTop->Val,
                sizeof(ExpressionStack) +
                sizeof(Value) +
                TypeStackSizeValue(StackTop->Val));
        }
    }

#ifdef DEBUG_EXPRESSIONS
    printf("ExpressionParse() done\n\n");
    ExpressionStackShow(Parser->pc, StackTop);
#endif
    
    return StackTop != NULL;
}

/* parse an expression and return as integer */
long ExpressionParseInt(ParseState *Parser)
{
    long Result = 0;
    Value *Val;

    if (!ExpressionParse(Parser, &Val))
        ProgramFail(Parser, "expression expected");

    if (Parser->Mode == RunModeRun) {
        if (!IS_NUMERIC_COERCIBLE_PLUS_POINTERS(Val, true))
            ProgramFail(Parser, "integer value expected instead of %t", Val->Typ);

        Result = ExpressionCoerceInteger(Val);
        VariableStackPop(Parser, Val);
    }

    return Result;
}
