#include <stdint.h>
#include <stdarg.h>
#include <std.h>

#include <kterm.h>
#include <x86.h>
#include <string.h>

void systemcall0(unsigned int a);
void systemcall1(unsigned int a, unsigned int b);
void systemcall2(unsigned int a, unsigned int b, unsigned int c);
void systemcall3(unsigned int a, unsigned int b, unsigned int c, unsigned int d);

