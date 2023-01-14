#ifndef OV2_CSV_H
#define OV2_CSV_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

struct csv_file {
	char const* filename;
	FILE* file;
	char* line;
	size_t line_length;
	size_t line_capacity;
	size_t line_number;
	size_t column_number;
};

struct csv_file* csv_open(char const* filename);
void csv_close(struct csv_file* csv);
bool csv_read_line(struct csv_file* csv);
bool csv_read_uint(struct csv_file* csv, unsigned int* value);
bool csv_read_uchar(struct csv_file* csv, unsigned char* value);
bool csv_read_string(struct csv_file* csv, char const** value);

#endif //OV2_CSV_H
