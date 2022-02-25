#
# Check for FLEX.
#
# This macro verifies that flex is installed.  If successful, then
# 1) $LEX is set to "flex" (to emulate lex calls)
# 2) BISON is set to bison

AC_DEFUN([AC_PROG_FLEX],
[
  AC_PROG_LEX(noyywrap)
  if test "$LEX" != "flex"; then
    AC_MSG_WARN([flex not found. kater cannot be built.])
  else
    AC_SUBST(FLEX,[flex],[location of flex])
  fi
])
