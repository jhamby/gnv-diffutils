$! File: convert_make_check_junit.com
$!
$! This file processs the output of make check to create a junit
$! format report.
$!
$! P1 is the output of make check.
$! P2 is the testsuite name.
$! P3 is the default test class name.
$! P4 may be a skip file in the future.
$!
$! 03-Jun-2013	J. Malmberg	diffutils version
$!
$!=========================================================================
$!
$ infile = p1
$ testsuite = p2
$ def_testclass = p3
$!
$ test_class = def_testclass
$ fail_msg = "failed"
$ fail_type = "check"
$ skip_reason = "known issue"
$!
$ arch_name = f$edit(f$getsyi("arch_name"), "UPCASE")
$!
$ open/read t_in 'infile'
$ gosub create_junit_test_header
$! Loop through list of tests.
$test_loop:
$   read/end=test_loop_end t_in line_in
$   line_in = f$edit(line_in, "trim")
$   key = f$extract(0, 6, line_in)
$   if (key .eqs. "PASS: ") .or. (key .eqs. "XFAIL:")
$   then
$	test = f$element(1, ":", line_in)
$	test = f$edit(test, "trim")
$	gosub junit_report_pass
$   else
$	if (key .eqs. "FAIL: ") .or. (key .eqs. "ERROR:") .or. -
           (key .eqs. "XPASS:")
$	then
$	    test = f$element(1, ":", line_in)
$	    test = f$edit(test, "trim")
$	    gosub junit_report_fail
$	else
$	    if key .eqs. "SKIP: "
$	    then
$		test = f$element(1, ":", line_in)
$		test = f$edit(test, "trim")
$		gosub junit_report_skip
$	    endif
$	endif
$   endif
$   goto test_loop
$test_loop_end:
$ gosub finish_junit_test
$!
$ close t_in
$!
$!
$all_exit:
$  exit
$!
$!
$create_junit_test_header:
$       junit_count = 0
$       temp_fdl = "sys$disk:[]stream_lf.fdl"
$!
$       junit_hdr_file = "sys$disk:[]test_output.xml"
$       if f$search(junit_hdr_file) .nes. "" then delete 'junit_hdr_file';*
$       junit_body_file = "sys$disk:[]test_body_tmp.xml"
$       if f$search(junit_body_file) .nes. "" then delete 'junit_body_file';*
$!!
$	arch_code = f$extract(0, 1, arch_name)
$       if arch_code .nes. "V"
$       then
$           create 'junit_hdr_file'/fdl="RECORD; FORMAT STREAM_LF;"
$           create 'junit_body_file'/fdl="RECORD; FORMAT STREAM_LF;"
$       else
$           if f$search(temp_fdl) .nes. "" then delete 'temp_fdl';*
$           create 'temp_fdl'
RECORD
        FORMAT          stream_lf
$           continue
$           create 'junit_hdr_file'/fdl='temp_fdl'
$           create 'junit_body_file'/fdl='temp_fdl'
$       endif
$       open/append junit 'junit_body_file'
$       return
$!
$!
$finish_junit_test:
$       open/append junit_hdr 'junit_hdr_file'
$       write junit_hdr "<?xml version=""1.0"" encoding=""UTF-8""?>"
$       write junit_hdr "<testsuite name=""''testsuite'"""
$       write junit_hdr " tests=""''junit_count'"">"
$       close junit_hdr
$       write junit "</testsuite>"
$       close junit
$       append 'junit_body_file' 'junit_hdr_file'
$       delete 'junit_body_file';*
$       return
$!
$!
$junit_report_skip:
$       write sys$output "Skipping test ''test' reason ''skip_reason'."
$       junit_count = junit_count + 1
$       write junit "  <testcase name=""''test'"""
$       write junit "   classname=""''test_class'"">"
$       write junit "     <skipped/>"
$       write junit "  </testcase>"
$       return
$!
$junit_report_fail_diff:
$       fail_msg = "failed"
$       fail_type = "diff"
$!      fall through to junit_report_fail
$junit_report_fail:
$       write sys$output "failing test ''test' reason ''fail_msg'."
$       junit_count = junit_count + 1
$       write junit "  <testcase name=""''test'"""
$       write junit "   classname=""''test_class'"">"
$       write junit -
  "     <failure message=""''fail_msg'"" type=""''fail_type'"" >"
$       write junit "     </failure>"
$       write junit "  </testcase>"
$       return
$!
$junit_report_pass:
$       junit_count = junit_count + 1
$       write junit "  <testcase name=""''test'"""
$       write junit "   classname=""''test_class'"">"
$       write junit "  </testcase>"
$       return
