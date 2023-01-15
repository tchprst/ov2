#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <limits.h>
#include "province_definitions.h"

void load_province_definitions(
	struct province_definition** definitions,
	size_t* count
) {
	char line[10240];
	FILE* file = fopen("map/definition.csv", "r");
	*definitions = NULL;
	*count = 0;
	if (file == NULL) {
		fprintf(stderr, "Failed to open province.csv:%s\n", strerror(errno));
		return;
	}

	/* We ignore the first line, as it is just a csv header */
	if (fgets(line, sizeof(line), file) == NULL) {
		fprintf(stderr, "Failed to read first line of province.csv:%s\n", strerror(errno));
	} else {
		bool success = true;
		while (fgets(line, sizeof(line), file) != NULL) {
			char* line_end = strchr(line, '\n');
			char* token;
			long r, g, b, id;
			if (line_end == NULL) {
				fprintf(stderr, "Line too long in province.csv\n");
				success = false;
				break;
			}
			*definitions = realloc(*definitions, sizeof(struct province_definition) * (*count + 1));
			if (*definitions == NULL) {
				fprintf(stderr, "Failed to allocate memory for province definitions\n");
				success = false;
				break;
			}
			/* format is csv: `province id;red;green;blue;name;x` */
			/* id may be empty and the `x` column is ignored and optional */
			if (line[0] == ';') {
				(*definitions)[*count].id = 0;
				token = strtok(line + 1, ";");
			} else {
				if (
					(token = strtok(line, ";")) == NULL
					|| (id = strtol(token, NULL, 10), errno == ERANGE)
					|| id > UINT_MAX || id < 0
				) {
					fprintf(stderr, "Failed to parse province id: %s\n", line);
					success = false;
					break;
				}
				(*definitions)[*count].id = id;
				token = strtok(NULL, ";");
			}
			if (
				token == NULL
				|| (r = strtol(token, NULL, 10), errno == ERANGE)
				|| r > UCHAR_MAX || r < 0
			) {
				fprintf(stderr, "Failed to parse province red: %s\n", line);
				success = false;
				break;
			}
			(*definitions)[*count].r = r;
			if (
				(token = strtok(NULL, ";")) == NULL
				|| (g = strtol(token, NULL, 10), errno == ERANGE)
				|| g > UCHAR_MAX || g < 0
			) {
				fprintf(stderr, "Failed to parse province green: %s\n", line);
				success = false;
				break;
			}
			(*definitions)[*count].g = g;
			if (
				(token = strtok(NULL, ";")) == NULL
				|| (b = strtol(token, NULL, 10), errno == ERANGE)
				|| b > UCHAR_MAX || b < 0
			) {
				fprintf(stderr, "Failed to parse province blue: %s\n", line);
				success = false;
				break;
			}
			(*definitions)[*count].b = b;
			if ((token = strtok(NULL, ";")) == NULL) {
				fprintf(stderr, "Failed to parse province name: %s\n", line);
				success = false;
				break;
			}
			(*definitions)[*count].name = strdup(token);
			if ((*definitions)[*count].name == NULL) {
				fprintf(stderr, "Failed to allocate memory for province name: %s\n", line);
				success = false;
				break;
			}
			(*count)++;
		}
		if (!success) {
			size_t i;
			for (i = 0; i < *count; i++) {
				free((void*)(*definitions)[i].name);
			}
			free(*definitions);
			*definitions = NULL;
			*count = 0;
		}
	}
	if (ferror(file)) {
		fprintf(stderr, "Failed to read province.csv:%s\n", strerror(errno));
	}
	if (fclose(file) != 0) {
		fprintf(stderr, "Warning: Failed to close province.csv:%s\n", strerror(errno));
	}
}