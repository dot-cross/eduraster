# simple-parser

A simple arithmetic parser written in C:
- Works with infix notation.
- Custom variables.
- Built-in constants.
- Built-in functions.

### Operators

#### Binary

| Name |   Description  | Associativity |
|:----:|:---------------|:-------------:|
|  +   | Addition       | Left to right |
|  -   | Substraction   | Left to right |
|  *   | Multiplication | Left to right |
|  /   | Division       | Left to right |
|  ^   | Exponentiation | Right to left |

#### Unary

| Name | Description |
|:----:|:------------|
|  -   | Negation    |
|  +   | Positive    |

#### Precedence

- Binary ^ has precedence over unary - and +, and binary *, /, + and -.
- Unary - and + have precedence over binary  *, /, + and -.
- Binary * and / have precedence over binary + and -.

### Built-in constants

|   Name   |   Description  |        Value           |
|:--------:|----------------|------------------------|
|    E     | Euler's number | 2.7182818284590452354  |
|  LOG2E   | log2(e)        | 1.4426950408889634074  |
|  LOG10E  | log10(e)       | 0.43429448190325182765 |
|   LN2    | ln(2)          | 0.69314718055994530942 |
|   LN10   | ln(10)         | 2.30258509299404568402 |
|    PI    | PI number      | 3.14159265358979323846 |
|   PI_2   | PI/2           | 1.57079632679489661923 |
|   PI_4   | PI/4           | 0.78539816339744830962 |
|  SQRT2   | sqrt(2)        | 1.41421356237309504880 |
| SQRT1_2  | sqrt(1/2)      | 0.70710678118654752440 |

### Built-in functions

|       Name         |               Description               |
|--------------------|-----------------------------------------|
| cos(x)             | Cosine of x                             |
| sin(x)             | Sine of x                               |
| tan(x)             | Tangent of x                            |
| log(x)             | Natural logarithm of x                  |
| sqrt(x)            | Square root of x                        |
| abs(x)             | Absolute value of x                     |
| asin(x)            | Arc sine of x                           |
| acos(x)            | Arc cosine of x                         |
| atan(x)            | Arc tangent of x                        |
| log10(x)           | Logarithm base 10 of x                  |
| floor(x)           | Rounds down x to nearest integer        |
| ceil(x)            | Rounds up x to nearest integer          | 
| sinh(x)            | Hyperbolic sine of x                    |
| cosh(x)            | Hyperbolic cosine of x                  |
| tanh(x)            | Hyperbolix tangent of x                 |
| max(x, y)          | Greatest of x and y                     |
| min(x, y)          | Smallest of x and y                     |
| fract(x)           | Fractional part of x                    |
| round(x)           | Rounds x to nearest integer             |
| sign(x)            | 1 if x is positive, -1 if x is negative |
| deg(x)             | Convert x to degrees                    |
| rad(x)             | Convert x to radians                    |
| lerp(x, y, alpha)  | Linear interpolation x + alpha*(y-x)    |
| random()           | Random number on range [0, 1]           |
| clamp(x, min, max) | Constrains x to lie between min and max |
| step(edge, x)      | 0.0 if x < edge, 1.0 if x > edge        |
| mod(x, y)          | Remainder of x divided by y             |

### API

```C

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

```
### Example

```C
#include <stdio.h>
#include "simple_parser.h"

int main(int argc, char *argv[]){
    struct var x = {"x", 0.5f};
    char *string = "1 - cos(2*pi*x)", *error;
    struct expression *exp = parse(string, &x, 1, &error);
    if(exp != NULL){
        printf("Value=%f\n", eval(exp));
        free_exp(exp);
    }else{
        fprintf(stderr, "Error: %s\n", error);
    }
    return 0;
}

```
