#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <multiboot.h>

#include <ktao.h>

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

char memorymapTypeLabels[][10] = {
	"?", "Available", "Reserved", "ACPI", "NVS", "Bad", "?"
};

void test_printMemoryMap(struct VGA_Target *target, multiboot_info_t *mbd) {
	for (unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *) (mbd->mmap_addr + i);
		
		ktao_print(target, memorymapTypeLabels[mmmt->type]);
		ktao_print(target, ": ");
		
		ktao_printf(target, "%p, ", mmmt->addr_low);
		if (mmmt->len_low < 1 << 10) {
			ktao_printf(target, "%iB", mmmt->len_low);
		} else if (mmmt->len_low < 1 << 20) {
			ktao_printf(target, "%iKiB", mmmt->len_low >> 10);
		} else if (mmmt->len_low < 1 << 30) {
			ktao_printf(target, "%iMiB", mmmt->len_low >> 20);
		} else {
			ktao_printf(target, "%iGiB", mmmt->len_low >> 30);
		}
		ktao_newline(target);
	}
}

void test_printAllChars(struct VGA_Target *target) {
	for (unsigned char c = 1; c < 255; c++) {
		ktao_putGlyph(target, c);
	}
	ktao_newline(target);
}


void test_printAllColors(struct VGA_Target *target) {
	for (int b = 0; b < 8; b++) {
		for (int f = 0; f < 16; f++) {
			ktao_setColor(target, f, b);
			ktao_putGlyph(target, 'X');
		}
		ktao_newline(target);
	}
	ktao_newline(target);
	ktao_print(target, "\er");
}
