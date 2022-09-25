#include <interrupts.h>
#include <std.h>
#include <stdint.h>
#include <kterm.h>

char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void handle_exception(struct x86_iframe *);
void handle_platform_irq(struct x86_iframe *);
void handle_systemcall(struct x86_iframe *);

void x86_exception_handler(struct x86_iframe* iframe) {
	if (iframe->vector <= 31) handle_exception(iframe);
	else if (iframe->vector <= 47) handle_platform_irq(iframe);
	else if (iframe->vector == 48) handle_systemcall(iframe);
	else kernelpanic(exception_messages[16]);
}

void handle_exception(struct x86_iframe* iframe) {
	kernelpanicWithNumber(exception_messages[iframe->vector], "error code", iframe->err_code);
}

void kterm_print1(char x);

// 0x0X syatem tasks, 0x1X debuging, 0x2X tasks, 0x3X permissions and allocations
void handle_systemcall(struct x86_iframe* iframe) {
	switch (iframe->ax) {
		case 0x01:
			plat_reboot();
		break;
		case 0x10:
			kterm_print1(iframe->bx & 0xFF);
		break;
		case 0x11:
			kterm_print("test\n");
		break;
		case 0x12:
			//kterm_print("test2\n");
			kterm_printf("%u", iframe->bx);
		break;
		default:
		break;
	};
}

IRQHandler irq_routines[16];

void handle_platform_irq(struct x86_iframe* frame) {
	int irq = frame->vector - 32;
    IRQHandler handler = irq_routines[irq];

    if (handler != NULL) handler(frame);
    
    if (irq != IRQ_PIT) pic_send_EOI(irq);
}

void irq_register_handler(int irq, IRQHandler handler) {
	irq_routines[irq & 0b1111] = handler;
}

void irq_unregister_handler(int irq) {
    irq_routines[irq & 0b1111] = NULL;
}

//void send_irq();
