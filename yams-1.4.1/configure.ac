# Process this file with autoconf to produce a configure script.
#
# To build the system, run: autoreconf [-i]

AC_PREREQ(2.59)
AC_INIT([yams],[1.4.1],[buenos@cs.hut.fi])
AC_CONFIG_SRCDIR([src/yams.c])


AC_SUBST(ke_PRE, 0.0.0)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE


# Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
dnl AC_PROG_LEX
AM_PROG_LEX
AC_PROG_YACC

# Checks for libraries.
# for solaris:
AC_CHECK_FUNC(gethostent, , AC_CHECK_LIB(nsl, gethostent))
AC_CHECK_FUNC(setsockopt, , AC_CHECK_LIB(socket, setsockopt))
AC_CHECK_FUNCS(brk)


# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h libintl.h locale.h malloc.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/param.h sys/select.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_VOLATILE
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_HEADER_TIME
dnl AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([atexit getcwd gethostbyname gettimeofday inet_ntoa memset select socket strcasecmp strdup strerror strtol])


AH_TEMPLATE([READLINE], [Enabled if we have libreadline])
AC_ARG_WITH(readline, AS_HELP_STRING([--with-readline[[[[[=PREFIX]]]]]],
		      [support fancy command input editing [[default=yes]]]))

LDSAVE=$LDFLAGS
CPPSAVE=$CPPFLAGS
yams_have_readline=n
yams_force_readline=n
case $with_readline in
     "no")
	;;
     *)
	if test "x$with_readline" != "x" ; then
	   yams_force_readline=y
	   if test "x$with_readline" != "xyes" ; then
	      # interpret the argument as a prefix
	      INCLUDEDIRS="-I$with_readline/include"
	      LIBDIRS="-L$with_readline/lib"
	      CPPFLAGS="$INCLUDEDIRS $CPPFLAGS"
	      LDFLAGS="$LIBDIRS $LDFLAGS"
	   fi
	fi

	# This readline check was taken from Gnu bc 1.06 and modified
        AC_CHECK_LIB(ncurses,tparm,TERMLIB=-lncurses,
            AC_CHECK_LIB(termcap,tgetent,TERMLIB=-ltermcap))
        LDFLAGS="$LDFLAGS $TERMLIB"
        AC_CHECK_LIB(readline,readline,
            [AC_CHECK_HEADER(readline/readline.h,
                [YAMSLIBS="-lreadline $TERMLIB  $YAMSLIBS";yams_have_readline=y])
	    ], , "$TERMLIB")
	;;
esac
LDFLAGS=$LDSAVE
CPPFLAGS=$CPPSAVE

if test "x$yams_have_readline" = "xn" ; then
   if test "x$yams_force_readline" = "xy" ; then
      AC_MSG_ERROR([cannot find the readline library])
   else
      if test "x$with_readline" != "xno" ; then
	 AC_MSG_NOTICE([No readline library found.])
      fi
   fi
else
   AC_MSG_NOTICE([Using the readline library.])
   AC_DEFINE(READLINE, 1)
fi

AH_TEMPLATE([POLLING_PTHREAD],[Enabled if POSIX threads should be used for I/O])
AH_TEMPLATE([POLLING_FORK],[Enabled if a separate process should be used for I/O])

AC_ARG_WITH(polling, AS_HELP_STRING([--with-polling=METHOD],
[how to poll for asynchronous input [[pthread]]])
AS_HELP_STRING([],['select': use select() from the main thread])
AS_HELP_STRING([],['pthread': use a separate POSIX thread for polling])
AS_HELP_STRING([],['fork': fork a separate process for polling]))

# check for polling method
case $with_polling in
     "fork")
         AC_MSG_NOTICE([Using a separate process for input polling])
         AC_DEFINE(POLLING_FORK, 1)
	 ;;
     "select")
         AC_MSG_NOTICE([Using nonblocking select() calls for input polling])
	 ;;
     "pthread" | "")
         yams_have_pthread=n
	 CSAVE=$CFLAGS
	 LIBSAVE=$LIBS
	 ACX_PTHREAD([yams_have_pthread=y;
	              CC="$PTHREAD_CC";
                      CFLAGS="$PTHREAD_CFLAGS $CFLAGS";
                      LIBS="$PTHREAD_LIBS $LIBS"])
	 AC_CHECK_HEADER([pthread.h], , [yams_have_pthread=n])

         yams_have_sem=n
	 AC_CHECK_HEADER([semaphore.h], [
	     AC_CHECK_FUNC(sem_init, [yams_have_sem=y], 
                 [AC_CHECK_LIB(rt, sem_init, [YAMSLIBS="-lrt $YAMSLIBS";yams_have_sem=y])])
         ])
	 if test "x$yams_have_sem" = "xy" ; then
	     AC_MSG_CHECKING([that sem_init actually works])
	     ok=no
	     # Hopefully this is portable enough:
	     $CC -o semtest $CPPFLAGS $CFLAGS $LDFLAGS $srcdir/semtest.c $YAMSLIBS $LIBS && ./semtest && ok=yes
	     if test "x$ok" = "xno" ; then
		 AC_MSG_RESULT(no)
		 yams_have_sem=n
	     else
		 AC_MSG_RESULT(yes)
	     fi
	 fi
	 CFLAGS=$CSAVE
	 LIBS=$LIBSAVE


	 if test "x$yams_have_sem" = "xy" ; then
	     AC_MSG_NOTICE([found POSIX semaphores])
	 else
             if test "x$with_polling" = "xpthread"; then
                AC_MSG_ERROR([cannot find POSIX semaphores])
             else
                AC_MSG_WARN([cannot find POSIX semaphores])
	     fi
	     yams_have_pthread=n
	 fi

	 if test "x$yams_have_pthread" = "xy" ; then
	     AC_MSG_NOTICE([Using POSIX threads for input polling])
	     AC_DEFINE(POLLING_PTHREAD, 1)
	 else
	     if test "x$with_polling" = "xpthread"; then
		AC_MSG_ERROR([cannot find POSIX threads])
	     else
		AC_MSG_WARN([cannot use POSIX threads])
		AC_MSG_NOTICE([Using nonblocking select() calls for input polling])
	     fi
	 fi
	 ;;
     *)
	 AC_MSG_ERROR([unsupported polling method: "$with_polling"])
	 ;;
esac

AC_SUBST(YAMSLIBS)
AC_SUBST(INCLUDEDIRS)
AC_SUBST(LIBDIRS)

# polling interval
AH_TEMPLATE([ASYNC_INPUT_POLL_INTERVAL],[Overrides the definition in src/async_input.c])
AC_ARG_WITH(polling-interval, AS_HELP_STRING([--with-polling-interval=N],
			      [how often the select polling method should call select(), in simulated clock cycles. [[100]]]))

if test -n "$with_polling_interval"; then
   if test "$with_polling_interval" -ge 1; then
      AC_DEFINE_UNQUOTED(ASYNC_INPUT_POLL_INTERVAL, $with_polling_interval)
   else
      AC_MSG_ERROR([polling interval must be a positive integer])
   fi
fi

# If we are using GNU gcc then add some extra flags
if test "x$GCC" = "xyes" ; then
   AC_SUBST(WARNINGFLAGS, ["-Wall"])
fi


# Generate PRIxnn and _C macros
AC_CHECK_TYPES([long long], [INTFLAGS=-DHAVE_LONG_LONG])
AC_MSG_NOTICE([creating integer formatting macros])
$CC $CFLAGS $INTFLAGS -o makeintformat $srcdir/makeintformat.c
./makeintformat > intformat.h


# Check for endianess
AC_C_BIGENDIAN

AC_CONFIG_FILES([Makefile man/Makefile src/Makefile info/Makefile])
AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])
AC_OUTPUT


# configure.ac ends here
