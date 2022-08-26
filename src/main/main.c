#include <x86.h>
#include <multiboot.h>
#include <interrupts.h>
#include <tasks.h>
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

void systemcall(unsigned int a);
void kterm_print1(char x);

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

void usermode_main() {
	systemcall(1);
	kterm_print(" in usermode!");
	while (systemTick < 4000) kterm_print("");
	systemcall(2);
	kterm_print(", will reboot now");
	while (systemTick < 5000) kterm_print("");
	systemcall(3);
	while (1) kterm_print("");
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
	
	//tasks_begin(&mainTask);
	
	//task_create(&taskB, taskB_main, taskB_stack+4096-1);
	
	kterm_print("Hello World");
	while (systemTick < 1000) kterm_print("");
	systemcall(1);
	while (systemTick < 2000) kterm_print("");
	systemcall(2);
	while (systemTick < 3000) kterm_print("");
	enterUsermode();
	//usermode_main();
	//systemcall(1);
	//while (1) {
		//task_wait(100);
		//kterm_print("\nrebooting");
		//while (keyboard_open()) {
			//unsigned char keycode = keycodeFromScancode(keyboard_get());
			//if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
				//keycode = keyboard_getCapital(keycode);
			//}
			//if (keycode == '`') {
				//kterm_print("\nrebooting");
				//plat_reboot();
				//task_close();
			//} else if (keycode > 0) {
				//kterm_print1(keycode);
			//}
		//}
	//}
	
	//task_close();
}
