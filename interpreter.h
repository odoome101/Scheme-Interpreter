#include <stdlib.h>
#include "value.h"

#ifndef INTERPRETER_H
#define INTERPRETER_H

struct Frame {
    Value *bindings;
    struct Frame *parent;
};
typedef struct Frame Frame;

/*
 * This function takes a list of S-expressions and call eval on 
 * each S-expression in the top-level environment and prints each
 * result 
 */
void interpret(Value *tree);

/*
 * The function takes a parse tree of a single S-expression and 
 * an environment frame in which to evaluate the expression and 
 * returns a pointer to a Value representating the value.
 */
Value *eval(Value *expr, Frame *frame);

#endif
