/* ECE-357 2019 Fall (Problem Set#2)
Problem 3: Recursive Filesystem Lister
author: Di (David) Mei
*/
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#define BUFF_SIZE 4096

void throwError(char *message, char *file);
int checkNum(char *input);
int checkm(time_t file_mtime, time_t current_time, int mtime, int check);
int checkv(int *file_sys, struct stat buf, int check);
char *getPermissions(mode_t mode);
void traverseDir(char *directory, int check_m, int mtime, int check_v);

/* main function: determine the directory to be found */
int main(int argc, char *argv[])
{
    char *directory = "."; // if no starting_path specified, default "."
    int opt, m_check = 0, v_check = 0, mtime = 0, count_opt = 0;
    // any option argument immediately following "-m" (no space between "-m" and argument) is illegal
    for (int i = 1; i<argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'm' && argv[i][2] != '\0') {
            throwError("Error: Command Invalid. Correct format is '-m mtime' and mtime is an integer.", NULL);
        }
    }
    // parse command "./program_name [options] filepath"
    // for the invoker of program, '-v' and '-m' are not suggested to be used as file names
    // options "-m mtime" and "-v" can appear multiple times in a command
    // if multiple "mtime" are specified by different value in a command, the last "mtime" is chosen
    while ((opt = getopt(argc, argv, "m:v")) != -1) {
        switch(opt){
            case 'm':
                count_opt++;
                m_check = 1;
                if (checkNum(optarg) && strcmp(optarg, "-") != 0) {
                    mtime = atoi(optarg);
                    count_opt++;
                    break;
                }
                else { // if mtime is invalid, report an error
                    throwError("Error: Command Invalid. Correct format is '-m mtime' and mtime is an integer.", NULL);
                }
            case 'v':
                count_opt++;
                v_check = 1;
                break;
            case '?': // if invalid options are detected
                throwError("Error: Command Invalid. Options are either '-m mtime' or '-v'.", NULL);
                break;
        }
        // if (total # of arguments)-(# of options and their arguments)=2, arguments left are program_name and filepath
        if (count_opt+2 == argc) {
            // make sure "-v" won't be reagarded as a filepath name
            if (strcmp(argv[count_opt+1], "-v") == 0) continue;
            directory = argv[argc-1];
            break;
        }
        // if (total # of arguments)-(# of options and their arguments)=1, argument left is a program_name
        else if (count_opt+1 == argc) { 
            break;
        }
    }
    // check if multiple filepaths are specified; if true, report error
    if (optind+1 < argc) {
        throwError("Error: Command Invalid. Correct format is './program_name [option] filepath'.", NULL);
    }
    // special case: if no options are specified
    else if (argc == 2 && strcmp(argv[1], "-v") != 0) {
        directory = argv[1];
    }
    traverseDir(directory, m_check, mtime, v_check); // begin traversal at given filepath name
    return 0;
}
/* error report */
void throwError(char *message, char *file)
{
    if (file) {
        fprintf(stderr, "%s '%s' (Error code %i: %s)\n", message, file, errno, strerror(errno));
    }
    else {
        fprintf(stderr, "%s\n", message);
    }
    exit(-1);
}
/* check whether a string is a number byte by byte */
int checkNum(char* input)
{
    int check = 1, i = 0;
    while (input[i] != '\0') {
        if (!isdigit(input[i])) {
            if (i == 0 && input[i] == '-') { // special case: negative value
                i++;
                continue;
            }
            else {
                check = 0;
                break;
            }
        }
        i++;
    }
    return check;
}
/* check "-m mtime" option and decide whether or not list node */
int checkm(time_t file_mtime, time_t current_time, int mtime, int check)
{
    if (check) { // option argument "mtime" is in seconds
        if (mtime > 0) {
            if (current_time-file_mtime > mtime) return 1;
        }
        else if (mtime < 0) {
            if (current_time-file_mtime < 0-mtime) return 1;  
        }
        else {
            return 1;
        }
    }
    else {
        return 1;
    }
    return 0;  
}
/* check "-v" argument and decide whether or not descend into the other volume */
int checkv(int *file_sys, struct stat buf, int check)
{
    if (check) {
        if (major(buf.st_dev) == file_sys[0] && minor(buf.st_dev) == file_sys[1]) {
            return 1;
        }
    }
    else {
        return 1;
    }
    return 0;  
}
/* get permission mask */
char *getPermission(mode_t mode)
{
    char *p = (char *)malloc(sizeof(char) * 11);
    if (p == NULL)
        throwError("Error: Fail to allocate dynamic memory.", NULL);
    p[0] = '-'; // default type: a file
    p[1] = (mode & S_IRUSR) ? 'r' : '-';
    p[2] = (mode & S_IWUSR) ? 'w' : '-';
    p[3] = (mode & S_IXUSR) ? 'x' : '-';
    p[4] = (mode & S_IRGRP) ? 'r' : '-';
    p[5] = (mode & S_IWGRP) ? 'w' : '-';
    p[6] = (mode & S_IXGRP) ? 'x' : '-';
    p[7] = (mode & S_IROTH) ? 'r' : '-';
    p[8] = (mode & S_IWOTH) ? 'w' : '-';
    p[9] = (mode & S_IXOTH) ? 'x' : '-';
    p[10] = '\0';
    return p;
}
/* read a filepath name and start traversal */
void traverseDir(char *directory, int check_m, int mtime, int check_v)
{
    DIR *dir;  // declare a directory stream
    struct dirent *sd; // dirent structure: d_ino, d_name[] (dirent.h)
    struct stat buf; // stat structure of a file (sys/stat.h)
    struct passwd *usr; // password structure: *pw_name, pw_uid, pw_gid ...(pwd.h)
    struct group *grp; // group structure: *gr_name, gr_gid, **gr_mem (grp.h)
    struct tm file_time, current_time; // time structure (time.h)
    char path[BUFF_SIZE], link[BUFF_SIZE], date[20], rdev_num[10], *group, *user, *perms;
    int dev_num[2]; // device number (major and minor) of the starting filepath
    int count = 0;  // number of read directories
    int mount_remind = 0; // whether a directory is a mounting point
    time_t now = time(NULL); // time() returns the current calender time

    if ((dir = opendir(directory)) == NULL) {
        throwError("Error: Unable to open directory", directory);
    }

    while ((sd = readdir(dir)) != NULL) {
        if (mount_remind) { // if the last read directory is a mounting point, print its info and a note
            if (checkm(buf.st_mtime, now, mtime, check_m)) // check "-m mtime"
                printf("%llu%9lli %s%5hu %s%15s%20lli %s %s\n", 
                    buf.st_ino, buf.st_blocks/2, perms, buf.st_nlink, user, group, buf.st_size, date, path);
            fprintf(stderr, "note: not crossing mount point at %s\n", path);
            mount_remind = 0;
        }

        snprintf(path, sizeof(path), "%s/%s", directory, sd->d_name); // store the current filepath name
        if (stat(path, &buf) < 0) { // obtain file information by a path name and store it to a buffer 'buf'
            throwError("Error: Unable to obtain stat for a path/file", path);
        };
        if ((grp = getgrgid(buf.st_gid)) == NULL) { // check group permission
            throwError("Error: Unable to get the group associated with file/directroy", directory);
        };
        if ((usr = getpwuid(buf.st_uid)) == NULL) { // check user permission
            throwError("Error: Unable to get the user associated with file/directroy", directory);
        };
        group = grp->gr_name;
        user = usr->pw_name;
        perms = getPermission(buf.st_mode); // get file permission mask
        count++;
        if (count == 1 && check_v) { // store device number of a starting_path
            dev_num[0] = major(buf.st_dev);
            dev_num[1] = minor(buf.st_dev);
        }

        localtime_r(&buf.st_mtime, &file_time); // get time of last file modification
        localtime_r(&now, &current_time); // get current time
        if (file_time.tm_year == current_time.tm_year) { // "recent" is defined to be within the same year
            strftime(date, sizeof(date), "%b %e %H:%M", &file_time); // put last file change time into certain format
        }
        else {
            strftime(date, sizeof(date), "%b %e  %Y", &file_time);
        }

        if (S_ISDIR(buf.st_mode)) { // if mode is a directory
            if ((strcmp(sd->d_name, "..") != 0) && (strcmp(sd->d_name, ".") != 0)) {
                // if not within the same filesystem, traversal will not descend into other volumes
                if (!checkv(dev_num, buf, check_v)) {
                    perms[0] = 'd';
                    mount_remind = 1;
                    continue;
                }
                traverseDir(path, check_m, mtime, check_v); // recursively read files (descend into dir contents)
            }
            if (!checkm(buf.st_mtime, now, mtime, check_m)) { // check "-m mtime"
                continue;
            }
            else if (strcmp(sd->d_name, ".") == 0) {
                perms[0] = 'd';
                printf("%llu%9lli %s%5hu %s%15s%20lli %s %s\n", 
                    buf.st_ino, buf.st_blocks/2, perms, buf.st_nlink, user, group, buf.st_size, date, directory);
            }
        }
        else if (S_ISREG(buf.st_mode)) { // if mode is a regular file
            if (!checkm(buf.st_mtime, now, mtime, check_m)) continue; // check "-m mtime"
            printf("%llu%9lli %s%5hu %s%15s%20lli %s %s\n", 
                buf.st_ino, buf.st_blocks/2, perms, buf.st_nlink, user, group, buf.st_size, date, path);
        }
        else if (S_ISLNK(buf.st_mode)) { // if mode is a symlink
            perms[0] = 'l';
            ssize_t len = readlink(path, link, sizeof(link)-1); // read symbolic link (unistd.h)
            if (!checkm(buf.st_mtime, now, mtime, check_m)) continue; // check "-m mtime"
            if (len < 0) {
                throwError("Error: Unable to read path of symbolic link", path);
            }
            else {
                link[len] = '\0';
            }
            printf("%llu%9lli %s%5hu %s %15s%20lli %s %s->%s\n", 
                buf.st_ino, buf.st_blocks/2, perms, buf.st_nlink, user, group, buf.st_size, date, path, link);
        }
        else if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode)) { // if mode is a character special or block special node
            perms[0] = S_ISCHR(buf.st_mode) ? 'c' : 'b';
            if (!checkm(buf.st_mtime, now, mtime, check_m)) continue;
            sprintf(rdev_num, "%d/%d", major(buf.st_rdev), minor(buf.st_rdev)); // obtain raw device number
            printf("%llu%9lli %s%5hu %s%15s%20s %s %s\n", 
                buf.st_ino, buf.st_blocks/2, perms, buf.st_nlink, user, group, rdev_num, date, path);
        }
        else {
            throwError("Error: This type of file is not yet supported", NULL);
        }
        free(perms); // free the allocated memory
    }

    if (closedir(dir) < 0) {
        throwError("Error: Unable to close directory", directory);
    }
}
