#include <stddef.h>
#include <stdint.h>

#include <string.h>
#include <ktao.h>

#define PC_VGA_BUFFER ((uint16_t *) 0xB8000)
#define GLOBAL_DEFAULT_COLOR make_vga_color(13, 0)

inline uint8_t make_vga_color(uint8_t fg, uint8_t bg) {
	return ((fg & 0b1111) | (bg & 0b111) << 4);
}

inline uint16_t make_vga_entry(unsigned char character, uint8_t color) {
	return character | ((uint16_t) color) << 8;
}

void ktao_clearAll() {
	uint16_t fill = make_vga_entry(0, GLOBAL_DEFAULT_COLOR);
	uint16_t *buffer = PC_VGA_BUFFER;
	for (size_t index = 0; index < PC_VGA_WIDTH * PC_VGA_HEIGHT; index++) {
		buffer[index] = fill;
	}
}

void ktao_initialiseToGlobalVGA(struct VGA_Target *target, int x, int y, int w, int h) {
	target->buffer = PC_VGA_BUFFER + (size_t) (PC_VGA_WIDTH * y + x);
	target->pitch = PC_VGA_WIDTH;
	
	target->width = w;
	target->height = h;
	
	ktao_default(target);
}

void ktao_default(struct VGA_Target *target) {
	target->column = 0;
	target->defaultcolor = GLOBAL_DEFAULT_COLOR;
	target->color = target->defaultcolor;
}

int ktao_index(struct VGA_Target *target, int x, int y) {
	return y * target->pitch + x;
}

int ktao_isWithin(struct VGA_Target *target, size_t index) {
	size_t x = index % target->pitch;
	size_t y = index / target->pitch;
	return x < target->width && y < target->height;
}

void ktao_set(struct VGA_Target *target, size_t index, uint16_t entry) {
	if (target->buffer == NULL) return;
	
	if (ktao_isWithin(target, index)) {
		target->buffer[index] = entry;
	}
}

uint16_t ktao_get(struct VGA_Target *target, size_t index) {
	if (target->buffer == NULL) return 0;
	if (ktao_isWithin(target, index)) {
		return target->buffer[index];
	} else {
		return make_vga_entry(0, target->defaultcolor);
	}
}

void ktao_clearEntries(struct VGA_Target *target) {
	if (target->buffer == NULL) return;
	for (size_t y = 0; y < target->height; y++) {
		for (size_t x = 0; x < target->width; x++) {
			ktao_set(target, ktao_index(target, x, y), make_vga_entry(0, target->color));
		}
	}
}

void ktao_shiftEntries(struct VGA_Target *target, size_t delta) {
	for (size_t y = 0; y < target->height; y++) {
		for (size_t x = 0; x < target->width; x++) {
			size_t dst = ktao_index(target, x, y);
			size_t src = dst - delta;
			ktao_set(target, dst, ktao_get(target, src));
		}
	}
}

void ktao_newline(struct VGA_Target *target) {
	ktao_shiftEntries(target, ktao_index(target, 0, -1));
	target->column = 0;
}

void ktao_putGlyph(struct VGA_Target *target, unsigned char c) {
	if (target->column >= target->width) {
		ktao_newline(target);
	}
	ktao_set(target, ktao_index(target, target->column++, target->height-1), make_vga_entry(c, target->color));
}

void ktao_setColor(struct VGA_Target *target, uint8_t fg, uint8_t bg) {
	target->color = make_vga_color(fg, bg);
}


int hexdigitvalue(char c) {
	return c >= '0' && c <= '9' ? c - '0' : (
		c >= 'a' && c <= 'f' ? c - 'a' + 10 : (
			c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0
		)
	);
}

int ktao_print1(struct VGA_Target *target, const char *string) {
	if (string[0] == 0x1B && (string[1] == 'r' || string[1] == 'i')) {		
		if (string[1] == 'r') {
			target->color = target->defaultcolor;
		} else if (string[1] == 'i') {
			target->color ^= 0b10000000;
		}
		
		return 2;
	} else if (string[0] == 0x1B && (string[1] == 'f' || string[1] == 'b') && string[2] != 0x00) {
		uint8_t value = hexdigitvalue(string[2]);
		
		if (string[1] == 'f') {
			target->color = make_vga_color(value, target->color >> 4);
		} else if (string[1] == 'b') {
			target->color = make_vga_color(target->color, value);
		}
		
		return 3;
	} else if (string[0] == 0x1B && (string[1] == 'g' || string[1] == 'c') && string[2] != 0x00 && string[3] != 0x00) {
		uint8_t value = (hexdigitvalue(string[2]) << 4) + hexdigitvalue(string[3]);
		
		if (string[1] == 'g') ktao_putGlyph(target, value);
		else if (string[1] == 'c') target->color = value;
		
		return 4;
	} else if (string[0] == 0x00) {
		return 0;
	} else if (string[0] == 0x08) {
		if (target->column > 0) target->column--;
	} else if (string[0] == 0x0A) {
		ktao_newline(target);
	} else if (string[0] > 0x20 && string[0] != 0x7F) {
		ktao_putGlyph(target, string[0]);
	} else {
		ktao_putGlyph(target, 0x0);
	}
	return 1;
}

void ktao_print(struct VGA_Target *target, const char *string) {
	while (*string != '\0') string += ktao_print1(target, string);
}

void ktao_println(struct VGA_Target *target,const char *string) {
	ktao_print(target, string);
	ktao_newline(target);
}

void ktao_printptr(struct VGA_Target *target, void *ptr) {
	char nb[20];
	ultos((unsigned long) ptr, 16, nb, 20);
	ktao_print(target, "0x");
	ktao_print(target, nb);
}

void ktao_printulong(struct VGA_Target *target, unsigned long num) {
	char nb[20];
	ultos(num, 10, nb, 20);
	ktao_print(target, nb);
}
void ktao_printlong(struct VGA_Target *target, long num) {
	char nb[20];
	ltos(num, 10, nb, 20);
	ktao_print(target, nb);
}

int ktao_vprintf1(struct VGA_Target *target, const char *format, va_list va) {
	if (format[0] == '%') {
		if (format[1] == 'i') {
			ktao_printlong(target, va_arg(va, long));
		} else if (format[1] == 'u') {
			ktao_printulong(target, va_arg(va, unsigned long));
		} else if (format[1] == 'p') {
			ktao_printptr(target, va_arg(va, void*));
		} else if (format[1] == 's') {
			ktao_print(target, va_arg(va, char*));
		} else {
			return 1 + ktao_print1(target, format);
		}
		return 2;
	} else {
		return ktao_print1(target, format);
	}
}


void ktao_vprintf(struct VGA_Target *target, const char *format, va_list va) {
	while (*format != '\0') format += ktao_vprintf1(target, format, va);
}

void ktao_printf(struct VGA_Target *target, const char *format, ...) {
	va_list va;
	va_start(va, format);
	ktao_vprintf(target, format, va);
	va_end(va);
}
