#include "loadsave.h"

// ssize_t write(int fd, const void *buf, size_t count)
// ssize_t read(int fd, void *buf, size_t count)

bool write_and_check(int fd, const void *buf, size_t count) {
    ssize_t bytes = write(fd, buf, count); 
    if (bytes == -1) {
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        return false;
    }
    return bytes == (ssize_t)count;
}

bool read_and_check(int fd, void *buf, size_t count) {
    ssize_t bytes = read(fd, buf, count); 
    if ( bytes == -1 ) {
        fprintf(stderr, "Error reading: %s\n", strerror(errno));
        return false;
    }
    return bytes == (ssize_t)count;
}

off_t lseek_and_check(int fd, off_t offset, int whence) {
    off_t result = lseek(fd, offset, whence);
    if (result == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
    return result;
}

bool save_header(int fd, Archive *arc) {
    (void)(arc);
    bool result = false;

    struct ArchiveHeader head;
    head.major = MAJOR_V;
    head.minor = MINOR_V;
    head.element_offset = arc->element_count > 0 ? 0 : -1;

    if (!write_and_check(fd, SIGNATURE, sizeof(SIGNATURE)))     goto DEFER;
    if (!write_and_check(fd, &head.major, sizeof(head.major)))            goto DEFER;
    if (!write_and_check(fd, &head.minor, sizeof(head.minor)))            goto DEFER;
    if (!write_and_check(fd, &head.element_offset, sizeof(head.element_offset)))  goto DEFER;

    result = true;
DEFER:
    return result;
}

bool save_element(int fd, Archive *arc, ElementInfo *elem, bool modify_archive) {
    bool result = false;

    struct ElementHeader head = {0};
    head.type = elem->type;
    head.attr = elem->attributes;
    head.content_offset = 0;
    head.content_size = element_get_save_size(elem) - sizeof(struct ElementHeader);
    head.next_offset = elem->next ? (int64_t) head.content_size : (int64_t) -1;
    if (strlen(elem->name) >= PATH_MAX) {
        fprintf(stderr, "Element name cannot exceed %d symbols\n", PATH_MAX-1);
        goto DEFER;
    }
    strcpy(head.name, elem->name);

    int64_t header_offset = lseek(fd, 0, SEEK_CUR);
    
    // element header
    if (!write_and_check(fd, &head, sizeof(head))) goto DEFER;
    // if (!write_and_check(fd, &head.type, sizeof(head.type)))                        goto DEFER;
    // if (!write_and_check(fd, &head.attr, sizeof(head.attr)))                        goto DEFER;
    // if (!write_and_check(fd, &head.next_offset, sizeof(head.next_offset)))          goto DEFER;
    // if (!write_and_check(fd, &head.content_offset, sizeof(head.content_offset)))    goto DEFER;
    // if (!write_and_check(fd, &head.content_size, sizeof(head.content_size)))        goto DEFER;
    // if (!write_and_check(fd, name, sizeof(name)))                                   goto DEFER;
    
    int64_t content_offset = lseek(fd, 0, SEEK_CUR);

    // element content
    switch (elem->type) {
    case ELEM_FILE:{
        if (element_write_content_to_fd(fd, arc, elem) != -1) result = true;  
    } break;
    
    case ELEM_DIR: {
        result = save_element_list(fd, arc, elem->content.dir.child, modify_archive);
    } break;
    
    default: {
        fprintf(stderr, "[SAVE] Unexpected element type: %d\n", elem->type);
        goto DEFER;
    } break;

    }
    
DEFER:
    if (result) {
        if (modify_archive) {
            if (elem->type == ELEM_FILE) {
                if (elem->content.file.source == SOURCE_DRIVE) {
                    if (elem->content.file.descriptor.drive.filepath) 
                        free(elem->content.file.descriptor.drive.filepath);
                    if (elem->content.file.descriptor.drive.fd
                        && elem->content.file.descriptor.drive.fd != -1)
                        close(elem->content.file.descriptor.drive.fd);
                }

                elem->content.file.source = SOURCE_ARCHIVE;
                elem->content.file.descriptor.archive.header_offset = header_offset;
                elem->content.file.descriptor.archive.content_offset = content_offset;
                elem->content.file.descriptor.archive.size = head.content_size;
            }
        }
    }

    return result;
}

bool save_element_list(int fd, Archive *arc, ElementInfo *elem, bool modify_archive) {
    ElementInfo *ptr;
    for (ptr = elem; ptr != NULL; ptr = ptr->next) {
        if (!save_element(fd, arc, ptr, modify_archive)) return false;
    }
    return true;
}

bool archive_save(Archive *arc) {
    if (!arc->fd || arc->fd == -1) {
        fprintf(stderr, "Fatal: dont have access to archive file %s\n", arc->arcpath);
        return false;
    }

    bool result = false;
    size_t orig_length = strlen(arc->arcpath);
    char *temp_path = malloc(orig_length + 4 + 1);
    strcpy(temp_path, arc->arcpath);
    temp_path[orig_length] = '.';
    temp_path[orig_length + 1] = 'o';
    temp_path[orig_length + 2] = 'l';
    temp_path[orig_length + 3] = 'd';
    temp_path[orig_length + 4] = '\0';
    
    int old_fd = -1;
    old_fd = open(temp_path, O_RDWR | O_CREAT | O_TRUNC);
    if (old_fd == -1) {
        fprintf(stderr, "Cannot open path %s : %s\n", temp_path, strerror(errno));
        goto DEFER;
    }
    
    if (write_fd_to_fd(old_fd, arc->fd, 0, lseek(arc->fd, 0, SEEK_END)) == -1) goto DEFER;
    
    if (ftruncate(arc->fd, 0) == -1) {
        fprintf(stderr, "Cannot truncate file %s : %s\n", arc->arcpath, strerror(errno));
        goto DEFER;    
    }
    lseek(arc->fd, 0, SEEK_SET);
    lseek(old_fd, 0, SEEK_SET);

    int fd = arc->fd;
    arc->fd = old_fd;

    if (!save_header(fd, arc)) goto DEFER;

    if (arc->element == NULL) {
        result = true;
        goto DEFER;
    }

    if (!save_element_list(fd, arc, arc->element, true)) goto DEFER;

    arc->fd = fd;
    result = true;
DEFER:
    if (temp_path) free(temp_path);
    if (old_fd != -1) close(old_fd);
    return result;
}

bool archive_save_as(Archive *arc, const char* arcpath) {
    int fd = -1;
    bool result = false;
    fd = open(arcpath, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd == -1) {
        fprintf(stderr, "Cannot open path %s : %s\n", arcpath, strerror(errno));
        return false;
    }

    if (!save_header(fd, arc)) goto DEFER;

    if (arc->element == NULL) {
        result = true;
        goto DEFER;
    }

    if (!save_element_list(fd, arc, arc->element, false)) goto DEFER;

    result = true;
DEFER:  
    if (fd != -1) close(fd);
    return result;
}

bool load_header(int fd, Archive *arc, struct ArchiveHeader *header) {
    (void)(arc);
    bool result = false;

    if (!read_and_check(fd, header->signature, sizeof(header->signature)))     goto DEFER;
    if (!read_and_check(fd, &header->major, sizeof(header->major)))            goto DEFER;
    if (!read_and_check(fd, &header->minor, sizeof(header->minor)))            goto DEFER;
    if (!read_and_check(fd, &header->element_offset, sizeof(header->element_offset)))  goto DEFER;

    result = true;
DEFER:
    return result;
}

bool load_element(int fd, Archive *arc, ElementInfo **dest, struct LoadContext *ctx) {
    bool result = false;
    ElementInfo *element = NULL;
    struct ElementHeader header;

    int64_t header_offset = ctx->offset;
    if (!read_and_check(fd, &header, sizeof(header))) goto DEFER;
    ctx->offset = lseek(fd, 0, SEEK_CUR);
    
    element = calloc(1, sizeof(ElementInfo));
    switch (header.type) {
        case ELEM_FILE:
        case ELEM_DIR:
            element->type = header.type;
            break;
        
        default: {
            fprintf(stderr, "[LOAD] Unexpected element type: %d\n", header.type);
            goto DEFER;
        } break;
    }
    element->name = strdup(header.name);
    element->attributes = header.attr;


    if (element->type == ELEM_FILE) {
        element->content.file.source = SOURCE_ARCHIVE;
        element->content.file.descriptor.archive.size = header.content_size;
        element->content.file.descriptor.archive.header_offset = header_offset;
        element->content.file.descriptor.archive.content_offset = ctx->offset + header.content_offset;
    } else {
        if (header.content_size != 0) {
            struct LoadContext old_ctx = *ctx;
            
            if ( (ctx->offset = lseek_and_check(fd, header.content_offset, SEEK_CUR)) == -1) goto DEFER;
            ctx->parent = element;
            ctx->prev = NULL;
            
            if (!load_element_list(fd, arc, &element->content.dir.child, ctx)) goto DEFER;
            
            *ctx = old_ctx;
        }
    }

    if (ctx->parent != NULL) {
        ctx->parent->content.dir.children_count++;
    }
    element->prev = ctx->prev;
    ctx->prev = element;

    
    *dest = element;
    ctx->next_offset = header.next_offset;
    result = true;
DEFER:
    if (!result) {
        if (element) {
            free_element(element);
        }
    } else arc->element_count++;
    return result;
}

bool load_element_list(int fd, Archive *arc, ElementInfo **dest, struct LoadContext *ctx) {
    ElementInfo **ptr = dest;
    while (ctx->next_offset != -1) {
        ctx->offset = lseek(fd, ctx->next_offset, SEEK_CUR);
        if (!load_element(fd, arc, ptr, ctx)) return false;
        ptr = &(*dest)->next;    
    }
    return true;
}

bool archive_load(Archive *arc, const char* arcpath) {
    bool result = false;
    
    int fd = open(arcpath, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Cannot open path %s : %s\n", arcpath, strerror(errno));
        return false;
    }
    arc->fd = fd;
    arc->arcpath = strdup(arcpath);
    
    struct LoadContext ctx = {0};
    struct ArchiveHeader header;
    if (!load_header(fd, arc, &header)) goto DEFER;
    
    if (header.signature[sizeof(header.signature)-1] != '\0' || strcmp(header.signature, SIGNATURE) != 0) {
        fprintf(stderr, "File seems to be not an s7k archive\n");
        goto DEFER;
    }
    
    if (header.major != MAJOR_V) {
        fprintf(stderr, "Major versions conflict: %u.%u (archive) vs %u.%u (programm)\n", header.major, header.minor, MAJOR_V, MINOR_V);
        goto DEFER;
    }

    if (header.minor != MINOR_V) {
        // TODO: ask user if he wants to proceed on minor versions conflict
        fprintf(stderr, "Minor versions conflict: %u.%u (archive) vs %u.%u (programm)\n", header.major, header.minor, MAJOR_V, MINOR_V);
        goto DEFER;
    }
    
    ctx.offset = lseek(fd, 0, SEEK_CUR);
    ctx.offset = lseek(fd, header.element_offset, SEEK_CUR);
    ctx.next_offset = header.element_offset;

    if (!load_element_list(fd, arc, &arc->element, &ctx)) goto DEFER;

    result = true;
DEFER:
    if (!result) {
        if (arc->element) {
            free_element_list(arc->element);
        }
    }
    return result;
}

uint64_t element_get_save_size(ElementInfo *elem) {
    uint64_t size = sizeof(struct ElementHeader);
    if (elem->type == ELEM_FILE) {
        size += element_get_content_size(elem);
    } else {
        ElementInfo *ptr;
        for (ptr = elem->content.dir.child; ptr != NULL; ptr = ptr->next) {
            size += element_get_save_size(ptr);
        }
    }
    return size;
}