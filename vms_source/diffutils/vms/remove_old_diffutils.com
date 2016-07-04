$! File: remove_old_diffutils.com
$!
$! This is a procedure to remove the old diffutils images that were installed
$! by the GNV kits and replace them with links to the new image.
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
$! 03-Jul-2016  J. Malmberg
$!
$!==========================================================================
$!
$vax = f$getsyi("HW_MODEL") .lt. 1024
$old_parse = ""
$if .not. VAX
$then
$   old_parse = f$getjpi("", "parse_style_perm")
$   set process/parse=extended
$endif
$!
$old_cutils = "diff"
$!
$!
$ i = 0
$cutils_loop:
$   file = f$element(i, ",", old_cutils)
$   if file .eqs. "" then goto cutils_loop_end
$   if file .eqs. "," then goto cutils_loop_end
$   call update_old_image 'file'
$   i = i + 1
$   goto cutils_loop
$cutils_loop_end:
$!
$!
$if .not. VAX
$then
$   set process/parse='old_parse'
$endif
$!
$!
$all_exit:
$  exit
$!
$! Remove old image or update it if needed.
$!-------------------------------------------
$update_old_image: subroutine
$!
$ file = p1
$! First get the FID of the new product image.
$! Don't remove anything that matches it.
$ new_product = f$search("GNV$GNU:[BIN]GNV$''file'.EXE")
$!
$ new_product_fid = "No_new_product_fid"
$ if new_product .nes. ""
$ then
$   new_product_fid = f$file_attributes(new_product, "FID")
$ endif
$!
$!
$!
$! Now get check the "''file'." and "''file'.exe"
$! May be links or copies.
$! Ok to delete and replace.
$!
$!
$ old_product_fid = "No_old_product_fid"
$ old_product = f$search("gnv$gnu:[bin]''file'.")
$ old_product_exe_fid = "No_old_product_fid"
$ old_product_exe = f$search("gnv$gnu:[bin]''file'.exe")
$ if old_product_exe .nes. ""
$ then
$   old_product_exe_fid = f$file_attributes(old_product_exe, "FID")
$ endif
$!
$ if old_product .nes. ""
$ then
$   fid = f$file_attributes(old_product, "FID")
$   if fid .nes. new_product_fid
$   then
$       if fid .eqs. old_product_exe_fid
$       then
$           set file/remove 'old_product'
$       else
$           delete 'old_product'
$       endif
$       if new_product .nes. ""
$       then
$           set file/enter='old_product' 'new_product'
$       endif
$   endif
$ endif
$!
$ if old_product_exe .nes. ""
$ then
$   if old_product_fid .nes. new_product_fid
$   then
$       delete 'old_product_exe'
$       if new_product .nes. ""
$       then
$           set file/enter='old_product_exe' 'new_product'
$       endif
$   endif
$ endif
$!
$ exit
$ENDSUBROUTINE ! Update old image
