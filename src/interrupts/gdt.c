#include <stdint.h>

struct gdt_entry {
	unsigned int limit_low              : 16;
	unsigned int base_low               : 24;
	unsigned int accessed               :  1;
	unsigned int read_write             :  1; // readable for code, writable for data
	unsigned int conforming_expand_down :  1; // conforming for code, expand down for data
	unsigned int code                   :  1; // 1 for code, 0 for data
	unsigned int code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	unsigned int DPL                    :  2; // privilege level
	unsigned int present                :  1;
	unsigned int limit_high             :  4;
	unsigned int available              :  1; // only used in software; has no effect on hardware
	unsigned int long_mode              :  1;
	unsigned int big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	unsigned int granularity            :  1; // 1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high              :  8;
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

struct gdt_entry gdt[6];
struct gdt_ptr gdt_p;

extern void gdt_flush();
void flush_tss();
void *getStackTop();

typedef _Bool bool;

void init_gdt_entry( int index, unsigned long base, unsigned long limit, uint16_t flags, unsigned char priviledge) {
    gdt[index].base_low = base;
    gdt[index].base_high = base >> 24;
    
    gdt[index].limit_low = limit;
    gdt[index].limit_high = limit >> 16;
    
    gdt[index].accessed = flags;
    flags >>= 1;
    gdt[index].read_write = flags;
    flags >>= 1;
    gdt[index].conforming_expand_down = flags;
    flags >>= 1;
    gdt[index].code = flags;
    flags >>= 1;
    gdt[index].code_data_segment = flags;
    flags >>= 1;
    
    gdt[index].DPL = priviledge;
    gdt[index].present = 1;
    
    gdt[index].available = flags;
    flags >>= 1;
    gdt[index].long_mode = flags;
    flags >>= 1;
    gdt[index].big = flags;
    flags >>= 1;
    gdt[index].granularity = flags;
    flags >>= 1;
}

// Note: some of the GDT entry struct field names may not match perfectly to the TSS entries.
struct tss_entry tssEntry;

void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
	tssEntry.esp0 = stack;
}

void gdt_install_flat() {
	gdt_p.base = (unsigned int) gdt;
	gdt_p.limit = (sizeof(struct gdt_entry) * 6) - 1;
	
	init_gdt_entry(0, 0, 0, 0, 0); // Null
	init_gdt_entry(1, 0, 0xFFFFF, 0b110011010, 0); // ring0 Code
	init_gdt_entry(2, 0, 0xFFFFF, 0b110010010, 0); // ring0 Data
	init_gdt_entry(3, 0, 0xFFFFF, 0b110111010, 3); // ring3 Code 
	init_gdt_entry(4, 0, 0xFFFFF, 0b110111010, 3); // ring3 Data
	init_gdt_entry(5, (uint32_t) &tssEntry, sizeof(tssEntry), 0b000001001, 0); // tss
	
	gdt_flush();
	
	 //Ensure the TSS is initially zero'd.
	{
		int n = sizeof(tssEntry);
		uint8_t *d = (uint8_t *)(void *)&tssEntry;
		while (n--) *d++ = 0;
	}
	
	tssEntry.ss0  = 0;  // Set the kernel stack segment.
	tssEntry.esp0 = (uint32_t) getStackTop(); // Set the kernel stack pointer.
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
	
	flush_tss();
}
