$! VMS_BUILD.COM
$!
$! This is the master script to build this package.
$! If the GNV_PCSI_* and STAGE_ROOT: logical names are set up
$! then a PCSI kit will be built.
$!
@[.vms]vms_prebuild.com
$!
