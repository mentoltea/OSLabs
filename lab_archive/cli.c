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
    for (ptr = first; ptr != NULL; ptr = ptr->next) {
        for (int t=0; t<tabs; t++) printf("\t");
        printf("%c %s %lu\n", types[ptr->type], ptr->name, element_get_content_size(ptr) );
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
    
    ElementInfo *dir = archive_get(&arc, "src");
    if (dir == NULL) {
        return 1;
    }

    ElementInfo *element = calloc(1, sizeof(ElementInfo));
    element->name = strdup("File.cpp");
    element->type = ELEM_FILE;
    element->content.file.source = SOURCE_DRIVE;
    element->content.file.descriptor.drive.filepath = strdup("./cli.c");
    archive_add(&arc, dir, element);
    
    // ElementInfo *element = calloc(1, sizeof(ElementInfo));
    // element->name = strdup("src");
    // element->type = ELEM_DIR;
    // archive_add(&arc, NULL, element);
    
    printf("----------------\n");
    print_archive(&arc, NULL, 0);
    archive_save(&arc);
    archive_free(&arc);
    return 0;
}