#ifndef OV2_FS_H
#define OV2_FS_H

#ifdef _WIN32
#include "win32/dirent.h.h"
#else
#include <dirent.h>
#endif
#include <stdbool.h>

bool has_ext(char const* path, char const* ext);

#endif //OV2_FS_H
