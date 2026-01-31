/* parse_macro.c - Macro and preprocessor directive parsing */
#include "parse_macro.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"

/* parse a #define macro definition and store it for later */
void ParseMacroDefinition(ParseState *Parser)
{
    char *MacroNameStr;
    Value *MacroName;
    Value *ParamName;
    Value *MacroValue;

    if (LexGetToken(Parser, &MacroName, true) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");

    MacroNameStr = MacroName->Val->Identifier;

    if (LexRawPeekToken(Parser) == TokenOpenMacroBracket) {
        /* it's a parameterized macro, read the parameters */
        enum LexToken Token = LexGetToken(Parser, NULL, true);
        ParseState ParamParser;
        int NumParams;
        int ParamCount = 0;

        ParserCopy(&ParamParser, Parser);
        NumParams = ParseCountParams(&ParamParser);
        MacroValue = VariableAllocValueAndData(Parser->pc, Parser,
            sizeof(struct MacroDef) + sizeof(const char*) * NumParams,
            false, NULL, true);
        MacroValue->Val->MacroDef.NumParams = NumParams;
        MacroValue->Val->MacroDef.ParamName = (char**)((char*)MacroValue->Val +
            sizeof(struct MacroDef));

        Token = LexGetToken(Parser, &ParamName, true);

        while (Token == TokenIdentifier) {
            /* store a parameter name */
            MacroValue->Val->MacroDef.ParamName[ParamCount++] =
                ParamName->Val->Identifier;

            /* get the trailing comma */
            Token = LexGetToken(Parser, NULL, true);
            if (Token == TokenComma)
                Token = LexGetToken(Parser, &ParamName, true);
            else if (Token != TokenCloseParen)
                ProgramFail(Parser, "comma expected");
        }

        if (Token != TokenCloseParen)
            ProgramFail(Parser, "close bracket expected");
    } else {
        /* allocate a simple unparameterized macro */
        MacroValue = VariableAllocValueAndData(Parser->pc, Parser,
            sizeof(struct MacroDef), false, NULL, true);
        MacroValue->Val->MacroDef.NumParams = 0;
    }

    /* copy the body of the macro to execute later */
    ParserCopy(&MacroValue->Val->MacroDef.Body, Parser);
    MacroValue->Typ = &Parser->pc->MacroType;
    LexToEndOfMacro(Parser);
    MacroValue->Val->MacroDef.Body.Pos =
        LexCopyTokens(&MacroValue->Val->MacroDef.Body, Parser);

    if (!TableSet(Parser->pc, &Parser->pc->GlobalTable, MacroNameStr, MacroValue,
                (char *)Parser->FileName, Parser->Line, Parser->CharacterPos))
        ProgramFail(Parser, "'%s' is already defined", MacroNameStr);
}

/* parse an #include statement */
void ParseIncludeStatement(ParseState *Parser, Value **LexerValue)
{
    if (LexGetToken(Parser, LexerValue, true) != TokenStringConstant)
        ProgramFail(Parser, "\"filename.h\" expected");
    
    IncludeFile(Parser->pc, (char *)(*LexerValue)->Val->Pointer);
}
