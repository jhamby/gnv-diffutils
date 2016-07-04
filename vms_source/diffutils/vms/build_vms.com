$! Build_vms.com
$!
$! Prodcedure to build diffutils on VMS.
$!
$! 26-Jun-2016	J. Malmberg
$!
$!-----------------------------------------------------------------------
$!
$ product_name = "diffutils"
$!
$ start_time = f$cvtime()
$!
$! Pre setup, including coping all files into the beginning of a search list.
$!
$ @[.vms]vms_prebuild.com
$!
$! Save the old default
$!
$ old_default = f$environment("default")
$!
$ if f$search("[.lib]config.h") .eqs. ""
$ then
$   bash ./vms_configure.sh
$ endif
$!
$ configure_time = f$time()
$!
$ file = "sys$disk:[]conftest.err"
$ if f$search(file) .nes. "" then delete 'file';*
$ file = "sys$disk:[]conftest.lis"
$ if f$search(file) .nes. "" then delete 'file';*
$ file = "sys$disk:[]conftest.dsf"
$ if f$search(file) .nes. "" then delete 'file';*
$!
$ bash vms/vms_make.sh
$!
$! Restore the old default
$!
$ write sys$output "Removing previously staged files"
$ @[.vms]stage_'product_name'_install.com remove
$ write sys$Output "Staging files to new_gnu:[...]"
$ @[.vms]stage_'product_name'_install.com
$!
$!
$ gnv_pcsi_prod = f$trnlnm("GNV_PCSI_PRODUCER")
$ gnv_pcsi_prod_fn = f$trnlnm("GNV_PCSI_PRODUCER_FULL_NAME")
$ stage_root = f$trnlnm("STAGE_ROOT")
$ if (gnv_pcsi_prod .eqs. "") .or. -
    (gnv_pcsi_prod_fn .eqs. "") .or. -
    (stage_root .eqs. "")
$ then
$   if gnv_pcsi_prod .eqs. ""
$   then
$       msg = "GNV_PCSI_PRODUCER not defined, can not build a PCSI kit."
$       write sys$output msg
$   endif
$   if gnv_pcsi_prod_fn .eqs. ""
$   then
$     msg = "GNV_PCSI_PRODUCER_FULL_NAME not defined, can not build a PCSI kit."
$       write sys$output msg
$   endif
$   if stage_root .eqs. ""
$   then
$       write sys$output "STAGE_ROOT not defined, no place to put kits"
$   endif
$   exit
$ endif
$!
$!
$ @[.vms]pcsi_product_'product_name'.com
$!
$!
$ set default 'old_default'
$!
$ end_time = f$cvtime()
$!
$ write sys$output "Start time = ''start_time'"
$ write sys$output "Configure time = ''configure_time'"
$ write sys$output "End time = ''end_time'"
