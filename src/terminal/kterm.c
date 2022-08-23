#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <kterm.h>
#include <x86.h>
#include <string.h>

uint16_t * const vga_buffer = (uint16_t *) 0xB8000;
const int vga_width = 80;
const int vga_height = 25;

uint8_t colour = 0x00;
int column = 0;

inline uint8_t make_vga_color(uint8_t fg, uint8_t bg) {
	return ((fg & 0b1111) | (bg & 0b111) << 4);
}

inline uint16_t make_vga_entry(unsigned char character, uint8_t color) {
	return character | ((uint16_t) color) << 8;
}

void kterm_clear() {
	int n = vga_width*vga_height;
	uint16_t *d = vga_buffer;
	uint16_t fill = make_vga_entry('\0', colour);
	
	while(n--) *d++ = fill;
}

void kterm_newline() {
    int n = vga_width*vga_height - vga_width;
	uint16_t *d = vga_buffer;
    while(n--) {
		*d = d[vga_width];
		d++;
	}
    
    n = vga_width;
	uint16_t fill = make_vga_entry('\0', colour);
	while(n--) vga_buffer[vga_width*(vga_height-1) + n] = fill;
	
	column = 0;
}

void kterm_newlineSoft() {
	if (column != 0) kterm_newline();
}

void kterm_glyph(uint8_t glyph) {
	if (column >= vga_width) kterm_newline();
	vga_buffer[vga_width*(vga_height-1) + column++] = make_vga_entry(glyph, colour);
}

enum {
	BASE, ESCAPE, GLYPH, FG, BG
} printState = BASE;

int hexdigitvalue(char c) {
	return c >= '0' && c <= '9' ? c - '0' : (
		c >= 'a' && c <= 'f' ? c - 'a' + 10 : (
			c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0
		)
	);
}

void kterm_print1(unsigned char c) {
	if (printState == BASE) {
		if (c == 0x1B) {
			printState = ESCAPE;
		} else if (c == 0x08) {
			if (column > 0) column--;
		} else if (c == 0x0A) {
			kterm_newline();
		} else if (c >= 0x20 && c != 0x7F) {
			kterm_glyph(c);
		}
	} else if (printState == ESCAPE) {
		if (c == 'G') {
			printState = GLYPH;
		} else if (c == 'F') {
			printState = FG;
		} else if (c == 'B') {
			printState = BG;
		} else if (c == 'J') {
			kterm_clear();
			column = 0;
			printState = BASE;
		} else if (c == 'I') {
			colour ^= 0b10000000;
			printState = BASE;
		} else {
			printState = BASE;
		}
	} else if (printState == GLYPH) {
		kterm_glyph(c);
		printState = BASE;
	} else if (printState == FG) {
		colour = make_vga_color(hexdigitvalue(c), colour >> 4);
		printState = BASE;
	} else if (printState == BG) {
		colour = make_vga_color(colour, hexdigitvalue(c));
		printState = BASE;
	}
}

void kterm_print(const char *string) {
	while (*string != '\0') kterm_print1(*string++);
}


void kterm_printul(unsigned long num, int base, int precision) {
	if (num == 0) {
		kterm_glyph('0');
		return;
	}
	uint8_t buffer[precision];
	int top = 0;
	buffer[top] = 0;
	while (num > 0 && top < precision) {
		buffer[top++] = num % base;
		num /= base;
	}
	while (top) {
		--top;
		if (buffer[top] >= 10) kterm_glyph('a' + buffer[top] - 10);
		else kterm_glyph('0' + buffer[top]);
	}
}

void kterm_printl(long num, int base, int precision) {
	if (num < 0) {
		kterm_glyph('-');
		kterm_printul(-num, base, precision);
	} else {
		kterm_printul(num, base, precision);
	}
}

void kterm_vprintf(const char *format, va_list va) {
	while (*format != '\0') {
		if (*format == '%') {
			format++;
			if (*format == 'i') {
				kterm_printl(va_arg(va, int), 10, 11);
			} else if (*format == 'u') {
				kterm_printul(va_arg(va, unsigned int), 10, 11);
			} else if (*format == 'p') {
				kterm_glyph('0');
				kterm_glyph('x');
				kterm_printul((unsigned int) va_arg(va, void*), 16, 9);
			} else if (*format == 's') {
				kterm_print(va_arg(va, char*));
			} else if (*format == 'c') {
				kterm_glyph(va_arg(va, int));
			} else {
				kterm_glyph(*format);
			}
		} else {
			kterm_print1(*format);
		}
		format++;
	}
}
void kterm_printf(const char *format, ...) {
	va_list va;
	va_start(va, format);
	kterm_vprintf(format, va);
	va_end(va);
}

