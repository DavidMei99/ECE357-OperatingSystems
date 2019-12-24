/* ECE-357 2019 Fall (Problem Set#6)
Problem 1: the spinlock mutex
author: Di (David) Mei
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#define NUM_CORES 6
#define BUF_SIZE 4096
#define ITER_NUM 1000000

int tas(volatile char *lock);

void spin_lock(char *lock)
{
    while(tas(lock) != 0)
        ;
}

void spin_unlock(char *lock)
{
    *lock = 0;
}

int main()
{
    int *map;
    char *map2;
    if ((map = (int *)mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error: unable to create a shared memory (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ((map2 = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error: unable to create a shared memory (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_CORES; i++){
        int pid = fork();
        if (pid == 0){
            spin_lock(map2);
            for (int j = 0; j < ITER_NUM; j++){
                *map += 1;
                // fprintf(stderr, "%d\n", *map); 
            }
            fprintf(stderr, "The last updated value for each process is %d\n", *map); // print *map to check atomicity
            spin_unlock(map2);
            exit(EXIT_SUCCESS);
        }
        else if (pid == -1){
            fprintf(stderr, "Error: unable to fork the process (%s)", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < NUM_CORES; i++){
        wait(NULL);
    }
    
    if (munmap(map, BUF_SIZE) == -1){
        fprintf(stderr, "Error: unable to unmap (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return 0;
}
