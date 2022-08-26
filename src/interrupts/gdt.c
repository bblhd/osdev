#include <stdint.h>

struct gdt_entry {
	unsigned int limit_low : 16;
	unsigned int base_low : 24;
	unsigned int flags_low : 8;
	unsigned int limit_high : 4;
	unsigned int flags_high : 4;
	unsigned int base_high : 8;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct tss_entry {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));

#define ACCESSED     1
#define CODE_DATA    (1 << 4) // Descriptor type (0 for system, 1 for code/data)
#define PRESENT      (1 << 7) // Present
#define AVAILABLE    (1 << 8) // Available for system use
#define _32BIT       (0b10 << 9)
#define LARGE_GRAIN  (1 << 11) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define RING0        0
#define RING3        (0b11 << 5)
 
#define DATA_RD         0b0000 // Read-Only
#define DATA_RDWR       0b0010 // Read/Write
#define DATA_RDEXPD     0b0100 // Read-Only, expand-down
#define DATA_RDWREXPD   0b0110 // Read/Write, expand-down
#define CODE_EX         0b1000 // Execute-Only
#define CODE_EXRD       0b1010 // Execute/Read
#define CODE_EXC        0b1100 // Execute-Only, conforming
#define CODE_EXRDC      0b1110 // Execute/Read, conforming

#define GDT_NORMAL  CODE_DATA | PRESENT | _32BIT | LARGE_GRAIN
#define GDT_TSS     PRESENT | CODE_EX | ACCESSED
 
void create_descriptor(struct gdt_entry *descriptor, uint32_t base, uint32_t limit, uint16_t flags) {
	descriptor->base_low = base & 0xFFFFFF;
	descriptor->base_high = base >> 24 & 0xFF;
	
	descriptor->limit_low = limit & 0xFFFF;
	descriptor->limit_high = limit >> 16 & 0xF;
	
	descriptor->flags_low = flags & 0xFF;
	descriptor->flags_high = flags >> 8 & 0xF;
}

// Note: some of the GDT entry struct field names may not match perfectly to the TSS entries.
struct tss_entry tssEntry;

struct gdt_entry gdt[6];
struct gdt_ptr gdt_p;

void *get_esp();
void *get_ss();
void *getStackTop();

void gdt_flush();
void tss_flush();

void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
	tssEntry.esp0 = stack;
}

void gdt_install_flat() {
	gdt_p.base = (unsigned int) gdt;
	gdt_p.limit = (sizeof(struct gdt_entry) * 6) - 1;
	
	create_descriptor(&gdt[0], 0, 0, 0);
	create_descriptor(&gdt[1], 0, 0xFFFFF, GDT_NORMAL | RING0 | CODE_EXRD);
	create_descriptor(&gdt[2], 0, 0xFFFFF, GDT_NORMAL | RING0 | DATA_RDWR);
	create_descriptor(&gdt[3], 0, 0xFFFFF, GDT_NORMAL | RING3 | CODE_EXRD);
	create_descriptor(&gdt[4], 0, 0xFFFFF, GDT_NORMAL | RING3 | DATA_RDWR);
	create_descriptor(&gdt[5], (uint32_t) &tssEntry, sizeof(struct tss_entry), GDT_TSS);
	
	gdt_flush();
	
	 //Ensure the TSS is initially zero'd.
	{
		int n = sizeof(tssEntry);
		uint8_t *d = (uint8_t *)(void *)&tssEntry;
		while (n--) *d++ = 0;
	}
	
	tssEntry.ss0 = (uint32_t) 0|(2*8); // Set the kernel stack segment.
	tssEntry.esp0 = (uint32_t) get_esp(); // Set the kernel stack pointer.
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
	
	tss_flush();
}
