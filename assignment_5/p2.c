/* ECE-357 2019 Fall (Problem Set#5)
Problem 2: mmap test problem (test#2&3)
author: Di (David) Mei
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#define BUF_SIZE 4096

/**
 * test#2 & test#3
 * create an empty file
 * map this file to memory with MAP_SHARED/MAP_PRIVATE
 * try to write some bytes to this memory
 */

void handler(int signum){
    fprintf(stderr, "signal %d received\n", signum);
    exit(signum);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < 32; i++){
        // in mac OS, signal 9 & 17 cannot be handled
        // in Linux, signal 9 & 19 cannot be handled
        if (i == 9 || i == 19)
            continue;
        if (signal(i, handler) == SIG_ERR){
            fprintf(stderr, "Error: unable to call signal() (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    int fd;
    char *test_write, *map;

    if (argc != 2){
        // correct input format "./p2 test_case_number"
        // test_case_number: 2 or 3
        fprintf(stderr, "Error: invalid input format\n");
        exit(EXIT_FAILURE);
    }

    if (atoi(argv[1]) == 2){ 
        fprintf(stderr, "Executing Test #2 (write to a MAP_SHARED region):\n");
    }
    else if (atoi(argv[1]) == 3){
        fprintf(stderr, "Executing Test #3 (write to a MAP_PRIVATE region):\n");
    }
    else{
        fprintf(stderr, "Invalid Test Case Number\n");
        exit(EXIT_FAILURE);
    }

    if ((fd = open("test_file", O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0){
        fprintf(stderr, "Error: unable to open file for writing (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    test_write = "ABC";
    size_t testsize = strlen(test_write);
    fprintf(stderr, "Writing a \"%s\" to mapped memory of an empty file \n", test_write);

    if (lseek(fd, testsize, SEEK_SET) == -1){
        fprintf(stderr, "Error: lseek error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // reference from https://gist.github.com/marcetcheverry/991042
    // special treatment when mapping an empty file
    if (write(fd, "\0", 1) < 0){
        fprintf(stderr, "Error: unable to write to 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // read-write mapping
    if (atoi(argv[1]) == 2){
        if ((map = mmap(0, testsize + 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
            fprintf(stderr, "Error: unable map for 'test_file' (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else{
        if ((map = mmap(0, testsize + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
            fprintf(stderr, "Error: unable map for 'test_file' (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // write "ABC" to mapped region
    memcpy(map, test_write, testsize);

    char *buffer = malloc(BUF_SIZE);
    int read_bytes, total_read = 0;
    if (lseek(fd, 0, SEEK_SET) == -1){
        fprintf(stderr, "Error: lseek error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    while((read_bytes = read(fd, buffer + total_read, testsize - total_read)) != 0){
        if (read_bytes < 0){
            fprintf(stderr, "Error: unable to read 'test_file' (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        total_read += read_bytes;
    }
    // check whether write is successful
    if (strcmp(buffer, test_write) != 0){
        fprintf(stderr, "File's bytes don't change\n");
        exit(EXIT_FAILURE);
    }

    if (munmap(map, testsize + 1) == -1){
        fprintf(stderr, "Error: unable to unmap (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (close(fd) < 0){
        fprintf(stderr, "Error: unable to close 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    free(buffer);

    return 0;
}
