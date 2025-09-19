#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
    bool n;
    bool b;
    bool E;
} Flags;
const char flag_string[] = "nbE";


typedef struct {
    Flags flags;
    size_t line_number;
} Context;

char peekc(FILE* fd) {
    char ch = getc(fd);
    ungetc(ch, fd);
    return ch;
}

#define DEFAULT_BUFFER_SIZE 1024*1024 // 1 MB
bool read_file_and_print(Context *ctx, const char* filepath, size_t buffer_size) {
    if (buffer_size==0) buffer_size = DEFAULT_BUFFER_SIZE;

    FILE* fd = fopen(filepath, "r");
    if (!fd) {
        return false;
    }
    
    char* buffer = malloc(sizeof(char) * buffer_size);
    if (!buffer) {
        fprintf(stderr, "Cannot allocate %lu bytes for buffer\n", buffer_size);
        return false;
    }
    
    while (!feof(fd)) {
        bool line_finished = false;
        char ch;
        
        char first = peekc(fd);

        if (ctx->flags.b) {
            if (first != EOF && first != '\n' && first != '\r') printf("%lu:\t", ctx->line_number);
        } else if (ctx->flags.n) printf("%lu:\t", ctx->line_number);

        bool non_empty_line = false;
        while (! line_finished ) { 
            size_t bytes_read;
            for (bytes_read=0; bytes_read < buffer_size-1; bytes_read++) {
                ch = getc(fd);
                if (ch == EOF || ch == '\n' || ch == '\r') {
                    char next = getc(fd);
                    if (ch != '\r' || (next != '\n' && next != EOF)) ungetc(next, fd);
                    line_finished = true;
                    break;
                }
                buffer[bytes_read] = ch;
            }
            if (bytes_read > 0) {
                non_empty_line = true;
                buffer[bytes_read] = '\0';
                printf("%s", buffer);
            }
        }
        if (ctx->flags.E) printf("$");
        printf("\n");


        if (ctx->flags.b) {
            if (non_empty_line) ctx->line_number++;
        }
        else if (ctx->flags.n) ctx->line_number++;
    }

    if (buffer) free(buffer);

    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments\n");
        exit(1);
    }

    Context ctx = {.flags = {0}, .line_number=1};
    int opt;

    while ((opt = getopt(argc, argv, flag_string)) != -1) {
        switch (opt) {
        case 'n':
            ctx.flags.n = true;
            break;
        case 'b':
            ctx.flags.b = true;
            break;
        case 'E':
            ctx.flags.E = true;
            break;
        
        case '?':
        default:
            exit(1);
            break;
        }
    }
    
    for (int i=1; i<argc; i++) {
        if (argv[i][0] == '-') continue;
        if (! read_file_and_print(&ctx, argv[i], 0) ) {
            exit(1);
        }
    }
}