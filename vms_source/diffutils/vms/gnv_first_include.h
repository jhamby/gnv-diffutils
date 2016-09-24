#define __UNIX_PUTC=1
#pragma message disable questcompare
#pragma message disable questcompare1
#pragma message disable notincrtl
#define SA_RESTART 0

#include "vms_lstat_hack.h"
#ifdef vfork
#undef vfork
#endif
#include "gnv_vms_iconv.h"

#include "vms_getrlimit_hack.h"

#define LLONG_MAX __INT64_MAX
#define LLONG_MIN __INT64_MIN

/* Issue identified by Stephen M Schweda port */
#include "vms_fake_path/stdio.h"
/* Bug in VMS stdio.h uses a VMS path for P_tmpdir */
#ifdef P_tmpdir
#undef P_tmpdir
#endif

/* Issue identified by Stephen M Schweda port */
#include "vms_fake_path/fcntl.h"
#define O_BINARY O_NOCTTY
static int vms_open(const char *file_spec, int flags, mode_t mode) {
    if ((flags & O_BINARY) == O_BINARY) {
        /* Remove fake O_BINARY flag */
        int new_flags = flags & ~O_BINARY;
        return open(file_spec, new_flags, mode, "ctx=stm");
    } else {
        return open(file_spec, flags, mode);
    }
}
#define open(fs, flgs, mode)  vms_open(fs, flgs, mode)

/* lib/binary-io.h #defines set_binary_mode as _setmode
 * The vms_crtl_init sets the pipe mode to stream already
 */
#define _setmode(__filenum, __mode) (0)

/* lib/binary-io.h #defines fileno as _fileno */
static int _fileno(FILE *fileptr) { return fileno(fileptr); }


/* Issue identified by Stephen M Schweda port */
#undef unlink
static int vms_unlink_all(const char * path) {
    int status;
    int status2;
    int cnt;
    cnt = 0;
    /* Limit the loops to the max version number */
    /* That should never be needed */
    do {
        status = vms_unlink(path);
        if (status < 0) {
            break;
        }
        status2 = access(path, F_OK | W_OK);
        cnt++;
    } while ((status2 >= 0) && (cnt < 65535));
    return status;
}
#define unlink vms_unlink_all

/* Issue identified by Stephen M Schweda port */

static size_t vms_fwrite(const void *ptr, size_t size, size_t num, FILE *fp) {
    if (size == 1) {
        int status;
        status = fwrite(ptr, num, size, fp);
        if (status == size) {
            return status * num;
        }
        return status;
    } else {
        return fwrite(ptr, size, num, fp);
    }
}
#define fwrite vms_fwrite

/* Issue identified by Stephen M Schewda port */
FILE * vms_popen(const char *command, const char *mode);
int vms_pclose(FILE * stream);
int vms_system(const char *string);
int vms_execvp (const char *file_name, char * argv[]);

#define popen vms_popen
#define pclose vms_pclose
#define system vms_system
#define execvp vms_execvp

/* Issue identified by Stephen M Schweda port */
#ifndef __VAX
#ifdef GDIFF_MAIN
#define initialize_main(argcp, argvp) vms_set_case_ignore(argcp argvp)

/* This may need to be moved into its own file */

#include "jpidef.h"
#include "descrip.h"
#include "ppropdef.h"
#include "stsdef.h"

int LIB$GETJPI(int const *item_code,
               int const *process_id,
               struct dsc$descriptor_s * process_name,
               void * result_number,
               struct dsc$descriptor_s * result_string,
               unsigned short * result_length);

extern bool ignore_file_name_case;

static vms_set_case_ignore(int argcp, char **argvp) {

    int status;
    int result_temp;

    ignore_file_name_case = 1;
    status = LIB$GETJPI(JPI$_CASE_LOOKUP_TEMP, 0, 0, &result_temp, 0, 0);
    if ($VMS_STATUS_SUCCESS(status)) {
        if (result_temp == PPROP$K_CASE_SENSITIVE) {
            ignore_file_name_case = 0;
        }
    }
}
#endif /* GDIFF_MAIN */
#endif /* __VAX */
