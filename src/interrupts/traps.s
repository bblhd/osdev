
%define NUM_INT 0x31
%define NUM_EXC 0x14

%define CODE_SELECTOR 0|(1*8)
%define DATA_SELECTOR 0|(2*8)

extern x86_exception_handler

section .text

_isr:
%assign i 0
%rep 0x31
	align 16 ; align isr stubs to 16 byte boundaries
	%if i != 8 && (i < 10 || i > 14) && i != 17
		push 0
	%endif
	push i ; interrupt number
    jmp interrupt_common
	
	%assign i i+1
%endrep

extern set_kernel_stack

interrupt_common:
    push gs ; save segment registers
    push fs
    push es
    push ds
    
    pusha ; save general purpose registers
    
    mov eax, 0|(2*8) ; put known good value in segment registers
    mov gs, eax
    mov fs, eax
    mov es, eax
    mov ds, eax

    mov eax, esp ; store pointer to iframe
    push eax

	    call x86_exception_handler

    pop eax ; drop pointer to iframe
    
	;mov eax, [SwitchRequested]
	;cmp eax, 1
	;jne CleanUp ;Somewhere in the code there was a request to switch
		;mov [SwitchRequested], 0
		;call GetNextThread ;This sets "Running" to the next task(thread)
		;mov eax, [Running] ;Get the Task struct address
		;mov eax, [eax+12]  ;Pointer to PageDirectory of the task
		;mov eax, [eax+0x2000] ; Physical address of the Tables
		;mov cr3, eax ; enter virtual memory
		
		;mov ebx, [Running] ; Get the pointer again
		;mov esp, [ebx+4] ; Set ESP0
	;CleanUp:
	
	;push esp
	;call set_kernel_stack
	;add esp, 4

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
    
    mov ecx, 0x31
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
	%assign i 0
	%rep 0x32
	    dw 0 ; low 16 bits of ISR offset (_isr#i & 0FFFFh)
	    dw 0|(1*8) ; selector
	    db 0
	    
		%if i == 0x30
			db 0b11101110 ; system call
		%else
			db 0b10001110 ; present, ring 0, 32-bit interrupt gate
		%endif
	    dw 0 ; high 16 bits of ISR offset (_isr#i / 65536)
		%assign i i+1
	%endrep
_idt_end:


