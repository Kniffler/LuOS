#ifndef __LUOS_SETTING_LISTS__
#define __LUOS_SETTING_LISTS__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pico/malloc.h>

const float list_growth = 1.6;

typedef struct list_value
{
	bool free_string;
	union {
		char *data_string;
		int64_t data_int;
		float data_float;
	};
} lval_t;

typedef struct list {
	size_t capacity;
	size_t length;

	bool treat_dynamically;

	lval_t *data;
} list_t;

extern int list_create(list_t *subject, int32_t size, /*int32_t string_size,*/ bool can_resize)
{
	// If string size is set to anything other than 0, we will allocate the data with data_string
	size = (size <= 0) ? 1 : size;
	// string_size = (string_size < 0) ? 32 : string_size;

	subject->capacity = size;
	subject->data = calloc(size, sizeof(lval_t));

	if(!subject->data) { return -1; }

	// if(string_size)
	// {
	// 	for(int32_t i = 0; i < size; i++)
	// 	{
	// 		subject->data[i].data_string = calloc(string_size, sizeof(char));
	// 		subject->data[i].free_string = true;
	// 		if(!subject->data[i].data_string) { return (i+2)*-1; } // So starting from 0, we'd print -2, -3, -4, etc.
	// 	}
	// }

	subject->treat_dynamically = can_resize;
	subject->length = 0;

	return size;
}

extern void list_free(list_t *src)
{
	for(size_t i = 0; i < src->length; i++)
	{
		if(src->data[i].free_string) { free(src->data[i].data_string); }
	}
	free(src->data);
}

static int list_overflow_test(list_t *src)
{
	if(src->length+1>src->capacity && !src->treat_dynamically) { return -1; }
	if(src->length+1>src->capacity && src->treat_dynamically)
	{
		src->data = realloc(src->data, (int)(src->capacity*list_growth));
		src->capacity = (int)(src->capacity*list_growth);
		if(!src->data) { return -2; }
	}
	return 0;
}

extern int list_add_string(list_t *src, char *obj, bool allocate)
{
	int overflow = list_overflow_test(src);
	if(overflow<0) { return overflow; }

	src->data[src->length++].data_string = (allocate) ? calloc(strlen(obj), sizeof(char)) : obj;

	if(allocate && !src->data[src->length].data_string) { return -3; }

	if(allocate)
	{
		strcpy(src->data[src->length].data_string, obj);
		src->data[src->length].free_string = true;
	}

	return src->length-1; // Index we added it at
}

extern int list_add_int(list_t *src, int64_t obj)
{
	int overflow = list_overflow_test(src);
	if(overflow<0) { return overflow; }

	src->data[src->length++].data_int = obj;

	return src->length-1; // Index we added it at
}
extern int list_add_float(list_t *src, float obj)
{
	int overflow = list_overflow_test(src);
	if(overflow<0) { return overflow; }

	src->data[src->length++].data_float = obj;

	return src->length-1; // Index we added it at
}

// extern lval_t list_get(list_t)
// {
// 	return 0;
// }

#endif // __LUOS_SETTING_LISTS__
