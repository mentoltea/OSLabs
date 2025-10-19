#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include "archive.h"

bool func_add_file_to_archive(Archive *arc, const char* filepath, int flags);
bool func_add_dir_to_archive(Archive *arc, const char* filepath, int flags);

bool func_delete_element_from_archive(Archive *arc, const char* filepath, int flags);

bool func_print_archive_info(Archive *arc, int flags);

bool func_print_archive_help(Archive *arc, const char* filepath, int flags);

#endif // FUNCTIONALITY_H