dnl  
dnl  $Revision: 1.1 $  $Date: 2004/02/24 20:46:57 $
dnl  
dnl  
dnl
dnl  AC_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION]) 
dnl  
dnl  Version 1.3 (2001/03/02) 
dnl  Author Guido Draheim <guidod@gmx.de>, original by Alexandre Oliva 
dnl  
dnl  Description
dnl  This macro _AC_DEFINEs VARNAME to the expansion of the DIR variable, 
dnl  taking care of fixing up ${prefix} and such. 
dnl  
dnl  Note that the 3 argument form is only supported with autoconf 2.13 
dnl  and later (i.e. only where _AC_DEFINE supports 3 arguments). 
dnl  
dnl  Examples: 
dnl      Aa_DEFINE_DIR(NATADIR, datadir)
dnl      AC_DEFINE_DIR(PROG_PATH, bindir, [Location of installed binaries])
dnl  
AC_DEFUN([AC_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_define_dir=`eval echo [$]$2`
  ac_define_dir=`eval echo [$]ac_define_dir`
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir"),
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir", $3))
])


