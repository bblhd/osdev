#ifndef THREADS_H
#define THREADS_H

struct Task {
	struct Task *next;
	void *stack;
	unsigned int id;
	//struct {
		//unsigned int reciever;
		//unsigned int payload;
	//} message;
	//struct {
		//unsigned int timeout;
		//unsigned int condition;
		//unsigned int waitingFor;
	//} wait;
};

void task_create(struct Task *, void (*)(void), void *);
void task_wait(int);
void task_yield();
void task_close();
void tasks_begin();

#endif
