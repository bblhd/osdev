#include <x86.h>
#include <multiboot.h>
#include <interrupts.h>
#include <pic.h>
#include <keyboard.h>

#include <std.h>
#include <string.h>
#include <math.h>

#include <physmem.h>

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

void enterUsermode();

void causeDivisionByZeroError() {
	volatile int x = 0;
	volatile int *ptr = &x;
	*ptr = 1 / x;
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) kernelpanic("Multiboot magic number bad");
	if(!(mbd->flags >> 6 & 0x1)) kernelpanic("Multiboot header bad");
	
	physmem_init(mbd);
	void **initialPageTable = physmem_alloc();
	
	//we will fill all 1024 entries in the table, mapping 4 megabytes
	for(size_t i = 0; i < 1024; i++) {
		// As the address is page aligned, it will always leave 12 bits zeroed.
		// Those bits are used by the attributes ;)
		//first_page_table[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
		initialPageTable++;
	}
	void *initialPageDirectory = physmem_alloc();
	
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));
	
	if(x86_pc_init() != X86_OK) kernelpanic("Kernel initialisation failed");
	
	plat_hide_cursor();
		
	kterm_print("\eB1\eFe\eJ");
	kterm_printf("systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	test_printMemoryMap(&multibootStorage);
	kterm_print("\nHi, welcome to my WIP operating system.\n\n");
}

void *get_esp();
