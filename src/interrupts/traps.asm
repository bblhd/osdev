
%define NUM_INT 0x31
%define NUM_EXC 0x14

%define CODE_SELECTOR 0x08
%define DATA_SELECTOR 0x10

extern x86_exception_handler

section .text

_isr:
%assign i 0
%rep NUM_INT
	align 16 ; align isr stubs to 16 byte boundaries
	%if i != 8 && (i < 10 || i > 14) && i != 17
		push 0
	%endif
	push i ; interrupt number
    jmp interrupt_common
	
	%assign i i+1
%endrep

interrupt_common:
    push gs ; save segment registers
    push fs
    push es
    push ds
    pusha ; save general purpose registers
    
    mov eax, DATA_SELECTOR ; put known good value in segment registers
    mov gs, eax
    mov fs, eax
    mov es, eax
    mov ds, eax

    mov eax, esp ; store pointer to iframe
    push eax

    call x86_exception_handler

    pop eax ; drop pointer to iframe

    popa ; restore general purpose registers
    pop ds ; restore segment registers
    pop es
    pop fs
    pop gs
    add esp, 8 ; drop exception number and error code
iret

global setup_idt
setup_idt:
    ; setup isr stub descriptors in the idt
    mov esi, _isr
    mov edi, _idt
    mov ecx, NUM_INT

	.Lloop:
	    mov ebx, esi
	    mov [edi], bx ; low word in IDT(n).low
	    shr ebx, 16
	    mov [edi+6], bx ; high word in IDT(n).high
	
	    add esi, 16 ; index the next ISR stub
	    add edi, 8 ; index the next IDT entry
    loop .Lloop

    lidt [_idtr]
ret

section .data

align 8
global _idtr

; interrupt descriptor table (IDT)
global _idt
global _idt_end

_idtr:
    dw _idt_end - _idt - 1 ; IDT limit
    dd _idt

_idt:
	%rep NUM_INT-1
	    dw 0 ; low 16 bits of ISR offset (_isr#i & 0FFFFh)
	    dw CODE_SELECTOR ; selector
	    db 0
		db 0x8e ; present, ring 0, 32-bit interrupt gate
	    dw 0 ; high 16 bits of ISR offset (_isr#i / 65536)
	%endrep
_idt_end:


