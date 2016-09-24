/* File: vms_popen_hack.c
 *
 * Provide a wrapper or replacement to the popen() and system() functions
 * that works with GNV.
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
 */

#ifndef _POSIX_EXIT
#define _POSIX_EXIT=1
#endif

#include <vms_fake_path/ctype.h>
#include <vms_fake_path/errno.h>
#include <vms_fake_path/stdio.h>
#include <vms_fake_path/stdlib.h>
#include <vms_fake_path/string.h>
#include <vms_fake_path/unixlib.h>
#include <vms_fake_path/unistd.h>

char * vms_get_foreign_cmd(const char * exec_name);
int vms_fifo_write_pipe(int *pipe_fd);
int vms_fifo_read_pipe(int *pipe_fd);
int vms_execvp (const char *file_name, char * argv[]);
FILE * vms_popen_helper(int *pipeno, pid_t pid, const char *mode);

/* Look up the foreign command for the shell, fall back to bash */
static const char * real_shell(const char * shell_name) {
    const char * exec_name;
    char * shell_exe;

    exec_name = strrchr(shell_name, '/');
    if (exec_name == NULL) {
        exec_name = shell_name;
    } else {
        exec_name++;
    }
    shell_exe = vms_get_foreign_cmd(exec_name);
    if (shell_exe == NULL) {
        exec_name = "bash";
    }
    return exec_name;
}

#if __CRTL_VER >= 70302000 && !defined(__VAX)
#define MAX_DCL_LINE_LENGTH 4095
#else
#define MAX_DCL_LINE_LENGTH 1023
#endif

#define MAX_TOKENS 254

struct token_s {
    char * value;
    int len;
    int quote;
    int posix_path;
    int vms_path;
    int qual;
    int space;
    int pipechar;
};

#define VMS_LSH_FNAME_LEN 4096
#if __CRTL_VER >= 70301000
# define transpath_parm transpath
#else
static char transpath[VMS_LSH_FNAME_LEN - 1];
#endif

/* Helper callback routine for converting Unix paths to VMS */
static int
to_vms_action (char * vms_spec, int flag, char * transpath_parm)
{
  strncpy (transpath, vms_spec, VMS_LSH_FNAME_LEN -2);
  transpath[VMS_LSH_FNAME_LEN - 2] = 0;
  return 0;
}

#ifdef __DECC
# pragma message save
  /* Undocumented extra parameter use triggers a ptrmismatch warning */
# pragma message disable ptrmismatch
#endif


/* If token is a Unix path:
 * Try to convert to VMS and add to path if room.
 * If no room do not add to path.
 * If can not translate to VMS, add original string.
 * Return 0 on success, -1 if nothing added.
 */
static int add_token_to_buf(struct token_s * token,
                            char * buf,
                            int * buf_len,
                            int posix_opt) {

    int status;
    int path_len;
    int new_string_len;
    new_string_len = *buf_len;

    if (token->posix_path) {
        /* Translate to a VMS path */
        char * vms_path;
        char * unix_path;

        vms_path = malloc(VMS_LSH_FNAME_LEN + 1);
        if (vms_path == NULL) {
            return -1;
        }
        unix_path = malloc(token->len + 1);
        if (unix_path == NULL) {
            int saved_errno;
            saved_errno = errno;
            free(vms_path);
            errno = saved_errno;
            return -1;
        }
        strncpy(unix_path, token->value, token->len);
        unix_path[token->len] = 0;

#if __CRTL_VER >= 70301000
        /* Current decc$to_vms is reentrant */
        status = decc$to_vms (unix_path, to_vms_action, 0, 0, vms_path);
#else
        /* Older decc$to_vms is not reentrant */
        status = decc$to_vms (unix_dir, to_vms_action, 0, 0);
        if (status > 0) {
            strncpy (vms_path, transpath, VMS_LSH_FNAME_LEN - dir_len);
            vms_path[VMS_LSH_FNAME_LEN] = 0;
        }
#endif
        free(unix_path);
        if (status > 0) {
            path_len = new_string_len + strlen(vms_path);
            if (path_len >= MAX_DCL_LINE_LENGTH) {
                free(vms_path);
                vms_path = NULL;
            } else {
                new_string_len = path_len;
            }
        } else {
            free(vms_path);
            vms_path = NULL;
        }
        if (vms_path != NULL) {
            strcat(buf, vms_path);
            free(vms_path);
            *buf_len = new_string_len;
            return 0;
        }
    }
    if (token->quote == '\'') {
        path_len = new_string_len + token->len;
        if (path_len < (MAX_DCL_LINE_LENGTH - 1)) {
            if (posix_opt) {
                buf[new_string_len] = '"';
                new_string_len++;
            }
            if (token->len > 2) {
                strncpy(&buf[new_string_len], &token->value[1], token->len - 2);
                new_string_len += (token->len - 2);
            }
            if ((token->len < 2) || (token->value[token->len - 1] != '\'')) {
                /* Missing trailing single quote? */
                buf[new_string_len] = token->value[token->len - 1];
                new_string_len++;
            }
            if (posix_opt) {
                buf[new_string_len] = '"';
                new_string_len++;
            }
            buf[new_string_len] = 0;
            *buf_len = new_string_len;
            return 0;
        }
    }

    /* Simple token copy */
    path_len = new_string_len + token->len;
    if (path_len < MAX_DCL_LINE_LENGTH) {
        strncat(buf, token->value, token->len);
        buf[path_len] = 0;
        new_string_len = path_len;
        *buf_len = new_string_len;
        return 0;
    }
    return -1;
}


/* Simplified parse of DCL or attempt to convert shell synxtax to DCL.
 * Generate a new DCL string based on input.
 * Detect Unix filenames and convert them to VMS format.
 * If pipe characaters are present add pipe command if missing.
 */
static char * dcl_string(char * string, void **argv_ptr) {
    char *new_string;
    int new_string_len;
    int no_pipe;
    int need_pipe;
    int i;
    int ti;
    int string_len;
    int posix_opt = 0;
    struct token_s tokens[MAX_TOKENS+1];
    int token_cnt = 0;
    char **argv;

    new_string = malloc(MAX_DCL_LINE_LENGTH + 1);

    memset(tokens, 0, sizeof (struct token_s) * MAX_TOKENS);
    string_len = strlen(string);
    i = 0;
    while (isspace(string[i]) && (i < string_len)) {
        i++;
    }
    /* Does string start with a PIPE command */
    no_pipe = strncasecmp(&string[i], "pipe ", 5);
    if (!no_pipe) {
        tokens[0].value = &string[i];
        tokens[0].len = 4;
        tokens[0].quote = 0;
        token_cnt++;
        i = i + 5;
    }

    need_pipe = 0;

    while ((i < string_len) && (token_cnt < MAX_TOKENS)) {
        int start_slash;
        int vms_path;
        int posix_path;

        /* Token always starts here */
        int j;
        j = 0;

        if (isspace(string[i])) {
            tokens[token_cnt].value = &string[i];
            while (isspace(string[i]) && (i < string_len)) {
                i++;
                j++;
            }
            tokens[token_cnt].len = j;
            tokens[token_cnt].space = 1;
            token_cnt++;
            j = 0;
            if (token_cnt >= MAX_TOKENS) {
                break;
            }
        }

        /* Simplified Parser for now */
        switch(string[i]) {
        case '"':
            /* Assume simple VMS quoting - No imbedded lexicals
             * Stop at next matching double quote or EOL
             */
            tokens[token_cnt].value = &string[i];
            tokens[token_cnt].quote = '"';
            while (i < string_len) {
                i++;
                if (string[i] == '"') {
                    if (string[i+1] != '"') {
                        i++;
                        j += 2;
                        break;
                    } else {
                        i++;
                        j++;
                    }
               }
               j++;
           }
           tokens[token_cnt].len = j;
           token_cnt++;
           j = 0;
           break;
        case '\'':
            /* A Unix tool thinks this is just a quote */
            tokens[token_cnt].value = &string[i];
            tokens[token_cnt].quote = '\'';
            while (i < string_len) {
               i++;
               if (string[i] == '\'') {
                   j += 2;
                   i++;
                   break;
               }
               j++;
           }
           tokens[token_cnt].len = j;
           token_cnt++;
           j = 0;
           break;
        case ':':
        case '=':
           tokens[token_cnt].value = &string[i];
           tokens[token_cnt].len = 1;
           token_cnt++;
           i++;
           break;
        case ')':
        case '(':
           tokens[token_cnt].value = &string[i];
           tokens[token_cnt].len = 1;
           token_cnt++;
           i++;
           break;
        case '<':
        case '>':
        case ';':
           need_pipe = 1;
           tokens[token_cnt].pipechar = 1;
           tokens[token_cnt].value = &string[i];
           tokens[token_cnt].len = 1;
           token_cnt++;
           i++;
           break;
        case '|':
        case '&':
           need_pipe = 1;
           tokens[token_cnt].pipechar = 1;
           tokens[token_cnt].value = &string[i];
           j++;
           if (string[i+1] == string[i]) {
               i++;
               j++;
           }
           tokens[token_cnt].len = j;
           token_cnt++;
           i++;
           break;
        case '2':
           if (string[i+1] == '>') {
               need_pipe = 1;
               tokens[token_cnt].pipechar = 1;
               tokens[token_cnt].value = &string[i];
               tokens[token_cnt].len = 2;
               token_cnt++;
               i += 2;
               break;
           }
        case '-':
            posix_opt = 1;
        default:
           start_slash = 0;
           tokens[token_cnt].value = &string[i];
           if (string[i] == '/') {
               tokens[token_cnt].qual = 1;  /* Guess */
               tokens[token_cnt].posix_path = 1; /* Guess */
               /* Need a slash count of 2 for a file */
               start_slash = 1;
               /* If a slash ends with a ':' or '='; not a file */
               i++;
               j++;
           }
           while (i < string_len) {
               int slash_cnt;
               int break_loop;
               break_loop = 0;
               slash_cnt = start_slash;
               switch(string[i]) {
               case '|':
               case '&':
                   break_loop = 1;
                   break;
               case '=':
                   if (start_slash != 0) {
                       /* VMS qualifier with value */
                       tokens[token_cnt].posix_path=0;
                   }
                   break_loop = 1;
                   break;
               case ':':
                   if (start_slash != 0) {
                       /* VMS qualifier with value */
                       tokens[token_cnt].posix_path=0;
                       break_loop = 1;
                       break;
                   } else {
                       if (string[i+1] != 0) {
                           tokens[token_cnt].vms_path = 1;
                       } else {
                         /* Don't know pass it through */
                         break_loop = 1;
                         break;
                       }
                   }
               case '^':
                   if (start_slash != 0) {
                       /* Unknown syntax, will probably error out */
                       break;
                   } else {
                       i++;
                       j++;
                       if (string[i+1] != 0) {
                           tokens[token_cnt].vms_path = 1;
                           i++;
                           j++;
                       } else {
                           /* Unknown syntax, will probably error out */
                           tokens[token_cnt].vms_path = 0;
                           break;
                       }
                   }
               case ';':
               case '<':
               case '>':
                   if (tokens[token_cnt].vms_path == 0) {
                       break_loop = 1;
                       break;
                   }
               case '[':
                   if (start_slash != 0) {
                       /* Unknown syntax, will probably error out */
                       break_loop = 1;
                       break;
                   } else {
                       tokens[token_cnt].vms_path = 1;
                   }
               case '/':
                   if (start_slash != 0) {
                       /* We have confirmed a posix path
                        * Restriction qualifiers must have leading space
                        */
                       slash_cnt++;
                       tokens[token_cnt].posix_path = 1;
                       tokens[token_cnt].qual = 0;
                   } else {
                     break_loop = 1;
                     break;
                   }
               default:
                   if (isspace(string[i])) {
                       /* End of token */
                       if ((posix_path == 1) && start_slash != 0) {
                           posix_path = 0;
                       }
                       break_loop = 1;
                       break;
                   }
               }
               if (!break_loop) {
                   i++;
                   j++;
               } else {
                   break;
               }
           }
           tokens[token_cnt].len = j;
           token_cnt++;
        }
    }

    if (no_pipe && need_pipe) {
        strcpy(new_string, "pipe ");
        new_string_len = 5;
    } else {
        new_string[0] = 0;
        new_string_len = 0;
    }

    ti = 0;
    if (token_cnt > ti) {
        add_token_to_buf(&tokens[ti], new_string, &new_string_len, posix_opt);
        ti++;
    }

    while ((ti < token_cnt) && (new_string_len < MAX_DCL_LINE_LENGTH)) {
        if (tokens[ti].pipechar) {
            if (tokens[ti-1].space == 0) {
                /* Need a leading space */
                strcat(new_string, " ");
                new_string_len++;
            }
        }
        add_token_to_buf(&tokens[ti], new_string, &new_string_len, posix_opt);
        ti++;
    }

    if (argv_ptr != NULL) {
        argv = malloc(sizeof(char *) * (token_cnt + 1));
        if (argv != NULL) {
            int ti;
            int argc;
            *argv_ptr = argv;
            memset(argv, 0, sizeof(char *) * (token_cnt + 1));
            ti = 0;
            argc = 0;
            while (ti < token_cnt) {
                if (tokens[ti].space == 0) {
                    argv[argc] = malloc(tokens[ti].len + 1);
                    if (argv[argc] == NULL) {
                        break;
                    }
                    if (tokens[ti].quote == 0) {
                        /* Check if Unix filename and convert? */
                        strncpy(argv[argc], tokens[ti].value, tokens[ti].len);
                        argv[argc][tokens[ti].len] = 0;
                    } else {
                        strncpy(argv[argc],
                                &tokens[ti].value[1], tokens[ti].len - 2);
                        argv[argc][tokens[ti].len - 2] = 0;
                    }
                    argc++;
                }
                ti++;
            }
        }
    }

    return new_string;
}

/* Convert a shell command to be a "sh -c "command"
 * This means changing all single quotes to double double quotes
 */
char * dcl_shell_string(const char * shell,
                        const char * string,
                        void ** argv_ptr) {
    char *new_string;
    char *shell_string;
    int new_string_len;
    int shell_string_len;
    int max_shell_len;
    int i, j;
    char ** argv;

    new_string = malloc(MAX_DCL_LINE_LENGTH + 1);

    /* bash "-c" "echo ""foo"">foo.bar" */
    strcpy(new_string, shell);
    strcat(new_string, " \"-c\" \"");
    new_string_len = strlen(new_string);

    /* Fix quoting for command to be DCL based */
    shell_string = &new_string[new_string_len];
    shell_string_len = 0;
    max_shell_len = MAX_DCL_LINE_LENGTH - new_string_len - 1;
    i = 0;
    j = 0;
    while ((string[i] != 0) && (j < max_shell_len - 1)){
        if (string[i] == '"') {
            shell_string[j] = '"';
            j++;
        }
        shell_string[j] = string[i];
        j++;
        i++;
    }
    shell_string[j] = 0;
    strcat(new_string, "\"");

    if (argv_ptr != NULL) {
        argv = malloc(sizeof(char *) * 4);
        if (argv != NULL) {
            *argv_ptr = argv;
            argv[0] = strdup(shell);
            argv[1] = strdup("c");
            argv[2] = strdup(string);
            argv[3] = 0;
        }
    }
    return new_string;
}

static FILE * vms_vm_popen(const char ** argv, const char *mode) {

int status;
int status2;
int save_errno;
int pipe_fd[2];
int child_fd[2];
pid_t child;

    if (mode[1] != 0) {
        errno = EINVAL;
        return NULL;
    }
    child_fd[0] = -1;
    child_fd[1] = -1;
    pipe_fd[0] = -1;
    pipe_fd[1] = -1;
    if (mode[0] == 'r') {
        status = vms_fifo_read_pipe(pipe_fd);
        if (status < 0) {
            return NULL;
        }
        child_fd[1] = pipe_fd[1];

    } else if (mode[0] == 'w') {
        status = vms_fifo_write_pipe(pipe_fd);
        if (status < 0) {
            return NULL;
        }
        child_fd[0] = pipe_fd[0];

    } else {
        errno = EINVAL;
        return NULL;
    }

    child = vfork();
    if (child < 0) {
        save_errno = errno;
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        errno = save_errno;
        return NULL;
    }
    if (child == 0) {
        status2 = decc$set_child_standard_streams(child_fd[0], child_fd[1], -1);
        vms_execvp(argv[0], argv);
    }
    save_errno = errno;
    status2 = decc$set_child_standard_streams(-1, -1, -1);

    /* Failure to launch */
    if (child == 0) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        errno = save_errno;
        return NULL;
    }

    /* Close end of pipe now open by the child */
    if (child_fd[0] != -1) {
        close(child_fd[0]);
    } else {
        close(child_fd[1]);
    }

    return vms_popen_helper(pipe_fd, child, mode);
}


/* Wrapper for popen() routine, adjusting for Bash or Posix paths */
FILE * vms_popen(const char *command, const char *mode) {

    char * shell;

    /* mode[0] == 'w' child read from stdin */
    /* mode[0] == 'r' child write to stdout */
    /* Above with mode[1] == 0.  All other values undefined behavior */

    shell = getenv("SHELL");
    if (shell == NULL) {
        /* If getenv("SHELL") is nothing, then use normal popen() */
        /* fix up command for VMS */
        char * dcl_cmd;
        char ** argv;
        dcl_cmd = dcl_string((char *)command, &argv);
        if (dcl_cmd != NULL) {
            FILE *pfp;
            int save_errno;
            int argc;
            pfp = vms_vm_popen(argv, mode);
            save_errno = errno;
            free(dcl_cmd);
            argc = 0;
            while (argv[argc] != 0) {
                free(argv[argc]);
                argc++;
            }
            free(argv);
            errno = save_errno;
            return pfp;
        } else {
            /* Malloc for DCL string failed? */
            return popen(command, mode);
        }
    } else {
        /* Command is changed to equivalent of:
         * execl(shell path, "sh", "-c", command, (char *)0);
         * if gnv -> bash "-c" "echo ""foo"">foo.bar"
         * Need to handle coverting getenv("SHELL") to find the shell
         * as it may be missing the path on OpenVMS.
         * Need to convert shell quoting to DCL quoting.
         */
        const char * shell_cmd;
        char * dcl_bash_string;
        char ** argv;
        shell_cmd = real_shell(shell);
        dcl_bash_string = dcl_shell_string(shell_cmd, command, &argv);
        if (dcl_bash_string != NULL) {
            FILE *pfp;
            int save_errno;
            int argc;
            pfp = vms_vm_popen(argv, mode);
            save_errno = errno;
            free(dcl_bash_string);
            argc = 0;
            while (argv[argc] != 0) {
                free(argv[argc]);
                argc++;
            }
            free(argv);
            errno = save_errno;
            return pfp;
        } else {
            return popen(command, mode);
        }
    }
}



int vms_system(const char *string) {

    int status;
    char * shell;

    shell = getenv("SHELL");
    if (shell == NULL) {
        char * dcl_cmd;
        dcl_cmd = dcl_string((char *)string, NULL);
        if (dcl_cmd != NULL) {
            int save_errno;
            status = system(dcl_cmd);
            save_errno = errno;
            free(dcl_cmd);
            errno = save_errno;
        } else {
            /* Malloc for DCL string failed? */
            status = system(string);
        }
        /* DCL utilities return 1 for success */
        if ((status & 0xFF00) == 256) {
            status = status & 0xFF;
        }
    } else {
        /* Command is changed to equivalent of:
         * execl(shell path, "sh", "-c", command, (char *)0);
         * if gnv -> bash "-c" "echo ""foo"">foo.bar"
         */
        const char * shell_cmd;
        char * dcl_bash_string;
        shell_cmd = real_shell(shell);
        dcl_bash_string = dcl_shell_string(shell_cmd, string, NULL);
        if (dcl_bash_string != NULL) {
            int save_errno;
            status = system(dcl_bash_string);
            save_errno = errno;
            free(dcl_bash_string);
            errno = save_errno;
        } else {
            /* Malloc failed, fall back to original string */
            status = system(string);
        }
    }
    return status;
}


#ifdef DEBUG

#include <vms_fake_path/string.h>

int main(int argc, char **argv) {

    int status;
    int exit_status;
    FILE * fptr;
    char code[2];

    exit_status = EXIT_SUCCESS;
    if (argc < 2) {
        exit(EXIT_FAILURE);
    }
    code[0] = 0;
    code[1] = 0;
    if (argc > 2) {
        code[0] = argv[2][0];
    }

    if (code[0] == 's') {
        status = vms_system(argv[1]);
        if (status < 0) {
            perror("vms_system");
            exit_status = EXIT_FAILURE;
        } else {
            puts("vms_system call worked");
        }
        return exit_status;
    }
    fptr = vms_popen(argv[1], code);
    if (fptr != NULL) {
        if (code[0] == 'w') {
            if (argc > 3) {
                int len;
                len = strlen(argv[3]);
                status = fwrite(argv[3], 1, len, fptr);
                if (status < 0) {
                    perror("fwrite");
                    exit_status = EXIT_FAILURE;
                }
            }
        } else if (code[0] == 'r') {
            char buffer[1024];
            status = fread(buffer, 1, 1023, fptr);
            if (status < 0) {
                perror("fread");
                exit_status = EXIT_FAILURE;
            } else {
                buffer[status] = 0;
                puts(buffer);
            }
        }
        status = pclose(fptr);
        if (status < 0) {
            perror("pclose");
            exit_status = EXIT_FAILURE;
        } else {
            fprintf(fptr, "pclose returned %d\n", status);
        }
    } else {
        perror("vms_popen");
        exit_status = EXIT_FAILURE;
    }
    return exit_status;
}
#endif
