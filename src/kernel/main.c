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

void printUnescapedString(struct VGA_Target *target, char *string) {
	char escapedCommandbuff[80];
	escapedCommandbuff[0] = '\0';
	for (int i = 0, j = 0; string[i] != '\0'; j++) {
		if (string[i] == '\\') {
			i++;
			if (string[i] == 'e') {
				escapedCommandbuff[j] = '\e';
			} else if (string[i] == 'b') {
				escapedCommandbuff[j] = '\b';
			} else if (string[i] == 'n') {
				escapedCommandbuff[j] = '\n';
			} else if (string[i] == 't') {
				escapedCommandbuff[j] = '\t';
			} else {
				escapedCommandbuff[j] = string[i];
			}
			i++;
		} else {
			escapedCommandbuff[j] = string[i++];
		}
		escapedCommandbuff[j+1] = '\0';
	}
	ktao_println(target, escapedCommandbuff);
}

void commandPrompt(struct VGA_Target *target, char *buffer, int len) {
	int historyCursor = commandMemory_head;
	
	int cursor = 0;
	
	buffer[cursor] = '\0';
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
				
				ktao_print(target, " ");
				for (int i = 0; i < strlen(buffer)+1; i++) {
					ktao_print(target, "\b \b");
				}
				memcpy(buffer, commandMemory[historyCursor], len);
				ktao_print(target, buffer);
				ktao_print(target, "\ei_\ei\b");
				cursor = strlen(buffer);
			} else if (keycode == 0x08 && cursor > 0) {
				ktao_print(target, "\b\ei_\ei \b\b");
				cursor--;
			} else if (keycode != '\n' && keycode != 0x08 && keycode != 0x00 && cursor < len) {
				if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
					keycode = keyboard_getCapital(keycode);
				}
				buffer[cursor++] = keycode;
				ktao_putGlyph(target, keycode);
				ktao_print(target, "\ei_\ei\b");
			}
			buffer[cursor] = '\0';
		}
	}
	
	memcpy(commandMemory[commandMemory_head], buffer, len);
	commandMemory_head = mod(commandMemory_head + 1, COMMANDMEM_LENGTH);
	commandMemory[commandMemory_head][0] = '\0';
	
	ktao_putGlyph(target, 0);
	ktao_newline(target);
}

void flipt_printBytecode(struct VGA_Target *target, uint8_t *compiled) {
	for (int i = 0; compiled[i] != OP_END || compiled[i+1] != OP_END;) {
		uint8_t op = compiled[i];
		int n = 1;
		
		if (
			op == OP_PUSHB || op == OP_PUSHBN || op == OP_PUSHOFFSETB
			|| op == OP_VALUEOFLB || op == OP_BINDLB || op == OP_UNBINDLB || op == OP_REBINDLB
		) n = 2;
		else if (
			op == OP_PUSHS || op == OP_PUSHSN || op == OP_SKIP || op == OP_PUSHOFFSETS
			|| op == OP_VALUEOFLS || op == OP_BINDLS || op == OP_UNBINDLS || op == OP_REBINDLS
		) n = 3;
		else if (
			op == OP_PUSHW || op == OP_PUSHWN || op == OP_PUSHOFFSETW
			|| op == OP_VALUEOFLW || op == OP_BINDLW || op == OP_UNBINDLW || op == OP_REBINDLW
		) n = 5;
		else if (op == OP_PUSHSTRLB) n = *(uint8_t *) (compiled+i+1) + 2;
		else if (op == OP_PUSHSTRLS) n = *(uint16_t *) (compiled+i+1) + 3;
		else if (op == OP_PUSHSTRLW) n = *(uint32_t *) (compiled+i+1) + 5;
		
		ktao_printf(target, "[%i - %i] %s ", i, i+n-1, flipt_opNames[op]);
		i++; n--;
		
		if (op == OP_PUSHSTRLB) {
			i++; n--;
		} else if (op == OP_PUSHSTRLS) {
			i+=2; n-=2;
		} else if (op == OP_PUSHSTRLW) {
			i+=4; n-=4;
		} else if (
			op == OP_SKIP || op == OP_PUSHS || op == OP_PUSHSN || op == OP_PUSHOFFSETS
			|| op == OP_VALUEOFLS || op == OP_BINDLS || op == OP_UNBINDLS || op == OP_REBINDLS
		) {
			ktao_printf(target, "%u, ", *(uint16_t *) (compiled+i));
			i+=2; n-=2;
		} else if (
			op == OP_PUSHW || op == OP_PUSHWN || op == OP_PUSHOFFSETW
			|| op == OP_VALUEOFLW || op == OP_BINDLW || op == OP_UNBINDLW || op == OP_REBINDLW
		) {
			ktao_printf(target, "%u, ", *(uint32_t *) (compiled+i));
			i+=4; n-=4;
		}
		
		while (n-- > 0) {
			ktao_printf(target, "%u, ", (unsigned int) compiled[i]);
			i++;
		}
		ktao_printf(target, "\n");
	}
}

extern int flipt_globalStack[256];
extern int flipt_globalStack_top;

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) kernelpanic("Multiboot magic number bad");
	if(!(mbd->flags >> 6 & 0x1)) kernelpanic("Multiboot header bad");
	
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));

    if(X86_OK != x86_pc_init()) kernelpanic("Kernel initialisation failed");
	
	plat_hide_cursor();
	
	ktao_clearAll();
	ktao_initialiseToGlobalVGA(&sidebarTarget, PC_VGA_WIDTH-22,0, 22,PC_VGA_HEIGHT);
	ktao_initialiseToGlobalVGA(&mainTarget, 0,0, PC_VGA_WIDTH-22-2,PC_VGA_HEIGHT);
    
    ktao_printf(&mainTarget, "systemTick initialised to %i hz\n\n", SYSTEM_TICKS_PER_SEC);
	
	test_print(&sidebarTarget);

    ktao_print(&mainTarget, "type commands in the prompt below.\n\n");

	while (1) {
		char commandbuff[mainTarget.width-2];
		commandPrompt(&mainTarget, commandbuff, mainTarget.width-3);
		
		flipt_globalStack_top = 0;
		uint8_t compiled[128];

		flipt_compile(commandbuff, (void *) compiled);
		//flipt_printBytecode(&mainTarget, compiled);
		
		flipt_interpret(compiled);
		ktao_print(&mainTarget, "= ");
		for (int i = 0; i < flipt_globalStack_top; i++) {
			ktao_printf(&mainTarget, "%i, ", flipt_globalStack[i]);
		}
		ktao_print(&mainTarget, "\n");
	}
}
