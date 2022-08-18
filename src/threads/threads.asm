
global switch_to_task
; void switch_to_task(void **, void *);
switch_to_task:
	cli
		push ebx
		push esi
		push edi
		push ebp
			mov eax, [esp+(4+1)*4]
			mov [eax], esp ; store old stack pointer
			mov esp, [esp+(4+2)*4] ; set new stack pointer
			; todo: adjust ESP0 (stack top) in TSS
			; todo: add support for paging or virtual memory or whatever
		pop ebp
		pop edi
		pop esi
		pop ebx
	sti
ret

global create_task
; void *create_task(void (*)(void), void *);
; pushes dummy values onto this stack so it can be switched to
create_task:
	mov ecx, [esp+1*4]
	mov eax, [esp+2*4]
	xchg esp, eax
		push ecx
		push 0
		push 0
		push 0
		push esp
	xchg esp, eax
ret
