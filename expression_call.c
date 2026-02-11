/* expression_call.c - Function and macro calls */
#include "expression_call.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "parse.h"
#include "heap.h"
#include "expression_stack.h"

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

#if 0
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
    const char *member_function_name = 0;
    Value *StructVar = 0;
    if (RunIt) {
        /* Check if FuncName contains a dot (member function call) */
        member_function_name = strchr(FuncName, '.');
        const char *LookupName = FuncName; 
        char MangledName[256];
        
        if (member_function_name) {
            /* It's a member function call - struct is already on StackTop */
            if (*StackTop == NULL || (*StackTop)->Val == NULL)
                ProgramFail(Parser, "internal error: expected struct on stack for member call");
            
            StructVar = (*StackTop)->Val;
            if (StructVar->Typ->Base != TypeStruct)
                ProgramFail(Parser, "member functions can only be called on structs");
            
            /* Build mangled name: StructTypeName.MemberName */
            const char *MemberName = member_function_name + 1;
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
    do 
    {   if (!ArgCount && member_function_name)
        {   puts("this set");
#if 1
            ParamArray[0] = VariableAllocValueFromExistingData(Parser,
                StructVar->Typ,(union AnyValue*) StructVar, 0, 0);
#else
            ParamArray[ArgCount] = VariableAllocValueFromType(Parser->pc, Parser,
                FuncValue->Val->FuncDef.ParamType[ArgCount], false, NULL, false);
            ParamArray[0]->Val->Pointer = StructVar;
  //          ArgCount = 1;  // CRITICAL: Mark that we have first parameter
  //          continue;      // Skip ExpressionParse for 'this'
#endif
        }
        else if (RunIt && ArgCount < FuncValue->Val->FuncDef.NumParams)
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
        // int this_count = member_function_name != 0;
        if(member_function_name)
        {   ArgCount++;
        }
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
            if(member_function_name)
            {   puts(" member VariableFree(Parser)");
   //             VariableFree(Parser->pc, ParamArray[0]);
            }
            VariableStackFramePop(Parser);
        } else {
            // Never encountered... FIXME: too many parameters?
            FuncValue->Val->FuncDef.Intrinsic(Parser, ReturnValue, ParamArray, ArgCount);
        }
        HeapPopStackFrame(Parser->pc);
    }
    
    Parser->Mode = OldMode;
}

#else

static void PrepareFunctionExecution(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName,
    Value **FuncValue, Value **ReturnValue, Value ***ParamArray);
static void ParseAndExecuteFunction(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt,
    enum LexToken InitialToken, Value *FuncValue,
    Value **ParamArray, Value *ReturnValue,Value* StructVar);
static void ExecuteUserDefinedFunction(ParseState *Parser,
    const char *FuncName, Value *FuncValue, Value **ParamArray,
    int ArgCount, Value *ReturnValue);

/* do a function call */
void ExpressionParseFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt)
{   enum RunMode OldMode = Parser->Mode;
    enum LexToken Token = LexGetToken(Parser, NULL, true);    /* open bracket */
    Value *FuncValue = NULL;
    Value **ParamArray = NULL;
    Value *ReturnValue = NULL;
    Value *StructVar = NULL;
    if (RunIt) {
        if(IsMemberFunction(FuncName))
        {   if (*StackTop == NULL || (*StackTop)->Val == NULL)
                ProgramFail(Parser, "internal error: expected struct on stack for member call");
            StructVar = (*StackTop)->Val;
            if (StructVar->Typ->Base != TypeStruct)
                ProgramFail(Parser, "member functions can only be called on structs");
#if 0
        // pop expression stack:
            if (*StackTop && (*StackTop)->Val == StructVar) {
                ExpressionStack *node = *StackTop;
                *StackTop = node->Next;
                /* Free the stack node */
                HeapPopStack(Parser->pc, node, 
                    sizeof(ExpressionStack) + sizeof(Value) + 
                    TypeStackSizeValue(StructVar));
            }
#endif
        }
        PrepareFunctionExecution(Parser, StackTop, FuncName, &FuncValue, 
            &ReturnValue, &ParamArray);
        if (!FuncValue) return; /* Macro was handled */
    } else {
        ExpressionPushInt(Parser, StackTop, 0);
        Parser->Mode = RunModeSkip;
    }
    ParseAndExecuteFunction(Parser, StackTop, FuncName, RunIt, Token,
        FuncValue, ParamArray, ReturnValue,StructVar);
    Parser->Mode = OldMode;
}

static void PrepareFunctionExecution(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName,
    Value **FuncValue, Value **ReturnValue, Value ***ParamArray)
{   Value *FuncValueLocal = NULL;
    /* Lookup function in global table */
    ShowX(">TableGet", "GlobalTable", FuncName, 0);
    if (!TableGet(&Parser->pc->GlobalTable, FuncName, &FuncValueLocal, NULL, NULL, NULL))
        ProgramFail(Parser, "identifier '%s' is undefined", FuncName);
    if (FuncValueLocal->Typ->Base == TypeMacro) {
        /* this is actually a macro, not a function */
        ExpressionParseMacroCall(Parser, StackTop, FuncName,
            &FuncValueLocal->Val->MacroDef);
        *FuncValue = NULL;
        return;
    }
    if (FuncValueLocal->Typ->Base != TypeFunction)
        ProgramFail(Parser, "%t is not a function - can't call",
            FuncValueLocal->Typ);
    /* Prepare function execution stack */
    ExpressionStackPushValueByType(Parser, StackTop,
        FuncValueLocal->Val->FuncDef.ReturnType);
    *ReturnValue = (*StackTop)->Val;
    HeapPushStackFrame(Parser->pc);
    *ParamArray = HeapAllocStack(Parser->pc,
        sizeof(Value*) * FuncValueLocal->Val->FuncDef.NumParams);
    if (*ParamArray == NULL)
        ProgramFail(Parser, "(ExpressionParseFunctionCall) out of memory");
    *FuncValue = FuncValueLocal;
}

static void ParseAndExecuteFunction(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt,
    enum LexToken InitialToken, Value *FuncValue,
    Value **ParamArray, Value *ReturnValue,Value* StructVar)
{   int ArgCount = 0;
    enum LexToken Token = InitialToken;
    /* Parse all arguments */
    const int is_member_function = IsMemberFunction(FuncName);
    do {
        if (!ArgCount && is_member_function && RunIt) {
            /* Handle 'this' parameter for member functions */
            puts("Pass member function 'this'");
            ParamArray[0] = VariableAllocValueFromExistingData(Parser,
                StructVar->Typ, (union AnyValue*) StructVar, 0, 0);
        } else if (RunIt && ArgCount < FuncValue->Val->FuncDef.NumParams) {
            ParamArray[ArgCount] = VariableAllocValueFromType(Parser->pc, Parser,
                FuncValue->Val->FuncDef.ParamType[ArgCount], false, NULL, false);
        }
        Value *Param;
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
    /* Execute the function if running */
    if (RunIt) {
        if (is_member_function) {
            ArgCount++;  /* Count 'this' parameter */
        }
        if (ArgCount < FuncValue->Val->FuncDef.NumParams)
            ProgramFail(Parser, "not enough arguments to '%s'", FuncName);
        if (FuncValue->Val->FuncDef.Intrinsic == NULL) {
            ExecuteUserDefinedFunction(Parser, FuncName, FuncValue, 
                 ParamArray, ArgCount, ReturnValue);
        } else {
            /* Intrinsic function call */
            FuncValue->Val->FuncDef.Intrinsic(Parser, ReturnValue, ParamArray, ArgCount);
        }
        HeapPopStackFrame(Parser->pc);
    }
}

static void ExecuteUserDefinedFunction(ParseState *Parser,
    const char *FuncName, Value *FuncValue, Value **ParamArray,
    int ArgCount, Value *ReturnValue)
{   int OldScopeID = Parser->ScopeID;
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
    for (int Count = 0; Count < FuncValue->Val->FuncDef.NumParams; Count++) {
        VariableDefine(Parser->pc, Parser,
            FuncValue->Val->FuncDef.ParamName[Count], ParamArray[Count],
            NULL, true);
    }
    Parser->ScopeID = OldScopeID;
    if (ParseStatement(&FuncParser, true) != ParseResultOk)
        ProgramFail(&FuncParser, "function body expected");
    if (FuncParser.Mode == RunModeRun &&
            FuncValue->Val->FuncDef.ReturnType != &Parser->pc->VoidType)
        ProgramFail(&FuncParser,
            "no value returned from a function returning %t",
            FuncValue->Val->FuncDef.ReturnType);
    else if (FuncParser.Mode == RunModeGoto)
        ProgramFail(&FuncParser, "couldn't find goto label '%s'",
            FuncParser.SearchGotoLabel);
    VariableStackFramePop(Parser);
}

#endif