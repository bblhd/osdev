
global taskSwitch
; void saveTask(void *);
taskSwitch:
	cli
		push ebx
		push esi
		push edi
		push ebp
			mov esp, [esp+(4+1)*4] ; set new stack pointer
			; todo: adjust ESP0 (stack top) in TSS
			; todo: add support for paging or virtual memory or whatever
		pop ebp
		pop edi
		pop esi
		pop ebx
	sti
ret

global gotoTask
; void gotoTask(void *);
gotoTask:
	cli
			mov esp, [esp+4] ; set new stack pointer
			; todo: adjust ESP0 (stack top) in TSS
			; todo: add support for paging or virtual memory or whatever
		pop ebp
		pop edi
		pop esi
		pop ebx
	sti
ret

global initialiseTaskStack
; void *initialiseTaskStack(void (*)(void), void *);
; pushes dummy values onto this stack so it can be switched to
initialiseTaskStack:
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

extern systemTick
extern threads_yield
global threads_wait
 ;void threads_wait(int);
 ;pushes dummy values onto this stack so it can be switched to
threads_wait:
	mov ecx, [systemTick]
	mov eax, [esp + 4]
	add eax, ecx
	loop:
		push eax
			call threads_yield
		pop eax
		hlt
		mov ecx, [systemTick]
	cmp eax, ecx
	jg loop
ret

