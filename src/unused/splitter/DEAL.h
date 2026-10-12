// The Dynamic Entry ALlocator (deal)

#ifndef __DYNAMIC_ENTRY_ALLOCATOR__
#define __DYNAMIC_ENTRY_ALLOCATOR__

#include <stdint.h>
#include <stdlib.h>
#include <pico/malloc.h>

const uint8_t growth_factor = 2;

#ifndef USE_CUSTOM_DEAL_TYPE
#ifdef USE_DEAL_UINT16
	typedef uint16_t* deal_store_t;
#else
	#ifdef USE_DEAL_VOID_P
		typedef void* deal_store_t;
	#else
		typedef uint8_t* deal_store_t;
	#endif
#endif
#endif

typedef struct deal_struct {
	struct deal_struct *next;
	deal_store_t data;
} deal_t;

static void stitch(deal_t *from, deal_t *to) { from->next = to; }

// extern void deal_create(deal_t *origin)
// {
// 	if(origin->data||origin->next) { return; }
// 	origin = calloc(1, sizeof(deal_t));
// }
static void deal_free_node(deal_t *node)
{
	node->next = NULL;
	// node->data = NULL;
	// #ifdef DEAL_FREE_DATA
// 	DEAL_FREE_DATA();
// #endif
	free(node);
}
extern void deal_free(deal_t *origin)
{
	if(!origin) { return; }
	if(origin->next) { deal_free(origin->next); }
	deal_free_node(origin);
}
extern void deal_add(deal_t *origin, deal_store_t obj)
{
	if(!origin||!obj) { return; }

	if(origin->next) { deal_add(origin->next, obj); return; }

	if(!origin->data)
	{
		origin->data = obj;
		return;
	}
	deal_t *newling = calloc(1, sizeof(deal_t));
	newling->data = obj;
	newling->next = NULL;

	origin->next = newling;
}
extern deal_store_t deal_pop_end(deal_t *origin)
{
	if(!origin || !origin->data) { return NULL; }
	if(!origin->next) // In case we only have 1 element
	{
		deal_store_t ret = origin->data;
		origin->data = NULL;
		return ret;
	}

	if(origin->next && origin->next->next) { return deal_pop_end(origin->next); }

	// origin->next HAS to exist, since we had a guard clause

	deal_store_t ret = origin->next->data;
	origin->next->data = NULL;
	deal_free_node(origin->next);
	origin->next = NULL;
	return (ret) ? ret : NULL;
}
extern size_t deal_get_length(deal_t *origin)
{
	if(!origin) { return 0; }
	if(!origin->data) { return 0; }
	if(origin->next) { return deal_get_length(origin->next)+1; }
	return 1;
}
extern deal_store_t deal_get(deal_t *origin, int16_t index)
{
	if(!origin) { return NULL; }
	if(index>0&&!origin->next) { return NULL; }
	if(index>0&&origin->next) { return deal_get(origin->next, index-1); }
	// deal_t *current = origin;
	// for(int16_t i = 0; i < index && current->next; i++)
	// {
	// 	current = current->next;
	// }
	return (origin->data) ? origin->data : NULL;
}
extern void deal_set(deal_t *origin, uint16_t index, deal_store_t obj)
{
	if(index>0&&origin->next) { deal_set(origin->next, index-1, obj); return; }

	origin->data = obj;
}

extern deal_store_t deal_pop_cascading(deal_t *origin, uint16_t index)
{
	if(index==deal_get_length(origin)-1) { return deal_pop_end(origin); }
	if(index>=deal_get_length(origin)) { return NULL; }
	if(index>1&&origin->next) { return deal_pop_cascading(origin->next, index-1); }

	if(!origin->next)
	{
		deal_store_t data = (origin->data) ? origin->data : NULL;
		if(data) { origin->data = NULL; }
		return data;
	}

	deal_t *to_delete = origin->next;
	deal_store_t to_return = origin->data;

	if(origin->next->next) { stitch(origin, origin->next->next); }

	deal_free_node(to_delete);
	return to_return;
}

// extern void deal_execute_per_node(deal_t *src, void(function)(deal_t *obj))
// {
// 	if(!src) { return; }
// 	function(src);
//
// 	if(!src->next) { return; }
// 	deal_execute_per_node(src->next, function);
//
// 	return;
// }

#endif // __DYNAMIC_ENTRY_ALLOCATOR__
