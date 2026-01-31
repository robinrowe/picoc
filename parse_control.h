/* parse_control.h - Control flow parsing interface */
#ifndef PARSE_CONTROL_H
#define PARSE_CONTROL_H

#include "parse.h"

/* Control flow parsing */
enum RunMode ParseBlock(ParseState *Parser, int AbsorbOpenBrace, int Condition);
void ParseFor(ParseState *Parser);
void ParseIfStatement(ParseState *Parser);
void ParseWhileStatement(ParseState *Parser);
void ParseDoWhileStatement(ParseState *Parser);
void ParseSwitchStatement(ParseState *Parser);
void ParseCaseStatement(ParseState *Parser);
void ParseDefaultStatement(ParseState *Parser);
void ParseReturnStatement(ParseState *Parser, Value **CValue);
void ParseGotoStatement(ParseState *Parser, Value **LexerValue);
void ParseDeleteStatement(ParseState *Parser, Value **LexerValue, Value **CValue);

#endif /* PARSE_CONTROL_H */
