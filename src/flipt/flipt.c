#include <stdint.h>

#include <string.h>
#include <filpt.h>

typedef _Bool bool;

char *flipt_operator_names[] = {
	"end",
	"pushb", "pushs", "pushw",
	"pushbn", "pushsn", "pushwn",
	"pushsmall", "pushlarge",
	"valueof", "bind", "unbind",
	"start", "stop",
	"call"
};

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

// must fix all this code

struct {
	char *symbol;
	uint8_t opcode;
} simpleOperations[] = {
	{.symbol=";", .opcode=OP_UNBIND},
	{.symbol="(", .opcode=OP_START},
	{.symbol=")", .opcode=OP_STOP},
	{.symbol="!", .opcode=OP_CALL},
	{.symbol="", .opcode=OP_END}
};

uint8_t pullSimpleOp(char **string) {
	for (int i = 0; simpleOperations[i].opcode != OP_END; i++) {
		char *matchstr = simpleOperations[i].symbol;
		
		int j = 0;
		while (
			matchstr[j] != '\0'
			&& matchstr[j] == *string[j]
		) j++;
		
		if (matchstr[j] == '\0') {
			*string+=j;
			return simpleOperations[i].opcode;
		}
	}
	return 0;
}

uint8_t doesStringStartWithOp(char *string) {
	for (int i = 0; simpleOperations[i].opcode != OP_END; i++) {
		char *matchstr = simpleOperations[i].symbol;
		
		int j = 0;
		while (
			matchstr[j] != '\0'
			&& matchstr[j] == string[j]
		) j++;
		
		if (matchstr[j] == '\0') return simpleOperations[i].opcode;
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
		
		if (source-start <= 255) {
			writeSingleByte(&out, OP_PUSHSMALL);
			writeSingleByte(&out, (int) (source-start));
		} else {
			writeSingleByte(&out, OP_PUSHLARGE);
			writeSingleShort(&out, (int) (source-start));
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
	} else {
		while (
			*source != '\0'
			&& !doesStringStartWithOp(source)
			&& *source != ' ' && *source != '\n' && *source != '\t'
			&& *source != '"'
			&& *source != '-' && !(*source >= '0' && *source <= '9')
			&& *source != ':'
		) source++;
		
		int index = getVariableNameIndex(start, source-start+1);
		
		while (*source == ' ' || *source == '\n' || *source == '\t') {
			source++;
		}
		
		if (*source == ':') {
			source++;
			writeSingleByte(&out, OP_BIND);
		} else {
			writeSingleByte(&out, OP_VALUEOF);
		}
		writeSingleShort(&out, index);
	}
	*sourceptr = source;
	*outptr = out;
}

void flipt_compile(char *source, void *out) {
	while (*source != '\0') {
		compileNextExpression(&source, &out);
	}
	writeSingleByte(&out, OP_END);
}
