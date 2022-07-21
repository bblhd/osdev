#include <x86.h>
#include <plat.h>
#include <ktao.h>
#include <debugprint.h>
#include <keyboard.h>
#include <multiboot.h>
#include <test_print.h>
#include <string.h>

#define SYSTEM_TICKS_PER_SEC            20
#define X86_OK 0
#define APP_PRIORITY 16

#define IDLE_STACK_SIZE_BYTES 1024*16
#define APP_STACK_SIZE 1024*16 

static uint8_t idle_thread_stack[IDLE_STACK_SIZE_BYTES];
static uint8_t app_stack[APP_STACK_SIZE];

int mod(int x, int y) {
	while (x < 0) x += y;
	while (x >= y) x -= y;
	return x;
}

int x86_pc_init(void) {
    gdt_install_flat();
    print("  GDT installed\n");

    setup_idt();
    print("  IDT installed\n");

    pit_init(SYSTEM_TICKS_PER_SEC);
    printf("  i8253 (PIT) initialized @%d hz\n", SYSTEM_TICKS_PER_SEC);

    pic_init();
    print("  i8259 (PIC) initialized\n");

    irq_register_handler(0, sys_tick_handler);
    print("  IRQ handler set: sys_tick_handler\n");

    irq_register_handler(1, sys_key_handler);
    print("  IRQ handler set: sys_key_handler\n");

    print("\n");

    return X86_OK;
}

struct VGA_Target sidebarTarget;
struct VGA_Target mainTarget;

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

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	
	/* Make sure the magic number matches for memory mapping*/
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		//terminal_println("invalid magic number, exiting.");
		goto failure;
	}
	
	/* Check bit 6 to see if we have a valid memory map */
	if(!(mbd->flags >> 6 & 0x1)) {
		//terminal_println("invalid memory map given by GRUB bootloader, exiting.");
		goto failure;
	}
	
	memcpy(&multibootStorage, mbd, sizeof(struct multiboot_info));
	
	ktao_clearAll();
	
	ktao_initialiseToGlobalVGA(&sidebarTarget, PC_VGA_WIDTH-22,0, 22,PC_VGA_HEIGHT);
	ktao_initialiseToGlobalVGA(&mainTarget, 0,0, PC_VGA_WIDTH-22-2,PC_VGA_HEIGHT);
	setDebugOutputSource(&mainTarget);
	
	plat_hide_cursor();
	
	test_print(&sidebarTarget);

    print("kernel_main()\n");

    if(X86_OK != x86_pc_init()) goto failure;

    x86_enable_int();

    print(" \ec2fHey x86!\er");
    print("\n\nPress ESC to reboot\n\n");
    
    print("type commands in the prompt below.\n\n");

    while (1) {
		int historyCursor = commandMemory_head;
		char commandbuff[80];
		int commandcursor = 0;
		commandbuff[commandcursor] = '\0';
		ktao_print(&mainTarget, "> \ei_\ei\b");
		char keycode;
		do {
			keycode = 0;
			if (keyboard_open()) {
				uint8_t scancode = keyboard_get();
				keycode = keycodeFromScancode(scancode);
				
				if (scancode == us_scancode_directory[SCANCODED_UP]) {
					if (mod(historyCursor - 1, COMMANDMEM_LENGTH) != commandMemory_head) {
						historyCursor = mod(historyCursor - 1, COMMANDMEM_LENGTH);
						for (int i = 0; i < strlen(commandbuff)+1; i++) {
							ktao_print(&mainTarget, "\b \b");
						}
						memcpy(commandbuff,commandMemory[historyCursor],80);
						ktao_print(&mainTarget, commandbuff);
						ktao_print(&mainTarget, "\ei_\ei\b");
						commandcursor = strlen(commandbuff);
					}
				} else if (scancode == us_scancode_directory[SCANCODED_DOWN]) {
					if (historyCursor != commandMemory_head) {
						historyCursor = mod(historyCursor + 1, COMMANDMEM_LENGTH);
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
			ktao_println(&mainTarget, "scancode: gets 1 keypress of keyboard input and prints out scancode");
		} else if (streq(commandbuff, "map")) {
			test_printMemoryMap(&mainTarget, &multibootStorage);
		} else if (streq(commandbuff, "chars")) {
			test_printAllChars(&mainTarget);
		} else if (streq(commandbuff, "colors")) {
			test_printAllColors(&mainTarget);
		} else if (streq(commandbuff, "reboot")) {
			plat_reboot();
		} else if (streqn(commandbuff, "print ", 6)) {
			char escapedCommandbuff[80];
			escapedCommandbuff[0] = '\0';
			for (int i = 6, j = 0; commandbuff[i] != '\0'; j++) {
				if (commandbuff[i] == '\\') {
					i++;
					if (commandbuff[i] == 'e') {
						escapedCommandbuff[j] = '\e';
						i++;
					} else if (commandbuff[i] == 'b') {
						escapedCommandbuff[j] = '\b';
						i++;
					} else if (commandbuff[i] == 'n') {
						escapedCommandbuff[j] = '\n';
						i++;
					} else if (commandbuff[i] == 't') {
						escapedCommandbuff[j] = '\t';
						i++;
					} else {
						escapedCommandbuff[j] = commandbuff[i++];
					}
				} else {
					escapedCommandbuff[j] = commandbuff[i++];
				}
				escapedCommandbuff[j+1] = '\0';
			}
			ktao_println(&mainTarget, escapedCommandbuff);
		} else if (streq(commandbuff, "scancode")) {
			while (keyboard_open()) keyboard_get();
			while (!keyboard_open());
			while (keyboard_open()) ktao_printf(&mainTarget, "%i\n", keyboard_get());
		} else {
			ktao_println(&mainTarget, "Not a valid command, try help");
		}
	}

failure:  

    print("[FAILURE]\n");
    x86_halt();
}
