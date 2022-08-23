// credit - https://stackoverflow.com/questions/61124564/convert-scancodes-to-ascii
// https://wiki.osdev.org/PS/2_Keyboard

#include <keyboard.h>

char keyboard_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
  '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
  0, '\\',
  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0, 
};

bool keyboard_downMap[128];

struct {
	bool caps, num, scroll;
	bool caps_r, num_r, scroll_r;
} keyboard_locks;

unsigned char us_scancode_directory[] = {
	1, //escape
	
	14, //backspace
	15, //tab
	57, //spacebar
	28, //enter
	
	72, //up arrow
	80, //down arrow
	75, //left arrow
	77, //right arrow
	
	82, //insert
	83, //delete
	
	71, //home key
	79, //end key
	
	73, //page up
	81, //page down
	
	59, //f1
	60, //f2
	61, //f3
	62, //f4
	63, //f5
	64, //f6
	65, //f7
	66, //f8
	67, //f9
	68, //f10
	87, //f11
	88, //f12
	
	58, //capslock
	69, //num lock
	70, //scroll lock
	
	29, //control
	
	42, //left shift
	54, //right shift
	
	91, //left meta
	92, //right meta
	
	56, //alt
	
	94, //power?
};

char keycodeFromScancode(uint8_t scancode) {
	return scancode < 128 ? keyboard_us[scancode] : 0;
}

char keyboard_getCapital(char c) {
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 'A';
	} else switch (c) {
		case '`': return '~';
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		case '0': return ')';
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case '\\': return '|';
		case ';': return ':';
		case '\'': return '"';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
		default: return c;
	}
}

#define POLL_QUEUE_SIZE 16
uint8_t pollStorage[POLL_QUEUE_SIZE];
unsigned int pollHead = 0;
unsigned int pollTail = 0;

unsigned int cyclic(unsigned int *value) {
	int returnvalue = *value;
	(*value)++;
	*value %= POLL_QUEUE_SIZE;
	return returnvalue;
}

void keyboard_keypress(uint8_t scancode) {
	if ((pollHead + 1) % POLL_QUEUE_SIZE != pollTail) {
		pollStorage[cyclic(&pollHead)] = scancode;
	}
	if (!keyboard_locks.caps && scancode == us_scancode_directory[SCANCODED_CAPS]) {
		keyboard_locks.caps = 1;
		keyboard_locks.caps_r = 1;
	} else if (!keyboard_locks.num && scancode == us_scancode_directory[SCANCODED_NUML]) {
		keyboard_locks.num = 1;
		keyboard_locks.num_r = 1;
	} else if (!keyboard_locks.scroll && scancode == us_scancode_directory[SCANCODED_SCROL]) {
		keyboard_locks.scroll = 1;
		keyboard_locks.scroll_r = 1;
	}
	keyboard_downMap[scancode] = 1;
}

void keyboard_keyrelease(uint8_t scancode) {
	if (!keyboard_locks.caps_r && scancode == us_scancode_directory[SCANCODED_CAPS]) {
		keyboard_locks.caps = 0;
	} else if (!keyboard_locks.num_r && scancode == us_scancode_directory[SCANCODED_NUML]) {
		keyboard_locks.num = 0;
	} else if (!keyboard_locks.scroll_r && scancode == us_scancode_directory[SCANCODED_SCROL]) {
		keyboard_locks.scroll = 0;
	}
	keyboard_locks.caps_r = 0;
	keyboard_locks.num_r = 0;
	keyboard_locks.scroll_r = 0;
	keyboard_downMap[scancode] = 0;
}

bool isExtendedKey = 0;

void keyboard_sendKeyEvent(uint8_t scancode) {
	if (scancode == 0xE0) {
		isExtendedKey = 1;
	} else {
		if (scancode < 0x80) {
			keyboard_keypress(scancode & 0b1111111);
	    } else {
			keyboard_keyrelease(scancode & 0b1111111);
	    }
		if (isExtendedKey) isExtendedKey = 0;
	}
}

int keyboard_open() {
	return pollHead != pollTail;
}

uint8_t keyboard_get() {
	if (keyboard_open()) {
		return pollStorage[cyclic(&pollTail)];
	}
	return 0;
}

int keyboard_down(uint8_t scancode) {
	return keyboard_downMap[scancode & 0b1111111];
}

bool keyboard_modifier(enum keyboard_modifier_name n) {
	switch (n) {
		case 0: return keyboard_down(us_scancode_directory[SCANCODED_CTRL]);
		case 1: return keyboard_down(us_scancode_directory[SCANCODED_ALT]);
		case 2: return keyboard_down(us_scancode_directory[SCANCODED_LSHIFT]) || keyboard_down(us_scancode_directory[SCANCODED_RSHIFT]);
		case 3: return keyboard_down(us_scancode_directory[SCANCODED_LMETA]) || keyboard_down(us_scancode_directory[SCANCODED_RMETA]);
		case 4: return keyboard_locks.caps;
		case 5: return keyboard_locks.num;
		case 6: return keyboard_locks.scroll;
		default: return 0;
	}
}
