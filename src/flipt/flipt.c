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
	int length = 0;
	if (*source == '"') {
		do {
			length++;
			source++;
			if (*source == '\\') {
				source++;
			}
		} while (*source != '"');
		
		writeExtendedOp(&out, OP_PUSHSTR, length);
		
		for (char *str = start+1; str < source; str++) {
			if (*str == '\\') {
				str++;
				if (*str == 'e') writeSingleByte(&out, '\e');
				else if (*str == 'b') writeSingleByte(&out, '\b');
				else if (*str == 'n') writeSingleByte(&out, '\n');
				else if (*str == 't') writeSingleByte(&out, '\t');
				else writeSingleByte(&out, *str);
			} else writeSingleByte(&out, *str);
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
		
		writeExtendedOp(&out, OP_START, 1<<8);
		uint16_t *length = out-2;
		
		void *compileStart = out;
		source++;
		flipt_subcompile(&source, &out, ')');
		writeSingleByte(&out, OP_END);
		source++;
		*length = (uint8_t *) out - (uint8_t *) compileStart;
	} else if (*source == ')') {
		source++;
	} else {
		uint8_t op = OP_GET;
		if (source[0] == ':') {
			op = OP_SET;
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
	while (**source != '\0' && **source != end) {
		compileNextExpression(source, out);
	}
}

void flipt_compile(char *source, void *out) {
	globalOutStart = out;
	flipt_subcompile(&source, &out, '\0');
	writeSingleByte(&out, OP_END);
	writeSingleByte(&out, OP_END);
}

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

void unbind(int index) {
	if (index >= 0) {
		for (int i = index; i < flipt_globalNamespace_top-1; i++) {
			flipt_globalNamespace_names[i] = flipt_globalNamespace_names[i+1];
			flipt_globalNamespace_values[i] = flipt_globalNamespace_values[i+1];
		}
		if (flipt_globalNamespace_top > 0) flipt_globalNamespace_top--;
	}
}

void bind(int id, int value) {
	int index = indexof(id);
	if (index < 0) {
		if (value == 0) return;
		index = flipt_globalNamespace_top++;
		flipt_globalNamespace_names[index] = id;
	}
	if (value == 0) unbind(index);
	else flipt_globalNamespace_values[index] = value;
}

void flipt_external(char *name, int value) {
	int id = getVariableNameIndex(name, strlen(name));
	bind(id, value);
}

void push(int **stack, int value) {
	**stack = value;
	++(*stack);
}
int pop(int **stack) {
	return *--*stack;
}
void swap(int *stack) {
	int swap = stack[-1];
	stack[-1] = stack[0];
	stack[0] = swap;
}
int peek(int **stack) {
	return *(*stack-1);
}

void dummyOutput(int) {}

void (*output_function)(int) = dummyOutput;

void flipt_setOutputFunction(void (*func)(int)) {
	output_function = func;
}

void flipt_interpret(uint8_t *bytecode, int **stack) {
	while (*bytecode != OP_END) {
		uint8_t op = *bytecode & 0b111111;
		uint8_t argsize = *bytecode >> 6;
		bytecode++;
		
		unsigned int value = 0;
		if (argsize == 1) value = *(uint8_t *) bytecode;
		else if (argsize == 2) value = *(uint16_t *) bytecode;
		else if (argsize == 3) value = *(uint32_t *) bytecode;
		if (argsize > 0) bytecode += 1 << (argsize-1);
		
		
		union {
			int operand;
			uint8_t *body;
		} a;
		union {
			uint8_t *cond;
			uint8_t *elsebody;
		} b;
		
		switch (op) {
			case OP_START:
				push(stack, (uint32_t) bytecode);
				bytecode += value;
			break;
			
			case OP_PUSH:
				push(stack, value);
			break;
			case OP_PUSHN:
				push(stack, -value);
			break;
			case OP_PUSHSTR:
				for (int i = value-1; i >= 0; i--) {
					push(stack, bytecode[i]);
				}
				bytecode+=value;
			break;
			
			case OP_GET:
				push(stack, valueof(value));
			break;
			case OP_SET:
				bind(value, pop(stack));
			break;
			
			case OP_ADD:
				push(stack, pop(stack) + pop(stack));
			break;
			case OP_SUB:
				a.operand = pop(stack);
				push(stack, pop(stack) - a.operand);
			break;
			case OP_MULT:
				push(stack, pop(stack) * pop(stack));
			break;
			case OP_DIV:
				a.operand = pop(stack);
				push(stack, pop(stack) / a.operand);
			break;
			case OP_MOD:
				a.operand = pop(stack);
				push(stack, pop(stack) % a.operand);
			break;
			
			case OP_DUP:
				push(stack, peek(stack));
			break;
			case OP_DEL:
				pop(stack);
			break;
			case OP_SWAP:
				swap(*stack);
			break;
			
			case OP_OUTPUT:
				output_function(pop(stack));
			break;
			
			case OP_LOAD:
				push(stack, *(int *) pop(stack));
			break;
			case OP_STORE:
				*(int *) pop(stack) = pop(stack);
			break;
			
			case OP_CALL:
				flipt_interpret((uint8_t *) pop(stack), stack);
			break;
			case OP_IF:
				a.body = (uint8_t *) pop(stack);
				
				if (pop(stack)) {
					flipt_interpret(a.body, stack);
				}
			break;
			case OP_WHILE:
				a.body = (uint8_t *) pop(stack);
				b.cond = (uint8_t *) pop(stack);
				
				flipt_interpret(b.cond, stack);
				while (pop(stack)) {
					flipt_interpret(a.body, stack);
					flipt_interpret(b.cond, stack);
				}
			break;
			case OP_IFELSE: 
				a.body = (uint8_t *) pop(stack);
				b.elsebody = (uint8_t *) pop(stack);
				
				if (pop(stack)) {
					flipt_interpret(a.body, stack);
				} else {
					flipt_interpret(b.elsebody, stack);
				}
			break;
		}
	}
}
