#ifndef X86_H
#define X86_H

//#include <stddef.h>
#include <stdint.h>

struct x86_iframe {
    uint32_t di, si, bp, sp, bx, dx, cx, ax;
    uint32_t ds, es, fs, gs;
    uint32_t vector;
    uint32_t err_code;
    uint32_t ip, cs, flags;
    uint32_t user_sp, user_ss;
};

#define plat_reboot() out8(0x64, 0xFE)
#define plat_hide_cursor() out16(0x03D4,0x200A)

static inline void out8(uint16_t port, uint8_t value) {
    asm volatile ("outb %[value], %[port]" :: [port] "d"(port), [value] "a"(value));
}

static inline void out16(uint16_t port, uint16_t value) {
    asm volatile ("outw %[value], %[port]" :: [port] "d"(port), [value] "a"(value));
}

static inline uint8_t in8(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %[port], %[value]" : [value] "=a"(value) : [port] "d" (port));
    return value;
}

static inline uint32_t x86_get_eflags(void) {
    uint32_t flags;
    asm volatile (
        "pushfl;"
        "popl %0"
        : "=rm" (flags)
        :: "memory"
    );
    return flags;
}

static inline void x86_set_eflags(uint32_t flags) {
    asm volatile (
        "pushl %0;"
        "popfl"
        :: "g" (flags)
        : "memory", "cc"
    );
}

static inline void x86_enable_int(void){
    asm volatile ("sti");
}

static inline void x86_disable_int(void){
    asm volatile ("cli");
}

static inline void x86_halt(void){
	asm volatile(
		"cli;"
		"hlt"
	);
}
#endif
