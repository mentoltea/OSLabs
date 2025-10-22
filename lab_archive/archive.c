#include "archive.h"

void free_element(ElementInfo* elem) {
    if (elem->name) free(elem->name);
    free_element_content(elem);
    free(elem);
}

void free_element_content(ElementInfo* elem) {
    if (elem->type == ELEM_DIR) {
        if (elem->content.dir.child) {
            free_element_list(elem->content.dir.child);
        }
        return;
    }
    
    assert(elem->type == ELEM_FILE);

    switch (elem->content.file.source) {
        case SOURCE_ARCHIVE: break;

        case SOURCE_DRIVE: {
            if (elem->content.file.descriptor.drive.filepath) {
                free(elem->content.file.descriptor.drive.filepath); 
            }
            if (elem->content.file.descriptor.drive.fd != 0 && elem->content.file.descriptor.drive.fd != -1) {
                close(elem->content.file.descriptor.drive.fd);
            }
        } break;

        case SOURCE_MEMORY: {
            if (elem->content.file.descriptor.memory.ptr) free(elem->content.file.descriptor.memory.ptr);
        } break;    

        default: {
            fprintf(stderr, "Unknown source type: %d\n", elem->content.file.source);
            exit(2);
        } break;
    }
}

void free_element_list(ElementInfo* elem) {
    ElementInfo *ptr = elem;
    ElementInfo *next = ptr->next;
    while (ptr != NULL) {
        next = ptr->next;
        free_element(ptr);
        ptr = next;
    }
}

void archive_free(Archive *arc) {
    if (arc->arcpath) {
        free(arc->arcpath);
        arc->arcpath = NULL;
    }
    if (arc->element) {
        free_element_list(arc->element);
        arc->element = NULL;
        arc->element_count = 0;
    }
    if (arc->fd) {
        close(arc->fd);
        arc->fd = 0;
    }
}

uint64_t element_get_content_size(ElementInfo *elem) {
    uint64_t size = 0;
    if (elem->type == ELEM_FILE) {
        switch (elem->content.file.source) {
            case SOURCE_DRIVE: { 
                struct FileDriveContentDescriptor *cd = &elem->content.file.descriptor.drive;
                
                if (!cd->fd) {
                    cd->fd = open(cd->filepath, O_RDONLY);
                    if (cd->fd == -1) {
                        fprintf(stderr, "Cannot open file %s : %s\n", cd->filepath, strerror(errno));
                    }
                }
                
                if (cd->fd == -1) {
                    fprintf(stderr, "Assuming %s has 0 size\n", cd->filepath);
                    break;
                }
                
                size = lseek(cd->fd, 0, SEEK_END);
            } break;
            case SOURCE_ARCHIVE: { size = elem->content.file.descriptor.archive.size; } break;
            case SOURCE_MEMORY: { size = elem->content.file.descriptor.memory.size; } break;
        }
    } else {
        ElementInfo *ptr;
        for (ptr = elem->content.dir.child; ptr != NULL; ptr = ptr->next) {
            size += element_get_content_size(ptr);
        }
    }
    return size;
}

int64_t element_write_content_to_fd(int fd, Archive *arc, ElementInfo *elem) {
    if (elem->type == ELEM_DIR) {
        fprintf(stderr, "Cannot write element of type DIR directly to fd\n");
        return -1;
    }
    
    int64_t written = 0;
    switch (elem->content.file.source) {
        case SOURCE_DRIVE: {
            struct FileDriveContentDescriptor *cd = &elem->content.file.descriptor.drive;
            if (!cd->fd) {
                cd->fd = open(cd->filepath, O_RDONLY);
                if (cd->fd == -1) {
                    fprintf(stderr, "Cannot open file %s : %s\n", cd->filepath, strerror(errno));
                }
            }
            
            if (cd->fd == -1) {
                fprintf(stderr, "Assuming %s has 0 size. Nothing written.\n", cd->filepath);
                break;
            }
            
            uint64_t size = lseek(cd->fd, 0, SEEK_END);
            
            written = write_fd_to_fd(fd, cd->fd, 0, size);
        } break;
        case SOURCE_ARCHIVE: {
            struct FileArchiveContentDescriptor *ad = &elem->content.file.descriptor.archive;
            if (!arc->fd || arc->fd==-1) {
                fprintf(stderr, "Fatal: dont have access to archive file %s\n", arc->arcpath);
                written = -1;
                break;
            }

            written = write_fd_to_fd(fd, arc->fd, ad->content_offset, ad->size);
        } break;
        case SOURCE_MEMORY: {
            struct FileMemoryContentDescriptor *memory = &elem->content.file.descriptor.memory;
            written = write(fd, memory->ptr, memory->size);
        } break;

        default: {
            fprintf(stderr, "Unexpected source type: %d\n", elem->content.file.source);
            return -1;
        } break;
    }

    return written;
}


#define BUFFER_SIZE 4096
static uint8_t READ_WRITE_BUFFER[BUFFER_SIZE];
int64_t write_fd_to_fd(int fdto, int fdfrom, int64_t offset_from, uint64_t size) {
    lseek(fdfrom, offset_from, SEEK_SET);
    uint64_t written = 0;
    while (written < size) {
        uint64_t current_read = size - written > BUFFER_SIZE ? BUFFER_SIZE : size - written;
        
        if (read(fdfrom, READ_WRITE_BUFFER, current_read) == -1) return -1;

        if (write(fdto, READ_WRITE_BUFFER, current_read) == -1) return -1;

        written += current_read;
    }
    return (int64_t) written;
}


ElementInfo *archive_get(Archive *arc, const char *archive_path) {
    ElementInfo *result = NULL;

    char* buff = strdup(archive_path);
    char *path = buff;
    size_t length = strlen(path);
    
    while (path[0] == '/') {
        path = path+1;
        length--;
        if (length == 0) {
            fprintf(stderr, "Cannot resolve path %s\n", archive_path);
            goto DEFER;
        }
    }
    while (path[length-1] == '/') {
        path[length-1] = '\0';
        length--;
        if (length == 0) {
            fprintf(stderr, "Cannot resolve path %s\n", archive_path);
            goto DEFER;
        }
    }
    
    char* cur_path = path;
    char* prev_path = NULL;
    char* top_name = NULL;

    ElementInfo *first_in_root = arc->element;
    ElementInfo *ptr = NULL;

    while (1) {
        char* delimitor = strchr(cur_path, '/');
        if (delimitor == NULL) {
            top_name = cur_path;
            cur_path = NULL;
        }
        else {
            *delimitor = '\0';
            top_name = cur_path;
            prev_path = cur_path;
            cur_path = delimitor + 1;
        }
        
        bool found = false;
        for (ptr = first_in_root; ptr != NULL; ptr = ptr->next) {
            if (strcmp(ptr->name, top_name) == 0) {
                if (cur_path == NULL) {
                    result = ptr;
                    goto DEFER;
                }
                
                if (ptr->type != ELEM_DIR) {
                    fprintf(stderr, "Element `%s` is not a dir and cannot have children\n", top_name);
                    goto DEFER;
                }
                
                found = true;
                first_in_root = ptr->content.dir.child;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "No element named `%s` found", top_name);
            if (prev_path && prev_path != top_name) fprintf(stderr, " in `%s`", prev_path);
            fprintf(stderr, "\n");
            goto DEFER;
        }
    } 

DEFER:
    if (buff) free(buff);
    return result;
}

void archive_delete(Archive *arc, const char *archive_path) {
    ElementInfo* element = archive_get(arc, archive_path);
    if (element == NULL) return;

    ElementInfo *prev = element->prev;
    ElementInfo *next = element->next;
    ElementInfo *parent = element->parent;

    if (prev) prev->next = next;
    if (next) next->prev = prev;
    if (parent) {
        if (parent->content.dir.child == element) parent->content.dir.child = next;
        parent->content.dir.children_count--;
    }
    if (arc->element == element) {
        arc->element = next;
    }
    arc->element_count--;
    arc->was_changed = true;

    free_element(element);
}

bool archive_add(Archive *arc, ElementInfo *root, ElementInfo *new) {
    ElementInfo **first;
    ElementInfo *parent;
    
    if (root == NULL) {
        first = &arc->element;
        parent = NULL;
    }
    else {
        if (root->type != ELEM_DIR) {
            fprintf(stderr, "Can add elements only to DIR. %s is not a dir.", root->name);
            return false;
        }
        first = &root->content.dir.child;
        parent = root;
    }
    new->parent = parent;
    
    ElementInfo **ptr = first;
    while (*ptr != NULL) {
        if (strcmp((*ptr)->name, new->name) == 0) {
            fprintf(stderr, "Element %s alredy exists in %s\n", new->name, root ? root->name : "/");
            return false;
        }
        ptr = &(*ptr)->next;
    }
    *ptr = new;
    
    if (root) root->content.dir.children_count++;
    arc->element_count++;
    if (new->type == ELEM_DIR) arc->element_count += new->content.dir.children_count;
    arc->was_changed = true;
    
    return true;
}

bool archive_replace(Archive *arc, ElementInfo *old, ElementInfo *new) {
    if (strcmp(old->name, new->name)!=0) {
        fprintf(stderr, "Cannot replace elements with different names: `%s` (old) and `%s` (new)\n", old->name, new->name);
        return false;
    }
    new->prev = old->prev;
    new->next = old->next;
    new->parent = old->next;

    if (new->prev) new->prev->next = new;
    if (new->next) new->next->prev = new;
    if (new->parent) {
        if (new->parent->content.dir.child == old) new->parent->content.dir.child = new;
    }
    if (arc->element == old) arc->element = new;

    free_element(old);
    return true;
}

void archive_flush(Archive *arc) {
    archive_save(arc);
}

bool archive_new(Archive *arc, const char* arcpath) {
    arc->arcpath = strdup(arcpath);
    arc->fd = open(arc->arcpath, O_RDWR | O_CREAT | O_EXCL, MODE_RW_USR | MODE_RW_GRP | MODE_RW_OTH);
    if (arc->fd == -1) {
        fprintf(stderr, "Cannot create new archive %s : %s\n", arcpath, strerror(errno));
        return false;
    }
    arc->element = NULL;
    arc->element_count = 0;
    arc->was_changed = true;
    return true;
}

ElementInfo *element_new_directory(const char* name, ElementAttributes attributes) {
    ElementInfo *elem = NULL;
    elem = calloc(1, sizeof(ElementInfo));
    elem->name = strdup(name);
    elem->type = ELEM_DIR;
    elem->attributes = attributes;
    return elem;
}

ElementInfo *element_new_file_from_fs(const char* name, ElementAttributes attributes, const char* filepath) {
    ElementInfo *elem = NULL;
    elem = calloc(1, sizeof(ElementInfo));
    elem->name = strdup(name);
    elem->type = ELEM_FILE;
    elem->attributes = attributes;
    elem->content.file.source = SOURCE_DRIVE;
    elem->content.file.descriptor.drive.filepath = strdup(filepath);
    return elem;
}

ElementInfo *element_new_file_from_memory(const char* name, ElementAttributes attributes, void* ptr, uint64_t size) {
    ElementInfo *elem = NULL;
    elem = calloc(1, sizeof(ElementInfo));
    elem->name = strdup(name);
    elem->type = ELEM_FILE;
    elem->attributes = attributes;
    elem->content.file.source = SOURCE_MEMORY;
    elem->content.file.descriptor.memory.ptr = malloc(size);
    memcpy(elem->content.file.descriptor.memory.ptr, ptr, size);
    elem->content.file.descriptor.memory.size = size;
    return elem;
}

ElementInfo *element_add_child(ElementInfo *root, ElementInfo *new) {
    if (root->type != ELEM_DIR) {
        fprintf(stderr, "Can add elements only to DIR. %s is not a dir.", root->name);
        return NULL;
    }
    new->parent = root;
    ElementInfo **first = &root->content.dir.child;

    ElementInfo **ptr = first;
    while (*ptr != NULL) {
        if (strcmp((*ptr)->name, new->name) == 0) {
            fprintf(stderr, "Element %s alredy exists in %s\n", new->name, root->name);
            return NULL;
        }
        ptr = &(*ptr)->next;
    }
    *ptr = new;

    root->content.dir.children_count++;

    return new;
}

bool element_swap_content(ElementInfo *old, ElementInfo *new) {
    ElementType oldtype = old->type;
    ElementAttributes oldattr = old->attributes;
    ElementContent oldcontent = old->content;

    old->type = new->type;
    old->attributes = new->attributes;
    old->content = new->content;
    
    new->type = oldtype;
    new->attributes = oldattr;
    new->content = oldcontent;
    
    return true;
}

ElementInfo *element_from_fs(const char* filepath, bool recursive) {
    ElementInfo *result = NULL;
    ElementInfo *element = NULL;
    DIR* dir = NULL;
    char* buff = NULL;
    struct stat info;
    
    if (stat(filepath, &info) != 0) {
        fprintf(stderr, "Cannot stat `%s`: %s\n", filepath, strerror(errno));
        goto DEFER;
    } 
    
    element = calloc(1, sizeof(ElementInfo));
    
    if (S_ISDIR(info.st_mode)) {
        element->type = ELEM_DIR;
    } else if (S_ISREG(info.st_mode)) {
        element->type = ELEM_FILE;
    } else {
        fprintf(stderr, "`%s` is not a file nor a directory\n", filepath);
        goto DEFER;   
    }
    element->attributes.st_mode = info.st_mode;
    
    buff = strdup(filepath);
    size_t length = strlen(buff);
    while (length > 0 && buff[length-1]=='/') {
        length--;
        buff[length] = '\0';
    }
    
    if (length == 0) {
        fprintf(stderr, "Invalid filepath: %s\n", filepath);
        goto DEFER;       
    }
    
    char *ptr = buff;
    char *chr = NULL;
    while ( (chr=strchr(ptr, '/')) != NULL ) {
        ptr = ++chr;
    }
    element->name = strdup(ptr);
    
    if (element->type == ELEM_FILE) {
        element->content.file.source = SOURCE_DRIVE;
        element->content.file.descriptor.drive.filepath = strdup(filepath);
    } else if (recursive) {
        dir = opendir(filepath);
        if (!dir) {
            fprintf(stderr, "%s\n", strerror(errno));
            goto DEFER;
        }
        struct dirent *entry;
        ElementInfo *entryinfo;
        char* full_path = NULL;

        size_t filepath_length = strlen(filepath);
        bool no_slash = !(filepath[filepath_length-1] == '/');
        for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
            size_t entryname_length = strlen(entry->d_name);

            if (entryname_length == 0) continue;
            if (entryname_length == 1 && entry->d_name[0]=='.') continue;
            if (entryname_length == 2 && entry->d_name[0]=='.' && entry->d_name[1]=='.') continue;
            
            full_path = malloc(filepath_length + entryname_length + no_slash + 1);
            memcpy(full_path, filepath, filepath_length);
            if (no_slash) full_path[filepath_length] = '/';
            memcpy(full_path + filepath_length + no_slash, entry->d_name, entryname_length);
            full_path[filepath_length + no_slash + entryname_length] = '\0';
            
            entryinfo = element_from_fs(full_path, recursive);
            if (entryinfo) {
                element_add_child(element, entryinfo);
            }

            free(full_path);
        }
    }
    
    result = element;
DEFER:
    if (dir) closedir(dir);
    if (buff) free(buff);
    if (!result) {
        if (element) free_element(element);
    }
    return result;
}

bool archive_element_to_fs(Archive *arc, const char* dir, ElementInfo* element, bool recursive) {
    bool result = false;
    int fd = 0;
    char *filepath = NULL;
    char *buff = NULL;

    struct stat dirinfo;
    if (stat(dir, &dirinfo) != 0) {
        fprintf(stderr, "Cannot stat `%s`: %s\n", dir, strerror(errno));
        goto DEFER;
    }
    
    if (!S_ISDIR(dirinfo.st_mode)) {
        fprintf(stderr, "`%s` is not a dir\n", dir);
        goto DEFER;
    }

    size_t dir_length = strlen(dir);
    bool no_slash = dir[dir_length-1]!='/';

    buff = malloc(dir_length + 1 + no_slash);
    memcpy(buff, dir, dir_length);
    if (no_slash) buff[dir_length] = '/';
    buff[dir_length + no_slash] = '\0';
    
    filepath = concat_strings(buff, element->name);
    
    if (element->type == ELEM_FILE) {
        fd = open(filepath, O_WRONLY | O_EXCL | O_CREAT | O_TRUNC, element->attributes.st_mode);
        
        if (fd == -1) {
            fprintf(stderr, "Cannot open file `%s`: %s\n", filepath, strerror(errno));
            goto DEFER;
        }

        if (element_write_content_to_fd(fd, arc, element) == -1) {
            goto DEFER;
        }
    } else {
        if (mkdir(filepath, element->attributes.st_mode) != 0) {
            fprintf(stderr, "Cannot create directory `%s`: %s\n", filepath, strerror(errno));
            goto DEFER;
        }
        if (recursive) {
            ElementInfo *ptr = NULL;
            for (ptr = element->content.dir.child; ptr != NULL; ptr = ptr->next) {
                archive_element_to_fs(arc, filepath, ptr, recursive);
            }
        }
    }

    result = true;
DEFER:
    if (buff) free(buff);
    if (filepath) free(filepath);
    if (fd && fd != -1) close(fd);
    return result;
}

char *concat_strings(const char* restrict str1, const char* restrict str2) {
    size_t size1 = strlen(str1);
    size_t size2 = strlen(str2);
    char* result = malloc(size1 + size2 + 1);
    memcpy(result, str1, size1);
    memcpy(result + size1, str2, size2);
    result[size1 + size2] = '\0';
    return result;
}