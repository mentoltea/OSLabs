#include "functionality.h"

bool func_add_to_archive(Archive *arc, const char* arcpath, const char* filepath, bool recursive) {
    bool result = false;
    ElementInfo *new = NULL;

    bool to_root = false;
    ElementInfo *dir = NULL;

    if (strcmp(arcpath, "/") == 0) to_root = true;
    else dir = archive_get(arc, arcpath);

    if (!to_root && dir == NULL) goto DEFER;
    
    new = element_from_fs(filepath, recursive);

    if (new == NULL) goto DEFER;

    if (!archive_add(arc, dir, new)) goto DEFER;

    result = true;
DEFER:
    if (!result) {
        if (new) free_element(new);
    }
    return result;
}

bool func_extract_from_archive(Archive *arc, const char* arcpath, const char* extract_to, bool recursive) {
    bool result = false;
    bool from_root = false;
    
    if (strcmp(arcpath, "/") == 0) from_root = true;
    
    if (from_root) {
        ElementInfo *ptr = NULL;
        for (ptr = arc->element; ptr != NULL; ptr = ptr->next) {
            if (!archive_element_to_fs(arc, extract_to, ptr, recursive)) goto DEFER;
        } 
    } else {
        ElementInfo *element = archive_get(arc, arcpath);
        if (element == NULL) goto DEFER;
        if (!archive_element_to_fs(arc, extract_to, element, recursive)) goto DEFER;
    }
    
    result = true;
DEFER:
    return result;
}

bool func_delete_from_archive(Archive *arc, const char* arcpath) {
    bool from_root = false;
    
    if (strcmp(arcpath, "/") == 0) from_root = true;
    
    if (from_root) {
        if (arc->element) free_element_list(arc->element);
        arc->element = NULL;
        arc->element_count = 0;
        arc->was_changed = true;
    } else {
        archive_delete(arc, arcpath);
    }
    
    return true;
}

char types[] = "FD";
void print_archive(Archive *arc, ElementInfo* root, int tabs) {
    ElementInfo *first;
    if (root == NULL)  first = arc->element;
    else first = root->content.dir.child;
    if (tabs == 0) {
        printf("Archive %s\n", arc->arcpath);
        printf("Number of elements: %lu\n", arc->element_count);
        if (arc->element_count > 0) printf("Tree:\n");
    }

    ElementInfo *ptr;
    static char prefixes[] = {'\0', 'K', 'M', 'G', 'T'};
    for (ptr = first; ptr != NULL; ptr = ptr->next) {
        for (int t=0; t<tabs; t++) printf("\t");

        uint64_t size = element_get_content_size(ptr);
        double dsize = size;
        uint8_t prefix_index = 0;
        int after_dot_count = 2;

        while (dsize > 1024 && prefix_index < sizeof(prefixes)) {
            dsize = dsize / 1024;
            prefix_index++;
        }

        if (dsize - (int)dsize == 0.f) {
            after_dot_count = 0;
        }

        printf("%c %s %.*f %cB\n", types[ptr->type], ptr->name, after_dot_count, dsize, prefixes[prefix_index] );
        if (ptr->type == ELEM_DIR) print_archive(arc, ptr, tabs+1);
    }
}

void func_print_archive_info(Archive *arc) {
    print_archive(arc, NULL, 0);
}

void func_print_archive_help() {
    printf("archiver [ARCHIVE] [FLAGS] [OPTIONS]\n");
    printf("\n");
    printf("         [ARCHIVE] Path to archive file\n");
    printf("\n");
    printf("         [FLAGS]\n");
    printf("            -r      Recursive\n");
    printf("\n");
    printf("         [OPTIONS]\n");
    printf("            -s\n");
    printf("                    Prints archive info and file tree\n");
    printf("            -h\n");
    printf("                    Prints this message\n");
    printf("            -n  [FILEPATH]\n");
    printf("                    Creates a new archive with given filepath\n");
    printf("            -a  [FILEPATH]\n");
    printf("            -a  [ARCPATH] [FILEPATH]\n");
    printf("            -i  [FILEPATH]\n");
    printf("            -i  [ARCPATH] [FILEPATH]\n");
    printf("                    Adds filepath to archive at arcpath\n");
    printf("                    If arcpath is skipped, adds to the root of the archive\n");
    printf("                    When combined with -r flag, recursively adds all children (for directories)\n");
    printf("            -e [ARCPATH]\n");
    printf("            -e [ARCPATH] [OUTPATH]\n");
    printf("                    Extracts element from arcpath to folder outpath\n");
    printf("                    If outpath is missing, elements are extracted to current directory\n");
    printf("                    When combined with -r flag, recursively extracts all children (for directories)\n");
    printf("            -d [ARCPATH]\n");
    printf("                    Deletes element from archive\n");
}