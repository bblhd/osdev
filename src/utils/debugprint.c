#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <string.h>
#include <ktao.h>
#include <debugprint.h>

struct VGA_Target *debugOutput;

void setDebugOutputSource(struct VGA_Target *target) {
	debugOutput = target;
}

void printf(const char *format, ...) {
	va_list va;
	va_start(va, format);
	ktao_vprintf(debugOutput, format, va);
	va_end(va);
}

void print(const char *string) {
	ktao_print(debugOutput, string);
}

void putg(char g) {
	ktao_putGlyph(debugOutput, g);
}
