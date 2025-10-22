#include "archive.h"
#include "functionality.h"

enum Flags {
    NONE = 0,
    RECURSIVE = 1,
};

enum Target {
    T_NONE,
    HELP,
    STAT,
    NEW,
    ADD,
    EXTRACT,
    DELETE,
};

static char *arcpath = NULL;
static Archive archive = {0};
static int return_value = 0;

#define ARG_MAX 5
int arg_count(char * arguments[ARG_MAX]) {
    int i;
    for (i=0; i<ARG_MAX; i++) {
        if (arguments[i] == NULL) break;
    }
    return i;
}

void see_help() {
    fprintf(stderr, "See archiver -h for help\n");
}

bool expect_arg_count(char *from, int expected, int real) {
    if (expected > real) {
        fprintf(stderr, "%s: Too many arguments (Expected %d, got %d)\n", from, expected, real);
        see_help();
        return false;
    }
    if (expected < real) {
        fprintf(stderr, "%s: Too few arguments (Expected %d, got %d)\n", from, expected, real);
        see_help();
        return false;
    }
    return true;
}

bool apply(enum Flags flags, enum Target target, char * arguments[ARG_MAX]) {
    int argc = arg_count(arguments);

    switch (target) {
        case NONE: {
            if (argc != 0) {
                if (!expect_arg_count("Archive open", 1, argc)) return false;
                arcpath = arguments[0];
                if (!archive_load(&archive, arcpath)) {
                    fprintf(stderr, "Cannot load archive `%s`\n", arcpath);
                    return false;                
                }
            }
        } break;

        case HELP: {
            if (!expect_arg_count("help", 0, argc)) return false;
            func_print_archive_help();
        } break;

        case STAT: {
            if (!expect_arg_count("help", 0, argc)) return false;
            if (archive.arcpath == NULL) {
                fprintf(stderr, "stat: needs an archive opened\n");
                see_help();
                return false;
            }
            func_print_archive_info(&archive);
        } break;

        case NEW: {
            if (!expect_arg_count("help", 1, argc)) return false;
            char *arg = arguments[0];
            if (archive.arcpath != NULL) {
                fprintf(stderr, "new: archive `%s` is already opened\n", archive.arcpath);
                see_help();
                return false;
            }
            if (!archive_new(&archive, arg)) return false;
            arcpath = arg;
        } break;

        case ADD: {
            if (argc != 1 && argc != 2) {
                fprintf(stderr, "add: expected 1 or 2 arguments (got %d)\n", argc);
                see_help();
                return false;
            }
            if (archive.arcpath == NULL) {
                fprintf(stderr, "add: needs an archive opened\n");
                see_help();
                return false;
            }
            char *arg1 = arguments[0];
            char *arg2 = arguments[1];
            if (arg2) {
                if (!func_add_to_archive(&archive, arg1, arg2, flags&RECURSIVE)) return false;
            } else {
                if (!func_add_to_archive(&archive, "/", arg1, flags&RECURSIVE)) return false;
            }
        } break;
        
        case EXTRACT: {
            if (!expect_arg_count("extract", 2, argc)) return false;
            if (archive.arcpath == NULL) {
                fprintf(stderr, "extract: needs an archive opened\n");
                see_help();
                return false;
            }
            char *arg1 = arguments[0];
            char *arg2 = arguments[1];
            if (!func_extract_from_archive(&archive, arg1, arg2, flags&RECURSIVE)) return false;
        } break;

        case DELETE: {
            if (!expect_arg_count("delete", 1, argc)) return false;
            if (archive.arcpath == NULL) {
                fprintf(stderr, "delete: needs an archive opened\n");
                see_help();
                return false;
            }
            char *arg1 = arguments[0];
            if (!func_delete_from_archive(&archive, arg1)) return false;
        } break;
    }

    return true;
}

// -1 if not target
int get_target(char* arg) {
    switch (arg[1]) {
        case 'h': return HELP;
        case 's': return STAT;
        case 'n': return NEW; 
        case 'a': return ADD;
        case 'e': return EXTRACT;
        case 'd': return DELETE;
    }
    return -1;
}

// -1 if not flag
int get_flag(char* arg) {
    switch (arg[1]) {
        case 'r': return RECURSIVE;
    }
    return -1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments. See archiver -h to view usage\n");
        return 1;
    }

    enum Flags default_flags = NONE;
    enum Flags flags = NONE;
    enum Target target = T_NONE;
    char * arguments[ARG_MAX] = {0};
    
    int current_arg = 0;
    for (int ind = 1; ind < argc; ind++) {
        char *arg = argv[ind];

        if (arg[0] == '-') {
            int t = get_target(arg);
            if (t == -1) {
                // flag
                t = get_flag(arg);
                if (t == -1) {
                    fprintf(stderr, "Unexpected flag: %s\n", arg);
                    return 1;
                }
                
                flags = flags | t;
            } else {
                // new target
                if (!apply(flags, target, arguments)) {
                    return 1;
                }
                
                flags = default_flags;
                for (int i=0; i<ARG_MAX; i++) {
                    arguments[i] = NULL;
                }
                current_arg = 0;
                
                target = t;
            }
        } else {
            if (current_arg == ARG_MAX) {
                fprintf(stderr, "Too many arguments per target. Max arguments: %d\n", ARG_MAX);
                return 1;
            }
            arguments[current_arg++] = arg;
        }
    }
    
    if (target == T_NONE) {
        see_help();
    } else {
        if (!apply(flags, target, arguments)) return 1;
    }
    
    if (archive.arcpath != NULL) {
        if (archive.was_changed) archive_save(&archive);
    }

    return return_value;
}