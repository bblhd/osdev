#include <x86.h>
#include <plat.h>
#include <ktao.h>
#include <keyboard.h>
#include <multiboot.h>
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

struct VGA_Target sidebarTarget;
struct VGA_Target mainTarget;

void sys_other_handler(x86_iframe_t* frame){
	ktao_printf(&sidebarTarget, "IRQ %i\n", frame->vector-32);
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

#define COMMANDMEM_LENGTH 20
char commandMemory[COMMANDMEM_LENGTH][80];
int commandMemory_head = 0;

void commandPrompt(struct VGA_Target *target, char *buffer, int len) {
	int historyCursor = commandMemory_head;
	int cursor = 0;
	buffer[cursor] = '\0';
	
	if (mainTarget.column > 0) ktao_print(&mainTarget, "\n");
	
	ktao_print(target, "> \ei_\ei\b");
	char keycode = 0;
	
	while (keycode != '\n') {
		asm volatile ("hlt"); //halts to give the cpu a rest
		if (keyboard_open()) {
			uint8_t scancode = keyboard_get();
			keycode = keycodeFromScancode(scancode);
			
			if (scancode == us_scancode_directory[SCANCODED_UP] || scancode == us_scancode_directory[SCANCODED_DOWN]) {
				if (scancode == us_scancode_directory[SCANCODED_UP]) {
					if (mod(historyCursor - 1, COMMANDMEM_LENGTH) != commandMemory_head) {
						historyCursor = mod(historyCursor - 1, COMMANDMEM_LENGTH);
					}
				} else if (scancode == us_scancode_directory[SCANCODED_DOWN]) {
					if (historyCursor != commandMemory_head) {
						historyCursor = mod(historyCursor + 1, COMMANDMEM_LENGTH);
					}
				}
				
				memcpy(buffer, commandMemory[historyCursor], len);
			} else if (scancode == us_scancode_directory[SCANCODED_RIGHT]) {
				if (cursor < len && buffer[cursor] != '\0') cursor++;
			} else if (scancode == us_scancode_directory[SCANCODED_LEFT]) {
				if (cursor > 0) cursor--;
			} else if (keycode == 0x08) {
				if (cursor > 0) cursor--;
				buffer[cursor] = '\0';
			} else if (keycode == 0x1B) {
				plat_reboot();
			} else if (keycode != '\n' && keycode != 0x00 && cursor < len) {
				if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
					keycode = keyboard_getCapital(keycode);
				}
				int i = strlen(buffer)+1;
				for (; i > cursor; i--) {
					buffer[i] = buffer[i-1];
				}
				buffer[cursor++] = keycode;
			}
			
			ktao_clearBottomRow(target);
			ktao_print(target, "> ");
			int i = 0;
			for (; buffer[i] != '\0' && i < cursor; i++) {
				ktao_putGlyph(target, buffer[i]);
			}
			ktao_print(target, "\ei_\ei");
			for (; buffer[i] != '\0' && i < len; i++) {
				ktao_putGlyph(target, buffer[i]);
			}
		}
	}
	
	memcpy(commandMemory[commandMemory_head], buffer, len);
	commandMemory_head = mod(commandMemory_head + 1, COMMANDMEM_LENGTH);
	commandMemory[commandMemory_head][0] = '\0';
	
	ktao_putGlyph(target, 0);
	ktao_newline(target);
}

char *flipt_opNames[64];

void flipt_printBytecode(struct VGA_Target *target, uint8_t *compiled) {
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
		
		ktao_printf(target, "%i: %i|%s[%u] ", compiled + pos, op, flipt_opNames[op], value);
		
		if (op == OP_PUSHSTR) {
			while (value-- > 0) {
				ktao_printf(target, "%u, ", (unsigned int) compiled[i]);
				i++;
			}
		}
		
		ktao_printf(target, "\n");
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
	flipt_globalStack_top = 0;
	uint8_t compiled[128];

	flipt_compile(command, (void *) compiled);
	flipt_interpret(compiled);
	cacheFliptFunctions(compiled, 128);
}

int ktao_print1(struct VGA_Target *target, const char *string);

char mainTargetPutchar_buffer[6];
int mainTargetPutchar_i = 0;

void mainTargetPutchar(int c) {
	mainTargetPutchar_buffer[mainTargetPutchar_i++] = c;
	mainTargetPutchar_buffer[mainTargetPutchar_i] = '\0';
	
	int i = mainTargetPutchar_i;
	char *b = mainTargetPutchar_buffer;
	
	if (
		i == 1 && b[0] != 0x1b
		|| i==2 && b[1] == 'i' && b[1] == 'r'
		|| i==3 && b[1] == 'f' && b[1] == 'b'
		|| i==4 && b[1] == 'g' && b[1] == 'c'
	) {
		ktao_print1(&mainTarget, mainTargetPutchar_buffer);
		mainTargetPutchar_i = 0;
		mainTargetPutchar_buffer[mainTargetPutchar_i] = '\0';
	}
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
	flipt_external("memorymap", (int) flipt_memorymapStorage);

    if(X86_OK != x86_pc_init()) kernelpanic("Kernel initialisation failed");
    
	keyboard_setPowerAction(reboot);
	
	plat_hide_cursor();
	
	ktao_clearAll();
	ktao_initialiseToGlobalVGA(&sidebarTarget, PC_VGA_WIDTH-22,0, 22,PC_VGA_HEIGHT);
	ktao_initialiseToGlobalVGA(&mainTarget, 0,0, PC_VGA_WIDTH-22-2,PC_VGA_HEIGHT);
    
    ktao_printf(&mainTarget, "systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	
	test_print(&sidebarTarget);

    ktao_print(&mainTarget, "type commands in the prompt below.\n\n");
    
    flipt_setupOpNames(flipt_opNames);
    flipt_setOutputFunction(mainTargetPutchar);
    
	runCommand("((..)(.)??):print");
	runCommand("(0|(..)(..10%48+|10/)??#):tostring");

	while (1) {
		char commandbuff[mainTarget.width-2];
		commandPrompt(&mainTarget, commandbuff, mainTarget.width-3);
		runCommand(commandbuff);
	}
}
