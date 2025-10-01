#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <signal.h>

void print_pid() {
    pid_t self = getpid();
    pid_t parent = getppid();

    printf("[%d] Parent: %d\n", self, parent);
}

void parent_task() {

}

void child_task() {
    pid_t pid = getpid();
    int lim = 100;
    int c = 0;
    int c_lim = 5;
    for (int i=0; i<lim; i++) {
        printf("\r");
        printf("\e[K");
        printf("[%d] ", pid);
        printf("%d/%d ", i, lim);
        printf("Child working");
        for (int j=0; j<c+1; j++) {
            printf(".");
        }
        fflush(stdout);
        
        c++;
        c = c%c_lim;
        usleep(100*1000);
    }
    printf("\n");
}

void atexit_func() {
    printf("[%d] Exited\n", getpid());
}

void onexit_func(int code, void* args) {
    printf("[%d] Is going to exit with code %d\n", getpid(), code);
}

void handler_int(int sig) {
    printf("[%d] RECEIVED SIGINT\n", getpid());
    exit(sig);
}

void handler_term(int sig, siginfo_t *info, void *context) {
    printf("[%d] RECEIVED SIGNTERM\n", getpid());
    exit(sig);
}

int main() {
    atexit(atexit_func);
    on_exit(onexit_func, NULL);

    struct sigaction act = {0};
    act.sa_flags = 0;
    act.sa_sigaction = &handler_term;
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        fprintf(stderr, "Cannot sigaction: %s\n", strerror(errno));
        return 1;
    }
    
    signal(SIGINT, handler_int);
    
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Cannot fork: %s\n", strerror(errno));
        return 1;
    }
    
    if (pid == 0) {
        print_pid();
        
        child_task();
        exit(0);
    }

    print_pid();
    
    int wstatus = 0;
    do {
        if (waitpid(pid, &wstatus, 0) == -1) {
            fprintf(stderr, "Parent fail waitpid: %s\n", strerror(errno));
            exit(1);    
        }

        if (WIFEXITED(wstatus)) {
            printf("%d exited with status %d\n", pid, WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("%d killed by signal %d\n", pid, WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            printf("%d stopped by signal %d\n", pid, WSTOPSIG(wstatus));
        } else if (WIFCONTINUED(wstatus)) {
            printf("%d continued\n", pid);
        }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    
    parent_task();

    return 0;
}