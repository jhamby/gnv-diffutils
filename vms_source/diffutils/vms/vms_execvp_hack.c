/* File: vms_execvp_hack.c
 *
 * Wrapper for the execvp() routine.
 *
 * Copyright 2016, John Malmberg
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * 20-Aug-2016 J. Malmberg
 *==========================================================================
 */

#ifndef __USE_STD_STAT
#define __USE_STD_STAT=1
#endif

/* The vms_fake_path is a non-existant logical name which causes
 * the C compiler to use the VMS provided headers from the system text
 * libraries instead of any locally provided header files, which
 * some GNU utilities provide.
 */
#include <vms_fake_path/errno.h>
#include <vms_fake_path/fcntl.h>
#include <vms_fake_path/unistd.h>
#include <vms_fake_path/stdlib.h>
#include <vms_fake_path/string.h>

char * vms_get_foreign_cmd(const char * exec_name);


/* Future optimizations:
   Return an array of structures that also return the length of the strings.
   If used with vfork() then the last parsed PATH should be cached in static
   storage.
*/


static char ** split_path(const char * string) {

    int i;
    const char * test_str;
    char ** result;
    int result_size;

    result_size = 100 * sizeof(char *);
    result = malloc(result_size);
    if (result == NULL) {
       return NULL;
    }

    i = 0;
    result[0] = NULL;
    test_str = string;
    do {
        const char * colon_ptr;
        const char * str_start;
        int str_len;

        /* Unlikely, but allow array expansion */
        if (i == (result_size -1)) {
            char ** result2;
            result2 = result;
            result_size += 100 * sizeof(char *);
            result = malloc(result_size);
            if (result == NULL) {
                /* If we can not expand the array, go with what we have */
                return result2;
            }
            memcpy(result, result2, (i * sizeof(char *)));
            free(result2);
        }

        str_start = test_str;
        colon_ptr = strchr(str_start, ':');
        if (colon_ptr != NULL) {

            /* Found a path, copy into the array */
            str_len = colon_ptr - str_start;
            colon_ptr++;
            test_str = colon_ptr;
        } else {
            str_len = strlen(str_start);
            test_str = NULL;
        }

        if (str_len != 0) {
            result[i] = malloc(str_len + 1);
            if (result[i] != NULL) {
                strncpy(result[i], str_start, str_len);
                result[i][str_len] = 0;
            } else {
                test_str = NULL;
            }
        }
        i++;
        result[i] = NULL;

    } while (test_str != NULL);

    return result;
}

/* Search for a command, return NULL command not found. */
/* TODO: When an image is found, find it's real name as */
/* as it may be an installed image with privileges and  */
/* we need to use it to simulate setuid */
static char * search_for_command(const char * exec_name,
                                 char ** path_list) {
    char * testpath;
    int testpath_len;
    int exec_name_len;
    int found;
    int i;
    char * slash;

    exec_name_len = strlen(exec_name);
    testpath_len = 4096 + exec_name_len;
    testpath = malloc(testpath_len + 1);
    if (testpath == NULL) {
        return NULL;
    }

    /* If exec_name has a / in it, it has a path */
    slash = strchr(exec_name, '/');
    if (slash != NULL) {
        strcpy(testpath, exec_name);
        return testpath;
    }

    i = 0;
    found = 0;
    testpath[0] = 0;

    /* For each path in path look for exec_name */
    while (path_list[i] != 0) {
        int status;
        int path_len;

        path_len = strlen(path_list[i]);
        if (testpath_len < (path_len + exec_name_len + 6)) {
            char * testpath2;
            int new_len;

            new_len = path_len + exec_name_len + 6 + 100;
            testpath2 = malloc(new_len);
            if (testpath2 == NULL) {
                break;
            } else {
                free(testpath);
                testpath = testpath2;
                testpath_len = new_len;
                testpath[0] = 0;
            }
        }
        strcpy(testpath, path_list[i]);
        strcat(testpath, "/");
        strcat(testpath, exec_name);

        status = access(testpath, X_OK);
        if (status < 0) {
            /* Also look for filename.exe */
            strcat(testpath, ".exe");
            status = access(testpath, X_OK);
        }
        if (status >= 0) {
           found = 1;
           break;
        }
        i++;
    }
    if (!found) {
        char * vms_foreign;
        char * shell;
        shell = getenv("SHELL");
        if (shell == NULL) {
            vms_foreign = "1";
        } else {
            vms_foreign = getenv("GNV_VMS_FOREIGN");
        }
        if ((vms_foreign != NULL) && (vms_foreign[0] == '1')) {
            char * foreign_cmd;
            foreign_cmd = vms_get_foreign_cmd(exec_name);
            if (foreign_cmd != NULL) {
                int foreign_cmd_len;
                foreign_cmd_len = strlen(foreign_cmd);
                if (foreign_cmd_len < testpath_len) {
                    strcpy(testpath, foreign_cmd);
                    found = 1;
                }
            }
        }
    }
    if (found) {
        return testpath;
    } else {
        free(testpath);
        return NULL;
    }
}

/* Extract the interpreter execpath from the buffer */
static char * getinterp(char * buf, int len, int * index) {
    char *execpath_start;
    char *execpath_end;
    char *execpath;
    int execpath_len;
    int i;

    /* Find start of the string */
    i = 0;
    execpath_start = buf;
    while ((*execpath_start == ' ') || (*execpath_start == '\t')) {
        execpath_start++;
        i++;
        if (i > len) {
            return NULL;
        }
    }
    execpath_end = strpbrk(execpath_start, " \t\r\n");
    if (execpath_end == NULL) {
        return NULL;
    }
    execpath_len = execpath_end - execpath_start;

    execpath = malloc(execpath_len + 1);
    strncpy(execpath, execpath_start, execpath_len);
    execpath[execpath_len] = 0;

    if (index != NULL) {
        *index = i + execpath_len;
    }
    return execpath;
}


/* Find an interpreter if this is a script */
static char * get_script_execname(const char * filename,
                                 char ** path_list) {

    int fd;
    int len;
    int i;
    char * execpath;

    execpath = NULL;
    len = -1;
    char buf[81];
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        len = read(fd, buf, 80);
        close(fd);
        if (len > 2 && buf[0] == '#' && buf[1] == '!') {
            int i;
            buf[len] = 0;
            execpath = getinterp(&buf[2], len - 2, &i);
            if ((!strncmp(execpath, "/usr/bin/env", 13)) ||
                (!strncmp(execpath, "/bin/env", 9))) {
                int new_i;
                char * new_execname;

                /* Skip calling env image on VMS */
                new_execname = getinterp(&buf[i], len - i + 2, &new_i);
                if (new_execname != NULL) {
                    char * new_execpath;
                    new_execpath = search_for_command(new_execname, path_list);
                    free(new_execname);
                    if (new_execpath != NULL) {
                        free(execpath);
                        execpath = new_execpath;
                    }
                }
            }
        }
    }
    return execpath;
}

/* Free the path list */
static void free_path(char ** path_list) {
    int i;
    i = 0;
    while (path_list[i] != NULL) {
        free(path_list[i]);
        i++;
    }
    free(path_list);
}


/* Wrapper for the CRTL execvp that actually uses the PATH environment
 * variable and looks up the shebang for scripts.
 */
int vms_execvp (const char *file_name, char * argv[]) {

    char * path;
    char ** path_list;
    int i;
    char *execpath;
    char *interpreter;
    int result;
    int saved_errno;

    path = getenv("PATH");
    if (path != NULL) {
        /* Need to split the path into an array */
        path_list = split_path(path);
    } else {
        /* If know path environment variable fake one with "." */
        path_list = split_path(".");
    }
    if (path_list == NULL) {
        return -1;
    }

    execpath = search_for_command(file_name, path_list);
    if (execpath == NULL) {
        int saved_errno;
        saved_errno = errno;
        free_path(path_list);
        errno = saved_errno;
        return -1;
    }

    interpreter = get_script_execname(execpath, path_list);
    if (interpreter != NULL) {
        result = execv(interpreter, argv);
        saved_errno = errno;
        free(interpreter);
    } else {
        result = execv(execpath, argv);
        saved_errno = errno;
    }
    free(execpath);
    return result;
}
