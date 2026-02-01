/* parse_statement.c - Core statement parser */
#include "parse.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "type.h"
#include "heap.h"
#include "platform.h"
#include "parse_declaration.h"
#include "parse_control.h"
#include "parse_macro.h"

#ifdef DEBUGGER
#include "debugger.h"
#endif

/* parse a statement */
enum ParseResult ParseStatement(ParseState *Parser, int CheckTrailingSemicolon)
{
    int Condition;
    enum LexToken Token;
    Value *CValue = 0;
    Value *LexerValue = 0;
    Value *VarValue = 0;
    ParseState PreState;

#ifdef DEBUGGER
    /* if we're debugging, check for a breakpoint */
    if (Parser->DebugMode && Parser->Mode == RunModeRun)
        DebugCheckStatement(Parser);
#endif

    /* take note of where we are and then grab a token */
    ParserCopy(&PreState, Parser);
    Token = LexGetToken(Parser, &LexerValue, true);

    switch (Token) {
    case TokenEOF:
        return ParseResultEOF;
    case TokenDotDot:    
    case TokenScoper:     
        Token = LexGetToken(Parser, &LexerValue, true);
        if(VariableGetDefined(Parser->pc, Parser, LexerValue->Val->Identifier, &VarValue,true)){
            if (VarValue->Typ->Base == Type_Type) {
                *Parser = PreState;
                ParseDeclaration(Parser, Token);
                CheckTrailingSemicolon = false;
                break;
            }
        }
        break;
    case TokenIdentifier:
        /* might be a typedef-typed variable declaration or an expression */
        if(VariableGetDefined(Parser->pc, Parser, LexerValue->Val->Identifier, &VarValue,false)){
            if (VarValue->Typ->Base == Type_Type) {
                *Parser = PreState;
                ParseDeclaration(Parser, Token);
                CheckTrailingSemicolon = false;
                break;
            }
        } else {
            /* it might be a goto label */
            enum LexToken NextToken = LexGetToken(Parser, NULL, false);
            if (NextToken == TokenColon) {
                /* declare the identifier as a goto label */
                LexGetToken(Parser, NULL, true);
                if (Parser->Mode == RunModeGoto &&
                        LexerValue->Val->Identifier == Parser->SearchGotoLabel)
                    Parser->Mode = RunModeRun;
                CheckTrailingSemicolon = false;
                break;
            }
        }
        /* fall through to expression */
    case TokenAsterisk:
    case TokenAmpersand:
    case TokenIncrement:
    case TokenDecrement:
    case TokenOpenParen:
        *Parser = PreState;
        ExpressionParse(Parser, &CValue);
        if (Parser->Mode == RunModeRun)
            VariableStackPop(Parser, CValue);
        break;
    case TokenLeftBrace:
        ParseBlock(Parser, false, true);
        CheckTrailingSemicolon = false;
        break;
    case TokenIf:
        ParseIfStatement(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenWhile:
        ParseWhileStatement(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenDo:
        ParseDoWhileStatement(Parser);
        break;
    case TokenFor:
        ParseFor(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenSemicolon:
        CheckTrailingSemicolon = false;
        break;
    case TokenIntType:
    case TokenShortType:
    case TokenCharType:
    case TokenLongType:
    case TokenFloatType:
    case TokenDoubleType:
    case TokenVoidType:
    case TokenStructType:
    case TokenUnionType:
    case TokenEnumType:
    case TokenSignedType:
    case TokenUnsignedType:
    case TokenStaticType:
    case TokenAutoType:
    case TokenRegisterType:
    case TokenExternType:
        *Parser = PreState;
        CheckTrailingSemicolon = ParseDeclaration(Parser, Token);
        break;
    case TokenHashDefine:
        ParseMacroDefinition(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenHashInclude:
        ParseIncludeStatement(Parser, &LexerValue);
        CheckTrailingSemicolon = false;
        break;
    case TokenSwitch:
        ParseSwitchStatement(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenCase:
        ParseCaseStatement(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenDefault:
        ParseDefaultStatement(Parser);
        CheckTrailingSemicolon = false;
        break;
    case TokenBreak:
        if (Parser->Mode == RunModeRun)
            Parser->Mode = RunModeBreak;
        break;
    case TokenContinue:
        if (Parser->Mode == RunModeRun)
            Parser->Mode = RunModeContinue;
        break;
    case TokenReturn:
        ParseReturnStatement(Parser, &CValue);
        break;
    case TokenTypedef:
        ParseTypedef(Parser);
        break;
    case TokenGoto:
        ParseGotoStatement(Parser, &LexerValue);
        break;
    case TokenDelete:
        ParseDeleteStatement(Parser, &LexerValue, &CValue);
        break;
    default:
        *Parser = PreState;
        return ParseResultError;
    }
    if (CheckTrailingSemicolon) {
        if (LexGetToken(Parser, NULL, true) != TokenSemicolon)
            ProgramFail(Parser, "';' expected");
    }

    return ParseResultOk;
}

/* parse a statement, but only run it if Condition is true */
enum ParseResult ParseStatementMaybeRun(ParseState *Parser,
    int Condition, int CheckTrailingSemicolon)
{
    if (Parser->Mode != RunModeSkip && !Condition) {
        enum RunMode OldMode = Parser->Mode;
        int Result;
        Parser->Mode = RunModeSkip;
        Result = ParseStatement(Parser, CheckTrailingSemicolon);
        Parser->Mode = OldMode;
        return (enum ParseResult)Result;
    } else {
        return ParseStatement(Parser, CheckTrailingSemicolon);
    }
}
