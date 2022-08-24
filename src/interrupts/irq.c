#include <interrupts.h>
#include <std.h>

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

void handle_exception(x86_iframe_t *);
void handle_platform_irq(x86_iframe_t *);
void handle_systemcall(x86_iframe_t *);

void x86_exception_handler(x86_iframe_t* iframe) {
	if (iframe->vector <= 31) handle_exception(iframe);
	else if (iframe->vector <= 47) handle_platform_irq(iframe);
	else if (iframe->vector == 0x80) handle_systemcall(iframe);
	else kernelpanic(exception_messages[16]);
}

void handle_exception(x86_iframe_t* iframe) {
	kernelpanic(exception_messages[iframe->vector]);
}

void handle_systemcall(x86_iframe_t* iframe) {
	DISREGARD(iframe);
	//switch (iframe->ax) {
		//case 0x01:
			//break;
	//};
}

IRQHandler irq_routines[16];

void handle_platform_irq(x86_iframe_t* frame) {
	int irq = frame->vector - 32;
    IRQHandler handler = irq_routines[irq];

    if (handler != NULL) handler(frame);
    
    if (irq != IRQ_PIT) pic_send_EOI(irq);
}

typedef void (*IRQHandler)(x86_iframe_t*);

void irq_register_handler(int irq, IRQHandler handler) {
	irq_routines[irq & 0b1111] = handler;
}

void irq_unregister_handler(int irq) {
    irq_routines[irq & 0b1111] = NULL;
}

//void send_irq();
