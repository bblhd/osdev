#include <std.h>
#include <kterm.h>

void *threadStackPointers[64];
int threadStackPointers_top = 0;
int currentThreadID = 0;

void *get_esp();
void gotoTask(void *);
void taskSwitch(void *);
void *initialiseTaskStack(void (*)(void), void *);

void threads_create(void (*function)(void), void *stack) {
	threadStackPointers[threadStackPointers_top++] = initialiseTaskStack(function, stack);
}

void threads_validate() {
	if (currentThreadID >= threadStackPointers_top) currentThreadID = 0;
	if (threadStackPointers_top <= 0) kernelpanic("No threads available");
}

void threads_begin() {
	threads_validate();
	gotoTask(threadStackPointers[currentThreadID]);
}

void threads_close() {
	threadStackPointers_top--;
	for (int i = currentThreadID; i < threadStackPointers_top; i++) {
		threadStackPointers[i] = threadStackPointers[i+1];
	}
	threads_validate();
	taskSwitch(threadStackPointers[currentThreadID]);
}

void threads_yield() {
	threadStackPointers[currentThreadID] = get_esp();
	currentThreadID++;
	threads_validate();
	taskSwitch(threadStackPointers[currentThreadID]);
}
