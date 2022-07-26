#ifndef FLIPT_H
#define FLIPT_H

void flipt_compile(char *source, void *out);

enum Opcodes {
	OP_END,
	OP_PUSHB, OP_PUSHS, OP_PUSHW,
	OP_PUSHBN, OP_PUSHSN, OP_PUSHWN,
	OP_PUSHSMALL, OP_PUSHLARGE,
	OP_VALUEOF, OP_BIND, OP_UNBIND,
	OP_START, OP_STOP,
	OP_CALL
};

extern char *flipt_operator_names[];

#endif
