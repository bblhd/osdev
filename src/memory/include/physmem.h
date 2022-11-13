#ifndef PHYSMEM_H
#define PHYSMEM_H

#include <multiboot.h>

void physmem_init(multiboot_info_t *mbd);
void *physmem_alloc();
void physmem_free(void *ptr);

#endif
