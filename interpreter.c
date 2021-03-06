/*
 * This program implements the evaluator for Scheme.
 *
 * Authors: Yitong Chen, Yingying Wang, Megan Zhao.
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "linkedlist.h"
#include "interpreter.h"
#include "talloc.h"
#include "tokenizer.h"

/*
 * Print a representation of the contents of a linked list.
 */
void displayEval(Value *list, bool newline){
    assert(list != NULL);
    Value *cur = list;
    while(cur != NULL){
	switch(cur->type){
            case INT_TYPE:
                printf("%i ",cur->i);
                break;
            case DOUBLE_TYPE:
                printf("%f ",cur->d);
                break;
            case STR_TYPE:
                printf("\"");
                printf("%s",cur->s);
                printf("\" ");
                break;
            case SYMBOL_TYPE:
                printf("%s ",cur->s);
            	break;
            case BOOL_TYPE:
                printf("%s ", cur->s);
                break;
	    case CONS_TYPE:
                {if (car(cur)->type == CONS_TYPE) {
                    printf("(");
                    displayEval(car(cur), false); 
                    printf(")");
		    if (cdr(cur)->type != NULL_TYPE &&
			cdr(cur)->type != CONS_TYPE){
			printf(" . ");
		    }
                } else {
                    displayEval(car(cur), false);
		    if (cdr(cur)->type != NULL_TYPE &&
			cdr(cur)->type != CONS_TYPE){
			printf(". ");
		    }		
                }
                }
                break;
            case NULL_TYPE:
                printf("()");
                break;
            case VOID_TYPE: 
                newline = false;
                break;
            case CLOSURE_TYPE:
                printf("#procedure ");
                break;
            default:
                printf(" ");
                break;     
        }
        if (newline) {
            printf("\n");
        }
	if (cur->type == CONS_TYPE && cdr(cur)->type != NULL_TYPE){ 
	   cur = cdr(cur);
	} else{
	   cur = NULL;
	}
    }
}


/* 
 * Helper function for displaying evaluation err message.
 */
void evaluationError(){
    printf("Evaluation error!\n");
    texit(1);
}


/* 
 * Helper function to verify that all formal parameters are 
 * identifiers.
 */
bool verifyFormal(Value *formals) {
    Value *cur = formals;
    if (formals->type == CONS_TYPE) {
	while (cur->type != NULL_TYPE) {
        	if (car(cur)->type != SYMBOL_TYPE) {
            		return false;
        	}
        	cur = cdr(cur);
    	}
    }
    return true;
}


/* 
 * Check whether there are duplicated identifiers in the 
 * formal parameters.
 */
char *containsDuplicate(Value *formals) {
    Value *cur = formals;
    if (cur->type == CONS_TYPE) {
        while (cur->type != NULL_TYPE) {
            Value *next = cdr(cur);
            while (next->type != NULL_TYPE) {
            	if (!strcmp(car(cur)->s, car(next)->s)) {
                    return car(cur)->s;
            	}
            	next = cdr(next);
            }
            cur = cdr(cur);
    	}
    }
    return NULL;
}


/*
 * Helper function to lookup symbols in the given environment.
 */
Value *lookUpSymbol(Value *expr, Frame *frame){
    // Get binding of the frame in which the expression is evaluated
    Frame *curF = frame;
    Value *binding = curF->bindings;
    while (curF != NULL){
       binding = curF->bindings;
       assert(binding != NULL);      
       while (binding->type != NULL_TYPE){
           Value *curBinding = car(binding);
           Value *name = car(curBinding);

           if (name->type == NULL_TYPE) {
               break;
           }
	       Value *value = car(cdr(curBinding));
//	       assert(name->type == SYMBOL_TYPE);
	       if (!strcmp(name->s, expr->s)){
                return value;	      
	       }
           binding = cdr(binding);
       }
    curF = curF->parent;
    }
    printf("The symbol %s is unbounded! ", expr->s);
    evaluationError();
    return NULL;
}

/*
 * Helper function to evaluate the IF special form.
 */
Value *evalIf(Value *args, Frame *frame){
    if (length(args) != 3 && length(args) != 2){
        printf("Number of arguments for 'if' has to be 2 or 3. ");
        evaluationError();
    }
    if (eval(car(args), frame)->type == BOOL_TYPE &&
	!strcmp(eval(car(args), frame)->s, "#f")){
	    if (cdr(cdr(args))->type != NULL_TYPE){
            return eval(car(cdr(cdr(args))), frame);
        } else{
            Value *emptyValue = talloc(sizeof(Value));
            if (!emptyValue){
                printf("Error! Not enough memory!\n");
            }
            emptyValue->type = VOID_TYPE;
            return emptyValue;
        }
    }
    return eval(car(cdr(args)), frame);
}


/* 
 * Helper function to check whether a variable is already bounded in
 * the current frame.
 *
 * Returns current binding if the variable is already bounded;
 *         null if the variable is not bounded.
 */
Value *isBounded(Value *var, Frame *frame) {
    Value *binding = frame->bindings;
    while (binding->type != NULL_TYPE){
       Value *curBinding = car(binding);
	   Value *name = car(curBinding);
	   Value *value = car(cdr(curBinding));
	   assert(name->type == SYMBOL_TYPE);
	   if (!strcmp(name->s, var->s)){
            return curBinding;	      
	   }
       binding = cdr(binding);
	}
    return NULL;
        
}


/* 
 * Helper function to create new let bindings.
 */
void addBindingLocal(Value *var, Value *expr, Frame *frame){
    if (isBounded(var, frame)) {
        printf("Duplicate identifier in local binding. ");
        evaluationError();
    }
    Value *nullTail = makeNull();
    if (!nullTail) {
        texit(1);
    }
    Value *list = cons(expr, nullTail);
    list = cons(var, list);
    Value *bindings = frame->bindings;
    frame->bindings = cons(list, bindings);
}


/* 
 * Helper function to create new define bindings.
 */
void addBindingGlobal(Value *var, Value *expr, Frame *frame){
    Value *curBinding = isBounded(var, frame);
    Value *nullTail = makeNull();
    if (!nullTail) {
        texit(1);
    }
    // Modify existing binding
    if (curBinding) {
        curBinding->c.cdr = cons(expr, nullTail);
    } 
    // Create new binding
    else { 
        Value *list = cons(expr, nullTail);
        list = cons(var, list);
        Value *bindings = frame->bindings;
        frame->bindings = cons(list, bindings);
    }
}


/*
 * Bind the primitive functions in the top-level environment.
 */
void bind(char *name, Value *(*function)(Value *), Frame *frame) {
    Value *value = makeNull();
    if (!value) {
        texit(1);
    }
    Value *nameVar = talloc(sizeof(Value));
    if (!name) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    nameVar->type = SYMBOL_TYPE;
    nameVar->s = name;
    value->type = PRIMITIVE_TYPE;
    value->pf = function;
    addBindingGlobal(nameVar, value, frame);
}


/*
 * Helper function to evaluate the AND special form.
 */
Value *evalAnd(Value *args, Frame *frame){
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    result->type = BOOL_TYPE;
    if (length(args) == 0) {
        result->s = "#t";
        return result;
    }
    Value *body = args;
    assert(body->type == CONS_TYPE);
    while (cdr(body)->type != NULL_TYPE) {
        Value *curValue = eval(car(body), frame);
        if (curValue->type == BOOL_TYPE && (!strcmp(curValue->s, "#f"))) {
            result->s = "#f";
            return result;
        }
        body = cdr(body);
    }
    return eval(car(body),frame);

}

/*
 * Helper function to evaluate the OR special form.
 */
Value *evalOr(Value *args, Frame *frame){
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    result->type = BOOL_TYPE;
    if (length(args) == 0) {
        result->s = "#f";
        return result;
    }
    Value *body = args;
    assert(body->type == CONS_TYPE);
    while (cdr(body)->type != NULL_TYPE) {
        Value *curValue = eval(car(body), frame);
        if (!(curValue->type == BOOL_TYPE && (!strcmp(curValue->s, "#f")))) {
            return curValue;
        }
        body = cdr(body);
    }
    return eval(car(body),frame);
}

/*
 * Helper function to evaluate the BEGIN special form.
 */
Value *evalBegin(Value *args, Frame *frame) {
    assert(args->type == NULL_TYPE || args->type == CONS_TYPE);
    Value *body = args;
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    result->type = VOID_TYPE;
    if (body->type == NULL_TYPE)
	return result;
    while (cdr(body)->type != NULL_TYPE) {
	Value *curValue = eval(car(body),frame);
	body = cdr(body); 
   }
   return eval(car(body),frame); 
}

/*
 * Helper function to evaluate the LET special form by 
 * creating bindings and then evaluate the body.
 */
Value *evalLet(Value *args, Frame *frame){
    Value *cur = car(args);
    if (!isNull(cur) && cur->type != CONS_TYPE) {
        printf("Invalid syntax in 'let'. ");
        evaluationError();
    }
    if (isNull(cdr(args))) {
        printf("Empty body in 'let'. ");
        evaluationError();
    }
    Value *body = cdr(args);
    Frame *frameG = talloc(sizeof(Frame));
    if (!frameG) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    frameG->parent = frame;
    frameG->bindings = makeNull();
    while (cur != NULL && cur->type != NULL_TYPE){
        if (car(cur)->type != CONS_TYPE || length(car(cur)) != 2) {
            printf("Invalid syntax in 'let' bindings. ");
            evaluationError();
        }
	    Value *v = eval(car(cdr(car(cur))), frame);
        if (car(car(cur))->type != SYMBOL_TYPE) {
            printf("Invalid syntax in 'let'. Not a valid identifier! ");
            evaluationError();
        }    
	    addBindingLocal(car(car(cur)), v, frameG);
	    cur = cdr(cur);
    }
    // Evaluate all expressions but 
    // only return the last expression in body
    while (cdr(body)->type != NULL_TYPE){
        eval(car(body), frameG);
        body = cdr(body);
    }
    body = car(body);
    return eval(body, frameG);    
}

/*
 * Helper function to evaluate the LETREC special form by 
 * creating bindings and then evaluate the body.
 */
Value *evalLetrec(Value *args, Frame *frame){
    Value *cur = car(args);
    if (!isNull(cur) && cur->type != CONS_TYPE) {
        printf("Invalid syntax in 'letrec'. ");
        evaluationError();
    }
    if (isNull(cdr(args))) {
        printf("Empty body in 'letrec'. ");
        evaluationError();
    }
    Value *body = cdr(args);
    Frame *frameG = talloc(sizeof(Frame));
    if (!frameG) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    frameG->parent = frame;
    frameG->bindings = makeNull();
    while (cur != NULL && cur->type != NULL_TYPE){
        if (car(cur)->type != CONS_TYPE || length(car(cur)) != 2) {
            printf("Invalid syntax in 'letrec' bindings. ");
            evaluationError();
        }
    	Value *v = eval(car(cdr(car(cur))), frameG);
        if (car(car(cur))->type != SYMBOL_TYPE) {
            printf("Invalid syntax in 'letrec'. Not a valid identifier! ");
            evaluationError();
        }    
	    addBindingLocal(car(car(cur)), v, frameG);
	    cur = cdr(cur);
    }
    // Evaluate all expressions but 
    // only return the last expression in body
    while (cdr(body)->type != NULL_TYPE){
        eval(car(body), frameG);
        body = cdr(body);
    }
    body = car(body);
    return eval(body, frameG);    
}

/*
 * Helper function to evaluate the LET* special form by 
 * creating bindings and then evaluate the body.
 */
Value *evalLetstar(Value *args, Frame *frame){
    Value *cur = car(args);
    if (!isNull(cur) && cur->type != CONS_TYPE) {
        printf("Invalid syntax in 'let*'. ");
        evaluationError();
    }
    if (isNull(cdr(args))) {
        printf("Empty body in 'let*'. ");
        evaluationError();
    }
    Value *body = cdr(args);
    Frame *lastFrame = frame;
    while (cur != NULL && cur->type != NULL_TYPE){   
    	Frame *frameG = talloc(sizeof(Frame));
    	if (!frameG) {
        	printf("Error! Not enough memory!\n");
        	texit(1);
    	}
    	frameG->parent = lastFrame;
    	frameG->bindings = makeNull();
    	if (car(cur)->type != CONS_TYPE || length(car(cur)) != 2) {
            printf("Invalid syntax in 'let*' bindings. ");
            evaluationError();
        }
	    Value *v = eval(car(cdr(car(cur))), frameG);
        if (car(car(cur))->type != SYMBOL_TYPE) {
            printf("Invalid syntax in 'let*'. Not a valid identifier! ");
            evaluationError();
        }    
    	addBindingLocal(car(car(cur)), v, frameG);
    	lastFrame = frameG;
    	cur = cdr(cur);
    }
    // Evaluate all expressions but 
    // only return the last expression in body
    while (cdr(body)->type != NULL_TYPE){
        eval(car(body), lastFrame);
        body = cdr(body);
    }
    body = car(body);
    return eval(body, lastFrame);    
}

/*
 * Helper function to evaluate the COND special form. 
 */
Value *evalCond(Value *args, Frame *frame){
    assert(args->type == CONS_TYPE || args->type == NULL_TYPE);
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    result->type = VOID_TYPE;
    Value *clauses = args;
    while (clauses->type != NULL_TYPE){
        Value *curClause = car(clauses);
        Value *test = car(curClause);
        if (test->type == SYMBOL_TYPE && (!strcmp(test->s, "else"))) {
            if (cdr(clauses)->type == NULL_TYPE) {
                Value *body = cdr(curClause);
                while (cdr(body)->type != NULL_TYPE) {
                    eval(car(body), frame);
                    body = cdr(body);
                }
                return eval(car(body), frame);
            } else {
                printf("Error! 'Else' clause must be last\n");
                evaluationError();
            }
        }
        if (!(eval(test, frame)->type == BOOL_TYPE && 
                (!strcmp(eval(test,frame)->s, "#f")))) {
            Value *body = cdr(curClause);
            while (cdr(body)->type != NULL_TYPE) {
                eval(car(body), frame);
                body = cdr(body);
            }
            return eval(car(body), frame);
        }
        clauses = cdr(clauses);
    }
    return result; 
}


/*
 * Helper function to evaluate the DEFINE special form by 
 * creating bindings and then evaluate the body.
 */
Value *evalDefine(Value *args, Frame *frame){
    if (frame->parent != NULL) {
        printf("'define' expressions only allowed"
               " in the global environment. ");
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    if (car(args)->type != SYMBOL_TYPE) {
        printf("Invalid syntax in 'define'. "
               "First argument must be a symbol. ");
        evaluationError();
    }
    result->type = VOID_TYPE;
    if (length(args) != 2) {
        printf("Invalid syntax in 'define'. Multiple expressions"
               " after identifier! ");
        evaluationError();
    } 
    Value *expr = eval(car(cdr(args)), frame);
    addBindingGlobal(car(args), expr, frame);
    return result;
}

/*
 * Helper function to evaluate the SET! special form by 
 * changing bindings and then evaluate the body.
 */
Value *evalSet(Value *args, Frame *frame){
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    if (car(args)->type != SYMBOL_TYPE) {
        printf("Invalid syntax in 'set!'. "
               "First argument must be a symbol. ");
        evaluationError();
    }
    result->type = VOID_TYPE;
    if (length(args) != 2) {
        printf("Invalid syntax in 'set!'. Multiple expressions"
               " after identifier! ");
        evaluationError();
    } 
    Frame *curFrame = frame;
    while (curFrame != NULL) {
	    if (lookUpSymbol(car(args), curFrame)) {
	        Value *newBindings = makeNull(); 
	        Value *newValue = eval(car(cdr(args)),frame);
   	        Value *oldBindings = curFrame->bindings;
	        assert(oldBindings != NULL);
    	    while (oldBindings->type != NULL_TYPE) {
	    	    Value *curBinding = car(oldBindings);
		        Value *name = car(curBinding);
		        assert(name->type == SYMBOL_TYPE);
	   	        if (!strcmp(name->s, car(args)->s)){
		            curBinding = cons(name, cons(newValue, makeNull()));
	    	    }
		        newBindings = cons(curBinding, newBindings);
       		    oldBindings = cdr(oldBindings);
	        }
	        curFrame->bindings = newBindings;
	        return result;
	    }
	    curFrame = curFrame->parent;
    }
    return result;
}


/* 
 * Helper function to evaluate the LAMBDA special form by
 * creating a Value object of closure type.
 */
Value *evalLambda(Value *args, Frame *frame) {
    if (length(args) < 2) {
        printf("There has to be at least 2 arguments for 'lambda'. ");
        evaluationError();
    }
    Value *closure = talloc(sizeof(Value));
    if (!closure) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    closure->type = CLOSURE_TYPE;
    // All formals should be identifiers
    if (!verifyFormal(car(args))) {
        printf("All formal parameters should be identifiers. ");
        evaluationError();
    }
    // Check whether formal parameters are duplicated
    if (containsDuplicate(car(args))) {
        printf("Duplicated identifiers %s in lambda. ", containsDuplicate(car(args)));
        evaluationError();
    }
    closure->closure.formal = car(args);
    closure->closure.body = cdr(args);
    closure->closure.frame = frame;
    return closure;
}


/*
 * Implementing the Scheme primitive +.
 */
Value *primitiveAdd(Value *args) {
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    result->type = INT_TYPE;
    double result_num = 0;    
    Value *cur_arg = args; 
    while (cur_arg->type != NULL_TYPE) {
        Value *cur_num = car(cur_arg);
        if (cur_num->type != INT_TYPE) {
            if (cur_num->type == DOUBLE_TYPE) {
                result->type = DOUBLE_TYPE;
                result_num += cur_num->d;
            } else {
                printf("Expected numerical arguments for addition. ");
                evaluationError();
            }
        } else {
            result_num += cur_num->i;    
        }
        cur_arg = cdr(cur_arg);
    }
    
    if (result->type == INT_TYPE) {
        result->i = (int) result_num;
    } else {
        result->d = result_num;
    }
    return result;
}


/*
 * Implementing the Scheme primitive *.
 */
Value *primitiveMult(Value *args) {
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    result->type = INT_TYPE;
    double result_num = 1;
    
    Value *cur_arg = args; 
    while (cur_arg->type != NULL_TYPE) {
        Value *cur_num = car(cur_arg);
        if (cur_num->type != INT_TYPE) {
            if (cur_num->type == DOUBLE_TYPE) {
                result->type = DOUBLE_TYPE;
                result_num *= cur_num->d;
            } else {
                printf("Expected numerical arguments for multiplication. ");
                evaluationError();
            }
        } else {
            result_num *= cur_num->i;    
        }
        cur_arg = cdr(cur_arg);
    }
    
    if (result->type == INT_TYPE) {
        result->i = (int) result_num;
    } else {
        result->d = result_num;
    }
    return result;
}


/*
 * Implementing the Scheme primitive -.
 */
Value *primitiveSub(Value *args) {
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    if (length(args) == 0) {
        printf("Arity mismatch. Expected: at least 1. Given: 0. ");
        evaluationError();
    }

    result->type = car(args)->type;
    double result_num;
    
    if (result->type == INT_TYPE) {
        if (length(args) == 1) {
            result->i = 0 - car(args)->i;
            return result;
        } else {
            result_num = car(args)->i;
        } 
    } else if (result->type == DOUBLE_TYPE) {
        if (length(args) == 1) {
            result->d = 0 - car(args)->d;
            return result;
        } else {
            result_num = car(args)->d;
        }
    } else {
        printf("Expected numerical arguments for subtraction. ");
        evaluationError();
    }

    Value *cur_arg = cdr(args); 
    while (cur_arg->type != NULL_TYPE) {
        Value *cur_num = car(cur_arg);
        if (cur_num->type != INT_TYPE) {
            if (cur_num->type == DOUBLE_TYPE) {
                result->type = DOUBLE_TYPE;
                result_num -= cur_num->d;
            } else {
                printf("Expected numerical arguments for subtraction. ");
                evaluationError();
            }
        } else {
            result_num -= cur_num->i;    
        }
        cur_arg = cdr(cur_arg);
    }
    
    if (result->type == INT_TYPE) {
        result->i = (int) result_num;
    } else {
        result->d = result_num;
    }
    return result;
}


/*
 * Implementing the Scheme primitive /.
 */
Value *primitiveDiv(Value *args) {
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    if (length(args) == 0) {
        printf("Arity mismatch. Expected: at least 1. Given: 0. ");
        evaluationError();
    }
    result->type = car(args)->type;
    double result_num;
    
    if (result->type == INT_TYPE) {
        if (length(args) == 1) {
            if (car(args)->i == 0) {
                printf("/: division by 0. ");
                evaluationError();
            }
            result->i = 1 / car(args)->i;
            return result;
        } else {
            result_num = car(args)->i;
        } 
    } else if (result->type == DOUBLE_TYPE) {
        if (length(args) == 1) {
            if (car(args)->d == 0) {
                printf("/: division by 0. ");
                evaluationError();
            }
            result->d = 1 / car(args)->d;
            return result;
        } else {
            result_num = car(args)->d;
        }
    } else {
        printf("Expected numerical arguments for division. ");
        evaluationError();
    }

    Value *cur_arg = cdr(args); 
    while (cur_arg->type != NULL_TYPE) {
        Value *cur_num = car(cur_arg);
        if (cur_num->type != INT_TYPE) {
            if (cur_num->type == DOUBLE_TYPE) {
                result->type = DOUBLE_TYPE;
                if (cur_num->d == 0) {
                    printf("/: division by 0. ");
                    evaluationError();
                }   
                result_num /= cur_num->d;
            } else {
                printf("Expected numerical arguments for subtraction. ");
                evaluationError();
            }
        } else {
            if (cur_num->i == 0) {
                printf("/: division by 0. ");
                evaluationError();
            }
            result_num /= cur_num->i;    
        }
        cur_arg = cdr(cur_arg);
    }

    if (result->type == INT_TYPE && (int) result_num == result_num) {
        result->i = result_num;
    } else {
        result->type = DOUBLE_TYPE;
        result->d = result_num;
    }
    return result;
}


/*
 * Implementing the Scheme primitive null? function.
 */
Value *primitiveIsNull(Value *args) {
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", length(args));
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    result->type = BOOL_TYPE;
    if (isNull(car(args))) {
        result->s = "#t";
    } else {
        result->s = "#f";
    }
    return result;
}


/*
 * Implementing the Scheme primitive car function.
 */
Value *primitiveCar(Value *args) {
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", length(args));
        evaluationError();
    }
    if (car(args)->type != CONS_TYPE) {
        printf("Contract violation. Expected: non-empty list. ");
        evaluationError();
    }
    return car(car(args));
}


/*
 * Implementing the Scheme primitive cdr function.
 */
Value *primitiveCdr(Value *args) {
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", length(args));
        evaluationError();
    }
    if (car(args)->type != CONS_TYPE) {
        printf("Contract violation. Expected: non-empty list. ");
        evaluationError();
    }
    return cdr(car(args));
}


/*
 * Implementing the Scheme primitive cons function.
 */
Value *primitiveCons(Value *args) {
    if (length(args) != 2) {
        printf("Arity mismatch. Expected: 2. Given: %i. ", length(args));
        evaluationError();
    }
    return cons((car(args)),car(cdr(args)));
}


/* 
 * Implementing the Scheme primitive <= function.
 */
Value *primitiveLeq(Value *args) {
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    if (length(args) < 2) {
        printf("Arity mismatch. Expected: at least 2. Given: %i. ", 
               length(args));
        evaluationError();
    }
    result->type = BOOL_TYPE; 
    result->s = "#t";
    double cur_largest;
    
    if (car(args)->type == INT_TYPE) {
        cur_largest = car(args)->i;
    } else if (car(args)->type == DOUBLE_TYPE) {
        cur_largest = car(args)->d;
    } else {
        printf("type: %i\n", car(args)->type);
        printf("Expected numerical arguments for <=. ");
        evaluationError();
    }

    Value *cur_arg = cdr(args); 
    while (cur_arg->type != NULL_TYPE) {
        Value *cur_num = car(cur_arg);
        if (cur_num->type != INT_TYPE) {
            if (cur_num->type == DOUBLE_TYPE) {
                if (cur_largest <= cur_num->d) {
                    cur_largest = cur_num->d;
                } else {
                    result->s = "#f";
                    return result;
                }
            } else {
                printf("Expected numerical arguments for <=. ");
                evaluationError();
            }
        } else {
            if (cur_largest <= cur_num->i) {
                cur_largest = cur_num->i;
            } else {
                result->s = "#f";
                return result;
            }    
        }
        cur_arg = cdr(cur_arg);
    }

    return result;
    
}


/*
 * Implementing the Scheme primitive pair? function.
 */
Value *primitiveIsPair(Value *args) {
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", 
               length(args));
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    result->type = BOOL_TYPE; 
    if (car(args)->type == CONS_TYPE) {
        result->s = "#t";
    } else {
        result->s = "#f";
    }
    return result;
}


/*
 * Implementing the Scheme primitive eq? function.
 */
Value *primitiveIsEq(Value *args) {
    if (length(args) != 2) {
        printf("Arity mismatch. Expected: 2. Given: %i. ", 
               length(args));
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    } 
    result->type = BOOL_TYPE;
    bool resultBool = true;
    
    Value *first = car(args);
    Value *second = car(cdr(args));
    
    switch (first->type) {
        case BOOL_TYPE:
            resultBool = (second->type == BOOL_TYPE &&
                          !strcmp(first->s, second->s) );
            break;
        case SYMBOL_TYPE:
            resultBool = (second->type == SYMBOL_TYPE &&
                         !strcmp(first->s, second->s));
            break;
        case INT_TYPE:
            resultBool = (second->type == INT_TYPE &&
                          first->i == second->i);
            break;
        case DOUBLE_TYPE:
            resultBool = (second->type == DOUBLE_TYPE &&
                          first->d == second->d);
            break;
        case STR_TYPE:
            resultBool = (second->type == STR_TYPE &&
                         !strcmp(first->s, second->s));
            break;
        case NULL_TYPE:
            resultBool = second->type == NULL_TYPE;
            break;
        case CONS_TYPE:
            resultBool = (second->type == CONS_TYPE &&
                          &first->c == &second->c);
            break;
        case CLOSURE_TYPE:
            resultBool = (second->type == CLOSURE_TYPE &&
                          &first->closure == &second->closure);
            break;
        case PRIMITIVE_TYPE:
            resultBool = (second->type == PRIMITIVE_TYPE &&
                          &first->pf == &second->pf);
            break;
        default:
            resultBool = (second->type == CLOSURE_TYPE &&
                          &first == &second);
            break;
    }
    if (resultBool) {
        result->s = "#t";
    } else {
        result->s = "#f";
    }
    return result;
}


/*
 * Helper function that applies a function to a given set of 
 * arguments.
 *
 * Right now only supports applying closure type functions.
 */
Value *apply(Value *function, Value *args, Frame *frame) {
    // Apply primitive f
    if (function->type == PRIMITIVE_TYPE) {
        return (function->pf)(args);
    }
    if (function->type != CLOSURE_TYPE) {
        printf("Expected the first argument to be a procedure! ");
        evaluationError();
    }
    Value *formal = function->closure.formal;
    Value *body = function->closure.body;
    Frame *parentFrame = function->closure.frame;
    if (formal->type == CONS_TYPE && length(formal) != length(args)) {
        printf("Expected %i arguments, supplied %i. ", 
               length(formal), length(args));
        evaluationError();
    }
    Frame *newFrame = talloc(sizeof(Frame));
    if (!newFrame) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    newFrame->parent = parentFrame;
    newFrame->bindings = makeNull();
     Value *curFormal = formal;
    Value *curActual = args;
    if (curFormal->type == CONS_TYPE) { 
	    while (curFormal->type != NULL_TYPE) {
            addBindingLocal(car(curFormal), car(curActual), newFrame);
            curFormal = cdr(curFormal);
            curActual = cdr(curActual);
    	}
    } else {
	    addBindingLocal(curFormal, curActual, newFrame);
    }
    // Evaluate multiple expressions
    while (cdr(body)->type != NULL_TYPE){
        eval(car(body), newFrame);
        body = cdr(body);
    }
    return eval(car(body), newFrame);
}


/* 
 * Implementing the Scheme primitive apply function.
 */
Value *primitiveApply(Value *args) {
    // Retrieve procedure
    Value *procedure = car(args);
    
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    } 
    // Create the list of arguments
    Value *arguments = makeNull();
    
    Value *cur_arg = cdr(args);
    while (cdr(cur_arg)->type != NULL_TYPE) {        
        arguments = cons(car(cur_arg), arguments);
        cur_arg = cdr(cur_arg);
    }
//    printf("type: %i\n", cur_arg->type);
    if (!primitiveIsPair(cur_arg)
        && car(cur_arg)->type != NULL_TYPE) {
        printf("Contract violation. Last argument must be a proper list. ");
        evaluationError();
    } 
    else {
        cur_arg = car(cur_arg);
//        printf("type: %i\n", cur_arg->type);
        while (cur_arg->type != NULL_TYPE) {
            if (cur_arg->type != CONS_TYPE) {
                printf("Contract violation. Last argument must be a proper list. ");
                evaluationError();
            }
            arguments = cons(car(cur_arg), arguments);
            cur_arg = cdr(cur_arg);
        }
    }
    
    Frame *frame = talloc(sizeof(Frame));
    frame->bindings = makeNull();
    frame->parent = NULL;
    
    return apply(procedure, arguments, frame);
}


/* 
 * Implements the primitive load function.
 */
Value *primitiveLoad(Value *arg) {
    char *filename = car(arg)->s;
    FILE *stream;
    stream = fopen(filename, "r");
    if (stream == NULL) {
        printf("Cannot open file \"%s\". ", filename);
        evaluationError();
    } 
    Value *list = tokenize(stream);
    if (list == NULL) {
        texit(1);
    }
    Value *tree = parse(list);
    if (tree == NULL) {
        texit(1);
    }    
    return tree;
}


/* 
 * Helper function to be display error message in primitive
 * procedures
 */
Value *primitiveEvalError (Value *errorMessage){
    printf("%s\n", car(errorMessage)->s);
    evaluationError();
    Value *values = makeNull();
        if (!values) {
            texit(1);
        }
    values->type = VOID_TYPE;
    return values;
} 
/* 
 * Implementing the Scheme primitive number? function.
 */
Value *primitiveNumberCheck (Value *args){
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", 
               length(args));
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    
    result->type = BOOL_TYPE; 
    if (car(args)->type == INT_TYPE || car(args)->type ==DOUBLE_TYPE) {
        result->s = "#t";
    } else {
        result->s = "#f";
    }
    return result;
}
/* 
 * Implementing the Scheme primitive integer? function.
 */
Value *primitiveIntegerCheck (Value *args){
    if (length(args) != 1) {
        printf("Arity mismatch. Expected: 1. Given: %i. ", 
               length(args));
        evaluationError();
    }
    Value *result = talloc(sizeof(Value));
    if (!result) {
        printf("Error! Not enough memory!\n");
        evaluationError();
    }
    
    result->type = BOOL_TYPE; 
    if (car(args)->type == INT_TYPE) {
        result->s = "#t";
    } else {
        result->s = "#f";
    }
    return result;
}



/*
 * The function takes a parse tree of a single S-expression and 
 * an environment frame in which to evaluate the expression and 
 * returns a pointer to a Value representating the value.
 */
Value *eval(Value *expr, Frame *frame){
    switch (expr->type) {
	case INT_TYPE:
	    return expr;
	    break;
	case DOUBLE_TYPE:
	    return expr;
	    break;
	case STR_TYPE:
	    return expr;
	    break;
	case BOOL_TYPE:
	    return expr;
	    break;
	case SYMBOL_TYPE: 
	    return lookUpSymbol(expr, frame); 		
	    break;
	case CONS_TYPE: {
	    Value *first = car(expr);
	    Value *args = cdr(expr);
	    if (!strcmp(first->s, "if")){
    		return evalIf(args, frame);
	    } 
	    else if (!strcmp(first->s, "quote")){
    		if (length(args) != 1){
                    printf("Number of arguments for 'quote' has to be 1. "); 
                    evaluationError();
            }
            return car(args);
	    }
        else if (!strcmp(first->s, "and")) {
            return evalAnd(args, frame);
        }
        else if (!strcmp(first->s, "or")) {
            return evalOr(args, frame);
        }
        else if (!strcmp(first->s, "begin")) {
            return evalBegin(args, frame);
        }
        else if (!strcmp(first->s, "cond")) {
            return evalCond(args, frame);
        }
	    else if (!strcmp(first->s, "let")) { 
	    	return evalLet(args, frame);
	    }
	    else if (!strcmp(first->s, "letrec")) {
		return evalLetrec(args, frame);
	    }
	    else if (!strcmp(first->s, "let*")) {
		return evalLetstar(args, frame);
	    }
	    else if (!strcmp(first->s, "define")) {
            	return evalDefine(args, frame);
            }
	    else if (!strcmp(first->s, "set!")) {
	    	return evalSet(args, frame);
	    }
       	else if (!strcmp(first->s, "lambda")) {
            return evalLambda(args, frame);
        }
	    else{
                // Stores the result of recursively evaluating e1...en
                Value *values = makeNull();
                if (!values) {
                    texit(1);
                }

                // Special treatment for load
                if (first->type == SYMBOL_TYPE && !strcmp(first->s, "load")) {
                    Value *loadFunction = eval(first, frame);
                    Value *loadTree = (loadFunction->pf)(args);
                    Value *curLoad = loadTree;
                    while (curLoad != NULL && curLoad->type == CONS_TYPE){
                        eval(car(curLoad), frame);
                        curLoad = cdr(curLoad);
                    }
                    Value *voidResult = talloc(sizeof(Value));
                    voidResult->type = VOID_TYPE;
                    return voidResult;
                    
                } else {
                    Value *cur = expr;
                    while (cur->type != NULL_TYPE) {
                        Value *cur_value = eval(car(cur), frame);
                        values = cons(cur_value, values);
                        cur = cdr(cur);
                    }
                    values = reverse(values);
                    Value *function = car(values);
                    Value *actual = cdr(values);
                    return apply(function, actual, frame);
                }          
	    }		
	    break;
	}
	default:
	    evaluationError();
	    break;
    }
    return NULL;   
}


/*
 * This function takes a list of S-expressions and call eval on 
 * each S-expression in the top-level environment and prints each
 * result 
 */
void interpret(Value *tree, Frame *topFrame){
  /*  Frame *topFrame = talloc(sizeof(Frame));
    if (!topFrame) {
        printf("Error! Not enough memory!\n");
        texit(1);
    }
    topFrame->bindings = makeNull();
    topFrame->parent = NULL;
*/
    // Bind the primitive functions
    bind("+", primitiveAdd, topFrame);
    bind("*", primitiveMult, topFrame);
    bind("-", primitiveSub, topFrame);
    bind("/", primitiveDiv, topFrame);
    bind("<=", primitiveLeq, topFrame);
    bind("eq?", primitiveIsEq, topFrame);
    bind("pair?", primitiveIsPair, topFrame);
    bind("null?", primitiveIsNull, topFrame);
    bind("apply", primitiveApply, topFrame);
    bind("car", primitiveCar, topFrame);
    bind("cdr", primitiveCdr, topFrame);
    bind("cons", primitiveCons, topFrame);
    bind("load", primitiveLoad, topFrame);
    //to be used in math.scm&list.scm
    bind("number?", primitiveNumberCheck, topFrame);
    bind("evaluationError", primitiveEvalError, topFrame);
    bind("integer?", primitiveIntegerCheck, topFrame);
    
    // Evaluate the program
    Value *cur = tree;
    while (cur != NULL && cur->type == CONS_TYPE){
    	Value *result = eval(car(cur), topFrame);
        if (result->type == CONS_TYPE){
            printf("(");
            displayEval(result, false);
            printf(")\n");
        } else {
            displayEval(result, true);
        }
        cur = cdr(cur);
    }
}

