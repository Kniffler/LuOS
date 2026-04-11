#ifndef __DYNAMIC_ENTRY_ALLOCATOR__
#define __DYNAMIC_ENTRY_ALLOCATOR__

#include <stdint.h>
#include <string.h>
#include <pico/malloc.h>

#ifdef USE_DEAL_UINT16
typedef deal_store_t uint16_t
#endif

typedef struct deal_struct {
	uint16_t capacity;
	uint16_t length;

} deal_t;

#endif // __DYNAMIC_ENTRY_ALLOCATOR__
