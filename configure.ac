# configure.ac
# Copyright (C) 2002-2005 David Cortier  <Cesar@ircube.org>
#                         Romain Bignon  <Progs@ir3.org>
#                         Benjamin Beret <kouak@kouak.org>
#
# * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
# site web: http://www.ircdreams.org
#
# Services pour serveur IRC. Supporté sur Ircdreams v3
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: configure.ac,v 1.16 2006/01/15 18:43:45 progs Exp $

AC_PREREQ(2.59)
AC_INIT(SDreams, 2.0, http://www.bugsounet.fr)
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([bin/convert.c])
AM_CONFIG_HEADER(config.h)

AC_PREFIX_DEFAULT(~/Dreams/SDreams)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PATH_PROGS(RMPROG, rm, /bin/rm)
AC_PROG_LN_S

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypt':
AC_CHECK_LIB([crypt], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h ctype.h errno.h signal.h sys/resource.h netinet/in.h errmsg.h])

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

# Liens symboliques

dnl --with-symlink lets us set the name of the symlink; defaults to "SDreams"
AC_MSG_CHECKING([what name to give the symlink])
AC_ARG_WITH([symlink],
[AC_HELP_STRING([--with-symlink=name],[Name to give the symlink; if name is "no", no symlink will be created])],
[unet_cv_with_symlink=$with_symlink],
[AC_CACHE_VAL(with_symlink,
[with_symlink="SDreams"])])

if test x"$with_symlink" = xyes; then
    with_symlink="SDreams"
fi

AC_MSG_RESULT([$with_symlink])

if test x"$with_symlink" = xno; then
    INSTALL_RULE=install-no-symlink
    SYMLINK=
else
    INSTALL_RULE=install-with-symlink
    SYMLINK=$with_symlink
fi
AC_SUBST(INSTALL_RULE)
AC_SUBST(SYMLINK)

# Defines

#  PUREIRCU
AC_ARG_ENABLE(	ircu,
		[AC_HELP_STRING([--enable-ircu],[if you use ircu2.10.12])],
		[
			if test "x${enableval}" != "xno"; then
				PUREIRCU=1
			fi
		],
		[PUREIRCU=0 ]
	)

if test "x${PUREIRCU}" = "x1"; then
	AC_DEFINE_UNQUOTED(PUREIRCU, 1, [use of ircu2.10.12])
fi

# OPLEVELS

AC_ARG_ENABLE(  oplevels,
                [AC_HELP_STRING([--enable-oplevels],[if you want oplevels])],
                [
                        if test "x${enableval}" != "xno"; then
                                HAVE_OPLEVELS=1
                        fi
                ],
                [HAVE_OPLEVELS=0 ]
        )

if test "x${HAVE_OPLEVELS}" = "x1"; then
        AC_DEFINE_UNQUOTED(HAVE_OPLEVELS, 1, [use of oplevels])
fi

#  USEBSD
rev=`uname -s`
if test ${rev} = "FreeBSD" || test ${rev} = "OpenBSD" || test ${rev} = "NetBSD"; then
	AC_DEFINE_UNQUOTED(USEBSD, 1, [System])
fi

#  MD5TRANSITION
AC_ARG_ENABLE(	md5trans,
		[AC_HELP_STRING([--enable-md5trans],[to activate if you use SDreams before 11/09/04 and that it is your first update])],
		[
			if test "x${enableval}" != "xno"; then
				MD5TRANSITION=1
			fi
		],
		[MD5TRANSITION=0 ]
	)
if test "x${MD5TRANSITION}" = "x1"; then
	AC_DEFINE_UNQUOTED(MD5TRANSITION, 1, [Defined if there are crypt() pass in database])
fi

#  USE_NICKSERV
AC_ARG_ENABLE(	nickserv,
		[AC_HELP_STRING([--disable-nickserv],[disable nickserv module])],
		[
			if test "x${enableval}" = "xno"; then
				USE_NICKSERV=0
			fi
		],
		[USE_NICKSERV=1 ]
	)
if test "x${USE_NICKSERV}" = "x1"; then
	AC_DEFINE_UNQUOTED(USE_NICKSERV, 1, [Defined if nickserv module is enabled])
fi

#  USE_MEMOSERV
AC_ARG_ENABLE(	memoserv,
		[AC_HELP_STRING([--disable-memoserv],[disable memoserv module])],
		[
			if test "x${enableval}" = "xno"; then
				USE_MEMOSERV=0
			fi
		],
		[USE_MEMOSERV=1 ]
	)
if test "x${USE_MEMOSERV}" = "x1"; then
	AC_DEFINE_UNQUOTED(USE_MEMOSERV, 1, [Defined if memoserv module is enabled])
fi

#  USE_WELCOMESERV
AC_ARG_ENABLE(	welcomeserv,
		[AC_HELP_STRING([--disable-welcomeserv],[disable welcomeserv module])],
		[
			if test "x${enableval}" = "xno"; then
				USE_WELCOMESERV=0
			fi
		],
		[USE_WELCOMESERV=1 ]
	)
if test "x${USE_WELCOMESERV}" = "x1"; then
	AC_DEFINE_UNQUOTED(USE_WELCOMESERV, 1, [Defined if welcomeserv module is enabled])
fi

#  HAVE_VOTE
AC_ARG_ENABLE(	votes,
		[AC_HELP_STRING([--disable-votes],[disable votes module])],
		[
			if test "x${enableval}" = "xno"; then
				HAVE_VOTE=0
			fi
		],
		[HAVE_VOTE=1 ]
	)
if test "x${HAVE_VOTE}" = "x1"; then
	AC_DEFINE_UNQUOTED(HAVE_VOTE, 1, [Defined if votes module is enabled])
fi

#  HAVE_IPV6
AC_ARG_ENABLE(  ipv6,
                [AC_HELP_STRING([--enable-ipv6],[enable ipv6 support])],
                [
                        if test "x${enableval}" = "xyes"; then
                                HAVE_IPV6=1
                        fi
                ],
                [HAVE_IPV6=0 ]
        )
if test "x${HAVE_IPV6}" = "x1"; then
        AC_DEFINE_UNQUOTED(HAVE_IPV6, 1, [Defined if ipv6 support is enabled])
fi


#  SQLLOG
x="0";
AC_ARG_ENABLE(	sqllog,
		[AC_HELP_STRING([--enable-sqllog],[enable sql logs])],
		[
			AM_PATH_MYSQLCLIENT(,[
				AC_DEFINE_UNQUOTED(SLQLLOG,1,"Use sql logs")
				AC_CHECK_HEADER(mysql/mysql.h,AC_DEFINE(MYSQL_HEADER_PREFIX,1,"Has mysql/mysql.h"))
				AC_SUBST(MYSQL)
				x="1";
			],AC_MSG_ERROR([If you don't have mysql you have to disable sql logs !]))

			if test "$x" = "1"; then
				LIBS="$LIBS$MYSQLCLIENT_LIBS "
				CFLAGS="$CFLAGS $MYSQLCLIENT_CFLAGS "
				LDFLAGS="$LDFLAGS $MYSQLCLIENT_LDFLAGS "
				SQLLOG=1
			fi
		],
		[SQLLOG=0 ]
	)
if test "x${SQLLOG}" = "x1"; then
	AC_DEFINE_UNQUOTED(SQLLOG, 1, [Defined if sql logs are enabled])
fi

#  WEB2CS
AC_ARG_ENABLE(	web2cs,
		[AC_HELP_STRING([--enable-web2cs],[enable web2cs module (see website/WEB2CS)])],
		[
			if test "x${enableval}" != "xno"; then
				WEB2CS=1
			fi
		],
		[WEB2CS=0 ]
	)
if test "x${WEB2CS}" = "x1"; then
	AC_DEFINE_UNQUOTED(WEB2CS, 1, [Defined if web2cs module is enabled])
fi

#  HAVE_CRYPTHOST
AC_ARG_ENABLE(	crypthosts,
		[AC_HELP_STRING([--disable-crypthosts],[disable if you don't use Ircdreams v3])],
		[
			if test "x${enableval}" = "xno"; then
				HAVE_CRYPTHOST=0
			fi
		],
		[HAVE_CRYPTHOST=1 ]
	)
if test "x${HAVE_CRYPTHOST}" = "x1"; then
	AC_DEFINE_UNQUOTED(HAVE_CRYPTHOST, 1, [Defined if hostname are crypted])
fi

#  HAVE_TRACK
AC_ARG_ENABLE(	track,
		[AC_HELP_STRING([--disable-track],[disable tracks])],
		[
			if test "x${enableval}" = "xno"; then
				HAVE_TRACK=0
			fi
		],
		[HAVE_TRACK=1 ]
	)
if test "x${HAVE_TRACK}" = "x1"; then
	AC_DEFINE_UNQUOTED(HAVE_TRACK, 1, [Defined if tracks are enabled])
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
AC_ARG_ENABLE(	warnings,
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

#  MAXMEMOS
AC_ARG_WITH(	maxmemos,
		[AC_HELP_STRING([--with-maxmemos],[maximum memo numbers which one can send to the same person [5]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a a number])
			else
				MAXMEMOS=${withval}
			fi
		],
		[MAXMEMOS=5 ]
	)
AC_DEFINE_UNQUOTED(MAXMEMOS, ${MAXMEMOS}, [Maximum memo numbers which one can send to the same person])

#  MEMOLEN
AC_ARG_WITH(	memolen,
		[AC_HELP_STRING([--with-memolen],[maximum length of memos [250]])],
		[
			if test "x$withval" = "xno" || test "x$withval" = "xyes"; then
				AC_MSG_ERROR([You have to give a number])
			else
				MEMOLEN=${withval}
			fi
		],
		[MEMOLEN=250 ]
	)
AC_DEFINE_UNQUOTED(MEMOLEN, ${MEMOLEN}, [Maximum length of memos])

#  ADMINREG
AC_ARG_ENABLE(	adminreg,
		[AC_HELP_STRING([--enable-adminreg],[admins can use REGISTER to record usernames])],
		[
			if test "x${enableval}" != "xno"; then
				ADMINREG=1
			fi
		],
		[ADMINREG=0 ]
	)
if test "x${ADMINREG}" = "x1"; then
	AC_DEFINE_UNQUOTED(ADMINREG, 1, [Defined if admins can use REGISTER to record usernames])
fi

#  HAVE_SECURE
AC_ARG_ENABLE(	secure,
		[AC_HELP_STRING([--disable-secure],[commands which requires private information are typed only in form /<ROBOT>])],
		[
			if test "x${enableval}" = "xno"; then
				HAVE_SECURE=0
			fi
		],
		[HAVE_SECURE=1 ]
	)
if test "x${HAVE_SECURE}" = "x1"; then
	AC_DEFINE_UNQUOTED(HAVE_SECURE, 1, [Defined if ommands which requires private information are typed only in form /<ROBOT>])
fi

#  OLDADMINLIST
AC_ARG_ENABLE(	oldadminlist,
		[AC_HELP_STRING([--enable-oldadminlist],[ADMIN command show all admins (deprecated)])],
		[
			if test "x${enableval}" != "xno"; then
				OLDADMINLIST=1
			fi
		],
		[OLDADMINLIST=0 ]
	)
if test "x${OLDADMINLIST}" = "x1"; then
	AC_DEFINE_UNQUOTED(OLDADMINLIST, 1, [Defined if ADMIN command show all admins])
fi

#  BINDIR
if test "x${prefix}" = "xNONE"; then
	AC_DEFINE_UNQUOTED(BINDIR, "${ac_default_prefix}", [prefix])
else
	AC_DEFINE_UNQUOTED(BINDIR, "${prefix}", [prefix])
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

echo "Resume of configuration :"
echo ""
echo "System : ${rev}"

if test "x${PUREIRCU}" = "x1"; then
	echo "Use of ircu2.10.12"
elif test "x${HAVE_CRYPTHOST}" = "x1"; then
	echo "Use of Ircdreams v3"
else
	echo "Use of ircu2.10.11"
fi

if test "x${HAVE_OPLEVELS}" = "x1"; then
	echo "  (with oplevels support)"
fi

if test "x${HAVE_IPV6}" = "x1"; then
	echo "  (with ipv6 support)"
fi

echo "Modules :"
if test "x${USE_NICKSERV}" = "x1"; then
	echo "	Nickserv"
fi
if test "x${USE_MEMOSERV}" = "x1"; then
	echo "	Memoserv"
fi
if test "x${USE_WELCOMESERV}" = "x1"; then
	echo "	Welcomeserv"
fi
if test "x${HAVE_VOTE}" = "x1"; then
	echo "	Votes"
fi
if test "x${SQLLOG}" = "x1"; then
	echo "	Sql logs"
fi
if test "x${WEB2CS}" = "x1"; then
	echo "	Web2cs"
fi
if test "x${HAVE_TRACK}" = "x1"; then
	echo "	Tracks"
fi

echo "Installation in : ${prefix}"
echo "Database in : ${DBDIR}"
echo "Configuration file is : ${FICHIER_CONF}"
echo ""
echo "You can run 'make' now."
