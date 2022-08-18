; accepts stack pointer of next task in eax
; returns stack pointer of previous task in eax
switch_to_task:
	cli
		push ebx
		push ecx
		push edx
		
		push esi
		push edi
		push ebp
			xchg esp, eax ; set new stack pointer, return old
			; todo: adjust ESP0 (stack top) in TSS
			; todo: add support for paging or virtual memory or whatever
		pop ebp
		pop edi
		pop esi
		
		pop edx
		pop ecx
		pop ebx
	sti
ret

; accepts stack pointer of task in eax
; returns new top of stack in eax
; pushes dummy values onto this stack so it can be switched to
create_task:
	xchg esp, eax
		push 0
		push 0
		push 0
		
		push 0
		push 0
		push 0
	xchg esp, eax
ret

global c_switch_to_task
; void *c_switch_to_task(void *);
c_switch_to_task:
	pop eax
	call switch_to_task
ret

global c_create_task
; void *c_create_task(void *);
c_create_task:
	pop eax
	call create_task
ret
