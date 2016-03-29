#ifndef __SIMPLE_PARSER__
#define __SIMPLE_PARSER__

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/*
 * Structure for arithmetic expression.
 */
struct expression;

/*
 * Structure for variables.
 */
struct var {
    char *name;
    float value;
};

/*
 * Parse a string and returns a pointer to an expression structure. Returns NULL on error.
 * Parameters:
 * string: Input string.
 * vars: Array of variables. Pass NULL if the expression does not need variables.
 * var_size: Number of variables. Pass 0 if the expression does not need variables.
 * error: If an error occurred, an error message will be written. Pass NULL if you don't need an error message.
 */
struct expression* parse(char *string, struct var *vars, int var_size, char **error);

/*
 * Evaluates an expression. Returns 0.0 if exp is NULL.
 * Parameters:
 * exp: Pointer to expression structure.
 */
float eval(struct expression *exp);

/*
 * Frees an expression structure.
 * Parameters:
 * exp: Pointer to expression structure.
 */
void free_exp(struct expression *exp);

#endif
