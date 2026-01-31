/* expression_call.h - Function and macro call interface */
#ifndef EXPRESSION_CALL_H
#define EXPRESSION_CALL_H

#include "expression.h"

/* Function and macro calls */
void ExpressionParseFunctionCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *FuncName, int RunIt);
void ExpressionParseMacroCall(ParseState *Parser,
    ExpressionStack **StackTop, const char *MacroName,
    struct MacroDef *MDef);

#endif /* EXPRESSION_CALL_H */
