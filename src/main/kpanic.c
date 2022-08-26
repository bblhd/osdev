#include <stdint.h>

void kernelpanic_ultos(unsigned long num, int base, char *out, int max) {
	int i = 0;
	if (num == 0) {
		out[i++] = '0';
		out[i++] = '\0';
		return;
	}
	int j = i;
	while (num > 0 && i < max-1) {
		if (num % base >= 10) {
			out[i++] = 'a' + (num % base) - 10;
		} else {
			out[i++] = '0' + (num % base);
		}
		num /= base;
	}
	out[i--] = '\0';
	char temp;
	while (j < i) {
		temp = out[j];
		out[j++] = out[i];
		out[i--] = temp;
	}
}

uint16_t *kernelpanic_buffer = (uint16_t *) 0xB8000;
unsigned int kernelpanic_width = 80;
unsigned int kernelpanic_height = 25;
unsigned int kernelpanic_column = 0;
unsigned int kernelpanic_row = 0;

uint16_t kernelpanic_entry(unsigned char c, uint8_t bg, uint8_t fg) {
	return c | ((fg & 0b1111) << 8) | ((bg & 0b111) << 12);
}

void kernelpanic_newline() {
	kernelpanic_column = 0;
	kernelpanic_row++;
	if (kernelpanic_row >= kernelpanic_height - 1) {
		kernelpanic_row = 0;
	}
}

void kernelpanic_print(char *string) {
	if (string == (char *) 0) return;
	for (int i = 0; string[i] != '\0'; i++) {
		kernelpanic_buffer[kernelpanic_row * kernelpanic_width + kernelpanic_column] = kernelpanic_entry(string[i], 4, 15);
		kernelpanic_column++;
		if (kernelpanic_column >= kernelpanic_width - 2) kernelpanic_newline();
	}
}

int kernelpanic_debugIndex = 0;

void kernelpanic_debug1() {
	kernelpanic_debugIndex = 0;
	for (unsigned int i = 0; i < kernelpanic_width * kernelpanic_height; i++) {
		kernelpanic_buffer[i] = kernelpanic_entry(' ', 4, 15);
	}
	kernelpanic_print("Kernel debug time, woooooo:");
	kernelpanic_newline();
}

void kernelpanic_debug2(int x) {
	kernelpanic_print("[ ");
	char nbuff[10];
	kernelpanic_ultos(kernelpanic_debugIndex++, 10, nbuff, 10);
	kernelpanic_print(nbuff);
	kernelpanic_print(": ");
	kernelpanic_ultos(x, 10, nbuff, 10);
	kernelpanic_print(nbuff);
	kernelpanic_print(" ], ");
}

void kernelpanic_debug3() {
	kernelpanic_newline();
	kernelpanic_print("Kernel debugging is over for now, but theres always next time");
	asm volatile ("cli \n hlt"); //halt
}


void kernelpanic(char *message) {
	asm volatile ("cli");
	for (unsigned int i = 0; i < kernelpanic_width * kernelpanic_height; i++) {
		kernelpanic_buffer[i] = kernelpanic_entry(' ', 4, 15);
	}
	kernelpanic_print("Serious unrecoverable error, the program responsible says:");
	kernelpanic_newline();
	kernelpanic_print("  ");
	kernelpanic_print(message);
	kernelpanic_newline();
	kernelpanic_newline();
	kernelpanic_print("Take this smiley face as an apology :^)");
	
	asm volatile ("hlt");
}
void kernelpanicWithNumber(char *message, char *name, unsigned int num) {
	asm volatile ("cli");
	for (unsigned int i = 0; i < kernelpanic_width * kernelpanic_height; i++) {
		kernelpanic_buffer[i] = kernelpanic_entry(' ', 4, 15);
	}
	kernelpanic_print("Serious unrecoverable error, the program responsible says:");
	kernelpanic_newline();
	kernelpanic_print("  ");
	kernelpanic_print(message);
	kernelpanic_newline();
	kernelpanic_print("  ");
	kernelpanic_print(name);
	kernelpanic_print(" = ");
	char nbuff[33];
	kernelpanic_ultos(num, 16, nbuff, 33);
	kernelpanic_print("0x");
	kernelpanic_print(nbuff);
	kernelpanic_ultos(num, 10, nbuff, 33);
	kernelpanic_print(", ");
	kernelpanic_print(nbuff);
	kernelpanic_ultos(num, 2, nbuff, 33);
	kernelpanic_print(", 0b");
	kernelpanic_print(nbuff);
	kernelpanic_newline();
	kernelpanic_newline();
	kernelpanic_print("Take this smiley face as an apology :^)");
	
	asm volatile ("hlt");
}
