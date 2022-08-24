#include <std.h>
#include <kterm.h>
#include <tasks.h>

struct Task *currentTask = NULL;
struct Task **where = NULL;

void *get_esp();
void gotoTask(void *);
void taskSwitch(void *);
void *initialiseTaskStack(void (*)(void), void *);

void tasks_begin(struct Task *new) {
	new->stack = get_esp();
	
	currentTask = new;
	currentTask->next = currentTask;
	where = &currentTask->next;
}

void task_create(struct Task *new, void (*function)(void), void *stack) {
	new->stack = initialiseTaskStack(function, stack);
	
	if (currentTask != NULL) {
		*where = new;
		new->next = currentTask;
		where = &new->next;
	}
}

void task_yield() {
	if (currentTask != NULL) {
		currentTask->stack = get_esp();
		where = &currentTask->next;
		currentTask = *where;
		taskSwitch(currentTask->stack);
	}
}

void task_close() {
	if (currentTask != NULL && currentTask->next != currentTask) {
		currentTask->stack = get_esp();
		*where = currentTask->next;
		currentTask = *where;
		taskSwitch(currentTask->stack);
	}
}
