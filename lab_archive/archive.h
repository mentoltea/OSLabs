#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <error.h>
#include <errno.h>

#include "version.h"

#define MODE_RW_USR (S_IRUSR | S_IWUSR)
#define MODE_RW_GRP (S_IRGRP | S_IWGRP)
#define MODE_RW_OTH (S_IROTH | S_IWOTH)

struct Archive;
typedef struct Archive Archive;

struct ElementInfo;
typedef struct ElementInfo ElementInfo;

union ElementContent;
typedef union ElementContent ElementContent;

struct ElementAttributes;
typedef struct ElementAttributes ElementAttributes;

struct FileInfo;
typedef struct FileInfo FileInfo;

struct DirInfo;
typedef struct DirInfo DirInfo;

struct Archive {
    char* arcpath;
    int fd;

    ElementInfo *element;
    
    uint64_t element_count;

    bool was_changed;
};
typedef struct Archive Archive;


enum FileSource {
    SOURCE_DRIVE = 0,
    SOURCE_ARCHIVE,
    SOURCE_MEMORY,
};

struct FileDriveContentDescriptor {
    char* filepath;
    int fd;
};

struct FileArchiveContentDescriptor {
    int64_t header_offset;
    int64_t content_offset;
    uint64_t size;
};

struct FileMemoryContentDescriptor {
    void* ptr;
    uint64_t size;
};

union FileContentDescriptor {
    struct FileDriveContentDescriptor   drive;
    struct FileArchiveContentDescriptor archive;
    struct FileMemoryContentDescriptor  memory;
};


struct FileInfo {    
    enum FileSource source;
    union FileContentDescriptor descriptor;
};

struct DirInfo {
    struct ElementInfo *child; // poiter to first child. NOT A DYNAMIC ARRAY!
    uint64_t children_count;
};

typedef enum {
    ELEM_FILE = 0,
    ELEM_DIR,
} ElementType;

struct ElementAttributes {
    mode_t st_mode;
};

union ElementContent {
    struct FileInfo file;
    struct DirInfo  dir;
};

struct ElementInfo {
    struct ElementInfo *next;
    struct ElementInfo *prev;
    struct ElementInfo *parent;

    char* name;

    ElementType type;
    ElementAttributes attributes;
    ElementContent content;
};


bool archive_new(Archive *arc, const char* arcpath);
void archive_free(Archive *arc);

bool archive_add(Archive *arc, ElementInfo *root, ElementInfo *new);
bool archive_replace(Archive *arc, ElementInfo *old, ElementInfo *new);
void archive_delete(Archive *arc, const char *archive_path);
void archive_delete_element(Archive *arc, ElementInfo *elem);
ElementInfo *archive_get(Archive *arc, const char *archive_path);

bool archive_load(Archive *arc, const char* arcpath);
void archive_flush(Archive *arc);
bool archive_save(Archive *arc);
bool archive_save_as(Archive *arc, const char* arcpath);

uint64_t element_get_content_size(ElementInfo *elem);
// makes sense only for files
// returns number of bytes written
// -1 on error
int64_t element_write_content_to_fd(int fd, Archive *arc, ElementInfo *elem);

ElementInfo *element_new_directory(const char* name, ElementAttributes attributes);
ElementInfo *element_new_file_from_fs(const char* name, ElementAttributes attributes, const char* filepath);
// does copy memory
ElementInfo *element_new_file_from_memory(const char* name, ElementAttributes attributes, void* ptr, uint64_t size);

ElementInfo *element_from_fs(const char* filepath, bool recursive);

ElementInfo *element_add_child(ElementInfo *root, ElementInfo *new);
bool element_swap_content(ElementInfo *old, ElementInfo *new);

// NULL if unique
// Otherwise pointer to element with that name
ElementInfo *element_check_name_unique(Archive *arc, ElementInfo* root, const char* name);
ElementInfo *element_get_last_child(Archive *arc, ElementInfo* root);

void free_element(ElementInfo* elem);
void free_element_content(ElementInfo* elem);
void free_element_list(ElementInfo* elem);

int64_t write_fd_to_fd(int fdto, int fdfrom, int64_t offset_from, uint64_t size);

char *concat_strings(const char* restrict str1, const char* restrict str2);

ElementAttributes standart_attributes();

#endif // ARCHIVE_H