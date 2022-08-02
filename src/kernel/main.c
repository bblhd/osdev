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



char *flipt_opNames[64];

void flipt_printBytecode(uint8_t *compiled) {
	for (int i = 0; compiled[i] != OP_END || compiled[i+1] != OP_END;) {
		
		int pos = i;
		
		uint8_t op = compiled[i] & 0b111111;
		uint8_t argsize = compiled[i] >> 6;
		i++;
		
		unsigned int value = 0;
		if (argsize == 0b01) value = *(uint8_t *) (compiled+i);
		else if (argsize == 0b10) value = *(uint16_t *) (compiled+i);
		else if (argsize == 0b11) value = *(uint32_t *) (compiled+i);
		
		if (argsize != 0b00) i += 1 << (argsize-1);
		
		kterm_printf("%i: %i|%s[%u] ", compiled + pos, op, flipt_opNames[op], value);
		
		if (op == OP_PUSHSTR) {
			while (value-- > 0) {
				kterm_printf("%u, ", (unsigned int) compiled[i]);
				i++;
			}
		}
		
		kterm_print("\n");
	}
}

uint8_t globalFunctionSpace[512];
int globalFunctionSpace_top = 0;

void cacheFliptFunctions(uint8_t *program, int len) {
	for (int i = flipt_globalNamespace_top-1; i >= 0; i--) {
		uint8_t *start = (uint8_t *) (flipt_globalNamespace_values[i]);
		if (start >= program && start < program + len) {
			uint8_t *instr = start;
			while (*instr != OP_END) {
				uint8_t op = *instr & 0b111111;
				uint8_t argsize = *instr >> 6;
				
				unsigned int v = 0;
				if (argsize == 1) v = *(uint8_t *) (instr+1);
				else if (argsize == 2) v = *(uint16_t *) (instr+1);
				else if (argsize == 3) v = *(uint32_t *) (instr+1);
				
				instr += 1
					+ (argsize > 0 ? 1 << (argsize-1) : 0)
					+ (op == OP_PUSHSTR || op == OP_START ? v : 0);
			}
			flipt_globalNamespace_values[i] = (int) (globalFunctionSpace + globalFunctionSpace_top);
			
			for (uint8_t *i = start; i < instr; i++) {
				globalFunctionSpace[globalFunctionSpace_top++] = *i;
			}
			
			globalFunctionSpace[globalFunctionSpace_top++] = OP_END;
		}
	}
}

void runCommand(char *command) {
	uint8_t compiled[128];

	flipt_compile(command, (void *) compiled);
	int stack[256];
	int *stackpointer = (int*)stack;
	flipt_interpret(compiled, &stackpointer);
	
	//kterm_print("= ");
	//for (int i = 0; i < 256; i++) {
		//kterm_printf("%i, ", stack[i]);
	//}
	
	cacheFliptFunctions(compiled, 128);
}

void kterm_print1(char);

void ktermPrint1Int(int x) {
	kterm_print1(x & 0xff);
}

int flipt_memorymapStorage[96];

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
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
	flipt_external("memterm", (int) flipt_memorymapStorage);

    if(X86_OK != x86_pc_init()) kernelpanic("Kernel initialisation failed");
		
	kterm_print("\eb4\efe");
	kterm_clear();
	
    kterm_printf("systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	
	test_print();
	kterm_newlineSoft();
	test_printMemoryMap(&multibootStorage);

    kterm_print("type commands in the prompt below.\n\n");
    
    //flipt_setupOpNames(flipt_opNames);
    flipt_setOutputFunction(ktermPrint1Int);
    
	runCommand("((..)(.)??):print");
	runCommand("(0|(..)(..10%48+|10/)??#):tostring");

	while (1) {
		kprompt_prompt(runCommand);
	}
}
