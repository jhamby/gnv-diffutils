$! File: Build_project_release_notes.com
$!
$! Build the release note file from the three components:
$!    1. The 'product'_release_note_start.txt
$!    2. readme. file from the 'product' distribution.
$!    3. The 'product'_build_steps.txt.
$!
$! Set the name of the release notes from the GNV_PCSI_FILENAME_BASE
$! logical name.
$!
$! Copyright 2011, John Malmberg
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
$! 15-Mar-2011  J. Malmberg
$!
$!===========================================================================
$!
$ base_file = f$trnlnm("GNV_PCSI_FILENAME_BASE")
$ if base_file .eqs. ""
$ then
$   write sys$output "@MAKE_PCSI_PROJECT_KIT_NAME.COM has not been run."
$   goto all_exit
$ endif
$ kit_name = f$trnlnm("GNV_PCSI_KITNAME")
$ if kit_name .eqs. ""
$ then
$   write sys$output "@MAKE_PCSI_PROJECT_KIT_NAME.COM has not been run."
$   goto all_exit
$ endif
$ product = f$element(2, "-", kit_name)
$!
$ product_readme = f$search("sys$disk:[]readme.")
$ if product_readme .eqs. ""
$ then
$   product_readme = f$search("sys$disk:[]$README.")
$ endif
$ if product_readme .eqs. ""
$ then
$   write sys$output "Can not find ''product' readme file."
$   goto all_exit
$ endif
$!
$ product_copying = f$search("sys$disk:[]copying.")
$ if product_copying .eqs. ""
$ then
$   product_copying = f$search("sys$disk:[]$COPYING.")
$ endif
$ if product_copying .eqs. ""
$ then
$   write sys$output "Can not find product copying file."
$   goto all_exit
$ endif
$!
$ type/noheader sys$disk:[.vms]'product'_release_note_start.txt,-
        'product_readme',-
        'product_copying', -
        sys$disk:[.vms]'product'_build_steps.txt -
        /out='base_file'.release_notes
$!
$ purge 'base_file'.release_notes
$ rename 'base_file.release_notes ;1
$!
$all_exit:
$   exit
