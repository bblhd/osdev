#ifndef	_STRING_H
#define	_STRING_H

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);

size_t strlen(const char* str);
int streq(char *a, char *b);
int streqn(char *a, char *b, int n);

void ultos(unsigned long num, int base, char *out, int max);
void ltos(long num, int base, char *out, int max);

#endif
