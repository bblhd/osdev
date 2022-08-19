#ifndef THREADS_H
#define THREADS_H

void threads_create(void (*)(void), void *);
void threads_begin();
void threads_yield();
void threads_close();
void threads_wait(int);

#endif
