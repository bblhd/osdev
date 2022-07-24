#include <stdint.h>

#include <filpt.h>

typedef _Bool bool;

char *flipt_operator_names[] = {
	"end",
	"pushb", "pushs", "pushw",
	"pushbn", "pushsn", "pushwn",
	"pushsmall", "pushlarge",
	"named", "bind", "unbind"
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

void flipt_compile(char *source, void *out, int n) {
	while (*source != '\0' && n-- != 0) {
		while (*source == ' ' || *source == '\n' || *source == '\t') {
			source++;
		}
		if (*source == '"') {
			char *stringstart = source;
			do {
				source++;
				if (*source == '\\') {
					source++;
				}
			} while (*source != '"');
			writeSingleByte(&out, OP_PUSHSMALLBYTES);
			writeSingleByte(&out, (int) (source-stringstart-1));
			for (char *str = source-1; str != stringstart; str--) {
				writeSingleByte(&out, *str);
			}
			source++;
		} else if (*source == '{') {
			
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
		}
	}
	writeSingleByte(&out, OP_END);
}

