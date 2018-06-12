# Interpreter for Scheme (R5RS) written in C.
## Run this program
Open the terminal 
```
$ git clone https://github.com/yingyingww/Scheme-Interpreter.git
$ cd Scheme-Interpreter
$ make interpreter
```
Two options: 
You can either initiate REPL (read-eval-print loop) for interactive usage.
```
$ ./interpreter
```
Or you can create a new file, such as `input.scm`, and write scheme codes in this file.
```
$ ./interpreter < input.scm
```

## Phases of this project
### Phase 1: Linked List
 &nbsp; To implement a linked list that will be used throughout the interpreter project.
##### Main files at this phase: 
linkedlist.c linkedlist.h

### Phase 2: Talloc("tracked malloc")
 &nbsp; To create a garbage collector to manage memory usage throughout the interpreter project.
##### Main files at this phase: 
talloc.c talloc.h
### Phase 3: Tokenizer
 &nbsp; To implement a tokenizer for Scheme in C.
##### Main files at this phase: 
tokenizer.c tockenizer.h
### Phase 4: Parser
 &nbsp; To parse the tokens into a syntax tree.
##### Main files at this phase: 
parser.c parser.h

### Phase 5: Eval
 &nbsp; To evaluate the Scheme code. Specifically allow the evaluation of
bounded variables, if, let and quote special forms.
##### Main files at this phase:
interpreter.h interpreter.c

### Phase 6: Apply
 &nbsp; Extend the evaluator's ability to handle define and lambda special forms.
##### Main files at this phase:
interpreter.h interpreter.c

### Phase 7: Primitives
 &nbsp; To support applying Scheme primitive functions implemented in C; to implement a few primitive functions.
##### Main files at this phase:
interpreter.h interpreter.c

### Phase 8: Interpreter
 &nbsp; To add a myriad of features to the interpreter including:
##### Main files at this phase:
interpreter.h interpreter.c
&npsp;&nbsp;
##### Primitive procedures:
&nbsp; *, -, /, <=, eq?, pair?, and apply.
###### Main file:
interpreter.c

##### Library procedures:
 &nbsp; =, modulo, zero?, equal?, list, and append.
###### Main file:
lists.scm (include list and append)
math.scm (include =, modulo, zero?, and equal?)
##### Special forms: 
 &nbsp;  lambda, let*, letrec, and, or, cond, set!, and begin.
###### Main file:
interpreter.c


##### Extensions -
###### A simple interface
 &nbsp; The classic core of an interpreter is the read–eval–print loop, a.k.a. REPL. Adding this functionality to our code allows for interactive usage.
###### Main file:
tokenizer.c main.c

##### ' as the shorthand for quote special form NOT YET!!!!!
 &nbsp; For example, '(2 2 8) as the shorthand for (quote (2 2 8))
Currently doesn't work for nested lists
###### Main file:
tokenizer.c 

##### The function load
 &nbsp; The expression (load "tofu.scm") reads in the file and excutes the Scheme code within as if it were typed directly as part of the input.
###### Main file:
interpreter.c

##### More built-in functions to manipulate lists.
 &nbsp; In a file called lists.scm, implement the following functions (refer to R5RS, Dybvig, or Racket reference for specification) using only special forms and primitives that you've implemented (e.g., car, cdr, cons, null?, pair?, and apply).
###### Main file:
lists.scm

##### More built-in functions with regards to arithmetic.
 &nbsp; To implement the following functions using only special forms and primitives that we've implemented (e.g., +, -, *, /, and <=) in a file called math.scm.
###### Main file:
math.scm

## Author: 
* **Yitong Chen** - [yitongc19](https://github.com/yitongc19)
* **Yingying Wang** - [yingyingww](https://github.com/yingyingww)
* **Megan Zhao** - [meganzhao](https://github.com/meganzhao)
