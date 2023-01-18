#ifndef OV2_LOCALIZATION_H
#define OV2_LOCALIZATION_H

#include "parse.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct localization {
	char* key;
    	char* english;
	char* french;
	char* german;
	char* polish;
	char* spanish;
	char* italian;
	char* swedish;
	char* czech;
	char* hungarian;
	char* dutch;
	char* portuguese;
	char* russian;
	char* finnish;
};

/* Returns an empty list on failure */
void load_localizations(
	struct localization** localizations,
	size_t* count
);

void free_localizations(
	struct localization* locs,
	size_t count
);

void localize_ui_widgets(
	struct ui_widget* widgets,
	struct localization* locs,
	size_t locs_count
);

#endif /*OV2_LOCALIZATION_H*/
