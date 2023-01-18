#ifndef OV2_PROVINCE_DEFINITIONS_H
#define OV2_PROVINCE_DEFINITIONS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct province_definition {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned int id;
    char* name;
};

/* Returns an empty list on failure */
void load_province_definitions(
	struct province_definition** definitions,
	size_t* count
);

void free_province_definitions(
	struct province_definition* definitions,
	size_t count
);

#endif /*OV2_PROVINCE_DEFINITIONS_H*/
