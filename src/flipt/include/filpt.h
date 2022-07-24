#ifndef FLIPT_H
#define FLIPT_H

void flipt_compile(char *source, void *out);

enum Opcodes {
	OP_END,
	OP_PUSHB, OP_PUSHS, OP_PUSHW,
	OP_PUSHCODE, OP_PUSHSTRING,
	OP_NAMED, OP_BIND, OP_UNBIND
};

#endif
