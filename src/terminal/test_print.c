#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <multiboot.h>

#include <kterm.h>

void test_print() {
	kterm_print("   .--------------.\n");
	kterm_print("   |.------------.|\n");
	kterm_print("   ||            ||\n");
	kterm_print("   ||            ||\n");
	kterm_print("   ||            ||\n");
	kterm_print("   ||            ||\n");
	kterm_print("   |+------------+|\n");
	kterm_print("   +-..--------..-+\n");
	kterm_print("   .--------------.\n");
	kterm_print("  / /============\\ \\\n");
	kterm_print(" / /==============\\ \\\n");
	kterm_print("/____________________\\\n");
	kterm_print("\\____________________/\n");
	
	kterm_newline();
	
	kterm_print("Hello there!\n");
	kterm_print("Welcome to my WIP operating system!\n");
}

char memorymapTypeLabels[][10] = {
	"?", "Available", "Reserved", "ACPI", "NVS", "Bad", "?"
};

void test_printMemoryMap(multiboot_info_t *mbd) {
	for (unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *) (mbd->mmap_addr + i);
		
		kterm_printf("%s: ", memorymapTypeLabels[mmmt->type]);
		
		kterm_printf("%p, ", mmmt->addr_low);
		if (mmmt->len_low < 1 << 10) {
			kterm_printf("%iB", mmmt->len_low);
		} else if (mmmt->len_low < 1 << 20) {
			kterm_printf("%iKiB", mmmt->len_low >> 10);
		} else if (mmmt->len_low < 1 << 30) {
			kterm_printf("%iMiB", mmmt->len_low >> 20);
		} else {
			kterm_printf("%iGiB", mmmt->len_low >> 30);
		}
		kterm_newline();
	}
}

void test_printAllChars() {
	for (unsigned char c = 1; c < 255; c++) {
		kterm_glyph(c);
	}
	kterm_newline();
}


void test_printAllColors() {
	for (int b = 0; b < 8; b++) {
		for (int f = 0; f < 16; f++) {
			//ktao_setColor(target, f, b);
			kterm_glyph('X');
		}
		kterm_newline();
	}
	kterm_newline();
	kterm_print("\efe\eb3");
}
