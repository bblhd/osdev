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

void writePushOperation(void **out, int value) {
	uint8_t op = 0;
	if (value >= 0) {
		if (value < 1 << 8) op = OP_PUSHB;
		else if (value < 1 << 16) op = OP_PUSHS;
		else op = OP_PUSHW;
	} else {
		value = -value;
		if (value < 1 << 8) op = OP_PUSHBN;
		else if (value < 1 << 16) op = OP_PUSHSN;
		else op = OP_PUSHWN;
	}
	writeSingleByte(out, op);
	if (value < 1 << 8) {
		writeSingleByte(out, value);
	} else if (value < 1 << 16) {
		writeSingleShort(out, value);
	} else {
		writeSingleWord(out, value);
	}
}

void writePushOffsetOperation(void **out, int value) {
	uint8_t op = 0;
	if (value >= 0) {
		if (value < 1 << 8) op = OP_PUSHOFFSETB;
		else if (value < 1 << 16) op = OP_PUSHOFFSETS;
		else op = OP_PUSHOFFSETW;
	}
	writeSingleByte(out, op);
	if (value < 1 << 8) {
		writeSingleByte(out, value);
	} else if (value < 1 << 16) {
		writeSingleShort(out, value);
	} else {
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
		
		if (source-start < 1 << 8) {
			writeSingleByte(&out, OP_PUSHSTRLB);
			writeSingleByte(&out, (int) (source-start));
		} else if (source-start < 1 << 16) {
			writeSingleByte(&out, OP_PUSHSTRLS);
			writeSingleShort(&out, (int) (source-start));
		} else {
			writeSingleByte(&out, OP_PUSHSTRLW);
			writeSingleWord(&out, (int) (source-start));
		}
		
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
		if (isNegative) value = -value;
		writePushOperation(&out, value);
	} else if (doesStringStartWithOp(source)) {
		
		uint8_t opcode = pullSimpleOp(&source);
		writeSingleByte(&out, opcode);
		
	} else if (*source == '(') {
		
		writeSingleByte(&out, OP_SKIP);
		uint16_t *length = out;
		writeSingleShort(&out, 0);
		
		void *compileStart = out;
		source++;
		flipt_subcompile(&source, &out, ')');
		writeSingleByte(&out, OP_END);
		source++;
		*length = (uint8_t *) out - (uint8_t *) compileStart;
		
		writePushOperation(&out, (uint8_t *) compileStart - (uint8_t *) globalOutStart);
	} else if (*source != ')') {
		if (source[0] == ':' && source[1] == ':') {
			source+=2;
		} else if (source[0] == ':') {
			source++;
		} else if (source[0] == ';') {
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
		
		
		if (start[0] == ':' && start[1] == ':') {
			if (index < 1 << 8) writeSingleByte(&out, OP_REBINDLB);
			else if (index < 1 << 16) writeSingleByte(&out, OP_REBINDLS);
			else writeSingleByte(&out, OP_REBINDLW);
		} else if (start[0] == ':') {
			if (index < 1 << 8)	writeSingleByte(&out, OP_BINDLB);
			else if (index < 1 << 16) writeSingleByte(&out, OP_BINDLS);
			else writeSingleByte(&out, OP_BINDLW);
		} else if (start[0] == ';') {
			if (index < 1 << 8)	writeSingleByte(&out, OP_BINDLB);
			else if (index < 1 << 16) writeSingleByte(&out, OP_BINDLS);
			else writeSingleByte(&out, OP_BINDLW);
		} else {
			if (index < 1 << 8)	writeSingleByte(&out, OP_VALUEOFLB);
			else if (index < 1 << 16) writeSingleByte(&out, OP_VALUEOFLS);
			else writeSingleByte(&out, OP_VALUEOFLW);
		}
		if (index < 1 << 8) writeSingleByte(&out, index);
		else if (index < 1 << 16) writeSingleShort(&out, index);
		else writeSingleWord(&out, index);
		
		
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

void flipt_subinterpret(uint8_t *bytecode, uint8_t *start) {
	while (*bytecode != OP_END) {
		uint8_t op = *bytecode++;
		
		if (op == OP_PUSHB || op == OP_PUSHS || op == OP_PUSHW) {
			
			int value;
			
			if (op == OP_PUSHB) { value = *(uint8_t *) bytecode; bytecode+=1; }
			else if (op == OP_PUSHS) { value = *(uint16_t *) bytecode; bytecode+=2; }
			else if (op == OP_PUSHW) { value = *(uint32_t *) bytecode; bytecode+=4; }
			
			push(value);
			
		} else if (op == OP_PUSHBN || op == OP_PUSHSN || op == OP_PUSHWN) {
			
			int value;
			
			if (op == OP_PUSHBN) { value = *(uint8_t *) bytecode; bytecode+=1; }
			else if (op == OP_PUSHSN) { value = *(uint16_t *) bytecode; bytecode+=2; }
			else if (op == OP_PUSHWN) { value = *(uint32_t *) bytecode; bytecode+=4; }
			
			push(-value);
			
		} else if (op == OP_PUSHOFFSETB || op == OP_PUSHOFFSETS || op == OP_PUSHOFFSETW) {
			
			int value;
			
			if (op == OP_PUSHOFFSETB) { value = *(uint8_t *) bytecode; bytecode+=1; }
			else if (op == OP_PUSHOFFSETS) { value = *(uint16_t *) bytecode; bytecode+=2; }
			else if (op == OP_PUSHOFFSETW) { value = *(uint32_t *) bytecode; bytecode+=4; }
			
			push(value);
			
		} else if (op == OP_PUSHSTRLB || op == OP_PUSHSTRLS || op == OP_PUSHSTRLW) {
			
			int value;
			
			if (op == OP_PUSHSTRLB) { value = *(uint8_t *) bytecode; bytecode+=1; }
			else if (op == OP_PUSHSTRLS) { value = *(uint16_t *) bytecode; bytecode+=2; }
			else if (op == OP_PUSHSTRLW) { value = *(uint32_t *) bytecode; bytecode+=4; }
			
			for (int i = value-1; i >= 0; i--) push(bytecode[i]);
		
		} else switch (op) {
			case OP_SKIP: {
				int skipsize = *(uint16_t *) bytecode;
				bytecode += 2;
				bytecode += skipsize;
			} break;
			case OP_ADD: {
				push(pop() + pop());
			} break;
			case OP_SUB: {
				push(pop() - pop());
			} break;
			case OP_MULT: {
				push(pop() * pop());
			} break;
			case OP_DIV: {
				push(pop() / pop());
			} break;
			case OP_CALL: {
				flipt_subinterpret(start + pop(), start);
			} break;
		}
		
		int n = 1;
	}
}

void flipt_interpret(uint8_t *bytecode) {
	flipt_subinterpret(bytecode, bytecode);
}
