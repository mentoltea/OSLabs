#include "archive.h"
#include "loadsave.h"
#include "functionality.h"

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

int main() {
    Archive arc = {0};

    // if (!archive_new(&arc, "arschive.s7k")) {
    //     return 1;
    // }
    if (!archive_load(&arc, "arschive.s7k")) {
        return 1;
    }
    print_archive(&arc, NULL, 0);
    

    ElementInfo *file = archive_get(&arc, "src/File.c");
    if (file) {
        ElementAttributes attr = {0};
        ElementInfo *newfile = element_new_file_from_fs("file.c", attr, "./Makefile");

        element_swap_content(file, newfile);
        
        if (strcmp(file->name, "Makefile") != 0) {
            free(file->name);
            file->name = strdup("Makefile");
        }

        free_element(newfile);
    }
    
    printf("----------------\n");
    print_archive(&arc, NULL, 0);
    archive_save(&arc);
    archive_free(&arc);
    return 0;
}