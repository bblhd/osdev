#include <stdint.h>

#include <string.h>
#include <flipt.h>

typedef _Bool bool;

void writeSingleByte(void **out, uint8_t b) {
	*((uint8_t *) *out) = b;
	*out = (void *) (((uint8_t *) *out) + 1);
}
void writeSingleShort(void **out, uint16_t s) {
	*((uint16_t *) *out) = s;
	*out = (void *) (((uint16_t *) *out) + 1);
}
void writeSingleWord(void **out, uint32_t w) {
	*((uint32_t *) *out) = w;
	*out = (void *) (((uint32_t *) *out) + 1);
}

void writeExtendedOp(void **out, uint8_t opcode, uint32_t value) {
	if (value == 0) {
		writeSingleByte(out, opcode & 0b111111);
	} else if (value < 1 << 8) {
		writeSingleByte(out, opcode & 0b111111 | (1 << 6));
		writeSingleByte(out, value);
	} else if (value < 1 << 16) {
		writeSingleByte(out, opcode & 0b111111 | (2 << 6));
		writeSingleShort(out, value);
	} else {
		writeSingleByte(out, opcode & 0b111111 | (3 << 6));
		writeSingleWord(out, value);
	}
}

// must fix some of this code
extern struct {
	char *symbol;
	uint8_t opcode;
} flipt_opTable[];

uint8_t pullSimpleOp(char **string) {
	for (int i = 0; flipt_opTable[i].opcode != OP_END; i++) {
		char *matchstr = flipt_opTable[i].symbol;
		
		int j = 0;
		while (
			matchstr[j] == (*string)[j]
			&& matchstr[j] != '\0'
		) j++;
		
		if (matchstr[j] == '\0') {
			*string+=j;
			return flipt_opTable[i].opcode;
		}
	}
	return 0;
}

uint8_t doesStringStartWithOp(char *string) {
	for (int i = 0; flipt_opTable[i].opcode != OP_END; i++) {
		char *matchstr = flipt_opTable[i].symbol;
		
		int j = 0;
		while (
			matchstr[j] != '\0'
			&& matchstr[j] == string[j]
		) j++;
		
		if (matchstr[j] == '\0') return flipt_opTable[i].opcode;
	}
	return OP_END;
}

char globalNameIndex[256][16];
int getVariableNameIndex(char *name, int len) {
	if (len > 16) len = 16;
	int index = 0;
	while (
		globalNameIndex[index][0] != '\0'
		&& !streqn(globalNameIndex[index], name, len)
	) index++;
	
	if (globalNameIndex[index][0] == '\0') {
		memcpy(globalNameIndex[index], name, len);
	}
	return index;
}

void flipt_subcompile(char **source, void **out, char end);

void *globalOutStart;

void compileNextExpression(char **sourceptr, void **outptr) {
	char *source, *start;
	void *out = *outptr;
	source = *sourceptr;
	start = source;
	while (*source == ' ' || *source == '\n' || *source == '\t') {
		source++;
	}
	if (*source == '"') {
		do {
			source++;
			if (*source == '\\') {
				source++;
			}
		} while (*source != '"');
		
		writeExtendedOp(&out, OP_PUSHSTR, (int) (source-start));
		
		for (char *str = start+1; str != source; str++) {
			writeSingleByte(&out, *str);
		}
		writeSingleByte(&out, '\0');
		source++;
	} else if (source[0] >= '0' && source[0] <= '9' || source[0] == '-' && source[1] >= '0' && source[1] <= '9') {
		int isNegative = 0;
		if (*source == '-') {
			source++;
			isNegative = 1;
		}
		int value = 0;
		while (*source >= '0' && *source <= '9') {
			value = 10 * value + (*source++ - '0');
		}
		if (isNegative) writeExtendedOp(&out, OP_PUSHN, value);
		else writeExtendedOp(&out, OP_PUSH, value);
	} else if (doesStringStartWithOp(source)) {
		
		uint8_t opcode = pullSimpleOp(&source);
		writeSingleByte(&out, opcode);
		
	} else if (*source == '(') {
		
		writeExtendedOp(&out, OP_SKIP, 1<<8);
		uint16_t *length = out-2;
		
		void *compileStart = out;
		source++;
		flipt_subcompile(&source, &out, ')');
		writeSingleByte(&out, OP_END);
		source++;
		*length = (uint8_t *) out - (uint8_t *) compileStart;
		
		writeExtendedOp(&out, OP_PUSHOFFSET, (uint8_t *) out - (uint8_t *) compileStart);
	} else if (*source != ')') {
		uint8_t op = OP_VALUEOF;
		if (source[0] == ':' && source[1] == ':') {
			op = OP_REBIND;
			source+=2;
		} else if (source[0] == ':') {
			op = OP_BIND;
			source++;
		} else if (source[0] == ';') {
			op = OP_UNBIND;
			source++;
		}
		
		while (*source == ' ' || *source == '\n' || *source == '\t') {
			source++;
		}
		
		char *name = source;
		
		while (
			*source != '\0'
			&& !doesStringStartWithOp(source)
			&& *source != '(' && *source != ')'
			&& *source != ' ' && *source != '\n' && *source != '\t'
			&& *source != '"'
			&& *source != '-' && !(*source >= '0' && *source <= '9')
			&& *source != ':'
		) source++;
		
		int index = getVariableNameIndex(name, source-name+1);
		
		writeExtendedOp(&out, op, index);
	}
	*sourceptr = source;
	*outptr = out;
}

void flipt_subcompile(char **source, void **out, char end) {
	while (**source != end) {
		compileNextExpression(source, out);
	}
}

void flipt_compile(char *source, void *out) {
	globalOutStart = out;
	flipt_subcompile(&source, &out, '\0');
	writeSingleByte(&out, OP_END);
	writeSingleByte(&out, OP_END);
}

int flipt_globalStack[256];
int flipt_globalStack_top = 0;

int flipt_globalNamespace_names[256];
int flipt_globalNamespace_values[256];
int flipt_globalNamespace_top = 0;

int indexof(int id) {
	for (int i = flipt_globalNamespace_top-1; i >= 0; i--) {
		if (flipt_globalNamespace_names[i] == id) return i;
	}
	return -1;
}
int valueof(int id) {
	int index = indexof(id);
	if (index >= 0) return flipt_globalNamespace_values[index];
	else return 0;
}
void bind(int id, int value) {
	flipt_globalNamespace_names[flipt_globalNamespace_top] = id;
	flipt_globalNamespace_values[flipt_globalNamespace_top] = value;
	flipt_globalNamespace_top++;
}
void unbind(int id) {
	int index = indexof(id);
	if (index >= 0) {
		for (int i = index; i < flipt_globalNamespace_top-1; i++) {
			flipt_globalNamespace_names[i] = flipt_globalNamespace_names[i+1];
			flipt_globalNamespace_values[i] = flipt_globalNamespace_values[i+1];
		}
		if (flipt_globalNamespace_top > 0) flipt_globalNamespace_top--;
	}
}
void rebind(int id, int value) {
	int index = indexof(id);
	if (index >= 0) flipt_globalNamespace_values[index] = value;
}

void push(int value) {
	if (flipt_globalStack_top < 255) {
		flipt_globalStack[flipt_globalStack_top++] = value;
	}
}
int pop() {
	if (flipt_globalStack_top > 0) {
		return flipt_globalStack[--flipt_globalStack_top];
	}
	return 0;
}
int peek() {
	if (flipt_globalStack_top > 0) {
		return flipt_globalStack[flipt_globalStack_top-1];
	}
	return 0;
}

void dummyOutput(int) {}

void (*output_function)(int) = dummyOutput;

void flipt_setOutputFunction(void (*func)(int)) {
	output_function = func;
}

void flipt_interpret(uint8_t *bytecode) {
	while (*bytecode != OP_END) {
		uint8_t op = *bytecode & 0b111111;
		uint8_t argsize = *bytecode >> 6;
		bytecode++;
		
		unsigned int value = 0;
		if (argsize == 1) value = *(uint8_t *) bytecode;
		else if (argsize == 2) value = *(uint16_t *) bytecode;
		else if (argsize == 3) value = *(uint32_t *) bytecode;
		
		if (argsize > 0) bytecode += 1 << (argsize-1);
		
		if (op == OP_PUSH) {
			push(value);
		} else if (op == OP_PUSHN) {
			push(-value);
		} else if (op == OP_PUSHOFFSET) {
			push((uint32_t) bytecode - value - 1 - (1 << (argsize-1)));
		} else if (op == OP_PUSHSTR) {
			for (int i = value-1; i >= 0; i--) push(bytecode[i]);
			bytecode+=value;
		} else if (op == OP_VALUEOF) {
			push(valueof(value));
		} else if (op == OP_BIND) {
			bind(value, pop());
		} else if (op == OP_UNBIND) {
			unbind(value);
		} else if (op == OP_REBIND) {
			rebind(value, pop());
		} else if (op == OP_SKIP) {
			bytecode += value;
		} else if (op == OP_ADD) {
			push(pop() + pop());
		} else if (op == OP_SUB) {
			int b = pop(); int a = pop();
			push(a - b);
		} else if (op == OP_MULT) {
			push(pop() * pop());
		} else if (op == OP_DIV) {
			int b = pop(); int a = pop();
			push(a / b);
		} else if (op == OP_MOD) {
			int b = pop(); int a = pop();
			push(a % b);
		} else if (op == OP_DUP) {
			push(peek());
		} else if (op == OP_SWAP) {
			int b = pop(); int a = pop();
			push(b);
			push(a);
		} else if (op == OP_OUTPUT) {
			output_function(pop());
		} else if (op == OP_CALL) {
			flipt_interpret((uint8_t *) pop());
		} else if (op == OP_IF) {
			uint8_t *body = (uint8_t *) pop();
			
			if (pop()) {
				flipt_interpret(body);
			}
		} else if (op == OP_IFELSE) {
			uint8_t *thenbody = (uint8_t *) pop();
			uint8_t *elsebody = (uint8_t *) pop();
			
			if (pop()) {
				flipt_interpret(thenbody);
			} else {
				flipt_interpret(elsebody);
			}
		} else if (op == OP_WHILE) {
			uint8_t *body = (uint8_t *) pop();
			uint8_t *condition = (uint8_t *) pop();
			
			flipt_interpret(condition);
			while (pop()) {
				flipt_interpret(body);
				flipt_interpret(condition);
			}
		}
	}
}
