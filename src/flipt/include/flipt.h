#ifndef FLIPT_H
#define FLIPT_H

#include <stdint.h>

void flipt_setupOpNames(char *names[]);
void flipt_external(char *name, int value);
int flipt_compile(char *source, uint8_t *out, char *nameIndex);
void flipt_setOutputFunction(void (*func)(int));
void flipt_interpret(uint8_t *bytecode, int **stack, int *variables);

enum Opcodes {
	OP_END,
	OP_FUNCTION, OP_STRING,
	OP_UNBIND,
	OP_STORE, OP_LOAD, 
	OP_OUTPUT, OP_INPUT,
	OP_DUP, OP_DEL, OP_SWAP,
	OP_CALL, OP_IF, OP_WHILE, OP_IFELSE,
	OP_NOT, OP_NE, OP_EQ, OP_GT, OP_LT, OP_GE, OP_LE,
	OP_ADD, OP_SUB, OP_MULT, OP_DIV, OP_MOD
};
enum ExtendedOpcodes {
	OP_POSITIVE, OP_NEGATIVE,
	OP_VARIABLE, OP_BIND
};

#endif
