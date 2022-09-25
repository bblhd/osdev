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

global get_esp
get_esp:
	mov eax, esp
ret

global get_ss
get_ss:
	mov eax, ss
ret
