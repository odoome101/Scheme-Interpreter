#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "parser.h"
#include "talloc.h"
#include "linkedlist.h"

/*
 * Check whether the given token is an atom.
 */
bool isAtom(Value *token) {
    int tokenType = token->type;
    return (tokenType == BOOL_TYPE || tokenType == SYMBOL_TYPE ||
           tokenType == INT_TYPE || tokenType == DOUBLE_TYPE ||
           tokenType == STR_TYPE);
}

///*
// * Helper function to parse an input
// */
//Value *addToParseTree(Value *tree, int *depth, Value *token) {
//    Value *newNode = talloc(sizeof(Value));
//    if (!newNode) {
//        printf("Error! Out of memory!");
//        return NULL;
//    }
//    if (isAtom(token)) {
//        newNode = token;
//    } 
////    else if (token->type == OPEN_TYPE) {
////        *depth += 1;
////        newNode->type = CONS_TYPE;
////    } else if (token->type == CLOSE_TYPE) {
////        *depth -= 1;
////    }
//    return cons(newNode, tree);
//}

/* 
 * This function returns a parse tree representing a Scheme
 * program on the input of a linkedlist of tokens from that 
 * program.
 *
 * Returns a list representing the parse tree or NULL if 
 * the parsing fails.
 */
Value *parse(Value *tokens) {
    Value *tree = makeNull();
    Value *stack = makeNull();
    if (!tree || !stack) {
        return NULL;
    }
    
    int depth = 0;
    Value *current = tokens;

    while (current->type != NULL_TYPE) {
        Value *token = car(current);
        if (token->type == OPEN_TYPE) {
            depth ++;
            stack = cons(token, stack);
        } else if (token->type == CLOSE_TYPE) {
            // Pop until reaching (
            if (depth == 0) {
                printf("Error! Unbalanced use of parentheses!\n");
                return NULL;
            }
            depth --;
            // Access top item in the list
            Value *head = car(stack);
            // Create a list of items being popped
            Value *inner = makeNull();
            while (head->type != OPEN_TYPE) {
                inner = cons(head, inner);
                // Pop off the top item
                stack = cdr(stack);
                head = car(stack);
            }
            // Pop off the (
            stack = cdr(stack);
            // Push back on to the stack
            if (!isNull(inner)) {
                stack = cons(inner, stack);
            }
        } else {
            stack = cons(token, stack);
        }
        current = cdr(current);
    }
    if (depth != 0) {
        printf("Error! Unbalanced use of parentheses!\n");
        return NULL;
    }
    return stack;
}

/* 
 * This function displays a parse tree to the screen.
 */
void printTree(Value *tree) {
    assert(tree != NULL &&
           (tree->type == CONS_TYPE || tree->type == NULL_TYPE));
    if (isNull(tree)) {
        printf("()");
        return;
    }
    Value *cur = tree;
    while (cur != NULL && cur->type != NULL_TYPE) {
        if (isAtom(car(cur))) {
        
            switch(car(cur)->type) {
                case BOOL_TYPE:
                    printf("%s ", car(cur)->s);
                    break;
                case SYMBOL_TYPE:
                    if ((strcmp(car(cur)->s, " ") != 0) 
                        && (strcmp(car(cur)->s, "\\n") != 0)
                        && (strcmp(car(cur)->s, ";") != 0)
                        && (strcmp(car(cur)->s, "\\t") != 0)) {
                            printf("%s ", car(cur)->s);
                    }
                    break;
                case INT_TYPE:
                    printf("%d ", car(cur)->i);
                    break;
                case DOUBLE_TYPE:
                    printf("%f ", car(cur)->d);
                    break;
                case STR_TYPE:
                    printf("%s ", car(cur)->s);
                    break;
                default:
                    printf("ERROR\n");
            }
        } else {
            printf("(");
            printTree(car(cur));
            printf(") ");
        }
        cur = cdr(cur);
    }
}