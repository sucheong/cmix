dnl Authors:  Jens Peter Secher (jpsecher@diku.dk)
dnl           Arne Glenstrup (panic@diku.dk)
dnl           Henning Makholm (makholm@diku.dk)
dnl Content:  C-Mix system: local additions to autoconf
dnl
dnl Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
dnl Redistribution and modification are allowed under certain
dnl terms; see the file COPYING.cmix for details.

dnl DIKU_PATH_STDCPP
dnl ----------------
dnl
dnl try to find a standalone standard-conforming C preprocessor and
dnl put its name in output variable STDCPP
dnl Fall back to the value of AC_PROG_CPP if that fails
dnl
AC_DEFUN(DIKU_PATH_STDCPP, [dnl
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CPP])
AC_CACHE_CHECK([for standalone C preprocessor],
	       diku_cv_path_stdcpp,[dnl
  if test -z "$STDCPP" ; then
    diku_l=
    for diku_p in gcpp cpp.ansi cpp ; do
      for diku_a in bin lbin lib ; do
        for ac_dir in `echo $PATH: |\
	               sed -e s-/bin:-/${diku_a}:-g -e s/:\$// -e "s/:/ /g"` ;
	do
	  test -f $ac_dir/$diku_p && diku_l="$diku_l $ac_dir/$diku_p"
    done; done; done
    test x$GCC = xyes && diku_l="$diku_l "`$CC --print-prog-name=cpp`
    echo >&5 cpp candidates are: $diku_l
    # now we have a number of candidates. Use the first one that
    # matches our expectations:
    cat > conftest.c << EOF
#define grok(x) x ## x
foo/* */bar grok(mix)
#pragma bar
EOF
    for diku_p in $diku_l false ; do
      test -f conftest.i && cat conftest.i >&5
      echo $diku_p conftest.c >&5
      $diku_p conftest.c >conftest.i 2>&5 || continue
      grep foobar conftest.i >/dev/null 2>&5 && continue
      grep mixmix conftest.i >/dev/null 2>&5 || continue
      grep '#pragma bar' conftest.i >/dev/null 2>&5 || continue
      rm conftest.i
      diku_cv_path_stdcpp=$diku_p
      break
    done
    test -f conftest.i && cat conftest.i >&5
    rm conftest.c
    test -z "$diku_cv_path_stdcpp" && diku_cv_path_stdcpp="$CPP"
  else
    diku_cv_path_stdcpp="$STDCPP"
  fi
])
STDCPP=$diku_cv_path_stdcpp
AC_SUBST(STDCPP)
])

dnl DIKU_FUNC_PRINTF_BROKEN_LONGDOUBLE
dnl ----------------------------------
dnl
dnl define PRINTF_BROKEN_LG unless the *printf family seems
dnl to work reliably with long double values.

AC_DEFUN(DIKU_FUNC_PRINTF_BROKEN_LG, [dnl
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_CACHE_CHECK([whether printing of long doubles is broken],
		diku_cv_func_printf_broken_lg,[dnl
	AC_TRY_RUN([#include<stdio.h>
		#include<string.h>
		int main(void) {
			char buffer[30];
			sprintf(buffer,"%.10LG",1.234L);
			return !!strncmp(buffer,"1.23",4);
		}],
	  diku_cv_func_printf_broken_lg=no,
	  diku_cv_func_printf_broken_lg=yes,
	  diku_cv_func_printf_broken_lg=yes)
])
if test $diku_cv_func_printf_broken_lg = yes
then
  AC_DEFINE(PRINTF_BROKEN_LG)
fi
AC_LANG_RESTORE
])

dnl DIKU_FUNC_GETHOSTNAME
dnl ---------------------
dnl
dnl If unistd.h does not declare gethostname() but the header
dnl sys/systeminfo exists, defined the symbol
dnl GETHOSTNAME_FROM_SYSTEMINFO

AC_DEFUN(DIKU_FUNC_GETHOSTNAME, [dnl
AC_LANG_SAVE
AC_LANG_C
AC_CACHE_CHECK([for gethostname in <unistd.h>],diku_cv_func_gethostname,[dnl
	AC_EGREP_CPP(gethostname,[#include<unistd.h>],
		diku_cv_func_gethostname=yes,
		diku_cv_func_gethostname=no)
])
if test $diku_cv_func_gethostname = no
then
  AC_CHECK_HEADER(sys/systeminfo.h,AC_DEFINE(GETHOSTNAME_FROM_SYSTEMINFO))
fi
AC_LANG_RESTORE
])

dnl DIKU_FUNC_ISNAN
dnl ---------------------
dnl
dnl Define HAVE_ISNAN if math.h declares a isnan function or macro

AC_DEFUN(DIKU_FUNC_ISNAN, [dnl
AC_LANG_SAVE
AC_LANG_C
AC_CACHE_CHECK([for isnan in <math.h>],diku_cv_func_isnan,[dnl
	AC_TRY_COMPILE([#include <math.h>],[if(isnan(0.0));],
		diku_cv_func_isnan=yes,
		diku_cv_func_isnan=no)
])
if test $diku_cv_func_isnan = yes
then
  AC_DEFINE(HAVE_ISNAN)
fi
AC_LANG_RESTORE
])

dnl DIKU_VAR_END
dnl ------------
dnl
dnl If the *edata and *etext address variables are linked in,
dnl define the symbol ETEXT_LINKED_IN

AC_DEFUN(DIKU_VAR_END, [dnl
  AC_CACHE_CHECK([whether *etext etc. is known by the linker],
    diku_cv_var_end,[dnl
    AC_TRY_LINK(,[extern void *edata, *etext; return edata - etext;],
    diku_cv_var_end=yes, diku_cv_var_end=no)])
if test $diku_cv_var_end = yes
then
  AC_DEFINE(ETEXT_LINKED_IN)
fi])

dnl DIKU_ECHO
dnl ------------
dnl
dnl If Berkeley echo, set $ECHON to "echo -n" and $ECHOC to "",
dnl else              set $ECHON to "echo"    and $ECHOC to "'\c'"

AC_DEFUN(DIKU_ECHON, [dnl
  AC_MSG_CHECKING([whether echo is Berkeley style])
  if test `echo -n x` = '-n x'
  then
    ECHON=echo
    ECHOC="'\c'"
    AC_MSG_RESULT(no)
  else
    ECHON="echo -n"
    ECHOC=
    AC_MSG_RESULT(yes)
  fi
AC_SUBST(ECHON)
AC_SUBST(ECHOC)
])

dnl DIKU_PROG_INDENT
dnl ------------
dnl
dnl Find an appropriate indent program: prefer `gindent', then `indent'.
dnl If neither exist, choose `cat'
dnl Set output variable INDENT to the program that has been found.
dnl Set indent_takes_switches to `yes' if $INDENT takes switches,
dnl otherwise (if $INDENT = cat) set it to `no'

AC_DEFUN(DIKU_PROG_INDENT, [dnl
  AC_CHECK_PROGS(INDENT, gindent indent cat)
  case "$INDENT"
  in
	*indent) indent_takes_switches=yes ;;
	*) indent_takes_switches=no ;;
  esac
  AC_SUBST(INDENT)
])

dnl DIKU_CHECK_FILE_PATHS(FILE, PATHS [, ACTION-IF-FOUND [,
dnl          ACTION-IF-NOT-FOUND]])
dnl --------------------
dnl
dnl Check for a FILE in the specified PATHS.
dnl Default ACTON-IF-FOUND: set diku_cv_path_FILE to the path found,
dnl where . / + - in FILE has been changed to _ _ p _

AC_DEFUN(DIKU_CHECK_FILE_PATHS, [dnl
  diku_file_var=`echo $1 | sed 'y%./+-%__p_%'`
  AC_MSG_CHECKING([for $1 in specific locations])
  AC_CACHE_VAL([diku_cv_path_$diku_file_var],
    eval "diku_cv_path_$diku_file_var="
    for diku_specificpath in $2
    do
      eval diku_realpath=$diku_specificpath
      if test -f $diku_realpath/$1
      then
	eval "diku_cv_path_$diku_file_var='$diku_specificpath'"
	break
      fi
    done)
  if test x`eval echo '$'{diku_cv_path_$diku_file_var}` = x
  then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT([`eval echo '$'{diku_cv_path_$diku_file_var}/$1`])
  fi
  ifelse([$3], , ,
[if test x`eval echo '$'{diku_cv_path_$diku_file_var}` != x
  then
  $3
fi
])
  ifelse([$4], , ,
[if test x`eval echo '$'{diku_cv_path_$diku_file_var}` = x
  then
  $4
fi
])
])

dnl AC_EXEEXT in autoconf 2.13 is buggy.
ifelse(AC_ACVERSION,2.13,undefine([AC_EXEEXT]))

ifdef([AC_EXEEXT],[],[
dnl AC_EXEEXT
dnl ------------
dnl
dnl If the compiler generates foo.exe when instructed to generate foo,
dnl then set the output variable `exe' to `.exe'
dnl This is a quick hack that works differently from the macro in
dnl autoconf 2.13, so we use a private cache variable
AC_DEFUN(AC_EXEEXT, [dnl
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
  AC_CACHE_CHECK([whether the C++ compiler (${CXX}) generates .exe files],
    cmix_cv_exeext,[dnl
cat > conftest.$ac_ext <<EOF
dnl This sometimes fails to find confdefs.h, for some reason.
dnl [#]line __oline__ "[$]0"
[#]line __oline__ "configure"
#include "confdefs.h"
int main() { return 0; }
EOF
cmix_cv_exeext=no
if eval $ac_link; then
  if test -f conftest.exe
  then
    cmix_cv_exeext=yes
  fi
fi
rm -f conftest*]
)
if test $cmix_cv_exeext = yes
then
  EXEEXT='.exe'
else
  EXEEXT=
fi
AC_SUBST(EXEEXT)
AC_LANG_RESTORE
])
])

dnl In versions of autoconf prior to 2.13 AC_PROG_INSTALL did not
dnl set INSTALL_SCRIPT. Amend the macro so it is always set
define([DIKU_PREV_PROG_INSTALL],defn([AC_PROG_INSTALL]))
define([AC_PROG_INSTALL],[DIKU_PREV_PROG_INSTALL
test x${INSTALL_SCRIPT} != x || INSTALL_SCRIPT='$(INSTALL_PROGRAM)'
AC_SUBST(INSTALL_SCRIPT)
])

dnl DIKU_STDSYMS
dnl ------------
dnl
dnl Define any system-specific preprocessor symbols that might
dnl be necessary to make system headers work. Sigh, HP-SUX
AC_DEFUN(DIKU_ADDSTDSYM, [dnl
  diku_cv_stdsyms="$diku_cv_stdsyms $1"
  AC_DEFINE($1)])
AC_DEFUN(DIKU_STDSYMS, [dnl
AC_MSG_CHECKING([for necessary magic symbols])
  diku_cv_stdsyms=
  if test -f /usr/include/sys/stdsyms.h
  then
    if grep _HPUX_SOURCE /usr/include/sys/stdsyms.h > /dev/null
    then
      DIKU_ADDSTDSYM(_HPUX_SOURCE)
    fi
  fi
  AC_MSG_RESULT($diku_cv_stdsyms)
])

dnl AC_FUNC_ALLOCA in autoconf 2.12 and 2.13 is buggy.
ifelse(AC_ACVERSION,2.13,undefine([AC_FUNC_ALLOCA]))
ifelse(AC_ACVERSION,2.12,undefine([AC_FUNC_ALLOCA]))

ifdef([AC_FUNC_ALLOCA],[],[
AC_DEFUN(AC_FUNC_ALLOCA,
[AC_REQUIRE_CPP()dnl Set CPP; we run AC_EGREP_CPP conditionally.
# The Ultrix 4.2 mips builtin alloca declared by alloca.h only works
# for constant arguments.  Useless!
AC_CACHE_CHECK([for working alloca.h], ac_cv_header_alloca_h,
[AC_TRY_LINK([#include <alloca.h>], [char *p =(char*)alloca(2 * sizeof(int));],
  ac_cv_header_alloca_h=yes, ac_cv_header_alloca_h=no)])
if test $ac_cv_header_alloca_h = yes; then
  AC_DEFINE(HAVE_ALLOCA_H)
fi
AC_CACHE_CHECK([for alloca], ac_cv_func_alloca_works,
[AC_TRY_LINK([
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# ifdef _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# else
#  if HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   ifdef _AIX
 #pragma alloca
#   else
#    ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#    endif
#   endif
#  endif
# endif
#endif
], [char *p = (char *) alloca(1);],
  ac_cv_func_alloca_works=yes, ac_cv_func_alloca_works=no)])
if test $ac_cv_func_alloca_works = yes; then
  AC_DEFINE(HAVE_ALLOCA)
fi
if test $ac_cv_func_alloca_works = no; then
  # The SVR3 libPW and SVR4 libucb both contain incompatible functions
  # that cause trouble.  Some versions do not even contain alloca or
  # contain a buggy version.  If you still want to use their alloca,
  # use ar to extract alloca.o from them instead of compiling alloca.c.
  ALLOCA=alloca.${ac_objext}
  AC_DEFINE(C_ALLOCA)
AC_CACHE_CHECK(whether alloca needs Cray hooks, ac_cv_os_cray,
[AC_EGREP_CPP(webecray,
[#if defined(CRAY) && ! defined(CRAY2)
webecray
#else
wenotbecray
#endif
], ac_cv_os_cray=yes, ac_cv_os_cray=no)])
if test $ac_cv_os_cray = yes; then
for ac_func in _getb67 GETB67 getb67; do
  AC_CHECK_FUNC($ac_func, [AC_DEFINE_UNQUOTED(CRAY_STACKSEG_END, $ac_func)
  break])
done
fi
AC_CACHE_CHECK(stack direction for C alloca, ac_cv_c_stack_direction,
[AC_TRY_RUN([find_stack_direction ()
{
  static char *addr = 0;
  auto char dummy;
  if (addr == 0)
    {
      addr = &dummy;
      return find_stack_direction ();
    }
  else
    return (&dummy > addr) ? 1 : -1;
}
main ()
{
  exit (find_stack_direction() < 0);
}], ac_cv_c_stack_direction=1, ac_cv_c_stack_direction=-1,
  ac_cv_c_stack_direction=0)])
AC_DEFINE_UNQUOTED(STACK_DIRECTION, $ac_cv_c_stack_direction)
fi
AC_SUBST(ALLOCA)dnl
])
])



