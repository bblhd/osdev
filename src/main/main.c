#include <x86.h>
#include <multiboot.h>
#include <interrupts.h>
#include <threads.h>
#include <pic.h>
#include <keyboard.h>

#include <std.h>
#include <string.h>
#include <math.h>

#include <kterm.h>
#include <test_print.h>

#define SYSTEM_TICKS_PER_SEC 1000
#define X86_OK 0
#define APP_PRIORITY 16

int systemTick = 0;
void sys_tick_handler(x86_iframe_t* frame) {
	DISREGARD(frame);
	systemTick++;
	pic_send_EOI(IRQ_PIT);
}

void keyboard_sendKeyEvent(uint8_t scancode);
void sys_key_handler(x86_iframe_t* frame){
	DISREGARD(frame);
	keyboard_sendKeyEvent(in8(0x60));
}

int x86_pc_init() {
	gdt_install_flat();
	setup_idt();
	pit_init(SYSTEM_TICKS_PER_SEC);
	pic_init();
	x86_enable_int();
	
	irq_register_handler(0, sys_tick_handler);
	irq_register_handler(1, sys_key_handler);
	
	return X86_OK;
}

struct multiboot_info multibootStorage;

void kterm_print1(char x);

uint8_t taskA_stack[4096];
void taskA_main() {
	while (1) {
		threads_wait(100);
		while (keyboard_open()) {
			unsigned char keycode = keycodeFromScancode(keyboard_get());
			if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
				keycode = keyboard_getCapital(keycode);
			}
			if (keycode == '`') {
				kterm_print("\nrebooting");
				plat_reboot();
				threads_close();
			} else if (keycode > 0) {
				kterm_print1(keycode);
			}
		}
	}
	threads_close();
}

uint8_t taskB_stack[4096];
void taskB_main() {
	while (1) {
		kterm_print("\nbing bong");
		threads_wait(5000);
	}
	kterm_print("\nthread b closed");
	threads_close();
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
	
	threads_create(taskA_main, taskA_stack+4096-1);
	threads_create(taskB_main, taskB_stack+4096-1);
	
	threads_begin();
}
