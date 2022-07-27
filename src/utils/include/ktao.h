#ifndef KTAO_H
#define KTAO_H

#include <stdarg.h>

#define PC_VGA_WIDTH 80
#define PC_VGA_HEIGHT 25

/* Hardware text mode color constants. */
enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_YELLOW = 14,
	COLOR_WHITE = 15,
	COLOR_DEFAULT = 0xfe
};

struct VGA_Target {
	uint16_t *buffer;
	int pitch;
	
	int width, height;
	
	int column;
	uint8_t defaultcolor;
	uint8_t color;
};

uint8_t make_vga_color(uint8_t fg, uint8_t bg);
uint16_t make_vga_entry(unsigned char character, uint8_t color);

void ktao_clearAll();
void ktao_initialiseToGlobalVGA(struct VGA_Target *target, int x, int y, int w, int h);
void ktao_default(struct VGA_Target *target);

int ktao_index(struct VGA_Target *target, int x, int y);
int ktao_isWithin(struct VGA_Target *target, size_t index);
void ktao_set(struct VGA_Target *target, size_t index, uint16_t entry);
uint16_t ktao_get(struct VGA_Target *target, size_t index);

void ktao_clearEntries(struct VGA_Target *target);
void ktao_clearBottomRow(struct VGA_Target *target);
void ktao_shiftEntries(struct VGA_Target *target, size_t delta);

void ktao_newline(struct VGA_Target *target);
void ktao_putGlyph(struct VGA_Target *target, unsigned char c);
void ktao_setColor(struct VGA_Target *target, uint8_t fg, uint8_t bg);

void ktao_print(struct VGA_Target *target, const char *string);
void ktao_println(struct VGA_Target *target,const char *string);
void ktao_vprintf(struct VGA_Target *target, const char *format, va_list va);
void ktao_printf(struct VGA_Target *target, const char *format, ...);

#endif
