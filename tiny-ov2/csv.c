 #include "csv.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

struct csv_file* csv_open(char const* filename) {
	struct csv_file* csv = malloc(sizeof(struct csv_file));
	if (csv == NULL) {
		fprintf(stderr, "Failed to allocate memory for csv_file: %s\n", strerror(errno));
		return NULL;
	}

	csv->filename = filename;
	csv->file = fopen(filename, "r");
	if (csv->file == NULL) {
		fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
	}
	csv->line_capacity = 1024;
	csv->line = malloc(csv->line_capacity);
	if (csv->line == NULL) {
		fprintf(stderr, "Failed to allocate memory for %s: %s\n", filename, strerror(errno));
	}
	csv->line_length = 0;
	csv->line_number = 0;
	csv->column_number = 0;
	if (csv->line == NULL || csv->file == NULL) {
		csv_close(csv);
		csv = NULL;
	}
	return csv;
}

void csv_close(struct csv_file* csv) {
	if (csv->file != NULL && fclose(csv->file) != 0) {
		fprintf(stderr, "WARNING: Failed to close %s: %s\n", csv->filename, strerror(errno));
	}
	if (csv->line != NULL) free(csv->line);
	free(csv);
}

/* Anything after a '#' character is treated as a comment and ignored */
bool csv_read_line(struct csv_file* csv) {
	int c;
	csv->line_number++;
	csv->column_number = 0;
	csv->line_length = 0;
	while ((c = fgetc(csv->file)), c != EOF && c != '\n') {
		if (c == '#') {
			while ((c = fgetc(csv->file)) != EOF && c != '\n');
			break;
		}
		if (csv->line_length + 1 >= csv->line_capacity) {
			csv->line_capacity *= 2;
			csv->line = realloc(csv->line, csv->line_capacity);
			if (csv->line == NULL) {
				fprintf(stderr, "Failed to allocate memory for %s: %s\n", csv->filename, strerror(errno));
				return false;
			}
		}
		csv->line[csv->line_length++] = (char) c;
	}
	if (c == EOF && csv->line_length == 0) {
		return false;
	}
	if (csv->line_length + 1 >= csv->line_capacity) {
		csv->line_capacity *= 2;
		csv->line = reallocarray(csv->line, csv->line_capacity, 1);
		if (csv->line == NULL) {
			fprintf(stderr, "Failed to allocate memory for %s: %s\n", csv->filename,
				strerror(errno));
			return false;
		}
	}
	csv->line[csv->line_length] = '\0';
	return true;
}

bool csv_read_uint(struct csv_file* csv, unsigned int* value) {
	bool success = true;
	char const* token_start = csv->line + csv->column_number;
	char const* token_end = token_start;
	char const* token = NULL;
	unsigned long int conv_value;
	char* conv_endptr;
	while (*token_end != '\0' && *token_end != ';') token_end++;
	token = strndup(token_start, token_end - token_start);
	if (token == NULL) {
		fprintf(stderr, "Failed to allocate memory for %s: %s\n", csv->filename, strerror(errno));
	} else {
		conv_value = strtoul(token_start, &conv_endptr, 10);
		if (conv_value == ULONG_MAX) {
			fprintf(stderr, "Failed to convert '%.*s' to unsigned int at %s:%zu:%zu: %s\n",
				(int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number,
				strerror(errno));
			success = false;
		}
		if (conv_endptr != token_end) {
			fprintf(stderr, "Failed to convert '%.*s' to unsigned int at %s:%zu:%zu: %s\n",
				(int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number,
				"Invalid character");
			success = false;
		}
		if (conv_value > UINT_MAX) {
			fprintf(stderr, "Failed to convert '%.*s' to unsigned int at %s:%zu:%zu: %s\n",
				(int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number,
				"Value out of range");
			success = false;
		}
		if (success) {
			csv->column_number += (token_end - token_start) + 1;
			*value = (unsigned int) conv_value;
		}
		free((void*) token);
	}
	return success;
}

bool csv_read_uchar(struct csv_file* csv, unsigned char* value) {
	bool success = true;
	char const* token_start = csv->line + csv->column_number;
	char const* token_end = token_start;
	char const* token = NULL;
	unsigned long int conv_value;
	char* conv_endptr;
	while (*token_end != '\0' && *token_end != ';') token_end++;
	token = strndup(token_start, token_end - token_start);
	if (token == NULL) {
		fprintf(stderr, "Failed to allocate memory for %s: %s\n", csv->filename, strerror(errno));
	} else {
		conv_value = strtoul(token_start, &conv_endptr, 10);
		if (conv_value == ULONG_MAX) {
			fprintf(stderr, "Failed to convert '%.*s' to unsigned char at %s:%zu:%zu: %s\n",
				(int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number,
				strerror(errno));
			success = false;
		}
		if (conv_endptr != token_end) {
			fprintf(stderr, "WARNING: Ignoring invalid character '%c' in '%.*s' at %s:%zu:%zu\n",
				*conv_endptr, (int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number);
		}
		if (conv_value > UCHAR_MAX) {
			fprintf(stderr, "Failed to convert '%.*s' to unsigned char at %s:%zu:%zu: %s\n",
				(int) (token_end - token_start), token_start,
				csv->filename, csv->line_number, csv->column_number,
				"Value out of range");
			success = false;
		}
		if (success) {
			csv->column_number += (token_end - token_start) + 1;
			*value = (unsigned char) conv_value;
		}
		free((void*) token);
	}
	return success;
}

bool csv_read_string(struct csv_file* csv, char const** value) {
	char const* token_start = csv->line + csv->column_number;
	char const* token_end = token_start;
	while (*token_end != '\0' && *token_end != ';') token_end++;
	csv->column_number += (token_end - token_start) + 1;
	*value = strdup(token_start);
	if (*value == NULL) {
		fprintf(stderr, "Failed to allocate memory for %s: %s\n", csv->filename, strerror(errno));
		return false;
	}
	return true;
}