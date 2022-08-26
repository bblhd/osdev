extern gdt_p

global gdt_flush
gdt_flush:
    lgdt [gdt_p]
    mov ax, 0|(2*8)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
	jmp 0|(1*8):gdt_flush_end
gdt_flush_end: ret

global tss_flush
tss_flush:
	mov ax, 0|(5*8)
	ltr ax
ret

extern usermode_main

global enterUsermode
enterUsermode:
	mov ax, 3|(4*8); ring 3, 4th gdt entry
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax
 
	; set up the stack frame iret expects
	mov eax, esp
	push 3|(4*8)       ; ss, ring 3, 4th gdt entry
	push eax           ; esp
	push 0x0202         ; eflags
	push 3|(3*8)       ; cs, ring 3, 3rd gdt entry
	push usermode_main ; ip
iret

global get_esp
get_esp:
	mov eax, esp
ret

global get_ss
get_ss:
	mov eax, ss
ret
