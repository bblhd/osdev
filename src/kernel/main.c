#include <x86.h>
#include <plat.h>
#include <keyboard.h>
#include <multiboot.h>

#include <kterm.h>
#include <kprompt.h>
#include <test_print.h>
#include <flipt.h>

#include <string.h>
#include <math.h>

void kernelpanic(char *message);

#define SYSTEM_TICKS_PER_SEC 10
#define X86_OK 0
#define APP_PRIORITY 16

#define IDLE_STACK_SIZE_BYTES 1024*32
#define APP_STACK_SIZE 1024*32 

static uint8_t idle_thread_stack[IDLE_STACK_SIZE_BYTES];
static uint8_t app_stack[APP_STACK_SIZE];

int systemTick = 0;

void sys_tick_handler(x86_iframe_t* frame) {
	systemTick++;
	pic_send_EOI(IRQ_PIT);
}

void keyboard_sendKeyEvent(uint8_t scancode);
void sys_key_handler(x86_iframe_t* frame){
	//scancode https://wiki.osdev.org/PS/2_Keyboard
	keyboard_sendKeyEvent(in8(0x60));
}

void sys_other_handler(x86_iframe_t* frame){
	kterm_printf("IRQ %i\n", frame->vector-32);
}

void jump_usermode();
void test_user_function() {
	kterm_printf("Usermode\n");
	asm volatile ("cli");
}

void *get_esp(void);
void *kernelGlobalStackPointer;

int x86_pc_init() {
    gdt_install_flat();
    setup_idt();
    pit_init(SYSTEM_TICKS_PER_SEC);
    pic_init();
    x86_enable_int();

    irq_register_handler(0, sys_tick_handler);
    irq_register_handler(1, sys_key_handler);
    for (int i = 2; i < 16; i++) irq_register_handler(i, sys_other_handler);

    return X86_OK;
}

struct multiboot_info multibootStorage;

void flipt_printBytecode(uint8_t *compiled) {
	for (int i = 0; compiled[i] != OP_END || compiled[i-1] != OP_END;) {
		int pos = i;
		if (compiled[i] >= 0b10000000) {
			uint8_t ext_op = compiled[i] & 0b111;
			unsigned int value = 0;
			
			if (compiled[i] >> 5 == 0b111) {
				uint8_t argsize = (compiled[i] >> 3) & 0b11;
				
				i += 1;
				
				if (argsize == 0b00) value = *(uint8_t *) (compiled+i);
				else if (argsize == 0b01) value = *(uint16_t *) (compiled+i);
				else if (argsize == 0b10) value = *(uint32_t *) (compiled+i);
				
				i += 1<<argsize;
			} else {
				value = (compiled[i] >> 3) & 0b1111;
				i += 1;
			}
			
			kterm_printf("%i: e%u %u ", pos, ext_op, value);
		} else {
			uint8_t op = compiled[i] & 0b1111111;
			
			kterm_printf("%i: %u ", pos, op);
			
			i += 1;	
			
			if (op == OP_STRING) {
				while (compiled[i] != '\0') {
					kterm_printf("%c", compiled[i]);
					i++;
				}
			}		
		}
		kterm_print("\n");
	}
}

char fliptNameIndex[128] = {0};

void runCommand(char *command) {
	uint8_t compiled[256];

	flipt_compile(command, compiled, fliptNameIndex);
	
	//flipt_printBytecode(compiled);
	int stack[256];
	int *stackpointer = (int*)stack;
	flipt_interpret(compiled, &stackpointer, NULL);
	
	kterm_newlineSoft();
	kterm_print("= ");
	for (int *i = stack; i < stackpointer; i++) {
		kterm_printf("%i, ", *i);
	}
	//jump_usermode();
}

void kterm_print1(char);

void ktermPrint1Int(int x) {
	//asm volatile ("hlt");
	kterm_print1(x & 0xff);
}

int flipt_memorymapStorage[96];

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	kernelGlobalStackPointer = get_esp();
	
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) kernelpanic("Multiboot magic number bad");
	if(!(mbd->flags >> 6 & 0x1)) kernelpanic("Multiboot header bad");
	
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));
	
	int *writeto = flipt_memorymapStorage;
	*writeto++ = 0;
	for (unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *) (mbd->mmap_addr + i);
		*writeto++ = mmmt->addr_low;
		*writeto++ = mmmt->len_low;
		*writeto++ = mmmt->type;
		flipt_memorymapStorage[0]++;
	}
	//flipt_external("memterm", (int) flipt_memorymapStorage);

    if(X86_OK != x86_pc_init()) kernelpanic("Kernel initialisation failed");
		
	kterm_print("\eB1\eFe\eJ");
	
    kterm_printf("systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	test_printMemoryMap(&multibootStorage);
	
    kterm_print("\nHi, welcome to my WIP operating system.\n\n");

    kterm_print("Type commands in the prompt below.\n\n");
    
    //flipt_setOutputFunction(ktermPrint1Int);
    
	//runCommand("((..)(.)??#):print");
	//runCommand("(0|(..)(..10%48+|10/)??#):tostring");
	
	// {..256>}{27.71.1+...}??#
	// {..}{.}??#
	
	while (1) {
		kprompt_prompt(runCommand);
	}
}
