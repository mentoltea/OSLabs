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

#define READ_END 0
#define WRITE_END 1
int pipefd[2];

#define BUFFER_SIZE 4096
static char buffer[4096];

int main() {
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Cannot create a pipe: %s\n", strerror(errno));
        return 1;
    }
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Cannot fork\n");
        return 1;
    }
    
    if (pid == 0) {
        // child
        close(pipefd[WRITE_END]);
        unsigned int seconds = 5;
        int updates_per_second = 4;
        int sleep_per_update = 1000*1000 /updates_per_second;
        int c = 0;
        int c_lim = 5;


        usleep(sleep_per_update);
        for (int i=1; i<(int)seconds*updates_per_second; i++) {
            printf("\r");
            printf("\e[K");
            printf("Child sleeping");
            printf(" %d/%d", i+1, seconds*updates_per_second);
            for (int j=0; j<c; j++) {
                printf(".");
            }
            fflush(stdout);
            
            c++;
            c = c%c_lim;
            usleep(sleep_per_update);
        }
        printf("\n");
        

        ssize_t size;
        
        size = read(pipefd[READ_END], buffer, BUFFER_SIZE-1);
        if (size == -1) {
            fprintf(stderr, "Error reading: %s\n", strerror(errno));
            return 1;    
        }
        buffer[size-1] = '\0';
        printf("[pipe] %s\n", buffer);

        time_t current_time = time(0);
        char* timestr = ctime(&current_time);
        printf("Real current time: %s\n", timestr);
        close(pipefd[READ_END]);
        return 0;
    }

    // parent
    close(pipefd[READ_END]);
    
    const char* string = "Hello from parent. Current time: ";
    time_t current_time = time(0);
    char* timestr = ctime(&current_time);

    if (!write(pipefd[WRITE_END], string, strlen(string))) {
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        return 1;    
    }
    if (!write(pipefd[WRITE_END], timestr, strlen(timestr))) {
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        return 1;    
    }
    printf("Message sent\n");

    int wstatus;
    waitpid(pid, &wstatus, 0);
    close(pipefd[WRITE_END]);

    return 0;
}