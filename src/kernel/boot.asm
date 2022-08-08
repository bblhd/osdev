; multiboot header

alignment: equ 1 ; align loaded modules on page boundaries
MEMINFO: equ 1<<1 ; provide memory map
FLAGS: equ alignment | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC: equ 0x1BADB002       ; 'magic number' lets bootloader find the header
CHECKSUM: equ -(MAGIC + FLAGS) ; checksum of above, to prove we are multiboot

section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM

section .bss
resb 16384  ; 16K stack
sys_stack_top:

section .text

extern kernel_main

global _start
_start:
	cli
	mov esp, sys_stack_top

	push eax
	push ebx
	call kernel_main

	cli
	hlt
.Lhang:
	jmp .Lhang

extern gdt_p

global gdt_flush
gdt_flush:
    lgdt [gdt_p]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
	jmp 0x08:gdt_flush_end
gdt_flush_end: ret

