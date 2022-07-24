#include <stdint.h>

typedef _Bool bool;

enum Opcodes {
	OP_END,
	OP_PUSHB, OP_PUSHS, OP_PUSHW,
	OP_PUSHCODE, OP_PUSHSTRING,
	OP_NAMED, OP_BIND, OP_UNBIND
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

void writePushOperation(void **out, unsigned int value) {
	if (value < 1 << 8) {
		writeSingleByte(out, OP_PUSHB);
		writeSingleByte(out, value);
	} else if (value < 1 << 16) {
		writeSingleByte(out, OP_PUSHS);
		writeSingleShort(out, value);
	} else {
		writeSingleByte(out, OP_PUSHW);
		writeSingleWord(out, value);
	}
}

void flipt_compile(char *source, void *out) {
	int codeDepth = 0;
	
	while (*source != '\0') {
		if (codeDepth == 0) {
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
				writePushOperation(&out, 0);
				for (char *str = source-1; str != stringstart; str--) {
					writePushOperation(&out, *str);
				}
				source++;
			} else if (*source == '{') {
				
			}
		}
	}
	writeSingleByte(&out, OP_END);
}

