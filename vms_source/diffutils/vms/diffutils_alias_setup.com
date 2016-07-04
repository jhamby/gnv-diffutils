$! File: diffutils_alias_setup.com
$!
$! The PCSI procedure needs a helper script to set up and remove aliases.
$!
$! If p1 starts with "R" then remove instead of install.
$!
$! Copyright 2016, John Malmberg
$!
$! Permission to use, copy, modify, and/or distribute this software for any
$! purpose with or without fee is hereby granted, provided that the above
$! copyright notice and this permission notice appear in all copies.
$!
$! THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
$! WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
$! MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
$! ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
$! WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
$! ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
$! OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
$!
$!
$! 03-Jul-2016  J. Malmberg
$!
$!===========================================================================
$!
$ mode = "install"
$ code = f$extract(0, 1, p1)
$ if code .eqs. "R" .or. code .eqs. "r" then mode = "remove"
$!
$ arch_type = f$getsyi("ARCH_NAME")
$ arch_code = f$extract(0, 1, arch_type)
$!
$ if arch_code .nes. "V"
$ then
$   set proc/parse=extended
$ endif
$!
$ usr_bin = "cmp,diff,diff3,sdiff"
$!
$! list = bin_files
$! prefix = "[bin]"
$! gosub aliases_list
$!
$ list = usr_bin
$ prefix = "[usr.bin]"
$ gosub aliases_list
$!
$! list = usr_sbin
$! prefix = "[usr.sbin]"
$! gosub aliases_list
$!
$ exit
$!
$aliases_list:
$ i = 0
$alias_list_loop:
$   name = f$element(i, ",", list)
$   if name .eqs. "" then goto alias_list_loop_end
$   if name .eqs. "," then goto alias_list_loop_end
$   call do_alias "''name'" "''prefix'" "''name'"
$   i = i + 1
$   goto alias_list_loop
$alias_list_loop_end:
$ return
$!
$!
$do_alias: subroutine
$ if mode .eqs. "install"
$ then
$   call add_alias "''p1'" "''p2'" "''p3'"
$ else
$   call remove_alias "''p1'" "''p2'" "''p3'"
$ endif
$ exit
$ENDSUBROUTINE ! do_alias
$!
$!
$! P1 is the filename, p2 is the directory prefix
$add_alias: subroutine
$ file = "gnv$gnu:''p2'gnv$''p1'.EXE"
$ alias = "gnv$gnu:''p2'''p1'."
$ if f$search(file) .nes. ""
$ then
$   if f$search(alias) .eqs. ""
$   then
$       set file/enter='alias' 'file'
$   endif
$   alias1 = alias + "exe"
$   if f$search(alias1) .eqs. ""
$   then
$       set file/enter='alias1' 'file'
$   endif
$ endif
$ exit
$ENDSUBROUTINE ! add_alias
$!
$remove_alias: subroutine
$ file = "gnv$gnu:''p2'gnv''p1'.EXE"
$ file_fid = "No_file_fid"
$ alias = "gnv$gnu:''p2'''p1'."
$ if f$search(file) .nes. ""
$ then
$   fid = f$file_attributes(file, "FID")
$   if f$search(alias) .nes. ""
$   then
$       afid = f$file_attributes(alias, "FID")
$       if (afid .eqs. fid)
$       then
$           set file/remove 'alias';
$       endif
$   endif
$   alias1 = alias + "exe"
$   if f$search(alias1) .nes. ""
$   then
$       afid = f$file_attributes(alias1, "FID")
$       if (afid .eqs. fid)
$       then
$           set file/remove 'alias1';
$       endif
$   endif
$ endif
$ exit
$ENDSUBROUTINE ! remove_alias
