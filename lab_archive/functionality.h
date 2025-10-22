#ifndef FUNCTIONALITY_H
#define FUNCTIONALITY_H

#include "archive.h"

bool func_new_archive(const char* filepath);

bool func_add_to_archive(Archive *arc, const char* arcpath, const char* filepath, bool recursive);

bool func_extract_from_archive(Archive *arc, const char* arcpath, const char* extract_to, bool recursive);

bool func_delete_from_archive(Archive *arc, const char* arcpath);

void func_print_archive_info(Archive *arc);

void func_print_archive_help();

#endif // FUNCTIONALITY_H