#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <multiboot.h>

void *mainMemoryBase = NULL;
size_t mainMemoryLimit = 0;
size_t mainMemoryTop = 0;

#define PAGE_SIZE ((size_t) 4096)
#define PAGE_MASK ((size_t) 4095)

#define MAXHOLES 2048
typedef uint16_t holeOffset_t; //holeOffset_t should be bit-shifted left 12 and added to mainMemoryBase to get pointer

holeOffset_t holes[MAXHOLES];
size_t holesTop = 0;

static inline holeOffset_t convertToOffset(void *ptr) {
	return (holeOffset_t) ((ptr - mainMemoryBase) >> 12)
}
static inline void *convertFromOffset(holeOffset_t offset) {
	return (void *) (mainMemoryBase + (offset << 12))
}

static inline size_t roundUp(size_t value, size_t mask) {
    return (value + mask) & ~mask;
}
static inline size_t roundDown(size_t value, size_t mask) {
    return value & ~mask;
}

void physmem_init(multiboot_info_t *mbd) {
	for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *) mbd->mmap_addr;
		
		if (mmmt->type == 1 && mmmt->len_low > mainMemoryLimit) {
			mainMemoryBase = mmmt->addr_low;
			mainMemoryLimit = mmmt->len_low;
		} 
	}
	mainMemoryBase = (void *) roundUp((size_t) mainMemoryBase, PAGE_MASK);
	mainMemoryLimit = roundUp(mainMemoryLimit, PAGE_MASK);
}

void *physmem_alloc() {
	if (holesTop > 0) {
		return convertFromOffset(holes[holesTop--]);
	} else if (mainMemoryTop < mainMemoryLimit) {
		mainMemoryTop += PAGE_SIZE;
		return mainMemoryBase - PAGE_SIZE;
	} else {
		return NULL;
	}
}

void physmem_free(void *ptr) {
	if (ptr + PAGE_SIZE == mainMemoryBase + mainMemoryTop) {
		mainMemoryTop -= PAGE_SIZE;
		while (holesTop > 0 && convertFromOffset(holes[holesTop-1]) + PAGE_SIZE == mainMemoryBase + mainMemoryTop) {
			holesTop--;
			mainMemoryTop -= PAGE_SIZE;
		}
	} else {
		holeOffset_t offset = convertToOffset(ptr);
		size_t i = holesTop-1;
		for (; i > 0 && holes[i] > offset; i++) {
			holes[i+1] = holes[i];
		}
		holes[i] = offset;
		holesTop++;
	}
}
