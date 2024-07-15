#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

void print_size(const char *msg)
{
    printf("%s = %d\n", msg, sbrk(0));
}

int main(void)
{
    char *shmem;
    int pid, ppid = getpid();

    // Allocate one page of memory using malloc
    if ((shmem = malloc(PGSIZE)) == 0)
    {
        printf("malloc failed\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0)
    {
        printf("fork failed\n");
        free(shmem);
        exit(1);
    }

    if (pid == 0) // Child process
    {
        uint64 child_va;

        print_size("Child: initial size");

        // Share the memory with the parent process
        if ((child_va = map_shared_pages(ppid, getpid(), (uint64)shmem, PGSIZE)) < 0)
        {
            printf("map_shared_pages failed\n");
            exit(1);
        }
        print_size("Child: size after mapping shared memory");

        // Write the string to the shared memory
        strcpy((char *)child_va, "Hello daddy");

        // Unmap the shared memory
        if (unmap_shared_pages(child_va, PGSIZE) < 0)
        {
            printf("unmap_shared_pages failed\n");
            exit(1);
        }
        print_size("Child: size after unmapping shared memory");

        // Allocate memory using malloc()
        if (malloc(PGSIZE) == 0)
        {
            printf("malloc failed\n");
            exit(1);
        }
        print_size("Child: size after malloc");

        exit(0);
    }
    else // Parent process
    {
        wait(0);

        // Read and print the string from shared memory
        printf("Parent reads: %s\n", shmem);

        // Free the shared memory
        free(shmem);
    }

    exit(0);
}