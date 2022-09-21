#include <x86.h>
#include <multiboot.h>
#include <interrupts.h>
#include <tasks.h>
#include <pic.h>
#include <keyboard.h>

#include <std.h>
#include <string.h>
#include <math.h>

//#include <term.h>
#include <kterm.h>
#include <test_print.h>

#define SYSTEM_TICKS_PER_SEC 1000
#define X86_OK 0
#define APP_PRIORITY 16

int systemTick = 0;
void sys_tick_handler(struct x86_iframe* frame) {
	DISREGARD(frame);
	systemTick++;
	pic_send_EOI(IRQ_PIT);
}

void keyboard_sendKeyEvent(uint8_t scancode);
void sys_key_handler(struct x86_iframe* frame){
	DISREGARD(frame);
	keyboard_sendKeyEvent(in8(0x60));
}

int x86_pc_init() {
	gdt_install_flat();
	setup_idt();
	pit_init(SYSTEM_TICKS_PER_SEC);
	pic_init();
	
	irq_register_handler(0, sys_tick_handler);
	irq_register_handler(1, sys_key_handler);
	
	x86_enable_int();
	
	return X86_OK;
}

struct multiboot_info multibootStorage;

void systemcall0(unsigned int a);
void systemcall1(unsigned int a, unsigned int b);
void systemcall2(unsigned int a, unsigned int b, unsigned int c);
void systemcall3(unsigned int a, unsigned int b, unsigned int c, unsigned int d);

struct Task taskB;
uint8_t taskB_stack[4096];
void taskB_main() {
	while (1) {
		kterm_print("\nbing bong");
		task_wait(5000);
	}
	kterm_print("\nthread b closed");
	task_close();
}

struct Task mainTask;

void enterUsermode();

void causeDivisionByZeroError() {
	volatile int x = 0;
	volatile int *ptr = &x;
	*ptr = 1 / x;
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) kernelpanic("Multiboot magic number bad");
	if(!(mbd->flags >> 6 & 0x1)) kernelpanic("Multiboot header bad");
	
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));
	
	if(x86_pc_init() != X86_OK) kernelpanic("Kernel initialisation failed");
	
	plat_hide_cursor();
		
	kterm_print("\eB1\eFe\eJ");
	kterm_printf("systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	test_printMemoryMap(&multibootStorage);
	kterm_print("\nHi, welcome to my WIP operating system.\n\n");
	
	enterUsermode();
}

void term_print1(char c) {
	systemcall1(0x10, (unsigned int) c);
	
}

void *get_esp();

void term_print(char *string) {
	//term_print1('A');
	systemcall1(0x12, (unsigned int) get_esp());
	//if (string[0] != '\0') term_print1(string[1]);
	//systemcall1(0x12, (unsigned int) get_esp());
	//term_print1(string[0]);
	//if (*string == '\0') return;
	//string += 1;
	//term_print1('A');
	DISREGARD(string);
	//term_print1(string[0]);
	//while (*string != '\0')string++;
}


void term_printul(unsigned long num, int base, int precision) {
	if (num == 0) {
		term_print1('0');
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
		if (buffer[top] >= 10) term_print1('a' + buffer[top] - 10);
		else term_print1('0' + buffer[top]);
	}
}

void term_printl(long num, int base, int precision) {
	if (num < 0) {
		term_print1('-');
		term_printul(-num, base, precision);
	} else {
		term_printul(num, base, precision);
	}
}

void term_vprintf(const char *format, va_list va) {
	while (*format != '\0') {
		if (*format == '%') {
			format++;
			if (*format == 'i') {
				term_printl(va_arg(va, int), 10, 11);
			} else if (*format == 'u') {
				term_printul(va_arg(va, unsigned int), 10, 11);
			} else if (*format == 'p') {
				term_print1('0');
				term_print1('x');
				term_printul((unsigned int) va_arg(va, void*), 16, 9);
			} else if (*format == 's') {
				term_print(va_arg(va, char*));
			} else if (*format == 'c') {
				term_print1(va_arg(va, int));
			} else {
				term_print1(*format);
			}
		} else {
			term_print1(*format);
		}
		format++;
	}
}
void term_printf(const char *format, ...) {
	va_list va;
	va_start(va, format);
	term_vprintf(format, va);
	va_end(va);
}



void usermode_main() {
	systemcall0(0x11);
	term_print1('T');
	term_print1('e');
	term_print1('s');
	term_print1('t');
	term_print1('\n');
	term_print("in userspace");
	//term_print("in usermode!");
	while (systemTick < 1000) term_print1('\0');
	term_print1('T');
	term_print1('e');
	term_print1('s');
	term_print1('t');
	term_print1('\n');
	//systemcall0(2);
	//term_print(", will reboot now");
	//term_print("\nTest");
	//while (systemTick < 5000) kterm_print("");
	//systemcall0(0x01);
	while (1) term_print("");
}
