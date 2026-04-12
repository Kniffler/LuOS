#ifndef __LUOS_GENERAL_SETTINGS_MANAGER__
#define __LUOS_GENERAL_SETTINGS_MANAGER__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pico/malloc.h>
#include "LISTS.h"

typedef enum setting_type {
	// Numbers
	// SETTING_INT16,	// A signed 16 bit integer value
	// SETTING_INT32,	// A signed 32 bit integer value
	SETTING_INT64,		// A signed 64 bit integer value

	// SETTING_UINT16,	// An unsigned 16 bit integer value
	// SETTING_UINT32,	// An unsigned 32 integer value
	SETTING_UINT64,		// An unsigned 64 integer value

	SETTING_FLOAT,	// A float value
	SETTING_DOUBLE,	// A double value

	// Ranges
	SETTING_RANGE_FLOAT,	// A float value inside a set range
	SETTING_RANGE_DOUBLE,	// A double value inside a set range

	// SETTING_RANGE_INT16,	// A signed 16 bit integer value inside a set range
	// SETTING_RANGE_INT32,	// A signed 32 bit integer value inside a set range
	SETTING_RANGE_INT64,	// A signed 64 bit integer value inside a set range

	// SETTING_RANGE_UINT16,	// An unsigned 16 bit integer value inside a set range
	// SETTING_RANGE_UINT32,	// An unsigned 32 bit integer value inside a set range
	SETTING_RANGE_UINT64,		// An unsigned 64 bit integer value inside a set range

	// Lists
	SETTING_STRING_LIST_DYNAMIC,	// A list of strings, each of which is an option
	SETTING_INT_LIST_DYNAMIC,		// A list of signed 64 bit integers, each of which is an option
	SETTING_FLOAT_LIST_DYNAMIC,		// A list of floats, each of which is an option

	SETTING_STRING_LIST_STATIC,		// Same as the list before, except the number of options is set
	SETTING_INT_LIST_STATIC,		// Same as the list before, except the number of options is set
	SETTING_FLOAT_LIST_STATIC,		// Same as the list before, except the number of options is set

	// Misc.
	SETTING_STRING,	// A string
	SETTING_TOGGLE,	// A true or false setting
	SETTING_DEBUG	// Allows for the execution of any function - useful for debugging
} setting_type_t;

typedef struct range {
	union {
		int64_t upper_signed;
		uint64_t upper_unsigned;
		float upper_float;
		double upper_double;
	};
	union {
		int64_t lower_signed;
		uint64_t lower_unsigned;
		float lower_float;
		double lower_double;
	};
} range_t;

typedef union value {
	int64_t int_64;
	uint64_t uint_64;

	float v_float;
	double v_double;

	list_t v_list;

	range_t v_range;
	char *v_string;
	bool v_toggle_on;
} value_t;

typedef struct setting_struct {
	uint64_t ID;
	char *name;
	setting_type_t type;

	value_t value;
	value_t default_value;
} setting_t;
// The size of this struct is 48 Bytes (or so says my editor), so the memory
// it takes up is actually rather negligible, only 0.01875% on an RP2040 (48/256000 * 100)

extern int create_setting_from_interpretation_string(setting_t *setting, char *base, value_t default_value);
// extern int create_multiple_settings_by_interpretation_string(setting_t settings[], size_t setting_count, char *base);

#endif //__LUOS_GENERAL_SETTINGS_MANAGER__
