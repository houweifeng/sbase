havepthread=no
AC_DEFUN([AC_CHECK_EXTRA_OPTIONS],[
	AC_CHECK_LIB(pthread,pthread_create,havepthread=yes)
	AC_MSG_CHECKING(for support pthread)
        AC_ARG_ENABLE(pthread, [  --enable-pthread          compile for supporting pthread])
        if test -z "$enable_pthread" ; then
		enable_pthread="no"
       	elif test "$enable_pthread" = "yes" ; then
		enable_pthread="no"
		if test "$havepthread" = "yes" ; then
			CPPFLAGS="${CPPFLAGS} -DHAVE_PTHREAD"
			LDFLAGS="${LDFLAGS} -lpthread"
			enable_pthread="yes"
		fi
	fi
        AC_MSG_RESULT([$enable_pthread])
        AC_MSG_CHECKING(for debugging)
        AC_ARG_ENABLE(debug, [  --enable-debug          compile for debugging])
        if test -z "$enable_debug" ; then
                enable_debug="no"
        elif test "$enable_debug" = "yes" ; then
                CPPFLAGS="${CPPFLAGS} -g -D_DEBUG"
        fi
        AC_MSG_RESULT([$enable_debug])
])
