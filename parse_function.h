/* parse_function.h - Function parsing interface */
#ifndef PARSE_FUNCTION_H
#define PARSE_FUNCTION_H

#include "parse.h"

/* Function parsing */
Value *ParseFunctionDefinition(ParseState *Parser, ValueType *ReturnType,
    char *Identifier, ValueType *this_type);
Value *ParseMemberFunctionDefinition(ParseState *Parser, ValueType *StructType,
    ValueType *ReturnType, char *function_name);
int ParseCountParams(ParseState *Parser);

#endif /* PARSE_FUNCTION_H */
