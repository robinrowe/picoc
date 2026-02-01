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
    /* TokenDotDot, */ {14, 0, 0, ".."},      /* Prefix operator for global scope */
    /* TokenScopeRes, */ {14, 0, 0, "::"},       /* Prefix operator for global scope */
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
#if 0
        if (VariableDefined(Parser->pc, LexValue->Val->Pointer)) {
#endif
        if(VariableGetDefined(Parser->pc, Parser, LexValue->Val->Pointer, &VarValue,false)){
             if (VarValue->Typ == &Parser->pc->TypeType)
                return 1;
        }
    }

    return 0;
}

/* Helper function: Handle scope resolution operators .. and :: */
static int HandleScopeResolution(ParseState *Parser, ExpressionStack **StackTop,
                                 enum LexToken Token, int *PrefixState)
{
    /* Get the next token which should be an identifier */
    Value *IdentValue;
    enum LexToken NextToken = LexGetToken(Parser, &IdentValue, true);
    
    if (NextToken != TokenIdentifier) {
        ProgramFail(Parser, "identifier expected after '%s'", 
                   (Token == TokenDotDot) ? ".." : "::");
        return 0;
    }
    
    /* Look up in global scope */
    if (Parser->Mode == RunModeRun) {
        Value *GlobalValue = NULL;
        const char *Ident = IdentValue->Val->Identifier;
        
        ShowX("ScopeResolution", "GlobalTable", Ident, 0);
        if (!TableGet(&Parser->pc->GlobalTable, Ident,
                &GlobalValue, NULL, NULL, NULL)) {
            ProgramFail(Parser, "'%s' is not defined in global scope", Ident);
        }
        
        /* Push the global value onto stack */
        ExpressionStackPushLValue(Parser, StackTop, GlobalValue, 0);
    } else {
        /* In skip mode, push dummy value */
        ExpressionPushInt(Parser, StackTop, 0);
    }
    
    *PrefixState = 0;
    return 1; /* Success */
}

/* Helper function: Handle dot-this operator (.member) */
static int HandleDotThisOperator(ParseState *Parser, ExpressionStack **StackTop,
                                 const char *MemberName, int *PrefixState)
{
    /* .member means this->member in TrapC */
    
    if (Parser->Mode == RunModeRun) {
        /* Look up 'this' in current scope */
        Value *ThisValue = NULL;
        
        /* Try to get 'this' from local variables */
        VariableGetDefined(Parser->pc, Parser, "this", &ThisValue,false);
        
        /* Check if 'this' is a pointer */
        if (ThisValue->Typ->Base != TypePointer) {
            ProgramFail(Parser, "'this' must be a pointer in member functions");
        }
        
        /* Get the struct type from the pointer */
        ValueType *StructType = ThisValue->Typ->FromType;
        if (StructType->Base != TypeStruct && StructType->Base != TypeUnion) {
            ProgramFail(Parser, "'this' must point to a struct or union");
        }
        
        /* Look up the member in the struct */
        Value *MemberValue = NULL;
        ShowX("HandleDotThisOperator", "Members", MemberName, 0);
        if (!TableGet(StructType->Members, MemberName,
                &MemberValue, NULL, NULL, NULL)) {
            ProgramFail(Parser, "struct doesn't have a member called '%s'", MemberName);
        }
        
        /* Push 'this' pointer onto stack first (for arrow operator) */
        ExpressionStackPushLValue(Parser, StackTop, ThisValue, 0);
        
        /* Now we need to simulate this->member */
        /* We'll push the member access result */
        char *DerefDataLoc = (char *)ThisValue->Val->Pointer;
        if (DerefDataLoc == NULL) {
            ProgramFail(Parser, "NULL 'this' pointer access with '.' operator");
        }
        
        Value *Result = VariableAllocValueFromExistingData(Parser, 
            MemberValue->Typ,
            (union AnyValue*)(DerefDataLoc + MemberValue->Val->Integer),
            true, ThisValue->LValueFrom);
        
        ExpressionStackPushValueNode(Parser, StackTop, Result);
    } else {
        /* In skip mode, push dummy value */
        ExpressionPushInt(Parser, StackTop, 0);
    }
    
    *PrefixState = 0;
    return 1; /* Success */
}

/* Helper function: Handle prefix operators */
static int HandlePrefixOperator(ParseState *Parser, ExpressionStack **StackTop,
                                enum LexToken Token, int BracketPrecedence,
                                int *PrefixState, int *IgnorePrecedence,
                                int *TernaryDepth)
{
    int LocalPrecedence;
    int Precedence;
    
    if (OperatorPrecedence[(int)Token].PrefixPrecedence == 0) {
        ProgramFail(Parser, "operator not expected here");
        return 0;
    }

    LocalPrecedence = OperatorPrecedence[(int)Token].PrefixPrecedence;
    Precedence = BracketPrecedence + LocalPrecedence;

    if (Token == TokenOpenParen) {
        /* it's either a new bracket level or a cast */
        Value *LexValue;
        enum LexToken BracketToken = LexGetToken(Parser, &LexValue, false);
        if (IsTypeToken(Parser, BracketToken, LexValue) &&
                (*StackTop == NULL || (*StackTop)->Op != TokenSizeof)) {
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

            ExpressionStackCollapse(Parser, StackTop, Precedence+1,
                IgnorePrecedence);
            CastTypeValue = VariableAllocValueFromType(Parser->pc,
                Parser, &Parser->pc->TypeType, false, NULL, false);
            CastTypeValue->Val->Typ = CastType;
            ExpressionStackPushValueNode(Parser, StackTop, CastTypeValue);
            ExpressionStackPushOperator(Parser, StackTop, OrderInfix,
                TokenCast, Precedence);
        } else {
            /* boost the bracket operator precedence */
            return 1; /* Caller should increment BracketPrecedence */
        }
    } else if (Token == TokenDotDot || Token == TokenScoper) {
        /* Handle .. and :: operators (scope resolution) */
        return HandleScopeResolution(Parser, StackTop, Token, PrefixState);
    } else if (Token == TokenDot) {
        /* Check if this is .member (dot-this operator) */
        /* Peek ahead to see if next token is an identifier */
        ParseState PeekParser;
        Value *PeekIdent;
        
        ParserCopy(&PeekParser, Parser);
        enum LexToken NextToken = LexGetToken(&PeekParser, &PeekIdent, false);
        
        if (NextToken == TokenIdentifier) {
            /* It's .member - consume the identifier */
            LexGetToken(Parser, &PeekIdent, true);
            return HandleDotThisOperator(Parser, StackTop, 
                                         PeekIdent->Val->Identifier, PrefixState);
        } else {
            ProgramFail(Parser, "identifier expected after '.'");
            return 0;
        }
    } else {
        /* regular prefix operator (++, --, sizeof, +, -, *, &, !, ~) */
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

        ExpressionStackCollapse(Parser, StackTop, Precedence,
            IgnorePrecedence);
        ExpressionStackPushOperator(Parser, StackTop, OrderPrefix,
            Token, Precedence + TempPrecedenceBoost);
    }
    
    return 1;
}

/* Helper function: Handle infix or postfix operators */
static int HandleInfixOrPostfixOperator(ParseState *Parser, ExpressionStack **StackTop,
                                        enum LexToken Token, int BracketPrecedence,
                                        int *PrefixState, int *IgnorePrecedence,
                                        int *TernaryDepth, int *BracketPrecedenceOut)
{
    int Precedence;
    
    if (OperatorPrecedence[(int)Token].PostfixPrecedence != 0) {
        /* Handle postfix operators (++, --, [], ()) */
        switch (Token) {
        case TokenCloseParen:
        case TokenRightSquareBracket:
            if (BracketPrecedence == 0) {
                /* assume this bracket is after the end of the expression */
                return 0; /* Signal to caller that we're done */
            } else {
                /* collapse to the bracket precedence */
                ExpressionStackCollapse(Parser, StackTop,
                    BracketPrecedence, IgnorePrecedence);
                *BracketPrecedenceOut = BracketPrecedence - BRACKET_PRECEDENCE;
                return 1;
            }
            break;
        default:
            /* scan and collapse the stack to the precedence of this operator, then push */
            Precedence = BracketPrecedence +
                OperatorPrecedence[(int)Token].PostfixPrecedence;
            ExpressionStackCollapse(Parser, StackTop, Precedence,
                IgnorePrecedence);
            ExpressionStackPushOperator(Parser, StackTop,
                OrderPostfix, Token, Precedence);
            return 1;
        }
    } else if (OperatorPrecedence[(int)Token].InfixPrecedence != 0) {
        /* Handle infix operators (+, -, *, /, ., ->, etc.) */
        
        /* Special handling for dot and arrow operators */
        if ((Token == TokenDot || Token == TokenArrow) && Parser->Mode == RunModeRun) {
            ParseState PeekState;
            Value *MemberIdent;
            ParserCopy(&PeekState, Parser);
            
            /* Check if this is a member function call: obj.method() or ptr->method() */
            if (LexGetToken(&PeekState, &MemberIdent, true) == TokenIdentifier &&
                LexGetToken(&PeekState, NULL, false) == TokenOpenParen) {
                /* It's a member function call */
                const char* get_from_stack = NULL;
                char *FuncName = GetMangleName(Parser, StackTop, get_from_stack);
                if (FuncName) {
                    ExpressionParseFunctionCall(Parser, StackTop, FuncName, true);
                    *PrefixState = 0;
                    return 1;
                }
            }
            
            /* Regular member access (obj.member or ptr->member) */
            ExpressionGetStructElement(Parser, StackTop, Token);
            return 1;
        }
        
        /* Regular infix operator (not . or ->) */
        Precedence = BracketPrecedence +
            OperatorPrecedence[(int)Token].InfixPrecedence;

        /* Handle precedence direction (left-to-right vs right-to-left) */
        if (IS_LEFT_TO_RIGHT(OperatorPrecedence[(int)Token].InfixPrecedence)) {
            /* left-to-right: collapse down to this precedence */
            ExpressionStackCollapse(Parser, StackTop, Precedence,
                IgnorePrecedence);
        } else {
            /* right-to-left: only collapse down to next higher precedence */
            ExpressionStackCollapse(Parser, StackTop, Precedence+1,
                IgnorePrecedence);
        }

        /* Short-circuit evaluation for && and || */
        if ((Token == TokenLogicalOr || Token == TokenLogicalAnd) &&
                IS_NUMERIC_COERCIBLE((*StackTop)->Val)) {
            long LHSInt = ExpressionCoerceInteger((*StackTop)->Val);
            if (((Token == TokenLogicalOr && LHSInt) ||
                    (Token == TokenLogicalAnd && !LHSInt)) &&
                 (*IgnorePrecedence > Precedence)) {
                *IgnorePrecedence = Precedence;
            }
        }

        /* Push the operator on the stack */
        ExpressionStackPushOperator(Parser, StackTop,
            OrderInfix, Token, Precedence);
        *PrefixState = 1;  /* After infix operator, expect a prefix operator or value */

        /* Handle ternary operator depth tracking */
        switch (Token) {
        case TokenQuestionMark:
            (*TernaryDepth)++;
            break;
        case TokenColon:
            (*TernaryDepth)--;
            break;
        default:
            break;
        }

        /* Special handling for array subscript operator [ */
        if (Token == TokenLeftSquareBracket) {
            /* boost the bracket operator precedence for array indexing */
            *BracketPrecedenceOut = BracketPrecedence + BRACKET_PRECEDENCE;
        } else {
            *BracketPrecedenceOut = BracketPrecedence;
        }
        
        return 1;
    } else {
        /* Neither postfix nor infix precedence - operator not valid here */
        ProgramFail(Parser, "operator not expected here");
        return 0;
    }
}

/* In expression.c, in HandleValueOrType function: */
static int HandleValueOrType(ParseState *Parser, ExpressionStack **StackTop,
                             enum LexToken Token, Value *LexValue,
                             int *PrefixState, ParseState *PreState)
{
    if (Token == TokenIdentifier) {
        /* ParseTokenIdentifier needs Precedence and IgnorePrecedence parameters */
        /* We don't have them in this context, so pass dummy values */
        int dummyPrecedence = 0;
        int dummyIgnorePrecedence = DEEP_PRECEDENCE;
        ParseTokenIdentifier(Parser, StackTop, LexValue, PrefixState, 
                             &dummyPrecedence, &dummyIgnorePrecedence);
        return 1;
    } else if ((int)Token > TokenCloseParen &&
               (int)Token <= TokenCharacterConstant) {
        /* it's a value of some sort, push it */
        if (!(*PrefixState))
            ProgramFail(Parser, "value not expected here");

        *PrefixState = 0;
        ExpressionStackPushValue(Parser, StackTop, LexValue);
        return 1;
    } else if (IsTypeToken(Parser, Token, LexValue)) {
        /* it's a type. push it on the stack like a value.
            this is used in sizeof() */
        ValueType *Typ;
        char *Identifier;
        Value *TypeValue;

        if (!(*PrefixState))
            ProgramFail(Parser, "type not expected here");

        *PrefixState = 0;
        ParserCopy(Parser, PreState);
        TypeParse(Parser, &Typ, &Identifier, NULL);
        TypeValue = VariableAllocValueFromType(Parser->pc, Parser,
            &Parser->pc->TypeType, false, NULL, false);
        TypeValue->Val->Typ = Typ;
        ExpressionStackPushValueNode(Parser, StackTop, TypeValue);
        return 1;
    } else {
        /* it isn't a token from an expression */
        ParserCopy(Parser, PreState);
        return 0; /* Signal that we're done */
    }
}

/* parse an expression with operator precedence */
int ExpressionParse(ParseState *Parser, Value **Result)
{
    int PrefixState = 1;
    int Done = 0;
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
        Token = LexGetToken(Parser, &LexValue, 1);
        
        /* Check if it's an operator */
        if ((((int)Token > TokenComma && (int)Token <= (int)TokenOpenParen) ||
               (Token == TokenCloseParen && BracketPrecedence != 0)) &&
               (Token != TokenColon || TernaryDepth > 0)) {
            
            if (PrefixState) {
                /* Handle prefix operator */
                if (!HandlePrefixOperator(Parser, &StackTop, Token, BracketPrecedence,
                                         &PrefixState, &IgnorePrecedence, &TernaryDepth)) {
                    if (Token == TokenOpenParen) {
                        /* Open paren increases bracket precedence */
                        BracketPrecedence += BRACKET_PRECEDENCE;
                    }
                }
            } else {
                /* Handle infix or postfix operator */
                int NewBracketPrecedence = BracketPrecedence;
                int Continue = HandleInfixOrPostfixOperator(Parser, &StackTop, Token, 
                                                           BracketPrecedence, &PrefixState,
                                                           &IgnorePrecedence, &TernaryDepth,
                                                           &NewBracketPrecedence);
                
                if (!Continue) {
                    /* Operator indicated we're done (e.g., closing bracket at top level) */
                    ParserCopy(Parser, &PreState);
                    Done = 1;
                } else {
                    BracketPrecedence = NewBracketPrecedence;
                }
            }
        } else {
            /* Handle value, type, or end of expression */
            if (!HandleValueOrType(Parser, &StackTop, Token, LexValue, 
                                  &PrefixState, &PreState)) {
                Done = 1;
            }
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