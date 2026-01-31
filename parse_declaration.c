/* parse_declaration.c - Variable and type declaration parsing */
#include "parse_declaration.h"
#include "interpreter.h"
#include "variable.h"
#include "table.h"
#include "lex.h"
#include "type.h"
#include "parse_function.h"

/* declare a variable or function */
int ParseDeclaration(ParseState *Parser, enum LexToken Token)
{
    int IsStatic = false;
    int FirstVisit = false;
    char *Identifier;
    ValueType *BasicType;
    ValueType *Typ;
    Value *NewVariable = NULL;
    Engine *pc = Parser->pc;

    TypeParseFront(Parser, &BasicType, &IsStatic);
    do {
        TypeParseIdentPart(Parser, BasicType, &Typ, &Identifier);
        if ((Token != TokenVoidType && Token != TokenStructType &&
                Token != TokenUnionType && Token != TokenEnumType) &&
                Identifier == pc->StrEmpty)
            ProgramFail(Parser, "identifier expected");

        if (Identifier != pc->StrEmpty) {
            /* handle function definitions */
            if (LexGetToken(Parser, NULL, false) == TokenOpenParen) {
                ValueType *type_unknown = 0;
                ParseFunctionDefinition(Parser, Typ, Identifier, type_unknown);
                return false;
            } else {
                if (Typ == &pc->VoidType && Identifier != pc->StrEmpty)
                    ProgramFail(Parser, "can't define a void variable");

                if (Parser->Mode == RunModeRun || Parser->Mode == RunModeGoto) {
                    NewVariable = VariableDefineButIgnoreIdentical(Parser,
                        Identifier, Typ, IsStatic, &FirstVisit);
                    ShowX("ParseDeclaration", "NewVariable", Identifier, 0);
                }
#if 1
                if (Parser->Mode == RunModeSkip && Typ->Base == TypeStruct &&
                    Typ->Identifier != NULL && Identifier != pc->StrEmpty) {
                    /* Store strings mapping: variable name -> type name */
                    StoreVarType(pc, Identifier, Typ->Identifier);
                }
#endif
                if (LexGetToken(Parser, NULL, false) == TokenAssign) {
                    /* we're assigning an initial value */
                    LexGetToken(Parser, NULL, true);
                    ParseDeclarationAssignment(Parser, NewVariable,
                        !IsStatic || FirstVisit);
                }
            }
        }

        Token = LexGetToken(Parser, NULL, false);
        if (Token == TokenComma)
            LexGetToken(Parser, NULL, true);
    } while (Token == TokenComma);

    return true;
}

/* parse a typedef declaration */
void ParseTypedef(ParseState *Parser)
{
    char *TypeName;
    ValueType *Typ;
    ValueType **TypPtr;
    Value InitValue;

    TypeParse(Parser, &Typ, &TypeName, NULL);

    if (Parser->Mode == RunModeRun) {
        TypPtr = &Typ;
        InitValue.Typ = &Parser->pc->TypeType;
        InitValue.Val = (union AnyValue*)TypPtr;
        VariableDefine(Parser->pc, Parser, TypeName, &InitValue, NULL, false);
    }
}

/* assign an initial value to a variable */
void ParseDeclarationAssignment(ParseState *Parser,
    Value *NewVariable, int DoAssignment)
{
    Value *CValue = 0;

    if (LexGetToken(Parser, NULL, false) == TokenLeftBrace) {
        /* this is an array initializer */
        LexGetToken(Parser, NULL, true);
        ParseArrayInitializer(Parser, NewVariable, DoAssignment);
    } else {
        /* this is a normal expression initializer */
        if (!ExpressionParse(Parser, &CValue))
            ProgramFail(Parser, "expression expected");

        if (Parser->Mode == RunModeRun && DoAssignment) {
            ExpressionAssign(Parser, NewVariable, CValue, false, NULL, 0, false);
            VariableStackPop(Parser, CValue);
        }
    }
}

/* parse an array initializer and assign to a variable */
int ParseArrayInitializer(ParseState *Parser, Value *NewVariable,
    int DoAssignment)
{
    int ArrayIndex = 0;
    enum LexToken Token;
    Value *CValue = 0;

    /* count the number of elements in the array */
    if (DoAssignment && Parser->Mode == RunModeRun) {
        ParseState CountParser;
        int NumElements;

        ParserCopy(&CountParser, Parser);
        NumElements = ParseArrayInitializer(&CountParser, NewVariable, false);

        if (NewVariable->Typ->Base != TypeArray)
            AssignFail(Parser, "%t from array initializer", NewVariable->Typ,
                NULL, 0, 0, NULL, 0);

        if (NewVariable->Typ->ArraySize == 0) {
            NewVariable->Typ = TypeGetMatching(Parser->pc, Parser,
                NewVariable->Typ->FromType, NewVariable->Typ->Base, NumElements,
                NewVariable->Typ->Identifier, true);
            VariableRealloc(Parser, NewVariable, TypeSizeValue(NewVariable, false));
        }
#ifdef DEBUG_ARRAY_INITIALIZER
        PRINT_SOURCE_POS();
        printf("array size: %d \n", NewVariable->Typ->ArraySize);
#endif
    }

    /* parse the array initializer */
    Token = LexGetToken(Parser, NULL, false);
    while (Token != TokenRightBrace) {
        if (LexGetToken(Parser, NULL, false) == TokenLeftBrace) {
            /* this is a sub-array initializer */
            int SubArraySize = 0;
            Value *SubArray = NewVariable;
            if (Parser->Mode == RunModeRun && DoAssignment) {
                SubArraySize = TypeSize(NewVariable->Typ->FromType,
                    NewVariable->Typ->FromType->ArraySize, true);
                SubArray = VariableAllocValueFromExistingData(Parser,
                    NewVariable->Typ->FromType,
                    (union AnyValue*)(&NewVariable->Val->ArrayMem[0] +
                        SubArraySize*ArrayIndex),
                    true, NewVariable);
#ifdef DEBUG_ARRAY_INITIALIZER
                int FullArraySize = TypeSize(NewVariable->Typ,
                    NewVariable->Typ->ArraySize, true);
                PRINT_SOURCE_POS();
                PRINT_TYPE(NewVariable->Typ)
                printf("[%d] subarray size: %d (full: %d,%d) \n", ArrayIndex,
                    SubArraySize, FullArraySize, NewVariable->Typ->ArraySize);
#endif
                if (ArrayIndex >= NewVariable->Typ->ArraySize)
                    ProgramFail(Parser, "too many array elements");
            }
            LexGetToken(Parser, NULL, true);
            ParseArrayInitializer(Parser, SubArray, DoAssignment);
        } else {
            Value *ArrayElement = NULL;

            if (Parser->Mode == RunModeRun && DoAssignment) {
                ValueType *ElementType = NewVariable->Typ;
                int TotalSize = 1;
                int ElementSize = 0;

                /* int x[3][3] = {1,2,3,4} => handle it
                    just like int x[9] = {1,2,3,4} */
                while (ElementType->Base == TypeArray) {
                    TotalSize *= ElementType->ArraySize;
                    ElementType = ElementType->FromType;

                    /* char x[10][10] = {"abc", "def"} => assign "abc" to
                        x[0], "def" to x[1] etc */
                    if (LexGetToken(Parser, NULL, false) == TokenStringConstant &&
                            ElementType->FromType->Base == TypeChar)
                        break;
                }
                ElementSize = TypeSize(ElementType, ElementType->ArraySize, true);
#ifdef DEBUG_ARRAY_INITIALIZER
                PRINT_SOURCE_POS();
                printf("[%d/%d] element size: %d (x%d) \n", ArrayIndex, TotalSize,
                    ElementSize, ElementType->ArraySize);
#endif
                if (ArrayIndex >= TotalSize)
                    ProgramFail(Parser, "too many array elements");
                ArrayElement = VariableAllocValueFromExistingData(Parser,
                    ElementType,
                    (union AnyValue*)(&NewVariable->Val->ArrayMem[0] +
                        ElementSize*ArrayIndex),
                    true, NewVariable);
            }

            /* this is a normal expression initializer */
            if (!ExpressionParse(Parser, &CValue))
                ProgramFail(Parser, "expression expected");

            if (Parser->Mode == RunModeRun && DoAssignment) {
                ExpressionAssign(Parser, ArrayElement, CValue, false, NULL, 0,
                    false);
                VariableStackPop(Parser, CValue);
                VariableStackPop(Parser, ArrayElement);
            }
        }

        ArrayIndex++;

        Token = LexGetToken(Parser, NULL, false);
        if (Token == TokenComma) {
            LexGetToken(Parser, NULL, true);
            Token = LexGetToken(Parser, NULL, false);
        } else if (Token != TokenRightBrace)
            ProgramFail(Parser, "comma expected");
    }

    if (Token == TokenRightBrace)
        LexGetToken(Parser, NULL, true);
    else
        ProgramFail(Parser, "'}' expected");

    return ArrayIndex;
}
