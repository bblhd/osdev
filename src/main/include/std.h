#ifndef STD_H
#define STD_H

#define DISREGARD(p) (void)(p)
void kernelpanic(char *message);
void kernelpanicWithNumber(char *message, char *name, unsigned int n);
#define NULL (void *)0

#endif
