#include "simple_parser.h"

#define NODE_NUM 0
#define NODE_VAR 1
#define NODE_NEG 2
#define NODE_BIN 3
#define NODE_FUNC 4

#define BINOP_INV 0
#define BINOP_ADD 1
#define BINOP_SUB 2
#define BINOP_MUL 3
#define BINOP_DIV 4
#define BINOP_POW 5

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define sp_err(e, msg) if(e != NULL) *e = msg

struct node {
    int type;
    int btype;
    float value;
    struct var *var;
    float (*func)(struct node**);
    struct node **operands;
    int operands_size;
};

struct expression {
    char *string;
    struct node *root;
};

struct constant{
    char *name;
    int length;
    float value;
};

static const struct constant const_array[] = {
    { "E",        1, 2.7182818284590452354  },
    { "LOG2E",    5, 1.4426950408889634074  },
    { "LOG10E",   6, 0.43429448190325182765 },
    { "LN2",      3, 0.69314718055994530942 },
    { "LN10",     4, 2.30258509299404568402 },
    { "PI",       2, 3.14159265358979323846 },
    { "PI_2",     4, 1.57079632679489661923 },
    { "PI_4",     4, 0.78539816339744830962 },
    { "SQRT2",    5, 1.41421356237309504880 },
    { "SQRT1_2",  7, 0.70710678118654752440 }
};

static int const_size = sizeof(const_array) / sizeof(struct constant);

static float func_ceil(struct node **argv){
    return ceil(argv[0]->value);
}

static float func_floor(struct node **argv){
    return floor(argv[0]->value);
}

static float func_log10(struct node **argv){
    return log10(argv[0]->value);
}

static float func_tanh(struct node **argv){
    return tanh(argv[0]->value);
}

static float func_sinh(struct node **argv){
    return sinh(argv[0]->value);
}

static float func_cosh(struct node **argv){
    return cosh(argv[0]->value);
}

static float func_atan(struct node **argv){
    return atan(argv[0]->value);
}

static float func_acos(struct node **argv){
    return acos(argv[0]->value);
}

static float func_asin(struct node **argv){
    return asin(argv[0]->value);
}

static float func_cos(struct node **argv){
    return cos(argv[0]->value);
}

static float func_sin(struct node **argv){
    return sin(argv[0]->value);
}

static float func_tan(struct node **argv){
    return tan(argv[0]->value);
}

static float func_log(struct node **argv){
    return log(argv[0]->value);
}

static float func_sqrt(struct node **argv){
    return sqrt(argv[0]->value);
}

static float func_abs(struct node **argv){
    return fabs(argv[0]->value);
}

static float func_max(struct node **argv){
    float v0 = argv[0]->value;
    float v1 = argv[1]->value;
    return v0 > v1 ? v0: v1;
}

static float func_min(struct node **argv){
    float v0 = argv[0]->value;
    float v1 = argv[1]->value;
    return v0 < v1 ? v0: v1;
}

static float func_fract(struct node **argv){
    return argv[0]->value - floor(argv[0]->value);
}

static float func_round(struct node **argv){
    return nearbyintf(argv[0]->value);
}

static float func_sign(struct node **argv){
    return (argv[0]->value >= 0.0f) ? 1.0f: -1.0f;
}

static float func_degrees(struct node **argv){
    return argv[0]->value/M_PI*180.0f;
}

static float func_radians(struct node **argv){
    return argv[0]->value/180.0f*M_PI;
}

static float func_lerp(struct node **argv){
    float v0 = argv[0]->value;
    float v1 = argv[1]->value;
    float alpha = argv[2]->value;
    return v0 + alpha*(v1-v0);
}

static float func_random(struct node **argv){
    return (float)rand() / (float)RAND_MAX;
}

static float func_clamp(struct node **argv){
    float value = argv[0]->value;
    float min = argv[1]->value;
    float max = argv[2]->value;
    return (value < min) ? min: (value > max ? max: value);
}

static float func_step(struct node **argv){
    float edge = argv[0]->value;
    float value = argv[1]->value;
    return value < edge ? 0.0f: 1.0f;
}

static float func_mod(struct node **argv){
    float x = argv[0]->value;
    float y = argv[1]->value;
    return x - y*floor(x/y);
}

struct function{
    char *name;
    int length;
    int argc;
    float (*func)(struct node**);
};

static const struct function functions[] = {
    { "COS",    3, 1, func_cos     },
    { "SIN",    3, 1, func_sin     },
    { "TAN",    3, 1, func_tan     },
    { "LOG",    3, 1, func_log     },
    { "SQRT",   4, 1, func_sqrt    },
    { "ABS",    3, 1, func_abs     },
    { "ASIN",   4, 1, func_asin    },
    { "ACOS",   4, 1, func_acos    },
    { "ATAN",   4, 1, func_atan    },
    { "LOG10",  5, 1, func_log10   },
    { "FLOOR",  5, 1, func_floor   },
    { "CEIL",   4, 1, func_ceil    },
    { "SINH",   4, 1, func_sinh    },
    { "TANH",   4, 1, func_tanh    },
    { "COSH",   4, 1, func_cosh    },
    { "MAX",    3, 2, func_max     },
    { "MIN",    3, 2, func_min     },
    { "FRACT",  5, 1, func_fract   },
    { "ROUND",  5, 1, func_round   },
    { "SIGN",   4, 1, func_sign    },
    { "DEG",    3, 1, func_degrees },
    { "RAD",    3, 1, func_radians },
    { "LERP",   4, 3, func_lerp    },
    { "RANDOM", 6, 0, func_random  },
    { "CLAMP",  5, 3, func_clamp   },
    { "STEP",   4, 2, func_step    },
    { "MOD",    3, 2, func_mod     }
};

static int func_size = sizeof(functions) / sizeof(struct function);

static void free_node(struct node *node){
    if(node == NULL){
        return;
    }
    if(node->type == NODE_NUM || node->type == NODE_VAR){
        free(node);
    }else if(node->type == NODE_BIN){
        if(node->operands != NULL){
            if(node->operands[0] != NULL){
                free_node(node->operands[0]);
            }
            if(node->operands[1] != NULL){
                free_node(node->operands[1]);
            }
            free(node->operands);
        }
        free(node);
    }else if(node->type == NODE_NEG){
        if(node->operands != NULL && node->operands[0] != NULL){
            free_node(node->operands[0]);
            free(node->operands);
        }
        free(node);
    }else if(node->type == NODE_FUNC){
        if(node->operands != NULL){
            int i;
            for(i = 0; i < node->operands_size; i++){
                if(node->operands[i] != NULL){
                    free_node(node->operands[i]);
                }
            }
            free(node->operands);
        }
        free(node);
    }
}

void free_exp(struct expression *exp){
    if(exp != NULL){
        if(exp->string != NULL){
            free(exp->string);
        }
        if(exp->root != NULL){
            free_node(exp->root);
        }
        free(exp);
    }
}

static char* clear_string(const char *input, int *length){
    int count = 0;
    const char *in = input;
    char c, space = ' ';
    while((c = *in++) != '\0'){
        if(c != space)
            count++;
    }
    char *output = (char*)malloc((count+1)*sizeof(char));
    if(output == NULL){
        return NULL;
    }
    *length = count;
    output[count] = '\0';
    if(count > 0){
        in = input;
        char *out = output;
        while((c = *in++) != '\0'){
            if(c != space)
                *out++ = toupper(c);
        }
    }
    return output;
}

static int check_parenthesis(const char *input){
    int pcount = 0;
    char c;
    while((c = *input++) != '\0'){
        if(c == '('){
            pcount++;
        }else if(c == ')'){
            pcount--;
        }
        if(pcount < 0)
            break;
    }
    return pcount;
}

static char* clear_parenthesis(char *input, int *length){
    int p = 0;
    while(input[p] == '(' && input[*length-1-p] == ')'){
        p++;
    }
    int end = *length-1-p;
    int i, pcount = 0;
    for(i = p; i <= end && p > 0; i++){
        if(input[i] == '('){
            pcount++;
        }else if(input[i] == ')'){
            if(pcount == 0){
                p--;
            }else{
                pcount--;
            }
        }
    }
    *length = *length - 2*p;
    return input+p;
}

static int is_number(char *string, int length){
    char c = string[0];
    int dot_count = 0;
    if(!isdigit(c)){
        if(length == 1 || (c != '+' && c != '-' && c != '.')){
            return 0;
        }
        if(c == '.')
            dot_count = 1;
    }
    int i;
    for(i = 1; i < length; i++){
        c = string[i];
        if(c == '.'){
            dot_count++;
            if(dot_count > 1)
                return 0;
        }else if(!isdigit(c)){
            return 0;
        }
    }
    return 1;
}

static float parse_number(char *string){
    float val, power;
    int i = 0, sign;
    sign = (string[i] == '-') ? -1 : 1;
    if (string[i] == '+' || string[i] == '-')
        i++;
    for (val = 0.0f; isdigit(string[i]); i++)
        val = 10.0f * val + (string[i] - '0');
    if (string[i] == '.')
        i++;
    for (power = 1.0f; isdigit(string[i]); i++) {
        val = 10.0f * val + (string[i] - '0');
        power *= 10.0f;
    }
    return sign * val / power;
}

static const struct constant* is_constant(char *string, int length){
    const struct constant *c = NULL;
    int i;
    for(i = 0; i < const_size; i++){
        if(const_array[i].length == length && !strncmp(string, const_array[i].name, length)){
            c = &(const_array[i]);
            break;
        }
    }
    return c;
}

static int is_valid_varname(char *name, int length){
    char c = name[0];
    if(!isalpha(c) && c != '_'){
        return 0;
    }
    int i;
    for(i = 1; i < length; i++){
        if(!isalnum(name[i]) && name[i] != '_'){
            return 0;
        }
    }
    return 1;
}

static int check_vars(struct var *vars, int var_size, char **error){
    if(vars == NULL || var_size == 0){
        return 1;
    }
    int i;
    for(i = 0; i < var_size; i++){
        struct var *v = &(vars[i]);
        if(v->name == NULL){
            sp_err(error, "Invalid variable name: String is null");
            return 0;
        }
        if(v->name[0] == '\0'){
            sp_err(error, "Invalid variable name: String is empty");
            return 0;
        }
        char *name = v->name, c = v->name[0];
        if(!isalpha(c) && c != '_'){
            sp_err(error, "Invalid variable name: First character is not a letter or an underscore");
            return 0;
        }
        name++;
        while((c = *name++) != '\0'){
            if(!isalnum(c) && c != '_'){
                sp_err(error, "Invalid variable name: String contains invalid characters");
                return 0;
            }
        }
    }
    return 1;
}

static int strcmp_case(char *str1, char *str2, int length) {
    int i;
    for (i = 0; i < length; i++){
        char c1 = isupper(str1[i]) ? str1[i]: toupper(str1[i]);
        char c2 = isupper(str2[i]) ? str2[i]: toupper(str2[i]);
        if(c1 != c2){
            return c1 - c2;
        }
    }
    return 0;
}

static struct var* is_variable(char *string, int length, struct var *vars, int var_size){
    if(vars == NULL || var_size == 0){
        return NULL;
    }
    struct var *v = NULL;
    int i;
    for(i = 0; i < var_size; i++){
        int var_length = strlen(vars[i].name);
        if(var_length == length && !strcmp_case(vars[i].name, string, var_length)){
            v = &(vars[i]);
            break;
        }
    }
    return v;
}

static int is_binary(char *string, int length, int *bindex){
    int op = BINOP_INV, op_index = -1;
    char c, p = '\0';
    int i, pcount = 0;
    char str0 = string[0];
    for(i = 0; i < length; i++){
        c = string[i];
        if(c == '('){
            pcount++;
        }else if(c == ')'){
            pcount--;
        }
        if(pcount > 0)
            continue;
        if(c == '+'){
            if(p != '*' && p != '/' && 
               p != '+' && p != '-' && 
               p != '^' && p != '\0'){
                op = BINOP_ADD;
                op_index = i;
            }
        }else if(c == '-'){
            if(p != '*' && p != '/' &&
               p != '+' && p != '-' &&
               p != '^' && p != '\0'){
                op = BINOP_SUB;
                op_index = i;
            }
        }else if(c == '*'){
            if(op == BINOP_INV || op == BINOP_MUL || 
               op == BINOP_DIV || op == BINOP_POW){
                op = BINOP_MUL;
                op_index = i;
            }
        }else if(c == '/'){
            if(op == BINOP_INV || op == BINOP_MUL ||
               op == BINOP_DIV || op == BINOP_POW){
                op = BINOP_DIV;
                op_index = i;
            }
        }else if(c == '^'){
            if(op == BINOP_INV && (str0 != '-' && str0 != '+')){
                op = BINOP_POW;
                op_index = i;
            }
        }
        p = c;
    }
    *bindex = op_index;
    return op;
}

static const struct function* is_function(char *string, int length){
    const struct function *func = NULL;
    int i;
    for(i = 0; i < func_size; i++){
        const struct function *f = &(functions[i]);
        if(length >= (f->length+2) && !strncmp(f->name, string, f->length)
            && string[f->length] == '(' && string[length-1] == ')'){
            func = f;
            break;
        }
    }
    return func;
}

static struct node* parse_rec(char *string, int length, struct var *vars, int var_size, char **error){
    string = clear_parenthesis(string, &length);
    if(length == 0){
        sp_err(error, "Invalid expression");
        return NULL;
    }
    if(is_number(string, length)){
        struct node *num_node = (struct node*)malloc(sizeof(struct node));
        if(num_node == NULL){
            sp_err(error, "Out of memory");
            return NULL;
        }
        num_node->type = NODE_NUM;
        num_node->value = parse_number(string);
        num_node->operands = NULL;
        num_node->operands_size = 0;
        return num_node;
    }
    const struct constant *c = is_constant(string, length);
    if(c != NULL){
        struct node *num_node = (struct node*)malloc(sizeof(struct node));
        if(num_node == NULL){
            sp_err(error, "Out of memory");
            return NULL;
        }
        num_node->type = NODE_NUM;
        num_node->value = c->value;
        num_node->operands = NULL;
        num_node->operands_size = 0;
        return num_node;
    }
    struct var *v = is_variable(string, length, vars, var_size);
    if(v != NULL){
        struct node *var_node = (struct node*)malloc(sizeof(struct node));
        if(var_node == NULL){
            sp_err(error, "Out of memory");
            return NULL;
        }
        var_node->type = NODE_VAR;
        var_node->var = v;
        var_node->operands = NULL;
        var_node->operands_size = 0;
        return var_node;
    }else if(is_valid_varname(string, length)){
        sp_err(error, "Unknown variable on expression");
        return NULL;
    }
    int btype, bindex;
    btype = is_binary(string, length, &bindex);
    if(btype != BINOP_INV){
        int length_op1 = bindex;
        int length_op2 = length-(length_op1+1);
        if(!length_op1 || !length_op2){
            sp_err(error, "Empty operand on binary operation");
            return NULL;
        }
        char *str_op1 = string;
        char *str_op2 = string+(bindex+1);
        struct node *op1 = parse_rec(str_op1, length_op1, vars, var_size, error);
        if(op1 == NULL){
            return NULL;
        }
        struct node *op2 = parse_rec(str_op2, length_op2, vars, var_size, error);
        if(op2 == NULL){
            free_node(op1);
            return NULL;
        }
        struct node *bin_node = (struct node*)malloc(sizeof(struct node));
        if(bin_node == NULL){
            sp_err(error, "Out of memory");
            free_node(op1);
            free_node(op2);
            return NULL;
        }
        bin_node->type = NODE_BIN;
        bin_node->btype = btype;
        bin_node->operands = (struct node**)malloc(2*sizeof(struct node*));
        if(bin_node->operands == NULL){
            sp_err(error, "Out of memory");
            free_node(op1);
            free_node(op2);
            free(bin_node);
            return NULL;
        }
        bin_node->operands_size = 2;
        bin_node->operands[0] = op1;
        bin_node->operands[1] = op2;
        return bin_node;
    }
    const struct function *func = is_function(string, length);
    if(func != NULL){
        char *str_arg = string + (func->length+1);
        int length_arg = length - (func->length+2), argc = 0;
        if(length_arg > 0 ){
            if(str_arg[0] == ',' || str_arg[length_arg-1] == ','){
                sp_err(error, "Empty argument to function");
                return NULL;
            }
            int i, comma = 0, pcount = 0;
            char c, p = '\0';
            for(i = 0; i < length_arg; i++){
                c = str_arg[i];
                if(c == '('){
                    pcount++;
                }else if(c == ')'){
                    pcount--;
                }else if(c == ',' && pcount == 0){
                    if(p != ','){
                        comma++;
                    }else{
                        sp_err(error, "Empty argument to function");
                        return NULL;
                    }
                }
                p = c;
            }
            argc = comma+1;
        }
        if(argc < func->argc){
            sp_err(error, "Too few arguments to function");
            return NULL;
        }else if(argc > func->argc){
            sp_err(error, "Too many arguments to function");
            return NULL;
        }
        struct node *func_node = (struct node*)malloc(sizeof(struct node));
        if(func_node == NULL){
            sp_err(error, "Out of memory");
            return NULL;
        }
        func_node->type = NODE_FUNC;
        func_node->func = func->func;
        func_node->operands = NULL;
        func_node->operands_size = func->argc;
        if(func->argc > 0){
            func_node->operands = (struct node**)malloc(func->argc*sizeof(struct node*));
            if(func_node->operands == NULL){
                sp_err(error, "Out of memory");
                free(func_node);
                return NULL;
            }
            int i;
            for(i = 0; i < func_node->operands_size; i++){
                func_node->operands[i] = NULL;
            }
            int a = 0, s = 0, pcount = 0;
            for(i = 0; i < length_arg && a < func->argc-1; i++){
                if(str_arg[i] == '('){
                    pcount++;
                }else if(str_arg[i] == ')'){
                    pcount--;
                }else if(str_arg[i] == ',' && pcount == 0){
                    struct node *op = parse_rec(str_arg+s, i-s, vars, var_size, error);
                    if(op == NULL){
                        free_node(func_node);
                        return NULL;
                    }
                    func_node->operands[a++] = op;
                    s = i+1;
                }
            }
            struct node *op = parse_rec(str_arg+s, length_arg-s, vars, var_size, error);
            if(op == NULL){
                free_node(func_node);
                return NULL;
            }
            func_node->operands[a] = op;
        }
        return func_node;
    }
    if(string[0] == '+' || string[0] == '-'){
        int i, minus = 0;
        for(i = 0; i < length && (string[i] == '+' || string[i] == '-'); i++){
            if(string[i] == '-'){
                minus++;
            }
        }
        if(i == length){
            sp_err(error, "Empty operand on unary operation");
            return NULL;
        }
        struct node *op = parse_rec(string+i, length-i, vars, var_size, error);
        if(op == NULL){
            return NULL;
        }
        if((minus & 1) == 0){
            return op;
        }
        struct node *neg_node = (struct node*)malloc(sizeof(struct node));
        if(neg_node == NULL){
            sp_err(error, "Out of memory");
            return NULL;
        }
        neg_node->operands = (struct node**)malloc(sizeof(struct node*));
        if(neg_node->operands == NULL){
            sp_err(error, "Out of memory");
            free_node(op);
            free(neg_node);
            return NULL;
        }
        neg_node->type = NODE_NEG;
        neg_node->operands[0] = op;
        neg_node->operands_size = 1;
        return neg_node;
    }
    sp_err(error, "Invalid expression");
    return NULL;
}

struct expression* parse(char *string, struct var *vars, int var_size, char **error){
    if(string == NULL){
        sp_err(error, "String is null");
        return NULL;
    }
    if(string[0] == '\0'){
        sp_err(error, "String is empty");
        return NULL;
    }
    if(!check_vars(vars, var_size, error)){
        return NULL;
    }
    struct expression *exp = (struct expression*)malloc(sizeof(struct expression));
    if(exp == NULL){
        sp_err(error, "Out of memory");
        return NULL;
    }
    exp->root = NULL;
    int length;
    exp->string = clear_string(string, &length);
    if(exp->string == NULL){
        free_exp(exp);
        sp_err(error, "Out of memory");
        return NULL;
    }
    if(length == 0){
        free_exp(exp);
        sp_err(error, "Empty expression");
        return NULL;
    }
    if(check_parenthesis(exp->string) != 0){
        free_exp(exp);
        sp_err(error, "Invalid use of parenthesis");
        return NULL;
    }
    exp->root = parse_rec(exp->string, length, vars, var_size, error);
    if(exp->root == NULL){
        free_exp(exp);
        exp = NULL;
    }
    return exp;
}

static float eval_rec(struct node *node){
    if(node->type == NODE_VAR){
        node->value = node->var->value;
    }else if(node->type == NODE_BIN){
        float v1 = eval_rec(node->operands[0]);
        float v2 = eval_rec(node->operands[1]);
        if(node->btype == BINOP_ADD){
            node->value = v1+v2;
        }else if(node->btype == BINOP_SUB){
            node->value = v1-v2;
        }else if(node->btype == BINOP_MUL){
            node->value = v1*v2;
        }else if(node->btype == BINOP_DIV){
            node->value = v1/v2;
        }else if(node->btype == BINOP_POW){
            node->value = (float)pow((double)v1,(double)v2);
        }
    }else if(node->type == NODE_NEG){
        node->value = -eval_rec(node->operands[0]);
    }else if(node->type == NODE_FUNC){
        int i;
        for(i = 0; i < node->operands_size; i++){
            eval_rec(node->operands[i]);
        }
        node->value = node->func(node->operands);
    }
    return node->value;
}

float eval(struct expression *exp){
    float value = 0.0f;
    if(exp != NULL){
        value = eval_rec(exp->root);
    }
    return value;
}
