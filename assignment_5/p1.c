/* ECE-357 2019 Fall (Problem Set#5)
Problem 2: mmap test problem (test#1)
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
#include <sys/stat.h>

/**
 * test#1
 * create a file with content "AAAAAA"
 * map this file to memory region with PROT_READ
 * try to write to this region with a single byte
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
    struct stat buf;

    if (argc != 1){
        // correct input format "./p1"
        fprintf(stderr, "Error: invalid input format\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stderr, "Executing Test #1 (write to r/o mmap):\n");

    if ((fd = open("test_file", O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0){
        fprintf(stderr, "Error: unable to open file for writing (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (write(fd, "AAAAAA", 6) < 0){
        fprintf(stderr, "Error: write error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "map[3]=='A'\n");

    if (fstat(fd, &buf) < 0){
        fprintf(stderr, "Error: fstat error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    test_write = "B";
    fprintf(stderr, "Writing a 'B'\n");

    // read-only mapping 
    if ((map = mmap(0, buf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED){
        fprintf(stderr, "Error: unable map for 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // write "B" to map[3]
    memcpy(map + 3, test_write, 1);
    // write fails without causing a signal
    if (map[3] != test_write[0])
        return 255;

    if (munmap(map, buf.st_size) == -1){
        fprintf(stderr, "Error: unable to unmap (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (close(fd) < 0){
        fprintf(stderr, "Error: unable to close 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
