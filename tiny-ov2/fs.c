#include "fs.h"
#include <string.h>

bool has_ext(char const* path, char const* ext) {
	path = strrchr(path, '.');
	return path && !strcmp(path, ext);
}