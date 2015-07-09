#ifndef _UTIL_H_
#define _UTIL_H_

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "errors.h"

#define PATH_SIZE 512

bool isnumber(char *str)
{
	if (!str[0])
		return false;
	while (*str) {
		if (!isdigit(*str))
			return false;
		++str;
	}
	return true;
}

long parse_size(char *size_string)
{
	int suffix_idx;
	char suffix;
	long size;

	suffix_idx = strlen(size_string) - 1;
	suffix = toupper(size_string[suffix_idx]);
	size_string[suffix_idx] = 0;
	if (!isnumber(size_string))
		return -ESZNOTNUM;
	size = strtoul(size_string, NULL, 0) * 1024;
	if (size <= 0)
		return -EINVSIZE;
	if (suffix == 'M')
		size *= 1024;
	else if (suffix == 'G')
		size *= 1024 * 1024;
	else if (suffix != 'K')
		return -EINVSZSUFFIX;
	return size;
}

int parse_nways(char *ways_string)
{
	if (!ways_string)
		return -ENONWAYS;
	int k = atoi(ways_string);
	if (k <= 0)
		return -EINVNWAYS;
	return k;
}

#define swap(array, i, j)	\
do {				\
	uint32_t temp;		\
	temp = array[i];	\
	array[i] = array[j];	\
	array[j] = temp;	\
} while (0)

size_t get_file_size(FILE *file)
{
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	return size;
}

#define scratchfile_block_next(file) {				\
	if (file.current_block && file.current_block->next) {	\
		file.current_block->position = 0;		\
		file.current_block = file.current_block->next;	\
		file.current_block->position = 4;		\
	}							\
}

#define scratchfile_has_data(file) file.first_block && \
				   file.first_block->size > 0

#define scratchfile_reset(file) {			\
	if (file.first_block && file.current_block) {	\
		file.current_block = file.first_block;	\
		file.current_block->position = 0;	\
	}						\
}

#endif
