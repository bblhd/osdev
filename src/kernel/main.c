#include <x86.h>
#include <plat.h>
#include <ktao.h>
#include <keyboard.h>
#include <multiboot.h>
#include <test_print.h>

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

int systemTime = 0;

void sys_tick_handler(x86_iframe_t* frame) {
	systemTime++;
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
    //for (int i = 2; i < 16; i++) irq_register_handler(i, sys_other_handler);

    return X86_OK;
}

void test_print(struct VGA_Target *target) {
	ktao_println(target, "   .--------------.   ");
	ktao_println(target, "   |.------------.|   ");
	ktao_println(target, "   ||            ||   ");
	ktao_println(target, "   ||            ||   ");
	ktao_println(target, "   ||            ||   ");
	ktao_println(target, "   ||            ||   ");
	ktao_println(target, "   |+------------+|   ");
	ktao_println(target, "   +-..--------..-+   ");
	ktao_println(target, "   .--------------.   ");
	ktao_println(target, "  / /============\\ \\  ");
	ktao_println(target, " / /==============\\ \\ ");
	ktao_println(target, "/____________________\\");
	ktao_println(target, "\\____________________/");
	
	ktao_newline(target);
	
	ktao_println(target, "Hello there!");
	ktao_println(target, "Welcome to my WIP\noperating system!");
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

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) kernelpanic("Multiboot magic number bad");
	if(!(mbd->flags >> 6 & 0x1)) kernelpanic("Multiboot header bad");
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));

    if(X86_OK != x86_pc_init()) kernelpanic("Kernel initialisation failed");
	
	plat_hide_cursor();
	
	ktao_clearAll();
	ktao_initialiseToGlobalVGA(&sidebarTarget, PC_VGA_WIDTH-22,0, 22,PC_VGA_HEIGHT);
	ktao_initialiseToGlobalVGA(&mainTarget, 0,0, PC_VGA_WIDTH-22-2,PC_VGA_HEIGHT);
    
	ktao_print(&mainTarget, "kernel initialisation:\n");
    ktao_print(&mainTarget, "  GDT installed\n");
    ktao_print(&mainTarget, "  IDT installed\n");
    ktao_printf(&mainTarget, "  i8253 (PIT) initialized @%i hz\n", SYSTEM_TICKS_PER_SEC);
    ktao_print(&mainTarget, "  i8259 (PIC) initialized\n");
	
	test_print(&sidebarTarget);

    ktao_printf(&mainTarget, "type commands in the prompt below.\n\n");

    while (1) {
		int historyCursor = commandMemory_head;
		char commandbuff[80];
		int commandcursor = 0;
		commandbuff[commandcursor] = '\0';
		ktao_print(&mainTarget, "> \ei_\ei\b");
		char keycode;
		do {
			keycode = 0;
			
			asm volatile ("hlt");
			if (keyboard_open()) {
				uint8_t scancode = keyboard_get();
				keycode = keycodeFromScancode(scancode);
				
				if (scancode == us_scancode_directory[SCANCODED_UP] || scancode == us_scancode_directory[SCANCODED_DOWN]) {
					int did = 1;
					if (scancode == us_scancode_directory[SCANCODED_UP]) {
						if (mod(historyCursor - 1, COMMANDMEM_LENGTH) != commandMemory_head) {
							historyCursor = mod(historyCursor - 1, COMMANDMEM_LENGTH);
						}
					} else if (scancode == us_scancode_directory[SCANCODED_DOWN]) {
						if (historyCursor != commandMemory_head) {
							historyCursor = mod(historyCursor + 1, COMMANDMEM_LENGTH);
						}
					} else did = 1;
					if (did == 1) {
						ktao_print(&mainTarget, " ");
						for (int i = 0; i < strlen(commandbuff)+1; i++) {
							ktao_print(&mainTarget, "\b \b");
						}
						memcpy(commandbuff,commandMemory[historyCursor],80);
						ktao_print(&mainTarget, commandbuff);
						ktao_print(&mainTarget, "\ei_\ei\b");
						commandcursor = strlen(commandbuff);
					}
				} else if (keycode == 0x08 && commandcursor > 0) {
					ktao_print(&mainTarget, "\b\ei_\ei \b\b");
					commandcursor--;
				} else if (keycode != '\n' && keycode != 0x08 && keycode != 0x00 && mainTarget.column < mainTarget.width-1) {
					if (keyboard_modifier(MODIFIER_SHIFT) || keyboard_modifier(MODIFIER_CAPS)) {
						keycode = keyboard_getCapital(keycode);
					}
					ktao_putGlyph(&mainTarget, keycode);
					ktao_print(&mainTarget, "\ei_\ei\b");
					commandbuff[commandcursor++] = keycode;
				}
				commandbuff[commandcursor] = '\0';
			}
			
		} while (keycode != '\n');
		
		memcpy(commandMemory[commandMemory_head], commandbuff, 80);
		commandMemory_head = mod(commandMemory_head + 1, COMMANDMEM_LENGTH);
		commandMemory[commandMemory_head][0] = '\0';
		
		ktao_putGlyph(&mainTarget, 0);
		ktao_newline(&mainTarget);
		
		if (streq(commandbuff, "help")) {
			ktao_println(&mainTarget, "help: prints this dialogue");
			ktao_println(&mainTarget, "reboot: reboots the computer");
			ktao_println(&mainTarget, "map: prints out memory map");
			ktao_println(&mainTarget, "chars: prints out all characters");
			ktao_println(&mainTarget, "colors: prints out color test");
			ktao_println(&mainTarget, "print <message>: prints message");
			ktao_println(&mainTarget, "sideprint <message>: prints message to sidebar");
			ktao_println(&mainTarget, "scancode: gets 1 keypress and prints scancode");
		} else if (streq(commandbuff, "map")) {
			test_printMemoryMap(&mainTarget, &multibootStorage);
		} else if (streq(commandbuff, "chars")) {
			test_printAllChars(&mainTarget);
		} else if (streq(commandbuff, "colors")) {
			test_printAllColors(&mainTarget);
		} else if (streq(commandbuff, "reboot")) {
			plat_reboot();
		} else if (streqn(commandbuff, "print ", 7)) {
			printUnescapedString(&mainTarget, commandbuff+6);
		} else if (streqn(commandbuff, "sideprint ", 11)) {
			printUnescapedString(&sidebarTarget, commandbuff+10);
		} else if (streq(commandbuff, "scancode")) {
			while (keyboard_open()) keyboard_get();
		    asm volatile ("hlt");
			while (!keyboard_open());
			while (keyboard_open()) ktao_printf(&mainTarget, "%i\n", keyboard_get());
		} else {
			ktao_println(&mainTarget, "Not a valid command, try help");
		}
	}
}
