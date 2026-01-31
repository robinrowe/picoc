/* parse_macro.h - Macro and preprocessor parsing interface */
#ifndef PARSE_MACRO_H
#define PARSE_MACRO_H

#include "parse.h"

/* Macro parsing */
void ParseMacroDefinition(ParseState *Parser);
void ParseIncludeStatement(ParseState *Parser, Value **LexerValue);

#endif /* PARSE_MACRO_H */
