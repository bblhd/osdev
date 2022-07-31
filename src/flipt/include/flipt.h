#ifndef FLIPT_H
#define FLIPT_H

#include <stdint.h>

extern int flipt_globalStack[256];
extern int flipt_globalStack_top;

extern int flipt_globalNamespace_names[256];
extern int flipt_globalNamespace_values[256];
extern int flipt_globalNamespace_top;

void flipt_setupOpNames(char *names[]);
void flipt_external(char *name, int value);
void flipt_compile(char *source, void *out);
void flipt_setOutputFunction(void (*func)(int));
void flipt_interpret(uint8_t *bytecode, int **stack);

enum Opcodes {
	OP_END, OP_START, 
	OP_SET, OP_GET,
	OP_STORE, OP_LOAD, 
	OP_OUTPUT, OP_INPUT,
	OP_PUSH, OP_PUSHN, OP_PUSHSTR,
	OP_DUP, OP_DEL, OP_SWAP,
	OP_CALL, OP_IF, OP_WHILE, OP_IFELSE,
	OP_NOT, OP_NE, OP_EQ, OP_GT, OP_LT, OP_GE, OP_LE,
	OP_ADD, OP_SUB, OP_MULT, OP_DIV, OP_MOD,
};

#endif
