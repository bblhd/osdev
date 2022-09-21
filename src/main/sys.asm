
global systemcall0
systemcall0:
	mov eax, [esp + 4]
	int 0x30
ret

global systemcall1
systemcall1:
	mov eax, [esp + 2*4]
	mov ecx, ebx
	mov ebx, [esp + 3*4]
	int 0x30
	mov ebx, ecx
ret

global systemcall2
systemcall2:
	push ebx
		mov eax, [esp + 2*4]
		mov ebx, [esp + 3*4]
		mov ecx, [esp + 4*4]
		int 0x30
	pop ebx
ret

global systemcall3
systemcall3:
	push ebx
		mov eax, [esp + 2*4]
		mov ebx, [esp + 3*4]
		mov ecx, [esp + 4*4]
		mov edx, [esp + 5*4]
		int 0x30
	pop ebx
ret
