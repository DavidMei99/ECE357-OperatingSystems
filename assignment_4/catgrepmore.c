/* ECE-357 2019 Fall (Problem Set#4)
Problem 3: fixing your cat
author: Di (David) Mei
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <errno.h>
#define BUFF_SIZE 4096
#define STDIN 0
#define STDOUT 1

void handler(int s);  // SIGINT handler
void phandler(int s); // SIGPIPE handler
// global variables: jump buffer, number of files/bytes checked
jmp_buf buf;
int file_check, byte_check;

int main(int argc, char **argv)
{
	// set signals by using sigaction
	struct sigaction sa, sa_pipe;
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, 0) == -1){
		fprintf(stderr, "Error: unable to invoke sigaction (%s)\n", strerror(errno));
		return -1;
	}
	sa_pipe.sa_flags = 0;
	sa_pipe.sa_handler = phandler;
	sigemptyset(&sa_pipe.sa_mask);
	if (sigaction(SIGPIPE, &sa_pipe, 0) == -1){
		fprintf(stderr, "Error: unable to invoke sigaction (%s)\n", strerror(errno));
		return -1;
	}
	// invalid command if its count is less than 3
	if (argc < 3){
		fprintf(stderr, "Error: incorrect command format\n");
		return -1;
	}
	// check each infile in command
	for (int i = 2; i < argc; i++){
		int in_fd;
		if ((in_fd = open(argv[i], O_RDONLY)) < 0){
			fprintf(stderr, "Error: unable to open file %s for reading (%s)\n", argv[i], strerror(errno));
			return -1;
		}
		// create two pipes
		int pipe1[2], pipe2[2];
		if (pipe(pipe1) == -1){
			fprintf(stderr, "Error: pipe connecting infile and grep fails (%s)\n", strerror(errno));
			return -1;
		}
		if (pipe(pipe2) == -1){
			fprintf(stderr, "Error: pipe connecting grep and more fails (%s)\n", strerror(errno));
			return -1;
		}
		// fork the first child for grep
		int status1, status2;
		int pid1 = fork();
		switch (pid1){
			case -1:
				fprintf(stderr, "Error: unable to create child process (%s)\n", strerror(errno));
				break;
			case 0: // child process
				if (dup2(pipe1[0], STDIN) < 0){
					fprintf(stderr, "Error: unable to redirect file %d to stdin (%s)\n", pipe1[0], strerror(errno));
					return -1;
				}
				if (dup2(pipe2[1], STDOUT) < 0){
					fprintf(stderr, "Error: unable to redirect file %d to stdout (%s)\n", pipe2[1], strerror(errno));
					return -1;
				}
				// close all unused or extra references
				if (close(pipe1[0]) < 0){
				    fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe1[1]) < 0){
				    fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe2[0]) < 0){
				    fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe2[1]) < 0){
				    fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				// exec grep
				if (execlp("grep", "grep", argv[1], NULL) == -1){
					fprintf(stderr, "Error: unable to execute grep command (%s)\n", strerror(errno));
				}
				return -1;
		}
		// fork the second child for more
		int pid2 = fork();
		switch (pid2){
			case -1: 
				fprintf(stderr, "Error: unable to create child process (%s)\n", strerror(errno));
				break;
			case 0: // child process
				if (dup2(pipe2[0], STDIN) < 0){
					fprintf(stderr, "Error: unable to redirect file %d to stdin (%s)\n", pipe2[0], strerror(errno));
					return -1;
				}
				// close all unused or extra references
				if (close(pipe1[0]) < 0){
				    fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe1[1]) < 0){
				    fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe2[0]) < 0){
				    fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				if (close(pipe2[1]) < 0){
				    fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
					return -1;
				}
				// exec more
				if (execlp("more", "more", NULL) == -1){
					fprintf(stderr, "Error: unable to execute more command (%s)\n", strerror(errno));
				}
				return -1;
		}
        // cat is the parent process
		char *buffer[BUFF_SIZE];
		int read_bytes, write_bytes;
		while ((read_bytes = read(in_fd, buffer, BUFF_SIZE)) != 0){
			if (read_bytes < 0){
				fprintf(stderr, "Error: unable to read from file %s (%s)\n", argv[i], strerror(errno));
				return -1;
			} 
			else{
				write_bytes = write(pipe1[1], buffer, read_bytes);
				while (write_bytes < read_bytes){
					if (write_bytes < 0){
						fprintf(stderr, "Error: unable to write from buffer to pipe: %s\n", strerror(errno));
						return -1;
					}
					write_bytes += write(pipe1[1], buffer + write_bytes, read_bytes - write_bytes);
				}
			}
		}
		// close all unused or extra references
		if (close(in_fd) < 0){
			fprintf(stderr, "Error: unable to close infile #%d (%s)\n", in_fd, strerror(errno));
			return -1;
		}
		if (close(pipe1[0]) < 0){
			fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
			return -1;
		}
		if (close(pipe1[1]) < 0){
			fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
			return -1;
		}
		if (close(pipe2[0]) < 0){
			fprintf(stderr, "Error: unable to close read end of pipe (%s)\n", strerror(errno));
			return -1;
		}
		if (close(pipe2[1]) < 0){
			fprintf(stderr, "Error: unable to close write end of pipe (%s)\n", strerror(errno));
			return -1;
		}
		file_check++; // update # of files processed
		byte_check+=write_bytes; // update # of bytes processed
		int grep = waitpid(pid1, &status1, 0); // wait for 1st child process to finish
		int more = waitpid(pid2, &status2, 0); // wait for 2nd child process to finish
        // use buf to remember the current position
		setjmp(buf);
	}
	return 0;
}
// signal handler for SIGINT (ctrl C)
void handler(int sig_num){
	if (sig_num == SIGINT){
		fprintf(stderr, "Number of Files Processed: %d\nNumber of Bytes Processed: %d\n", file_check, byte_check);
		return; // goes to the next infile
	}
}
// signal handler for SIGPIPE
void phandler(int sig_num){
	// if a pipe is broken, goto the next infile
    // return 1
	longjmp(buf, 1);
}
