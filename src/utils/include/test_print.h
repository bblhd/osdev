#ifndef	TEST_PRINT_H
#define	TEST_PRINT_H

void test_printMemoryMap(struct VGA_Target *target, multiboot_info_t* mbd);
void test_printAllChars(struct VGA_Target *target);
void test_printAllColors(struct VGA_Target *target);

#endif
