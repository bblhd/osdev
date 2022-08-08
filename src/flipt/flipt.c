#include <stdint.h>

#include <math.h>
#include <string.h>
#include <flipt.h>

typedef _Bool bool;

void writeSingleByte(uint8_t **out, uint8_t b) {
	**out = b;
	*out += 1;
}
void writeSingleShort(uint8_t **out, uint16_t s) {
	*((uint16_t *) *out) = s;
	*out = (uint8_t *) (((uint16_t *) *out) + 1);
}
void writeSingleWord(uint8_t **out, uint32_t w) {
	*((uint32_t *) *out) = w;
	*out = (uint8_t *) (((uint32_t *) *out) + 1);
}

void writeExtendedOp(uint8_t **out, uint8_t opcode, uint32_t value) {
	if (value < 0b1100) {
		writeSingleByte(out, 0b10000000 | ((value & 0b1111) << 3) | opcode & 0b111); //short form
	} else if (value < 1 << 8) {
		writeSingleByte(out, 0b11100000 | opcode & 0b111);
		writeSingleByte(out, value);
	} else if (value < 1 << 16) {
		writeSingleByte(out, 0b11101000 | opcode & 0b111);
		writeSingleShort(out, value);
	} else {
		writeSingleByte(out, 0b11110000 | opcode & 0b111);
		writeSingleWord(out, value);
	}
}

int isExtended(uint8_t instr) {
	return instr >= 0b10000000;
}

// must fix some of this code
struct {
	char *symbol;
	uint8_t opcode;
} flipt_opTable[] = {
	{.symbol="..", .opcode=OP_DUP},
	{.symbol="~=", .opcode=OP_NE},
	{.symbol="?!", .opcode=OP_IFELSE},
	{.symbol=">=", .opcode=OP_GE},
	{.symbol="??", .opcode=OP_WHILE},
	{.symbol="<=", .opcode=OP_LE},
	{.symbol="|", .opcode=OP_SWAP},
	{.symbol="#", .opcode=OP_DEL},
	{.symbol=".", .opcode=OP_OUTPUT},
	{.symbol="!", .opcode=OP_CALL},
	{.symbol="+", .opcode=OP_ADD},
	{.symbol="-", .opcode=OP_SUB},
	{.symbol="<", .opcode=OP_LT},
	{.symbol=">", .opcode=OP_GT},
	{.symbol="=", .opcode=OP_EQ},
	{.symbol="*", .opcode=OP_MULT},
	{.symbol="?", .opcode=OP_IF},
	{.symbol="~", .opcode=OP_NOT},
	{.symbol="/", .opcode=OP_DIV},
	{.symbol="%", .opcode=OP_MOD},
	{.symbol="@", .opcode=OP_LOAD},
	{.symbol="$", .opcode=OP_STORE},
	{.symbol="{", .opcode=OP_FUNCTION},
	{.symbol="}", .opcode=OP_END},
	{.symbol=";", .opcode=OP_UNBIND},
	{.symbol="", .opcode=OP_END},
};

uint8_t pullSimpleOp(char **string) {
	for (int i = 0; flipt_opTable[i].symbol[0] != '\0'; i++) {
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
	for (int i = 0; flipt_opTable[i].symbol[0] != '\0'; i++) {
		char *matchstr = flipt_opTable[i].symbol;
		
		int j = 0;
		while (
			matchstr[j] != '\0'
			&& matchstr[j] == string[j]
		) j++;
		
		if (matchstr[j] == '\0') return 1;
	}
	return OP_END;
}

#include <kterm.h>

int findNameInNameIndexOrAddAndFind(char *nameIndex, char *string, int n) {
	int id = 0;
	while (*nameIndex != '\0') {
		int j = 0;
		for (j = 0; *nameIndex != '\0' && j < n && string[j] == *nameIndex; j++) nameIndex++;
		if (*nameIndex == '\0' || j >= n) return id;
		while (*nameIndex != '\0') nameIndex++;
		id++;
		nameIndex++;
	}
	while (--n) {
		*nameIndex++ = *string++;
	}
	*nameIndex++ = 0;
	*nameIndex++ = 0;
	return id;
}

int compileNextExpression(char **source, uint8_t **out, char *nameIndex) {
	while (**source == ' ' || **source == '\n' || **source == '\t') {
		(*source)++;
	}
	if (**source == '"') {
		writeSingleByte(out, OP_STRING);
		(*source)++;
		uint8_t *outstart = *out;
		while (**source != '"') {
			if (**source == '\\') {
				(*source)++;
				if (**source == 'e') writeSingleByte(out, '\e');
				else if (**source == 'b') writeSingleByte(out, '\b');
				else if (**source == 'n') writeSingleByte(out, '\n');
				else if (**source == 't') writeSingleByte(out, '\t');
				else writeSingleByte(out, **source);
			} else writeSingleByte(out, **source);
			(*source)++;
		}
		for (uint8_t *i=outstart, *j=*out-1; i < j; i++, j--) {
			uint8_t swap = *i;
			*i = *j;
			*j = swap;
		}
		(*source)++;
		writeSingleByte(out, '\0');
	} else if (**source >= '0' && **source <= '9' || **source == '-' && (*source)[1] >= '0' && (*source)[1] <= '9') {
		int isNegative = 0;
		if (**source == '-') {
			(*source)++;
			isNegative = 1;
		}
		int value = 0;
		while (**source >= '0' && **source <= '9') {
			value = 10 * value + (*(*source)++ - '0');
		}
		if (isNegative) writeExtendedOp(out, OP_NEGATIVE, value);
		else writeExtendedOp(out, OP_POSITIVE, value);
	} else if (doesStringStartWithOp(*source)) {
		writeSingleByte(out, pullSimpleOp(source));
	} else {
		uint8_t op = OP_VARIABLE;
		if (**source == ':') {
			op = OP_BIND;
			(*source)++;
		}
		
		while (**source == ' ' || **source == '\n' || **source == '\t') {
			(*source)++;
		}
		
		char *name = *source;
		
		while (
			**source != '\0'
			&& !doesStringStartWithOp(*source)
			&& **source != ' ' && **source != '\n' && **source != '\t'
			&& **source != '"'
			&& **source != '-' && !(**source >= '0' && **source <= '9')
			&& **source != ':'
		) *source+=1;
		
		int index = findNameInNameIndexOrAddAndFind(nameIndex, name, *source-name+1);
		
		writeExtendedOp(out, op, index);
	}
	return 0;
}

int flipt_compile(char *source, uint8_t *out, char *nameIndex) {
	int err = 0;
	while (!err && *source != '\0') {
		err = compileNextExpression(&source, &out, nameIndex);
	}
	writeSingleByte(&out, OP_END);
	writeSingleByte(&out, OP_END);
	return err;
}

//void dummyOutput(int) {}

//void (*output_function)(int) = dummyOutput;

//void flipt_setOutputFunction(void (*func)(int)) {
	//output_function = func;
//}


void push(int **stack, int value) {
	**stack = value;
	++*stack;
}

int pop(int **stack) {
	return *--*stack;
}

void change(int **stack, int value) {
	(*stack)[-1] = value;
}

int peek(int **stack) {
	return (*stack)[-1];
}

int swap(int **stack) {
	return (*stack)[-1];
}


int getInstructionLength(uint8_t instr) {
	if (instr >= 0b11100000) {
		return 1 + 1 << ((instr >> 3) & 0b11);
	} else if (instr == OP_STRING) return -1;
	return 1;
}

uint8_t *findEnd(uint8_t *bytecode) {
	int depth = 1;
	do {
		if (*bytecode == OP_FUNCTION) depth++;
		else if (*bytecode == OP_END) depth--;
		
		int len = getInstructionLength(*bytecode);
		if (len < 0) {
			while (*bytecode != '\0') bytecode++;
			bytecode++;
		} else bytecode += len;
	} while (depth > 0);
	return bytecode;
}

void kterm_print1(char);

void flipt_interpret(uint8_t *bytecode, int **stack, int *variables) {
	while (*bytecode != OP_END) {
		if (*bytecode >= 0b10000000) {
			uint8_t ext_op = *bytecode & 0b111;
			unsigned int value = 0;
			
			if (*bytecode >> 5 == 0b111) {
				uint8_t argsize = (*bytecode >> 3) & 0b11;
				
				bytecode++;
				
				if (argsize == 0b00) value = *(uint8_t *) bytecode;
				else if (argsize == 0b01) value = *(uint16_t *) bytecode;
				else if (argsize == 0b10) value = *(uint32_t *) bytecode;
				
				bytecode += 1<<argsize;
			} else {
				value = (*bytecode >> 3) & 0b1111;
				
				bytecode++;
			}
			
			if (ext_op == OP_POSITIVE) {
				push(stack, value);
			} else if (ext_op == OP_NEGATIVE) {
				push(stack, -value);
				
			} else if (ext_op == OP_VARIABLE) {
			} else if (ext_op == OP_BIND) {
			}
		} else {
			uint8_t op = *bytecode & 0b1111111;
			bytecode++;
			
			int value;
			
			if (op == OP_FUNCTION) {
				push(stack, (int) (bytecode));
				bytecode = findEnd(bytecode);
			} else if (op == OP_STRING) {
				push(stack, 0);
				while (*bytecode != '\0') {
					push(stack, *bytecode++);
				}
				bytecode++;
				
			} else if (op == OP_UNBIND) {
				
			} else if (op == OP_STORE) {
			} else if (op == OP_LOAD) {
				
			} else if (op == OP_OUTPUT) {
				kterm_print1(pop(stack));
			} else if (op == OP_INPUT) {
				
			} else if (op == OP_DUP) {
				push(stack, peek(stack));
			} else if (op == OP_DEL) {
				pop(stack);
			} else if (op == OP_SWAP) {
				value = (*stack)[-1];
				(*stack)[-1] = (*stack)[-2];
				(*stack)[-2] = value;
				
			} else if (op == OP_CALL) {
				uint8_t *body = (uint8_t *) pop(stack);
				flipt_interpret(body, stack, variables);
				
			} else if (op == OP_IF) {
				uint8_t *body = (uint8_t *) pop(stack);
				if (pop(stack)) {
					flipt_interpret(body, stack, variables);
				}
			} else if (op == OP_WHILE) {
				uint8_t *body = (uint8_t *) pop(stack);
				uint8_t *condition = (uint8_t *) pop(stack);
				
				flipt_interpret(condition, stack, variables);
				while (pop(stack)) {
					flipt_interpret(body, stack, variables);
					flipt_interpret(condition, stack, variables);
				}
			} else if (op == OP_IFELSE) {
				uint8_t *body = (uint8_t *) pop(stack);
				uint8_t *elsebody = (uint8_t *) pop(stack);
				if (pop(stack)) {
					flipt_interpret(body, stack, variables);
				} else {
					flipt_interpret(elsebody, stack, variables);
				}
				
			} else if (op == OP_NOT) {
				push(stack, !pop(stack));
			} else if (op == OP_NE) {
				push(stack, pop(stack) != pop(stack));
			} else if (op == OP_EQ) {
				push(stack, pop(stack) == pop(stack));
			} else if (op == OP_GT) {
				push(stack, pop(stack) < pop(stack));
			} else if (op == OP_LT) {
				push(stack, pop(stack) > pop(stack));
			} else if (op == OP_GE) {
				push(stack, pop(stack) <= pop(stack));
			} else if (op == OP_LE) {
				push(stack, pop(stack) >= pop(stack));
				
			} else if (op == OP_ADD) {
				push(stack, pop(stack) + pop(stack));
			} else if (op == OP_SUB) {
				value = pop(stack);
				push(stack, pop(stack) - value);
			} else if (op == OP_MULT) {
				push(stack, pop(stack) * pop(stack));
			} else if (op == OP_DIV) {
				value = pop(stack);
				push(stack, pop(stack) / value);
			} else if (op == OP_MOD) {
				value = pop(stack);
				push(stack, mod(pop(stack), value));
			}
		}
	}
}
