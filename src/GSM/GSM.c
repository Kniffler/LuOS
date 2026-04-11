#include "GSM.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pico/malloc.h>

#define FIELD_MAX_LENGTH 256

// Conversions

static uint64_t pow_simple(uint64_t base, uint64_t exponent)
{
	for(uint64_t i = 0; i < exponent; i++)
	{
		base *= base;
	}
	return base;
}

static int64_t str_to_int(char *src)
{
	int64_t ret = 0;
	bool negative = false;

	for(size_t i = 0; i < strlen(src); i++)
	{
		char c = src[i];
		if(i==0&&c=='-') { negative = true; continue; }
		if(c-'0' > 0 && c-'0'<=9)
		{
			ret *= 10;
			ret += c-'0';
		}else
		{
			ret = 0;
		}
	}
	ret *= (negative) ? -1 : 1;
	return ret;
}
static uint64_t str_to_uint(char *src)
{
	uint64_t ret = 0;

	for(size_t i = 0; i < strlen(src); i++)
	{
		char c = src[i];
		if(c-'0' > 0 && c-'0'<=9)
		{
			ret *= 10;
			ret += c-'0';
		}
	}
	return ret;
}
static float str_to_float(char *src)
{
	float ret = 0;
	bool negative = false;
	bool after_decimal = false;

	for(size_t i = 0; i < strlen(src); i++)
	{
		char c = src[i];
		if(i==0&&c=='-') { negative = true; continue; }
		if(i==0&&c=='.') { after_decimal = true; continue; }
		if(c-'0' > 0 && c-'0'<=9)
		{
			ret *= (after_decimal) ? 1/10 : 10; // Reduce.
			ret += c-'0';
		}else
		{
			ret = 0;
		}
	}
	ret *= (negative) ? -1 : 1;
	return ret;
}

static double str_to_double(char *src)
{
	double ret = 0;
	bool negative = false;
	bool after_decimal = false;

	for(size_t i = 0; i < strlen(src); i++)
	{
		char c = src[i];
		if(i==0&&c=='-') { negative = true; continue; }
		if(i==0&&c=='.') { after_decimal = true; continue; }
		if(c-'0' > 0 && c-'0'<=9)
		{
			ret *= (after_decimal) ? 1/10 : 10; // Reduce.
			ret += c-'0';
		}else
		{
			ret = 0;
		}
	}
	ret *= (negative) ? -1 : 1;
	return ret;
}

static void resolve_setting_type(setting_t *setting, char *type)
{
	if(strcmp(type, "INT")==0)				{ setting->type=SETTING_INT64; }
	else if(strcmp(type, "UINT")==0)		{ setting->type=SETTING_UINT64; }
	else if(strcmp(type, "FLOAT")==0)		{ setting->type=SETTING_FLOAT; }
	else if(strcmp(type, "DOUBLE")==0)		{ setting->type=SETTING_DOUBLE; }
	else if(strcmp(type, "R_FLOAT")==0)		{ setting->type=SETTING_RANGE_FLOAT; }
	else if(strcmp(type, "R_DOUBLE")==0)	{ setting->type=SETTING_RANGE_DOUBLE; }
	else if(strcmp(type, "R_INT")==0)		{ setting->type=SETTING_RANGE_INT64; }
	else if(strcmp(type, "R_UINT")==0)		{ setting->type=SETTING_RANGE_UINT64; }
	else if(strcmp(type, "DL_STR")==0)		{ setting->type=SETTING_STRING_LIST_DYNAMIC; }
	else if(strcmp(type, "DL_INT")==0)		{ setting->type=SETTING_INT_LIST_DYNAMIC; }
	else if(strcmp(type, "DL_FLOAT")==0)	{ setting->type=SETTING_FLOAT_LIST_DYNAMIC; }
	else if(strcmp(type, "SL_STR")==0)		{ setting->type=SETTING_STRING_LIST_STATIC; }
	else if(strcmp(type, "SL_INT")==0)		{ setting->type=SETTING_INT_LIST_STATIC; }
	else if(strcmp(type, "SL_FLOAT")==0)	{ setting->type=SETTING_FLOAT_LIST_STATIC; }
	else if(strcmp(type, "STR")==0)			{ setting->type=SETTING_STRING; }
	else if(strcmp(type, "TOG")==0)			{ setting->type=SETTING_TOGGLE; }
	else if(strcmp(type, "DEBUG")==0)		{ setting->type=SETTING_DEBUG; }
}

static void interpret_setting_value_from_string(setting_t *setting, char *input)
{
	char *might_be_range = strchr(input, '/');

	uint16_t s1 = 0, s2 = 0;

	char range_low[strlen(input)];
	char range_high[strlen(input)];

	if(might_be_range)
	{
		char *higher_half = strtok(input, " / ");
		for(int i = 0; i < strlen(higher_half); i++)
		{
			range_high[s1++] = higher_half[i];
		}
		char *lower_half = strtok(NULL, " / ");
		for(int i = 0; i < strlen(lower_half); i++)
		{
			range_low[s2++] = lower_half[i];
		}
	}

	switch(setting->type)
	{
		case SETTING_INT64:
			setting->value.int_64 = str_to_int(input); break;
		case SETTING_UINT64:
			setting->value.uint_64 = str_to_uint(input); break;
		case SETTING_FLOAT:
			setting->value.v_float = str_to_float(input); break;
		case SETTING_DOUBLE:
			setting->value.v_double = str_to_double(input); break;
		case SETTING_RANGE_FLOAT:
			setting->value.v_range.lower_float = str_to_float(range_low);
			setting->value.v_range.upper_float = str_to_float(range_high);
		break;
		case SETTING_RANGE_DOUBLE:
			setting->value.v_range.lower_double = str_to_float(range_low);
			setting->value.v_range.upper_double = str_to_float(range_high);
		break;
		case SETTING_RANGE_INT64:
			setting->value.v_range.lower_signed = str_to_int(range_low);
			setting->value.v_range.upper_signed = str_to_int(range_high);
		break;
		case SETTING_RANGE_UINT64:
			setting->value.v_range.lower_unsigned = str_to_uint(range_low);
			setting->value.v_range.upper_unsigned = str_to_uint(range_high);
		break;
		case SETTING_STRING_LIST_DYNAMIC: break;
		case SETTING_INT_LIST_DYNAMIC: break;
		case SETTING_FLOAT_LIST_DYNAMIC: break;
		case SETTING_STRING_LIST_STATIC: break;
		case SETTING_INT_LIST_STATIC: break;
		case SETTING_FLOAT_LIST_STATIC: break;
		case SETTING_STRING: break;
		case SETTING_TOGGLE: break;
		case SETTING_DEBUG: break;
	}
}

int create_setting_from_interpretation_string(setting_t *setting, char *base, value_t default_value)
{
	/* A basic interpretation string will lool something like this:
	 * <ID><NAME><TYPE>:VALUE:\n\0
	 *		(The \n and \0 at the end do not forcibly have to be there.)
	 * Where ID is an unsigned 64 bit integer, unique to this setting, NAME is a string that does not contain "<" or ">"
	 * and TYPE is one of the following, which maps to the corresponding type enum:
	 * INT		: SETTING_INT64
	 * UINT		: SETTING_UINT64
	 * FLOAT	: SETTING_FLOAT
	 * DOUBLE	: SETTING_DOUBLE
	 * R_FLOAT	: SETTING_RANGE_FLOAT
	 * R_DOUBLE	: SETTING_RANGE_DOUBLE
	 * R_INT	: SETTING_RANGE_INT64
	 * R_UINT	: SETTING_RANGE_UINT64
	 * DL_STR	: SETTING_STRING_LIST_DYNAMIC
	 * DL_INT	: SETTING_INT_LIST_DYNAMIC
	 * DL_FLOAT	: SETTING_FLOAT_LIST_DYNAMIC
	 * SL_STR	: SETTING_STRING_LIST_STATIC
	 * SL_INT	: SETTING_INT_LIST_STATIC
	 * SL_FLOAT	: SETTING_FLOAT_LIST_STATIC
	 * STR		: SETTING_STRING
	 * TOG		: SETTING_TOGGLE
	 * DEBUG	: SETTING_DEBUG
	 *
	 * VALUE and VALUE_DEF now depend on the type, but generally each type will be accompanied by these value formats:
	 * INT/UINT: 		: n
	 * FLOAT/DOUBLE		: r.
	 * R_FLOAT/R_DOUBLE	: R./r.
	 * R_INT/R_UINT		: N/n
	 * DL_STR			: {s[0], s[1]...}
	 * DL_INT			: {n[0], n[1]...}
	 * DL_FLOAT			: {r[0], r[1]...}
	 * SL_STR			: e{s[0], s[1]... s[e-1]}
	 * SL_INT			: e{n[0], n[1]... n[e-1]}
	 * SL_FLOAT			: e{r[0], r[1]... r[e-1]}
	 * STR				: s
	 * TOG				: b
	 * DEBUG			: A
	 * 	Key:
	 * n is any integer, positive or negative
	 * e is an unsigned integer, indicating the size of the static list
	 * r is any decimal point number
	 * s is any string that is surrounded by a " character on each side
	 * b is a boolean value (t or f for true and false respectively)
	 * A is any of the above values, depending on context
	 *	-	WARNING: Misinterpretation of this value can lead to errors
	 * for ranges, the capital letter depicts the upper bound, and te lowercase letter the lower bound
	 * For lists, the "..." (ellipsis) does not depict the format, rather it indicates that the shown pattern continues.
	 * Lists are indexed by the number shown in brackets, however these will NOT be present in the string
	*/
	uint8_t stage = 0;
	/* Stages:
	 * 0: Looking for ID field
	 * 1: Found/interpreting ID field
	 * 2: Looking for NAME field
	 * 3: Found/interpreting NAME field
	 * 4: Looking for TYPE field
	 * 5: Found/interpreting TYPE field
	 * 6: Looking for value
	 * 7: Reading/writing value
	*/
	bool reading_value = 0;

	char read_field[FIELD_MAX_LENGTH];
	memset(read_field, ' ', FIELD_MAX_LENGTH); read_field[FIELD_MAX_LENGTH-1] = '\0';

	uint8_t writing_index = 0;

	for(uint64_t i = 0; i < strlen(base); i++)
	{
		char c = base[i];
		switch(stage)
		{
			case 0: case 2: case 4: if(c=='<') { stage++; } break;
			case 1:
				if(c=='>')
				{
					read_field[writing_index++] = '\0';
					setting->ID = str_to_uint(read_field);
					memset(read_field, ' ', FIELD_MAX_LENGTH); read_field[FIELD_MAX_LENGTH-1] = '\0';
					writing_index = 0; stage++; break;
				}
				read_field[writing_index++] = c;
			break;
			case 3:
				if(c=='>')
				{
					read_field[writing_index++] = '\0';
					strncpy(setting->name, read_field, writing_index);
					memset(read_field, ' ', FIELD_MAX_LENGTH); read_field[FIELD_MAX_LENGTH-1] = '\0';
					writing_index = 0; stage++; break;
				}
				read_field[writing_index++] = c;
			break;
			case 5:
				if(c=='>')
				{
					read_field[writing_index++] = '\0';
					resolve_setting_type(setting, read_field);
					memset(read_field, ' ', FIELD_MAX_LENGTH); read_field[FIELD_MAX_LENGTH-1] = '\0';
					writing_index = 0; stage++; break;
				}
				read_field[writing_index++] = c;
			break;
			case 6: if(c==':') { stage++; } break;
			case 7:
				if(c==':')
				{
					read_field[writing_index++] = '\0';

					memset(read_field, ' ', FIELD_MAX_LENGTH); read_field[FIELD_MAX_LENGTH-1] = '\0';
					writing_index = 0; stage++; break;
				}
				read_field[writing_index++] = c;
			break;
			default: break;
		}
	}
	return -1;
}
extern int create_multiple_settings_by_interpretation_string(setting_t settings[], size_t setting_count, char *base);
