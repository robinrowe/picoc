/* expression_coerce.h - Type coercion functions interface */
#ifndef EXPRESSION_COERCE_H
#define EXPRESSION_COERCE_H

#include "expression.h"

/* Type coercion functions */
intptr_t ExpressionCoerceInteger(Value *Val);
uintptr_t ExpressionCoerceUnsignedInteger(Value *Val);
double ExpressionCoerceFP(Value *Val);

#endif /* EXPRESSION_COERCE_H */
