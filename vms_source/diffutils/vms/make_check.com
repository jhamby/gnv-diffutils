$! Make_check.com
$!
$! Special handling for making and running diffutils tests.
$!
$! 26-Jun-2016	J. Malmberg
$!
$!------------------------------------------------------------------------
$!
$!
$! The CRTL can not handle if the default is set to a directory that
$! does not exist in the first member of a search list.
$!
$! For simplicity, assume sys$disk is not a search list by itself
$! ---------------------------------------------------------------
$ old_default = f$environment("default")
$!
$ sys_disk  = f$trnlnm("sys$disk") - ":"
$!
$ max_index = f$trnlnm(sys_disk,,,,, "max_index")
$ if max_index .ne. 0
$ then
$   base_disk = f$trnlnm(sys_disk,, 0)
$   i = 1
$   if f$search("'base_disk'[.vms]make_check.sh") .eqs. ""
$   then
$copy_loop:
$	if i .gt. max_index then goto copy_loop_end
$	diskn = f$trnlnm(sys_disk,, i)
$
$	if f$search("''diskn'[.vms]make_check.sh") .nes. ""
$	then
$!	    copy 'diskn'[.vms]make_check.sh 'base_disk'[.vms]
$!	    purge 'base_disk'[.vms]make_check.sh
$	endif
$	i = i + 1
$	goto copy_loop
$copy_loop_end:
$   endif
$   ! Temp hack to prevent hang
$   create 'base_disk'[.tests]new-file.
$   open/append test_fix 'base_disk'[.tests]new-file.
$   write test_fix "exit 1"
$   close test_fix
$   purge 'base_disk'[.tests]new-file.
$!
$   ! Temp hack because of known bug
$   create 'base_disk'[.tests]no-dereference.
$   open/append test_fix 'base_disk'[.tests]no-dereference.
$   write test_fix "exit 1"
$   close test_fix
$   purge 'base_disk'[.tests]no-dereference.
$!
$!   set default 'base_disk'
$ endif
$!
$ set nover
$ bash vms/make_check.sh
$!
$ @[.vms]convert_make_check_junit.com [.tests]make_check.out -
     "diffutils" "check"
$!
$!
$ set default 'old_default'
