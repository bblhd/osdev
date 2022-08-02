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

typedef _Bool bool;

// none of this code is very good, I apologise

int messageSize = 2;
char *promptMessage = ">";

#define PROMPTHISTORY_LENGTH 1024
char promptHistory[PROMPTHISTORY_LENGTH];
int promptBufferStart = 0;

void removeOldestHistory() {
	int oldestLength = strlen(promptHistory);
	int bufferLength = strlen(promptHistory + promptBufferStart);
	memcpy(promptHistory, promptHistory + oldestLength, promptBufferStart + bufferLength - oldestLength);
	promptBufferStart -= oldestLength;
}


int previousHistory(int this) {
	if (this > 0) this--;
	while (this > 0 && promptHistory[this-1] != '\0') this--;
	return this;
}

int nextHistory(int this) {
	if (this < promptBufferStart) this++;
	while (this < promptBufferStart && promptHistory[this-1] != '\0') this++;
	return this;
}

void setVGA(int i, unsigned char c) {
	vga_buffer[vga_width*(vga_height-1) + i] = c | (colour << 8);
}

void update_cursor(int x) {
	uint16_t pos = vga_width * (vga_height-1) + x;
 
	out8(0x3D4, 0x0F);
	out8(0x3D5, pos & 0xff);
	out8(0x3D4, 0x0E);
	out8(0x3D5, (pos >> 8) & 0xff);
}

void drawPromptText(int cursor) {
	int width = vga_width - messageSize - 1;
	int len = strlen(promptHistory + promptBufferStart);
	
	int start = cursor - (width>>1);
	if (start > len - (width)) {
		start = len - width;
	}
	if (start < 0) {
		start = 0;
	}
	
	{
		int i = 0;
		while (i < width && promptHistory[promptBufferStart + start + i] != '\0') {
			char v = promptHistory[promptBufferStart + start + i];
			setVGA(messageSize + i++, v);
		}
		while (i < width) {
			setVGA(messageSize + i++, 0);
		}
	}
	
	
	int visualCursor = width>>1;
	if (len < width || cursor < visualCursor) {
		visualCursor = cursor;
	} else if (cursor > len - visualCursor) {
		visualCursor = cursor - len + width;
	}
	update_cursor(messageSize + visualCursor);
}

void kprompt_prompt(void (*event)(char *)) {
	kterm_newlineSoft();
	for (int i = 0; promptMessage[i] != '\0'; i++) {
		setVGA(i, promptMessage[i]);
	}
	update_cursor(messageSize);
	
	int cursor = 0;
	int promptHistorySeek = promptBufferStart;
	promptHistory[promptBufferStart] = '\0';
	
	char keycode = 0;
	while (keycode != '\n') if (keyboard_open()) {
		do {
			uint8_t scancode = keyboard_get();
			keycode = keycodeFromScancode(scancode);
			
			if (keycode >= 0x20 && keycode != 127) {
				if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
					keycode = keyboard_getCapital(keycode);
				}
				int len = strlen(promptHistory + promptBufferStart);
				
				if (promptBufferStart + cursor + len >= PROMPTHISTORY_LENGTH) {
					removeOldestHistory();
				}
				
				memcpyr(
					promptHistory + promptBufferStart + cursor + 1,
					promptHistory + promptBufferStart + cursor,
					len + 1
				);
				
				promptHistory[promptBufferStart + cursor++] = keycode;
				
			} else if (scancode == us_scancode_directory[SCANCODED_LEFT]) {
				
				if (cursor > 0) cursor--;
				
			} else if (scancode == us_scancode_directory[SCANCODED_RIGHT]) {
				
				if (promptHistory[promptBufferStart + cursor] != '\0') {
					cursor++;
				}
				
			} else if (scancode == us_scancode_directory[SCANCODED_UP] || scancode == us_scancode_directory[SCANCODED_DOWN]) {
				
				if (scancode == us_scancode_directory[SCANCODED_UP]) {
					promptHistorySeek = previousHistory(promptHistorySeek);
				} else if (scancode == us_scancode_directory[SCANCODED_DOWN]) {
					promptHistorySeek = nextHistory(promptHistorySeek);
				}
				
				if (promptHistorySeek == promptBufferStart) {
					promptHistory[promptBufferStart] = '\0';
					cursor = 0;
				} else {
					int len = strlen(promptHistory + promptHistorySeek);
					
					if (promptBufferStart + len >= PROMPTHISTORY_LENGTH) {
						removeOldestHistory();
					}
					
					memcpy(
						promptHistory + promptBufferStart,
						promptHistory + promptHistorySeek,
						len + 1
					);
					
					cursor = len;
				}
			} else if (keycode == 0x08) {
				if (cursor > 0) {
					memcpy(
						promptHistory + promptBufferStart + cursor - 1,
						promptHistory + promptBufferStart + cursor,
						strlen(promptHistory + promptBufferStart)
					);
					cursor--;
				}
			} else if (keycode == 0x1B) {
				plat_reboot();
			}
		} while(keyboard_open());
		drawPromptText(cursor);
	} else {
		asm volatile ("hlt"); //halts to give the cpu a rest
	}
	
	drawPromptText(0);
	
	int width = vga_width - messageSize - 1;
	int len = strlen(promptHistory + promptBufferStart);
	if (len > width) {
		setVGA(vga_width - 1, '.');
		setVGA(vga_width - 2, '.');
		setVGA(vga_width - 3, '.');
	}
	
	kterm_newline();
	
	event(promptHistory + promptBufferStart);
	
	promptBufferStart += len+1;
}
