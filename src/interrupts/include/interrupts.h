#ifndef PLAT_H
#define PLAT_H

#define IRQ_PIT 0
#define IRQ_PS2 1

#include <x86.h>
#include <pic.h>

void gdt_install_flat(void);
void setup_idt(void);

void pit_init(uint32_t frequency);

typedef void (*IRQHandler)(struct x86_iframe*);
void irq_register_handler(int id, IRQHandler);
void irq_unregister_handler(int id);

#endif
