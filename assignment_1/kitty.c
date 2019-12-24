/* ECE-357 2019 Fall (Problem Set#1)
Problem 3: Use of system calls in a simple concatenation program
author: Di (David) Mei
*/ 
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#define BUFFER_SIZE 4096
#define STDIN 0
#define STDOUT 1
/* concatenate: concatenate input files to an output file */
int concatenate(int ifd, char *if_name, int ofd, char *of_name, int buff_size, int *read_sys, int *write_sys) {
    int read_bytes, write_bytes, temp = 0;
    char *buffer = malloc(buff_size); // create a buffer
    while ((read_bytes = read(ifd,buffer,buff_size)) > 0) {
        (*read_sys)++;
        temp += read_bytes;
        for (int i = 0; i < strlen(buffer); i++) { // check whether the infile is binary
            if(!(isprint(buffer[i])||isspace(buffer[i]))) {
                fprintf(stderr, "Warning: file %s is a binary file\n", if_name);
                break;
            }
        }
        while ((write_bytes = write(ofd,buffer,read_bytes)) != read_bytes) { // check whether partially writes and solve this problem
            (*write_sys)++;
            if (errno == EINTR) {
               continue;
            }
            if (write_bytes < 0) { // if write() returns negative, report error
                fprintf(stderr, "Error: unable to write file %s (%s)\n", of_name, strerror(errno));
                break;
            }
            read_bytes -= write_bytes;
            buffer += write_bytes; // make buffer rewrite file
        }
        (*write_sys)++;
    }
    (*read_sys)++;
    fprintf(stderr, "%d bytes transferred from file %s to file %s\n", temp, if_name, of_name);
    return read_bytes;
}
/* main function */
int main (int argc, char *argv[]) {
    char *outfile_name = "STANDARD_OUTPUT";
    int num_read, num_write = 0;
    int fd, out_fd = STDOUT;
    int buffer_size = BUFFER_SIZE; //set buffer size during the concatenation to be 4096
    if (argc == 1) {
        if (concatenate(STDIN, "STANDARD_INPUT", STDOUT, outfile_name, buffer_size, &num_read, &num_write) < 0) return -1;
    }
    else {
        while (--argc > 0) {
            argv++;
            if (strcmp(*argv, "-o") == 0) { // if argv has outfile operation
                argv++;
                argc--;
                outfile_name = *argv;
                /* open a outfile: read only, auto create file if not existed, enable to overwrite content */
                if ((out_fd = open(*argv, O_RDWR|O_CREAT|O_TRUNC, 0666)) == -1) {
                    fprintf(stderr, "Error: unable to open file %s for writing (%s)\n", *argv, strerror(errno));
                    return -1;
                }
                if (argc - 1 == 0) { // deal with command "./kitty -o out"
                    if (concatenate(STDIN, "STANDARD_INPUT", out_fd, outfile_name, buffer_size, &num_read, &num_write) < 0) return -1;
                }
            }
            else if (strcmp(*argv, "-") == 0) { // if argv includes hyphen
                if (concatenate(STDIN, "STANDARD_INPUT", out_fd, outfile_name, buffer_size, &num_read, &num_write) < 0) return -1;
            }
            else {
                if ((fd = open(*argv, O_RDONLY)) == -1) { // check whether an infile can be opened
                    fprintf(stderr, "Error: unable to open file %s for reading (%s)\n", *argv, strerror(errno));
                    return -1;
                }
                else { // concateate an infile to an outfile
                    if (concatenate(fd, *argv, out_fd, outfile_name, buffer_size, &num_read, &num_write) < 0) return -1;
                    if (close(fd) == -1) { // check whether an infile can be closed
                        fprintf(stderr, "Error: unable to close file %s (%s)\n", *argv, strerror(errno));
                        return -1;
                    }
                }
            }
        }
    }
    if (close(out_fd) == -1) { // check whether an outfile can be closed
        fprintf(stderr, "Error: unable to close file %s (%s)\n", outfile_name, strerror(errno));
        return -1;
    }
    fprintf(stderr, "Total number of read system calls: %d\n", num_read);
    fprintf(stderr, "Total number of write system calls: %d\n", num_write);
    return 0;
}
