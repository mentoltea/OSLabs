#ifndef LOADSAVE_H
#define LOADSAVE_H

#include "archive.h"

#define SIGNATURE "s7k arc"

struct ArchiveHeader {
    char signature[8]; // s7k arc\0
    uint32_t major;
    uint32_t minor;
    int64_t element_offset; // relative after header
};

struct ElementHeader {
    uint16_t type;
    ElementAttributes attr;
    int64_t next_offset; // -1 if no next
    int64_t content_offset;
    uint64_t content_size;
    char name[PATH_MAX];    
};

bool write_and_check(int fd, const void *buf, size_t count);
bool read_and_check(int fd, void *buf, size_t count);
off_t lseek_and_check(int fd, off_t offset, int whence);

bool save_header(int fd, Archive *arc);
bool load_header(int fd, Archive *arc, struct ArchiveHeader *header);

bool save_element_list(int fd, Archive *arc, ElementInfo *elem, bool modify_archive);
bool save_element(int fd, Archive *arc, ElementInfo *elem, bool modify_archive);

struct LoadContext {
    ElementInfo *parent;
    ElementInfo *prev;
    int64_t offset;
    int64_t next_offset;
};

bool load_element_list(int fd, Archive *arc, ElementInfo **dest, struct LoadContext *ctx);
bool load_element(int fd, Archive *arc, ElementInfo **dest,  struct LoadContext *ctx);

// content size + element header size for files
// sum for dirs
uint64_t element_get_save_size(ElementInfo *elem);

#endif 