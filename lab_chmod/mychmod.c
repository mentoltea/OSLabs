#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <stdbool.h>
#include <stdint.h>

void print_usage(FILE* fd);

typedef enum {
    MOD_ID, // does not change anything
    MOD_SET,
    MOD_ADD,
    MOD_SUB
} ModType;

typedef struct {
    int8_t permissions;
    ModType type;
} Mod;

typedef struct {
    Mod user;
    Mod group;
    Mod other;
} FileMod;

bool parse_mod(const char* modstr, FileMod* mod);

bool apply_mod(const char* filepath, FileMod* mod);

bool get_mod(const char* filepath, FileMod* mod);

uint8_t add_mods(uint8_t m1, uint8_t m2);
uint8_t sub_mods(uint8_t m1, uint8_t m2);

mode_t unit_mods(uint8_t user, uint8_t group, uint8_t others);

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Too few args\n");
        print_usage(stderr);
        exit(1);
    }
    
    const char* modstr = argv[1];
    const char* filepath = argv[2];
    
    FileMod mod = {0};
    
    if (!parse_mod(modstr, &mod)) {
        exit(1);
    }  
    
    if (!apply_mod(filepath, &mod)) {
        exit(1);
    }  

    // printf("permissions %u %u %u\n", mod.user.permissions, mod.group.permissions, mod.other.permissions);
    // printf("operations  %d %d %d\n", mod.user.type, mod.group.type, mod.other.type);

    return 0;
}

void print_usage(FILE* fd) {
    fprintf(fd, "Usage:\n");
    fprintf(fd, "\tchmod MODE PATH\n");
}

enum Target {
    NONE = 0,
    USER = 1,
    GROUP = 2,
    OTHER = 4
};

bool parse_mod(const char* str, FileMod* mod) {
    bool is_int = true;

    size_t length = strlen(str);

    if (length <= 3) {
        for (size_t i=0; i<length; i++) {
            if (str[i] < '0' || str[i] > '7') is_int = false;
        }
    } else is_int = false;

    if (is_int) {
        mod->other.permissions = str[0] - '0';
        mod->other.type = MOD_SET;
        
        if (length > 1) {
            mod->group.permissions = str[1] - '0';
            mod->group.type = MOD_SET;
        }
        
        if (length > 2) {
            mod->user.permissions = str[2] - '0';
            mod->user.type = MOD_SET;
        }

        return true;
    } 

    enum Target target = NONE;
    bool parsing_targets = true;
    ModType operation = MOD_ID;

    uint8_t perms = 0;

#define PERM_READ 0b000000100u
#define PERM_WRITE 0b000000010u
#define PERM_EXECUTE 0b000000001u

    for (size_t i=0; i<length; i++) {
        char c = str[i];

        if (parsing_targets) {
            if (c == 'u') {
                target = target | USER;
                continue;
            }
            if (c == 'g') {
                target = target | GROUP;
                continue;
            }
            if (c == 'a') {
                target = target | OTHER;
                continue;
            }

            if (c == '+' || c == '-') {
                operation = c == '+' ? MOD_ADD : MOD_SUB;
                parsing_targets = false;
                if (target == NONE) target = USER | GROUP | OTHER;
                continue;
            }

            fprintf(stderr, "Cannot identify %c in permission line\n", c);
            return false;
        }

        if (c == 'r') perms = perms | PERM_READ;
        else if (c == 'w') perms = perms | PERM_WRITE;
        else if (c == 'x') perms = perms | PERM_EXECUTE;
        else {
            fprintf(stderr, "Cannot identify %c in permission line\n", c);
            return false;
        }
    }

    if (target & USER) {
        mod->user.permissions = perms;
        mod->user.type = operation;
    } else mod->user.type = MOD_ID;

    if (target & GROUP) {
        mod->group.permissions = perms;
        mod->group.type = operation;
    } else mod->group.type = MOD_ID;

    if (target & OTHER) {
        mod->other.permissions = perms;
        mod->other.type = operation;
    } else mod->other.type = MOD_ID;

    return true;
}

bool apply_mod(const char* filepath, FileMod* newmod) {
    FileMod oldmod = {0};
    if (!get_mod(filepath, &oldmod)) return false;
    
    switch (newmod->user.type) {
        case MOD_ID:
            newmod->user.permissions = oldmod.user.permissions;
            break;
        case MOD_ADD:
            newmod->user.permissions = add_mods(oldmod.user.permissions, newmod->user.permissions);
            break;
        case MOD_SUB:
            newmod->user.permissions = sub_mods(oldmod.user.permissions, newmod->user.permissions);
            break;
        default:
            break;
    }

    switch (newmod->group.type) {
        case MOD_ID:
            newmod->group.permissions = oldmod.group.permissions;
            break;
        case MOD_ADD:
            newmod->group.permissions = add_mods(oldmod.group.permissions, newmod->group.permissions);
            break;
        case MOD_SUB:
            newmod->group.permissions = sub_mods(oldmod.group.permissions, newmod->group.permissions);
            break;
        default:
            break;
    }

    switch (newmod->other.type) {
        case MOD_ID:
            newmod->other.permissions = oldmod.other.permissions;
            break;
        case MOD_ADD:
            newmod->other.permissions = add_mods(oldmod.other.permissions, newmod->other.permissions);
            break;
        case MOD_SUB:
            newmod->other.permissions = sub_mods(oldmod.other.permissions, newmod->other.permissions);
            break;
        default:
            break;
    }

    mode_t newpermissions = unit_mods(newmod->user.permissions, newmod->group.permissions, newmod->other.permissions);
    if (chmod(filepath, newpermissions) == -1) {
        fprintf(stderr, "Error chmod'ing: %s\n", strerror(errno));
        return false;
    }

    return true;
}

bool get_mod(const char* filepath, FileMod* mod) {
    struct stat info;
    uint8_t mask = 0b111u;
    
    if (lstat(filepath, &info) != 0) {
        fprintf(stderr, "Error stat'ing %s : %s\n", filepath, strerror(errno));
        return false;
    }
    mode_t perm = info.st_mode;

    mod->other.permissions = perm & mask;
    perm = perm >> 3;
    mod->group.permissions = perm & mask;
    perm = perm >> 3;
    mod->user.permissions = perm & mask;
    
    return true;
}

uint8_t add_mods(uint8_t m1, uint8_t m2) {
    return m1 | m2;
}
uint8_t sub_mods(uint8_t m1, uint8_t m2) {
    return m1 & (~m2);
}

mode_t unit_mods(uint8_t user, uint8_t group, uint8_t others) {
    mode_t result = ((user << 3) << 3) | (group << 3) | others;
    return result;
}