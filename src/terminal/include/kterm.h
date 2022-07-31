#ifndef KTERM_H
#define KTERM_H

#include <stdarg.h>

void kterm_clear();
void kterm_newline();
void kterm_newlineSoft();
void kterm_glyph(unsigned char c);
void kterm_print(const char *string);
void kterm_printf(const char *format, ...);

#endif
