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
$   copy [.vms]new-file. 'base_disk'[.tests]new-file.
$   purge 'base_disk'[.tests]new-file.
$!
$   ! Temp hack because of known bug in CRTL tripped by coreutils
$   test_name = "no-dereference"
$   create 'base_disk'[.tests]'test_name'.
$   open/append test_fix 'base_disk'[.tests]'test_name'.
$   write test_fix "printf "":test-result: SKIP\n"" > ''test_name'.trs"
$   write test_fix "printf "":global-test-result: SKIP\n"" >> ''test_name'.trs"
$   write test_fix "printf "":recheck: yes\n"" >> ''test_name'.trs"
$   write test_fix "printf "":copy-in-global-log: yes\n"" >> ''test_name'.trs"
$   write test_fix "exit 1"
$   close test_fix
$   purge 'base_disk'[.tests]'test_name'.
$!
$!  help-version needs gzip to even hope to pass
$!  env utility also appears not to be working.
$!  help-version also needs a /dev/full to be implemented which is not
$!  going to happen soon.
$!  Script also looks like it is testing coreutils tools like mknod() which
$!  do not port to VMS.
$!----------------------------------------------
$!   if f$search("gnv$gnu:[*...]gzip") .eqs. ""
$!   then
$	test_name = "help-version"
$	create 'base_disk'[.tests]'test_name'.
$	open/append test_fix 'base_disk'[.tests]'test_name'.
$	write test_fix "printf "":test-result: SKIP\n"" > ''test_name'.trs"
$	write test_fix -
	  "printf "":global-test-result: SKIP\n"" >> ''test_name'.trs"
$	write test_fix "printf "":recheck: yes\n"" >> ''test_name'.trs"
$	write test_fix -
	  "printf "":copy-in-global-log: yes\n"" >> ''test_name'.trs"
$	write test_fix "exit 1"
$	close test_fix
$	purge 'base_disk'[.tests]'test_name'.
$!   endif
$!
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
