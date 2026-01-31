/* parse_control.c - Control flow statement parsing */
#include "parse_control.h"
#include "interpreter.h"
#include "variable.h"
#include "lex.h"
#include "type.h"
#include "heap.h"
#include "platform.h"
#include "table.h"
#include "expression.h"

/* parse a block of code and return what mode it returned in */
enum RunMode ParseBlock(ParseState *Parser, int AbsorbOpenBrace, int Condition)
{
    int PrevScopeID = 0;
    int ScopeID = VariableScopeBegin(Parser, &PrevScopeID);

    if (AbsorbOpenBrace && LexGetToken(Parser, NULL, true) != TokenLeftBrace)
        ProgramFail(Parser, "'{' expected");

    if (Parser->Mode == RunModeSkip || !Condition) {
        /* condition failed - skip this block */
        enum RunMode OldMode = Parser->Mode;
        Parser->Mode = RunModeSkip;
        while (ParseStatement(Parser, true) == ParseResultOk) {
            /* empty loop body */
        }
        Parser->Mode = OldMode;
    } else {
        /* just run it in its current mode */
        while (ParseStatement(Parser, true) == ParseResultOk) {
            /* empty loop body */
        }
    }

    if (LexGetToken(Parser, NULL, true) != TokenRightBrace)
        ProgramFail(Parser, "'}' expected");

    VariableScopeEnd(Parser, ScopeID, PrevScopeID);
    return Parser->Mode;
}

/* parse a "for" statement */
void ParseFor(ParseState *Parser)
{
    int Condition;
    ParseState PreConditional;
    ParseState PreIncrement;
    ParseState PreStatement;
    ParseState After;
    enum RunMode OldMode = Parser->Mode;
    int PrevScopeID = 0;
    int ScopeID = VariableScopeBegin(Parser, &PrevScopeID);

    if (LexGetToken(Parser, NULL, true) != TokenOpenParen)
        ProgramFail(Parser, "'(' expected");

    if (ParseStatement(Parser, true) != ParseResultOk)
        ProgramFail(Parser, "statement expected");

    ParserCopyPos(&PreConditional, Parser);
    if (LexGetToken(Parser, NULL, false) == TokenSemicolon)
        Condition = true;
    else
        Condition = ExpressionParseInt(Parser);

    if (LexGetToken(Parser, NULL, true) != TokenSemicolon)
        ProgramFail(Parser, "';' expected");

    ParserCopyPos(&PreIncrement, Parser);
    ParseStatementMaybeRun(Parser, false, false);

    if (LexGetToken(Parser, NULL, true) != TokenCloseParen)
        ProgramFail(Parser, "')' expected");

    ParserCopyPos(&PreStatement, Parser);
    if (ParseStatementMaybeRun(Parser, Condition, true) != ParseResultOk)
        ProgramFail(Parser, "statement expected");

    if (Parser->Mode == RunModeContinue && OldMode == RunModeRun)
        Parser->Mode = RunModeRun;

    ParserCopyPos(&After, Parser);

    while (Condition && Parser->Mode == RunModeRun) {
        ParserCopyPos(Parser, &PreIncrement);
        ParseStatement(Parser, false);

        ParserCopyPos(Parser, &PreConditional);
        if (LexGetToken(Parser, NULL, false) == TokenSemicolon)
            Condition = true;
        else
            Condition = ExpressionParseInt(Parser);

        if (Condition) {
            ParserCopyPos(Parser, &PreStatement);
            ParseStatement(Parser, true);

            if (Parser->Mode == RunModeContinue)
                Parser->Mode = RunModeRun;
        }
    }

    if (Parser->Mode == RunModeBreak && OldMode == RunModeRun)
        Parser->Mode = RunModeRun;

    VariableScopeEnd(Parser, ScopeID, PrevScopeID);
    ParserCopyPos(Parser, &After);
}

/* parse an "if" statement */
void ParseIfStatement(ParseState *Parser)
{
    int Condition;
    
    if (LexGetToken(Parser, NULL, true) != TokenOpenParen)
        ProgramFail(Parser, "'(' expected");
        
    Condition = ExpressionParseInt(Parser);
    
    if (LexGetToken(Parser, NULL, true) != TokenCloseParen)
        ProgramFail(Parser, "')' expected");
        
    if (ParseStatementMaybeRun(Parser, Condition, true) != ParseResultOk)
        ProgramFail(Parser, "statement expected");
        
    if (LexGetToken(Parser, NULL, false) == TokenElse) {
        LexGetToken(Parser, NULL, true);
        if (ParseStatementMaybeRun(Parser, !Condition, true) != ParseResultOk)
            ProgramFail(Parser, "statement expected");
    }
}

/* parse a "while" statement */
void ParseWhileStatement(ParseState *Parser)
{
    struct ParseState PreConditional;
    enum RunMode PreMode = Parser->Mode;
    
    if (LexGetToken(Parser, NULL, true) != TokenOpenParen)
        ProgramFail(Parser, "'(' expected");
        
    ParserCopyPos(&PreConditional, Parser);
    int Condition = 0;
    do {
        ParserCopyPos(Parser, &PreConditional);
        Condition = ExpressionParseInt(Parser);
        
        if (LexGetToken(Parser, NULL, true) != TokenCloseParen)
            ProgramFail(Parser, "')' expected");
            
        if (ParseStatementMaybeRun(Parser, Condition, true) != ParseResultOk)
            ProgramFail(Parser, "statement expected");
            
        if (Parser->Mode == RunModeContinue)
            Parser->Mode = PreMode;
            
    } while (Parser->Mode == RunModeRun && Condition);
    
    if (Parser->Mode == RunModeBreak)
        Parser->Mode = PreMode;
}

/* parse a "do-while" statement */
void ParseDoWhileStatement(ParseState *Parser)
{
    struct ParseState PreStatement;
    enum RunMode PreMode = Parser->Mode;
    int Condition;
    
    ParserCopyPos(&PreStatement, Parser);
    
    do {
        ParserCopyPos(Parser, &PreStatement);
        if (ParseStatement(Parser, true) != ParseResultOk)
            ProgramFail(Parser, "statement expected");
            
        if (Parser->Mode == RunModeContinue)
            Parser->Mode = PreMode;
            
        if (LexGetToken(Parser, NULL, true) != TokenWhile)
            ProgramFail(Parser, "'while' expected");
            
        if (LexGetToken(Parser, NULL, true) != TokenOpenParen)
            ProgramFail(Parser, "'(' expected");
            
        Condition = ExpressionParseInt(Parser);
        
        if (LexGetToken(Parser, NULL, true) != TokenCloseParen)
            ProgramFail(Parser, "')' expected");
            
    } while (Condition && Parser->Mode == RunModeRun);
    
    if (Parser->Mode == RunModeBreak)
        Parser->Mode = PreMode;
}

/* parse a "switch" statement */
void ParseSwitchStatement(ParseState *Parser)
{
    int Condition;
    
    if (LexGetToken(Parser, NULL, true) != TokenOpenParen)
        ProgramFail(Parser, "'(' expected");
        
    Condition = ExpressionParseInt(Parser);
    
    if (LexGetToken(Parser, NULL, true) != TokenCloseParen)
        ProgramFail(Parser, "')' expected");
        
    if (LexGetToken(Parser, NULL, false) != TokenLeftBrace)
        ProgramFail(Parser, "'{' expected");
        
    /* new block so we can store parser state */
    enum RunMode OldMode = Parser->Mode;
    int OldSearchLabel = Parser->SearchLabel;
    
    Parser->Mode = RunModeCaseSearch;
    Parser->SearchLabel = Condition;
    
    ParseBlock(Parser, true, (OldMode != RunModeSkip) && (OldMode != RunModeReturn));
    
    if (Parser->Mode != RunModeReturn)
        Parser->Mode = OldMode;
        
    Parser->SearchLabel = OldSearchLabel;
}

/* parse a "case" statement */
void ParseCaseStatement(ParseState *Parser)
{
    int Condition;
    
    if (Parser->Mode == RunModeCaseSearch) {
        Parser->Mode = RunModeRun;
        Condition = ExpressionParseInt(Parser);
        Parser->Mode = RunModeCaseSearch;
    } else {
        Condition = ExpressionParseInt(Parser);
    }
    
    if (LexGetToken(Parser, NULL, true) != TokenColon)
        ProgramFail(Parser, "':' expected");
        
    if (Parser->Mode == RunModeCaseSearch && Condition == Parser->SearchLabel)
        Parser->Mode = RunModeRun;
}

/* parse a "default" statement */
void ParseDefaultStatement(ParseState *Parser)
{
    if (LexGetToken(Parser, NULL, true) != TokenColon)
        ProgramFail(Parser, "':' expected");
        
    if (Parser->Mode == RunModeCaseSearch)
        Parser->Mode = RunModeRun;
}

/* parse a "return" statement */
void ParseReturnStatement(ParseState *Parser, Value **CValue)
{
    if (Parser->Mode == RunModeRun) {
        if (!Parser->pc->TopStackFrame ||
                Parser->pc->TopStackFrame->ReturnValue->Typ->Base != TypeVoid) {
            if (!ExpressionParse(Parser, CValue))
                ProgramFail(Parser, "value required in return");
                
            if (!Parser->pc->TopStackFrame) { /* return from top-level program? */
                PlatformExit(Parser->pc, ExpressionCoerceInteger(*CValue));
            } else {
                ExpressionAssign(Parser,
                    Parser->pc->TopStackFrame->ReturnValue, *CValue, true,
                    NULL, 0, false);
            }
            
            VariableStackPop(Parser, *CValue);
        } else {
            if (ExpressionParse(Parser, CValue))
                ProgramFail(Parser, "value in return from a void function");
        }
        
        Parser->Mode = RunModeReturn;
    } else {
        ExpressionParse(Parser, CValue);
    }
}

/* parse a "goto" statement */
void ParseGotoStatement(ParseState *Parser, Value **LexerValue)
{
    if (LexGetToken(Parser, LexerValue, true) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");
        
    if (Parser->Mode == RunModeRun) {
        /* start scanning for the goto label */
        Parser->SearchGotoLabel = (*LexerValue)->Val->Identifier;
        Parser->Mode = RunModeGoto;
    }
}

/* parse a "delete" statement */
void ParseDeleteStatement(ParseState *Parser, Value **LexerValue, Value **CValue)
{
    /* try it as a function or variable name to delete */
    if (LexGetToken(Parser, LexerValue, true) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");
        
    if (Parser->Mode == RunModeRun) {
        /* delete this variable or function */
        *CValue = TableDelete(Parser->pc, &Parser->pc->GlobalTable,
            (*LexerValue)->Val->Identifier);
            
        if (*CValue == NULL) {
            ProgramFail(Parser, "'%s' is not defined",
                (*LexerValue)->Val->Identifier);
        }

        VariableFree(Parser->pc, *CValue);
    }
}
