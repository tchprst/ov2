#include "province_definitions.h"
#include "csv.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

void load_province_definitions(
	struct province_definition** definitions,
	size_t* count
) {
	struct csv_file* csv = csv_open("map/definition.csv");
	*definitions = NULL;
	*count = 0;
	if (csv == NULL) {
		fprintf(stderr, "Failed to open map/definition.csv: %s\n", strerror(errno));
		return;
	}
	if (!csv_read_line(csv)) {
		fprintf(stderr, "Failed to read first line of map/definition.csv: %s\n", strerror(errno));
	} else {
		bool success = true;
		while (csv_read_line(csv)) {
			struct province_definition* new_definitions = reallocarray(*definitions, *count + 1, sizeof(struct province_definition));
			if (new_definitions == NULL) {
				fprintf(stderr, "Failed to allocate memory for province definitions: %s\n", strerror(errno));
				success = false;
				break;
			}
			*definitions = new_definitions;
			if (
				!csv_read_uint(csv, &(*definitions)[*count].id)
				|| !csv_read_uchar(csv, &(*definitions)[*count].r)
				|| !csv_read_uchar(csv, &(*definitions)[*count].g)
				|| !csv_read_uchar(csv, &(*definitions)[*count].b)
				|| !csv_read_string(csv, &(*definitions)[*count].name)
			) {
				fprintf(stderr, "Failed to read map/definition.csv");
				success = false;
				break;
			}
			(*count)++;
		}
		if (!success) {
			if (*definitions != NULL) {
				size_t i;
				for (i = 0; i < *count; i++) {
					free((void*) (*definitions)[i].name);
				}
				free(*definitions);
				*definitions = NULL;
			}
			*count = 0;
		}
	}
	csv_close(csv);
}