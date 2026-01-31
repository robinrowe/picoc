/* parse_declaration.h - Declaration parsing interface */
#ifndef PARSE_DECLARATION_H
#define PARSE_DECLARATION_H

#include "parse.h"

/* Declaration parsing */
int ParseDeclaration(ParseState *Parser, enum LexToken Token);
void ParseTypedef(ParseState *Parser);
void ParseDeclarationAssignment(ParseState *Parser,
    Value *NewVariable, int DoAssignment);
int ParseArrayInitializer(ParseState *Parser, Value *NewVariable,
    int DoAssignment);

#endif /* PARSE_DECLARATION_H */
