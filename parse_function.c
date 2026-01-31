/* parse_function.c - Function definition parsing */
#include "parse_function.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "type.h"

/* count the number of parameters to a function or macro */
int ParseCountParams(ParseState *Parser)
{
    int ParamCount = 0;
    enum LexToken Token = LexGetToken(Parser, NULL, true);
    
    if (Token != TokenCloseParen && Token != TokenEOF) {
        /* count the number of parameters */
        ParamCount++;
        while ((Token = LexGetToken(Parser, NULL, true)) !=
                TokenCloseParen && Token != TokenEOF) {
            if (Token == TokenComma)
                ParamCount++;
        }
    }

    return ParamCount;
}

/* parse a function definition and store it for later */
Value *ParseFunctionDefinition(ParseState *Parser,
    ValueType *ReturnType, char *Identifier, ValueType *this_type)
{
    int ParamCount = 0;
    char *ParamIdentifier;
    enum LexToken Token = TokenNone;
    ValueType *ParamType;
    ParseState ParamParser;
    Value *FuncValue;
    Value *OldFuncValue;
    ParseState FuncBody;
    Engine *pc = Parser->pc;
    
#ifdef MAGIC
    printf("MAGIC: ParseFunctionDefinition adding '%s'\n", Identifier);
#endif
    if (pc->TopStackFrame != NULL)
        ProgramFail(Parser, "nested function definitions are not allowed");

    LexGetToken(Parser, NULL, true);  /* open bracket */
    ParserCopy(&ParamParser, Parser);
    ParamCount = ParseCountParams(Parser);
    if (ParamCount > PARAMETER_MAX)
        ProgramFail(Parser, "too many parameters (%d allowed)", PARAMETER_MAX);
    
    if (this_type) {
        ParamCount++;
    }
    
    FuncValue = VariableAllocValueAndData(pc, Parser,
        sizeof(struct FuncDef) + sizeof(ValueType*)*ParamCount +
        sizeof(const char*)*ParamCount,
        false, NULL, true);
    FuncValue->Typ = &pc->FunctionType;
    FuncValue->Val->FuncDef.ReturnType = ReturnType;
    FuncValue->Val->FuncDef.NumParams = ParamCount;
    FuncValue->Val->FuncDef.VarArgs = false;
    FuncValue->Val->FuncDef.ParamType =
        (ValueType**)((char*)FuncValue->Val + sizeof(struct FuncDef));
    FuncValue->Val->FuncDef.ParamName =
        (char**)((char*)FuncValue->Val->FuncDef.ParamType +
            sizeof(ValueType*)*ParamCount);
    
    ParamCount = 0;
    /* ADD IMPLICIT 'this' PARAMETER FIRST */
    if (this_type) {
        ValueType *StructType = this_type;
        /* Create a pointer to the struct type */
        ValueType *ThisPtrType = TypeGetMatching(pc, Parser, StructType,
            TypePointer, 0, NULL, true);
        
        /* Add as first parameter */
        FuncValue->Val->FuncDef.ParamType[0] = ThisPtrType;
        FuncValue->Val->FuncDef.ParamName[0] = TableStrRegister(pc, "this", 4);
        ParamCount = 1;
    }

    /* NOW PARSE USER'S PARAMETERS */
    for (; ParamCount < FuncValue->Val->FuncDef.NumParams; ParamCount++) {
        /* harvest the parameters into the function definition */
        if (ParamCount == FuncValue->Val->FuncDef.NumParams-1 &&
                LexGetToken(&ParamParser, NULL, false) == TokenEllipsis) {
            /* ellipsis at end */
            FuncValue->Val->FuncDef.NumParams--;
            FuncValue->Val->FuncDef.VarArgs = true;
            break;
        } else {
            /* add a parameter */
            TypeParse(&ParamParser, &ParamType, &ParamIdentifier, NULL);
            if (ParamType->Base == TypeVoid) {
                /* this isn't a real parameter at all - delete it */
                FuncValue->Val->FuncDef.NumParams--;
            } else {
                FuncValue->Val->FuncDef.ParamType[ParamCount] = ParamType;
                FuncValue->Val->FuncDef.ParamName[ParamCount] = ParamIdentifier;
            }
        }

        Token = LexGetToken(&ParamParser, NULL, true);
        if (Token != TokenComma && ParamCount < FuncValue->Val->FuncDef.NumParams-1)
            ProgramFail(&ParamParser, "comma expected");
    }

    if (FuncValue->Val->FuncDef.NumParams != 0 && Token != TokenCloseParen &&
            Token != TokenComma && Token != TokenEllipsis && !this_type)
        ProgramFail(&ParamParser, "bad parameter");

    if (strcmp(Identifier, "main") == 0) {
        /* make sure it's int main() */
        if (FuncValue->Val->FuncDef.ReturnType != &pc->IntType &&
             FuncValue->Val->FuncDef.ReturnType != &pc->VoidType)
            ProgramFail(Parser, "main() should return an int or void");

        if (FuncValue->Val->FuncDef.NumParams != 0 &&
             (FuncValue->Val->FuncDef.NumParams != 2 ||
                FuncValue->Val->FuncDef.ParamType[0] != &pc->IntType))
            ProgramFail(Parser, "bad parameters to main()");
    }

    /* look for a function body */
    Token = LexGetToken(Parser, NULL, false);
    if (Token == TokenSemicolon) {
        LexGetToken(Parser, NULL, true);  /* prototype - absorb semicolon */
    } else {
        /* full function definition with a body */
        if (Token != TokenLeftBrace)
            ProgramFail(Parser, "bad function definition");

        ParserCopy(&FuncBody, Parser);
        if (ParseStatementMaybeRun(Parser, false, true) != ParseResultOk)
            ProgramFail(Parser, "function definition expected");

        FuncValue->Val->FuncDef.Body = FuncBody;
        FuncValue->Val->FuncDef.Body.Pos = LexCopyTokens(&FuncBody, Parser);

        /* check if function already in global table */
        ShowX(">Search: TableGet", "GlobalTable", Identifier, 0);
        if (TableGet(&pc->GlobalTable, Identifier, &OldFuncValue, NULL, NULL, NULL)) {
            if (OldFuncValue->Val->FuncDef.Body.Pos == NULL) {
                /* override an old function prototype */
                VariableFree(pc, TableDelete(pc, &pc->GlobalTable, Identifier));
            } else {
                ProgramFail(Parser, "'%s' is already defined", Identifier);
            }
        }
    }

    if (!TableSet(pc, &pc->GlobalTable, Identifier, FuncValue,
                (char*)Parser->FileName, Parser->Line, Parser->CharacterPos))
        ProgramFail(Parser, "'%s' is already defined", Identifier);

    return FuncValue;
}

Value *ParseMemberFunctionDefinition(ParseState *Parser, 
    ValueType *StructType, ValueType *ReturnType, char *function_name)
{
    const char* type_name = StructType->Identifier;
    char buffer[256];
    char* mangle_name = buffer;
    
    /* Create mangled name: Foo.hello */
    snprintf(mangle_name, sizeof(buffer), "%s.%s", type_name, function_name);
    
    /* Register the mangled name */
    char *RegisteredName = TableStrRegister(Parser->pc, mangle_name, strlen(mangle_name));
    
#ifdef MAGIC
    printf("MAGIC: TableStrRegister: Defining member function %s as global %s\n", 
           function_name, RegisteredName);
#endif
    
    /* This will store it in GlobalTable */
    return ParseFunctionDefinition(Parser, ReturnType, RegisteredName, StructType);
}
