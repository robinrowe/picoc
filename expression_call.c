/* expression_call.c - Function and macro calls */
#include "expression_call.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "parse.h"
#include "heap.h"

/* do a parameterized macro call */
void ExpressionParseMacroCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *MacroName,
    struct MacroDef *MDef)
{
    int ArgCount;
    enum LexToken Token;
    Value *ReturnValue = NULL;
    Value *Param = 0;
    Value **ParamArray = NULL;

    if (Parser->Mode == RunModeRun) {
        /* create a stack frame for this macro */
        ExpressionStackPushValueByType(Parser, StackTop, &Parser->pc->FPType);
        ReturnValue = (*StackTop)->Val;
        HeapPushStackFrame(Parser->pc);
        ParamArray = HeapAllocStack(Parser->pc,
            sizeof(Value*) * MDef->NumParams);
        if (ParamArray == NULL)
            ProgramFail(Parser, "(ExpressionParseMacroCall) out of memory");
    } else {
        ExpressionPushInt(Parser, StackTop, 0);
    }

    /* parse arguments */
    ArgCount = 0;
    do {
        if (ExpressionParse(Parser, &Param)) {
            if (Parser->Mode == RunModeRun) {
                if (ArgCount < MDef->NumParams)
                    ParamArray[ArgCount] = Param;
                else
                    ProgramFail(Parser, "too many arguments to %s()", MacroName);
            }

            ArgCount++;
            Token = LexGetToken(Parser, NULL, true);
            if (Token != TokenComma && Token != TokenCloseParen)
                ProgramFail(Parser, "comma expected");
        } else {
            /* end of argument list? */
            Token = LexGetToken(Parser, NULL, true);
            if (Token != TokenCloseParen)
                ProgramFail(Parser, "bad argument");
        }
    } while (Token != TokenCloseParen);

    if (Parser->Mode == RunModeRun) {
        /* evaluate the macro */
        ParseState MacroParser;
        int Count;
        Value *EvalValue;

        if (ArgCount < MDef->NumParams)
            ProgramFail(Parser, "not enough arguments to '%s'", MacroName);

        if (MDef->Body.Pos == NULL)
            ProgramFail(Parser,
                "ExpressionParseMacroCall MacroName: '%s' is undefined",
                MacroName);

        ParserCopy(&MacroParser, &MDef->Body);
        MacroParser.Mode = Parser->Mode;
        VariableStackFrameAdd(Parser, MacroName, 0);
        Parser->pc->TopStackFrame->NumParams = ArgCount;
        Parser->pc->TopStackFrame->ReturnValue = ReturnValue;
        
        for (Count = 0; Count < MDef->NumParams; Count++) {
            VariableDefine(Parser->pc, Parser, MDef->ParamName[Count],
                ParamArray[Count], NULL, true);
        }

        ExpressionParse(&MacroParser, &EvalValue);
        ExpressionAssign(Parser, ReturnValue, EvalValue, true, MacroName, 0, false);
        VariableStackFramePop(Parser);
        HeapPopStackFrame(Parser->pc);
    }
}

/* do a function call */
void ExpressionParseFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt)
{
    int ArgCount;
    enum LexToken Token = LexGetToken(Parser, NULL, true);    /* open bracket */
    enum RunMode OldMode = Parser->Mode;
    Value *ReturnValue = NULL;
    Value *FuncValue = NULL;
    Value *Param;
    Value **ParamArray = NULL;
    
    if (RunIt) {
        /* Check if FuncName contains a dot (member function call) */
        const char *isMemberFunction = strchr(FuncName, '.');
        const char *LookupName = FuncName; 
        char MangledName[256];
        
        if (isMemberFunction) {
            /* It's a member function call - struct is already on StackTop */
            if (*StackTop == NULL || (*StackTop)->Val == NULL)
                ProgramFail(Parser, "internal error: expected struct on stack for member call");
            
            Value *StructVar = (*StackTop)->Val;
            if (StructVar->Typ->Base != TypeStruct)
                ProgramFail(Parser, "member functions can only be called on structs");
            
            /* Build mangled name: StructTypeName.MemberName */
            const char *MemberName = isMemberFunction + 1;
            snprintf(MangledName, sizeof(MangledName), "%s.%s",
                     StructVar->Typ->Identifier, MemberName);
            LookupName = TableStrRegister(Parser->pc, MangledName, strlen(MangledName));
            
#ifdef VERBOSE
            printf("DEBUG: Member call %s -> mangled to %s\n", FuncName, LookupName);
#endif
        }
        
        /* get the function definition from variable tables */
        ShowX(">TableGet", "GlobalTable", LookupName, 0);
        
        /* Functions are always in GlobalTable - search there directly */
        if (!TableGet(&Parser->pc->GlobalTable, LookupName, &FuncValue, NULL, NULL, NULL))
            ProgramFail(Parser, "identifier '%s' is undefined", LookupName);
            
        if (FuncValue->Typ->Base == TypeMacro) {
            /* this is actually a macro, not a function */
            ExpressionParseMacroCall(Parser, StackTop, LookupName,
                &FuncValue->Val->MacroDef);
            return;
        }
        
        if (FuncValue->Typ->Base != TypeFunction)
            ProgramFail(Parser, "%t is not a function - can't call",
                FuncValue->Typ);
                
        ExpressionStackPushValueByType(Parser, StackTop,
            FuncValue->Val->FuncDef.ReturnType);
        ReturnValue = (*StackTop)->Val;
        HeapPushStackFrame(Parser->pc);
        ParamArray = HeapAllocStack(Parser->pc,
            sizeof(Value*) * FuncValue->Val->FuncDef.NumParams);
        if (ParamArray == NULL)
            ProgramFail(Parser, "(ExpressionParseFunctionCall) out of memory");
    } else {
        ExpressionPushInt(Parser, StackTop, 0);
        Parser->Mode = RunModeSkip;
    }
    
    /* parse arguments */
    ArgCount = 0;
    do {
        if (RunIt && ArgCount < FuncValue->Val->FuncDef.NumParams)
            ParamArray[ArgCount] = VariableAllocValueFromType(Parser->pc, Parser,
                FuncValue->Val->FuncDef.ParamType[ArgCount], false, NULL, false);
                
        if (ExpressionParse(Parser, &Param)) {
            if (RunIt) {
                if (ArgCount < FuncValue->Val->FuncDef.NumParams) {
                    ExpressionAssign(Parser, ParamArray[ArgCount], Param, true,
                        FuncName, ArgCount+1, false);
                    VariableStackPop(Parser, Param);
                } else {
                    if (!FuncValue->Val->FuncDef.VarArgs)
                        ProgramFail(Parser, "too many arguments to %s()", FuncName);
                }
            }
            ArgCount++;
            Token = LexGetToken(Parser, NULL, true);
            if (Token != TokenComma && Token != TokenCloseParen)
                ProgramFail(Parser, "comma expected");
        } else {
            /* end of argument list? */
            Token = LexGetToken(Parser, NULL, true);
            if (Token != TokenCloseParen)
                ProgramFail(Parser, "bad argument");
        }
    } while (Token != TokenCloseParen);
    
    if (RunIt) {
        /* run the function */
        if (ArgCount < FuncValue->Val->FuncDef.NumParams)
            ProgramFail(Parser, "not enough arguments to '%s'", FuncName);
            
        if (FuncValue->Val->FuncDef.Intrinsic == NULL) {
            /* run a user-defined function */
            int Count;
            int OldScopeID = Parser->ScopeID;
            ParseState FuncParser;
            
            if (FuncValue->Val->FuncDef.Body.Pos == NULL)
                ProgramFail(Parser,
                    "ExpressionParseFunctionCall FuncName: '%s' is undefined",
                    FuncName);
                    
            ParserCopy(&FuncParser, &FuncValue->Val->FuncDef.Body);
            VariableStackFrameAdd(Parser, FuncName,
                FuncValue->Val->FuncDef.Intrinsic ? FuncValue->Val->FuncDef.NumParams : 0);
            Parser->pc->TopStackFrame->NumParams = ArgCount;
            Parser->pc->TopStackFrame->ReturnValue = ReturnValue;
            
            if (Parser->pc->StructType) {
                /* Function parameters should not go out of scope */
                Parser->ScopeID = -1;
            }
            
            for (Count = 0; Count < FuncValue->Val->FuncDef.NumParams; Count++) {
                VariableDefine(Parser->pc, Parser,
                    FuncValue->Val->FuncDef.ParamName[Count], ParamArray[Count],
                    NULL, true);
            }
            
            Parser->ScopeID = OldScopeID;
            
            if (ParseStatement(&FuncParser, true) != ParseResultOk)
                ProgramFail(&FuncParser, "function body expected");
                
            if (RunIt) {
                if (FuncParser.Mode == RunModeRun &&
                        FuncValue->Val->FuncDef.ReturnType != &Parser->pc->VoidType)
                    ProgramFail(&FuncParser,
                        "no value returned from a function returning %t",
                        FuncValue->Val->FuncDef.ReturnType);
                else if (FuncParser.Mode == RunModeGoto)
                    ProgramFail(&FuncParser, "couldn't find goto label '%s'",
                        FuncParser.SearchGotoLabel);
            }
            VariableStackFramePop(Parser);
        } else {
            // FIXME: too many parameters?
            FuncValue->Val->FuncDef.Intrinsic(Parser, ReturnValue, ParamArray, ArgCount);
        }
        HeapPopStackFrame(Parser->pc);
    }
    
    Parser->Mode = OldMode;
}
