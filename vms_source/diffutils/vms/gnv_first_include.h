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
