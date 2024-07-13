#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int main(void)
{
    char *shmem;
    int pid;

    // Allocate a page of memory
    if ((shmem = malloc(PGSIZE)) == 0)
    {
        printf("malloc failed\n");
        exit(1);
    }

    // Write the string to the shared memory
    strcpy(shmem, "Hello child");

    pid = fork();
    if (pid < 0)
    {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) // Child process
    {
        // Read and print the string from shared memory
        printf("Child reads: %s\n", shmem);
        exit(0);
    }
    else // Parent process
    {
        // Share the memory with the child process
        if (map_shared_pages(getpid(), pid, (uint64)shmem, PGSIZE) < 0)
        {
            printf("map_shared_pages failed\n");
            exit(1);
        }

        wait(0);
        free(shmem);
    }

    exit(0);
}
