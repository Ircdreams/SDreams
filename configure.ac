#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# $Id: configure.ac,v 1.5 2005/12/23 20:19:14 bugs Exp $

AC_PREREQ(2.59)
AC_INIT(SDreams, 1.1, http://bugs.ircdreams.org)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([bin/convert.c])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypt':
AC_CHECK_LIB([crypt], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h mysql.h ctype.h errno.h signal.h sys/resource.h netinet/in.h errmsg.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([getpass gettimeofday inet_ntoa isascii memset select socket strcasecmp strchr strerror strncasecmp strtol strtoul])

# Defines

#  USEBSD
rev=`uname -s`
if test ${rev} = "FreeBSD" || test ${rev} = "OpenBSD" || test ${rev} = "NetBSD"; then
	AC_DEFINE_UNQUOTED(USEBSD, 1, [System])
fi

#  BINDIR
if test "x${prefix}" = "xNONE"; then
	AC_DEFINE_UNQUOTED(BINDIR, "${ac_default_prefix}", [prefix])
else
	AC_DEFINE_UNQUOTED(BINDIR, "${prefix}", [prefix])
fi

#  DBDIR
AC_ARG_WITH(	dbdir,
		[AC_HELP_STRING([--with-dbdir],[name of database directory in path [database]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a dir name])
			else
				DBDIR=${withval}
			fi
		],
		[DBDIR="database" ]
	)
AC_DEFINE_UNQUOTED(DBDIR, "${DBDIR}", [Database directory's name])

#  DEBUG
AC_ARG_ENABLE(	debug,
		[AC_HELP_STRING([--enable-debug],[enable debug])],
		[
			if test "x${enableval}" != "xno"; then
				DEBUG=1
			fi
		],
		[DEBUG=0 ]
	)
if test "x${DEBUG}" = "x1"; then
	AC_DEFINE_UNQUOTED(DEBUG, 1, [Defined if debug is enabled])
fi

#  WARNINGS
AC_ARG_ENABLE(  warnings,
		[AC_HELP_STRING([--enable-warnings],[enable warnings during compilation])],
		[
			if test "x${enableval}" != "xno"; then
				CFLAGS="$CFLAGS -Wall"
			fi
		],
		)
		
#  LOG_PARSES
AC_ARG_WITH(	parseslog,
		[AC_HELP_STRING([--with-parseslog],[parseslog file's name [logs/parses.log]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				LOG_PARSES=${withval}
			fi
		],
		[LOG_PARSES="logs/parses.log" ]
	)
AC_DEFINE_UNQUOTED(LOG_PARSES, "${LOG_PARSES}", [Parselog file's name])

#  LOG_FAUTH
AC_ARG_WITH(	fauthlog,
		[AC_HELP_STRING([--with-fauthlog],[failed auth log file's name [logs/failed_auth.log]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				LOG_FAUTH=${withval}
			fi
		],
		[LOG_FAUTH="logs/failed_auth.log" ]
	)
AC_DEFINE_UNQUOTED(LOG_FAUTH, "${LOG_FAUTH}", [Failed auth log file's name])

#  LOG_CHANS
AC_ARG_WITH(	chanslog,
		[AC_HELP_STRING([--with-chanslog],[channels commands log file's name [logs/chans.log]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				LOG_CHANS=${withval}
			fi
		],
		[LOG_CHANS="logs/chans.log" ]
	)
AC_DEFINE_UNQUOTED(LOG_CHANS, "${LOG_CHANS}", [Channels commands log file's name])

#  LOG_CMDS
AC_ARG_WITH(	cmdslog,
		[AC_HELP_STRING([--with-cmdslog],[commands log file's name [logs/cmds.log]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				LOG_CMDS=${withval}
			fi
		],
		[LOG_CMDS="logs/cmds.log" ]
	)
AC_DEFINE_UNQUOTED(LOG_CMDS, "${LOG_CMDS}", [Commands log file's name])

#  LOG_ERREURS
AC_ARG_WITH(	errorslog,
		[AC_HELP_STRING([--with-errorslog],[errors log file's name [logs/erreurs.log]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				LOG_ERREURS=${withval}
			fi
		],
		[LOG_ERREURS="logs/erreurs.log" ]
	)
AC_DEFINE_UNQUOTED(LOG_ERREURS, "${LOG_ERREURS}", [Errors log file's name])

#  FICHIER_CONF
AC_ARG_WITH(	confname,
		[AC_HELP_STRING([--with-confname],[configuration file's name [sdreams.conf]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a file name])
			else
				FICHIER_CONF=${withval}
			fi
		],
		[FICHIER_CONF="sdreams.conf" ]
	)
AC_DEFINE_UNQUOTED(FICHIER_CONF, "${FICHIER_CONF}", [Configuration file's name])

#  MEMOLEN
AC_ARG_WITH(    memolen,
                [AC_HELP_STRING([--with-memolen],[maximum length of memos [250]])],
                [
                        if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
                                AC_MSG_ERROR([You have to give a number])
                        else
                                MEMOLEN=${withval}
                        fi
                ],
                [MEMOLEN=512 ]
        )
AC_DEFINE_UNQUOTED(MEMOLEN, ${MEMOLEN}, [Maximum length of memos])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

echo "Resume of configuration :"
echo ""
echo "System : ${rev}"
echo "Installation in : ${prefix}"
echo "Database in : ${DBDIR}"
echo "Configuration file is : ${FICHIER_CONF}"
echo ""
echo "You can run 'make' or 'gmake' now."
