#include "localization.h"
#include "csv.h"
#include "fs.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

static void load_localizations_from_file(
	char const* path,
	struct localization** locs,
	size_t* count
) {
	struct csv_file* csv = csv_open(path);
	if (csv == NULL) {
		fprintf(stderr, "Failed to open '%s': %s\n",
		        path, strerror(errno));
		return;
	}
	if (!csv_read_line(csv)) {
		fprintf(stderr, "Failed to read first line of '%s': %s\n",
		        path, strerror(errno));
	} else {
		bool success = true;
		while (csv_read_line(csv)) {
			struct localization* loc = NULL;
			struct localization* new_locs = reallocarray(
				*locs, *count + 1, sizeof(struct localization)
			);
			if (new_locs == NULL) {
				fprintf(stderr, "Failed to allocate memory for "
				                "province definitions: %s\n",
				        strerror(errno));
				success = false;
				break;
			}
			*locs = new_locs;
			(*count)++;
			loc = &(*locs)[*count - 1];
			if (
				!csv_read_string(csv, &loc->key)
				|| !csv_read_string(csv, &loc->english)
				|| !csv_read_string(csv, &loc->french)
				|| !csv_read_string(csv, &loc->german)
				|| !csv_read_string(csv, &loc->polish)
				|| !csv_read_string(csv, &loc->spanish)
				|| !csv_read_string(csv, &loc->italian)
				|| !csv_read_string(csv, &loc->swedish)
				|| !csv_read_string(csv, &loc->czech)
				|| !csv_read_string(csv, &loc->hungarian)
				|| !csv_read_string(csv, &loc->dutch)
				|| !csv_read_string(csv, &loc->portuguese)
				|| !csv_read_string(csv, &loc->russian)
				|| !csv_read_string(csv, &loc->finnish)
			) {
				fprintf(stderr, "Failed to read '%s'.", path);
				success = false;
				break;
			}
		}
		if (!success) {
			if (*locs != NULL) {
				free_localizations(*locs, *count);
				*locs = NULL;
			}
			*count = 0;
		}
	}
	csv_close(csv);
}

void load_localizations(
	struct localization** localizations,
	size_t* count
) {
	*localizations = NULL;
	*count = 0;

	DIR* dir;
	struct dirent* entry;
	if ((dir = opendir("localisation")) == NULL) {
		fprintf(stderr, "Failed to open localization directory\n");
		return;
	}
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_REG) continue;
		char* path = malloc(strlen("localisation/")
		             + strlen(entry->d_name) + 1);
		if (path == NULL) {
			fprintf(stderr, "Failed to allocate memory for path.\n");
			break;
		}
		strcpy(path, "localisation/");
		strcat(path, entry->d_name);
		load_localizations_from_file(path, localizations, count);
		free(path);
	}
	closedir(dir);
}

void free_localizations(
	struct localization* locs,
	size_t count
) {
	size_t i;
	for (i = 0; i < count; i++) {
		if(locs[i].key != NULL) free(locs[i].key);
		if(locs[i].english != NULL) free(locs[i].english);
		if(locs[i].french != NULL) free(locs[i].french);
		if(locs[i].german != NULL) free(locs[i].german);
		if(locs[i].polish != NULL) free(locs[i].polish);
		if(locs[i].spanish != NULL) free(locs[i].spanish);
		if(locs[i].italian != NULL) free(locs[i].italian);
		if(locs[i].swedish != NULL) free(locs[i].swedish);
		if(locs[i].czech != NULL) free(locs[i].czech);
		if(locs[i].hungarian != NULL) free(locs[i].hungarian);
		if(locs[i].dutch != NULL) free(locs[i].dutch);
		if(locs[i].portuguese != NULL) free(locs[i].portuguese);
		if(locs[i].russian != NULL) free(locs[i].russian);
		if(locs[i].finnish != NULL) free(locs[i].finnish);
	}
	free(locs);
}

/* This returns a caller-owned string, and will free any existing `text` string
 * if a localization is found. */
static char* localize_text(char* text, struct localization* locs, size_t count) {
	size_t i;
	if(text == NULL) return NULL;
	for (i = 0; i < count; i++) {
		if (strcmp(locs[i].key, text) == 0) {
			if(text != NULL) free(text);
			return strdup(locs[i].english);
		}
	}
	return text;
}

void localize_ui_widgets(
	struct ui_widget* widgets,
	struct localization* locs,
	size_t locs_count
) {
	/* TODO: Tooltips. */
	size_t i;
	for (; widgets != NULL; widgets = widgets->next) {
		switch (widgets->type) {
		case TYPE_WINDOW:
			localize_ui_widgets(
				widgets->window.children,
				locs, locs_count
			);
			break;
		case TYPE_BUTTON:
			widgets->button.button_text = localize_text(
				widgets->button.button_text,
				locs, locs_count
			);
			break;
		case TYPE_TEXT_BOX:
			widgets->text_box.text = localize_text(
				widgets->text_box.text,
				locs, locs_count
			);
			break;
		case TYPE_INSTANT_TEXT_BOX:
			widgets->instant_text_box.text = localize_text(
				widgets->instant_text_box.text,
				locs, locs_count
			);
			break;
		case TYPE_SCROLLBAR:
			localize_ui_widgets(
				widgets->scrollbar.children,
				locs, locs_count
			);
			break;
		case TYPE_CHECKBOX:
			widgets->checkbox.button_text = localize_text(
				widgets->checkbox.button_text,
				locs, locs_count
			);
			break;
		case TYPE_EDIT_BOX:
			widgets->edit_box.text = localize_text(
				widgets->edit_box.text,
				locs, locs_count
			);
			break;
		case TYPE_EU3_DIALOG:
			localize_ui_widgets(
				widgets->eu3_dialog.children,
				locs, locs_count
			);
			break;
		case TYPE_ICON:
		case TYPE_OVERLAPPING_ELEMENTS_BOX:
		case TYPE_LIST_BOX:
		case TYPE_SHIELD:
		case TYPE_POSITION:
			break;
		default:
			assert(0);
		}
	}
}