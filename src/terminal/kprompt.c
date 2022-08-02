#include <stddef.h>
#include <stdint.h>

#include <plat.h>
#include <kterm.h>
#include <keyboard.h>

#include <math.h>
#include <string.h>

extern uint16_t * const vga_buffer;
extern const int vga_width;
extern const int vga_height;
extern uint8_t colour;

// none of this code is very good, I apologise

int messageSize = 2;
char *promptMessage = ">";

#define PROMPTMEM_LENGTH 1024
char promptMemory[PROMPTMEM_LENGTH];
char *prompts[32];
int promptMemory_bot = 0, promptMemory_top = 0;

int constrain(int i) {
	return i & 0b1111111111;
}

int getPreviousHistoryPrompt(int this) {
	if (promptMemory[constrain(this-1)] == '\0') {
		do {
			this = constrain(this-1);
		} while (promptMemory[constrain(this-1)] != '\0');
	}
	return -1;
}

int getNextHistoryPrompt(int this) {
	if (promptMemory[constrain(this-1)] == '\0') {
		do {
			this = constrain(this+1);
		} while (promptMemory[constrain(this-1)] != '\0');
	}
	return -1;
}

void setVGA(int i, unsigned char c) {
	vga_buffer[vga_width*(vga_height-1) + i] = c | (colour << 8);
}

void update_cursor(int x) {
	uint16_t pos = vga_width * (vga_height-1) + x;
 
	out8(0x3D4, 0x0F);
	out8(0x3D5, (uint8_t) (pos & 0xFF));
	out8(0x3D4, 0x0E);
	out8(0x3D5, (uint8_t) (pos >> 8 & 0x00FF));
}

void drawPromptTextTo(int cursor) {
	int width = vga_width - messageSize - 1;
	int len = cursor;
	while (promptMemory[constrain(promptMemory_top + len)] != '\0') len++;
	int start = max(0, min(len - width, cursor - (width>>1)));
	
	int i = 0;
	for (; i < width && promptMemory[constrain(promptMemory_top+start+i)] != '\0'; i++) {
		setVGA(messageSize + i, promptMemory[constrain(promptMemory_top + start + i)]);
	}
	
	for (; i < width; i++) {
		setVGA(messageSize + i, 0);
	}
	
	int visualCursor = len < width || cursor < width>>1 ? cursor : cursor > len - (width>>1) ? cursor - len + width: width>>1;
	update_cursor(visualCursor+messageSize);
}

void kprompt_prompt(void (*event)(char *)) {
	kterm_newlineSoft();
	
	int cursor = 0;
	promptMemory[constrain(promptMemory_top + cursor)] = '\0';
	update_cursor(messageSize);
	
	for (int i = 0; promptMessage[i] != '\0'; i++) {
		setVGA(i, promptMessage[i]);
	}
	
	int promptMemorySeek = promptMemory_top;
	
	char keycode = 0;
	
	while (keycode != '\n') if (keyboard_open()) {
		do {
			uint8_t scancode = keyboard_get();
			keycode = keycodeFromScancode(scancode);
			
			if (scancode == us_scancode_directory[SCANCODED_LEFT]) {
				if (cursor > 0) cursor--;
			} else if (scancode == us_scancode_directory[SCANCODED_RIGHT]) {
				if (promptMemory[constrain(promptMemory_top + cursor)] != '\0') cursor++;
			} else if (scancode == us_scancode_directory[SCANCODED_UP]) {
				if (promptMemorySeek != promptMemory_bot) {
					promptMemorySeek = getPreviousHistoryPrompt(promptMemorySeek);
				}
			} else if (scancode == us_scancode_directory[SCANCODED_DOWN]) {
				if (promptMemorySeek != promptMemory_top) {
					promptMemorySeek = getNextHistoryPrompt(promptMemorySeek);
				}
			} else if (keycode == 0x08) {
				for (int i = cursor; promptMemory[constrain(promptMemory_top + i - 1)] != '\0'; i++) {
					promptMemory[constrain(promptMemory_top + i-1)] = promptMemory[constrain(promptMemory_top + i)];
				}
				
				if (cursor > 0) cursor--;
			} else if (keycode == 0x1B) {
				plat_reboot();
			} else if (keycode != '\n' && keycode != '\0') {
				if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
					keycode = keyboard_getCapital(keycode);
				}
				
				int i = cursor;
				while (promptMemory[constrain(promptMemory_top + i)] != '\0') i++;
				
				promptMemory[constrain(promptMemory_top + i)] = '\0';
				for (; i != cursor - 1; i--) {
					promptMemory[constrain(promptMemory_top + i+1)] = promptMemory[constrain(promptMemory_top + i)];
				}
				promptMemory[constrain(promptMemory_top + cursor)] = keycode;
				cursor++;
				if (constrain(promptMemory_top+cursor) >= promptMemory_bot) {
					promptMemory_bot = getNextHistoryPrompt(promptMemory_bot);
				}
			}
		} while(keyboard_open());
		drawPromptTextTo(cursor);
	} else {
		asm volatile ("hlt"); //halts to give the cpu a rest
	}
	
	drawPromptTextTo(0);
	
	int width = vga_width - messageSize - 1;
	int len = cursor;
	while (promptMemory[constrain(promptMemory_top + len)] != '\0') len++;
	if (len > width) {
		setVGA(vga_width - 1, '.');
		setVGA(vga_width - 2, '.');
		setVGA(vga_width - 3, '.');
	}
	
	promptMemory_top = getNextHistoryPrompt(promptMemory_top);
	if (promptMemory_top > promptMemory_bot) promptMemory_bot = getNextHistoryPrompt(promptMemory_bot);
	
	kterm_newline();
}
