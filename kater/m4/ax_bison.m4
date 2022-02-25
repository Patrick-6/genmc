#
# Check for BISON
#
# This macro verifies that Bison is installed.  If successful, then
# 1) YACC is set to bison -y (to emulate YACC calls)
# 2) BISON is set to bison
#

AC_DEFUN([AC_PROG_BISON],
[
  AC_PROG_YACC
  if test "$YACC" != "bison -y"; then
    AC_MSG_WARN([bison not found. kater cannot be built.])
  else
    AC_SUBST(BISON, [bison], [location of bison])
  fi
])
