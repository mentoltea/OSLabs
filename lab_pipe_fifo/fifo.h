#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <string.h>
#include <errno.h>

const char fifo_path[] = "/tmp/fifo_example.123";

#define BUFFER_SIZE 4096
static char buffer[4096];