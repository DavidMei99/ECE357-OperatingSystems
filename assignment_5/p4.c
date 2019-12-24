/* ECE-357 2019 Fall (Problem Set#5)
Problem 2: mmap test problem (test#4)
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
#define BUF_SIZE 4096

/**
 * test#4
 * create a file with content
 * map this file to memory with MAP_SHARED
 * write a byte "X" one byte beyond this memory
 * extend file size by 16 bytes by lseeking and writing
 * check whether the byte "X" is visible
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
        if (i == 9 || i == 17)
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
        // correct input format "./p4"
        fprintf(stderr, "Error: invalid input format\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Executing Test #4 (write into a hole):\n");

    if ((fd = open("test_file", O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0){
        fprintf(stderr, "Error: unable to open file for writing (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // write some content to file
    if (write(fd, "AAAAAA", 6) < 0){
        fprintf(stderr, "Error: write error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fstat(fd, &buf) < 0){
        fprintf(stderr, "Error: fstat error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // read-write mapping with MAP_shared
    if ((map = mmap(0, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
        fprintf(stderr, "Error: unable map for 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    test_write = "X";
    fprintf(stderr, "Writing byte \"X\" one byte beyond 'test_file' mapped memory\n");
    // write byte "X" one byte beyond 'testfile'
    memcpy(map + buf.st_size, test_write, 1);

    // extend "test_file" and write another byte "X" at the end
    fprintf(stderr, "Extending 'test_file' by 16 bytes\n");
    if (lseek(fd, buf.st_size + 15, SEEK_SET) == -1){
        fprintf(stderr, "Error: lseek error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (write(fd, test_write, 1) < 0){
        fprintf(stderr, "Error: unable to write to 'test_file' (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(BUF_SIZE);
    int read_bytes, total_read = 0;
    if (lseek(fd, 0, SEEK_SET) == -1){
        fprintf(stderr, "Error: lseek error (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    while((read_bytes = read(fd, buffer + total_read, buf.st_size + 16 - total_read)) != 0){
        if (read_bytes < 0){
            fprintf(stderr, "Error: unable to read 'test_file' (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        total_read += read_bytes;
    }
    // check whether byte "X" is visible or not
    if (buffer[buf.st_size] != test_write[0]){
        fprintf(stderr, "X byte is still invisible\n");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(stderr, "X byte is visible now\n");
    }
    
    if (munmap(map, buf.st_size + 16) == -1){
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
