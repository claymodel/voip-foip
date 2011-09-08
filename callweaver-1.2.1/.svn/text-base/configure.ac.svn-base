# Main Autoconf configuration file for the CallWeaver.org project
# Copyright <c> 2005, 2006, 2007 by CallWeaver.org
#
# For a list of Authors check the AUTHORS file in the same directory
# as this file
#
# Work on the automake stuff was partly sponsored by Halo Kwadrat Sp. z o.o.
# User definable profiles for directory layouts by Sunrise Telephone Systems Ltd.
#
# This program is free software and it comes WITHOUT any warranty.
# You may redistribute it and/or modify it under the terms of the
# GNU General Public License (GPL) version 2 as published by the
# Free Software Foundation, but not any later version of the GPL.
# 
# Process this file with autoconf to produce a configure script.

######
#
# Initialisation of the GNU AutoTools Packages
#
######


AC_PREREQ(2.59)
AC_INIT(CallWeaver,1.2.1,callweaver-dev@callweaver.org)
AM_INIT_AUTOMAKE(CallWeaver,1.2.1)

dnl define GNU Source to make sure every compiler and tool 
dnl  knows that we use GNU compliant macros etc
AC_GNU_SOURCE
AC_LANG_C
AC_C_CONST

######
#
# Define and check for prerequisite programs
#
######

AC_PATH_PROG([GROUPADD], [groupadd])
AC_PATH_PROGS(GREP, ggrep grep,[echo Missing grep so skipping but I doubt we will get anywhere])
AC_PATH_PROGS(SVN, svn, [echo Missing subversion so some stuff will be borked],${PATH}:/opt/csw/bin)
AC_PATH_PROGS(SHELL,bash sh,[echo No compatible shell found])
AC_PATH_PROGS(SH,bash sh,[echo No compatible shell found])
AC_PATH_PROGS(M4,gm4 m4,[echo No m4 found, who will process my macros now ?])
AC_PATH_PROGS(SED,gsed sed,[echo sed not found, doh!])
AC_PATH_PROGS(CAT,cat,[echo cat not found, Doh!])
AC_PATH_PROGS(TR,tr,[echo tr not found, need this for upper/lowercase conversions])
AC_PATH_PROGS(UNAME,uname,[echo uname not found so no version info will be available])
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AM_PROG_LEX
AX_SYS_PERLSHARPBANG
AC_PROG_INSTALL
AC_FUNC_STRERROR_R


dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Libtool definitions
dnl ###
dnl ###########################################################################
dnl ###########################################################################

dnl check for dlopen support (for shared modules)
AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
dnl yes we will use libtool to create cygwin shared modules
AC_LIBTOOL_WIN32_DLL
dnl Disable static libraries by default. Can be enabled with --enable-static
AC_DISABLE_STATIC
AC_PROG_LIBTOOL   
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### OS Dependent Stuff
dnl ###
dnl ###########################################################################
dnl ###########################################################################

dnl Shamelessly stolen from editline configure.in to create the relevant cygwin stuff
dnl Platform-specific settings.  
case "${host}" in
  *-*-darwin*)
        AC_DEFINE([__Darwin__],,[Define if Darwin])
        AC_SUBST(__Darwin__)
	no_libcap=yes
	ostype=bsd
        ;;
  *-*-freebsd*)
	no_libcap=yes
	ostype=bsd
        ;;
  *-*-netbsd*)
	no_libcap=yes
	ostype=bsd
	;;
  *-*-openbsd*)
	no_libcap=yes
	ostype=bsd
	;;
  *-*-dragonfly*)
	no_libcap=yes
	ostype=bsd
	;;
  *-aix*)
    AC_DEFINE(AIX,,[Define if AIX])
     broken_types=yes
	no_libcap=yes
	ostype=aix
    ;;
  *-osf4*)
    AC_DEFINE(OSF1,,[Define if OSF1])
    tru64_types=yes
	no_libcap=yes
	ostype=osf
    ;;
  *-osf5.1*)
    AC_DEFINE(OSF1)
	no_libcap=yes
	ostype=osf
    ;;
  *-tru64*)
    AC_DEFINE(OSF1)
	tru64_types=yes
	no_libcap=yes
	ostype=osf
    ;;
  *-*-linux*)
	ostype=linux
	LARGEFILE_FLAGS="-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
	CFLAGS="$CFLAGS $LARGEFILE_FLAGS"
        ;;
  *cygwin*)
        AC_DEFINE([_WIN32_WINNT],[0x0500],[Heya, it's windows])
        AC_DEFINE([INET_ADDRSTRLEN],[16],[cygwin detection does not work for that, anybody has an idea ?])
        AC_DEFINE([INET6_ADDRSTRLEN],[46],[cygwin detection does not work for that, anybody has an idea ?])
	AC_DEFINE([__CYGWIN__],[1],[make sure __CYGWIN__ is defined ...])
	ostype=cygwin
        ;;
  *-*-solaris2*)
        AC_DEFINE([SOLARIS],[1],[needed for optional declarations to be visible])
	no_libcap=yes
	ostype=solaris
	force_generic_timers=yes
        ;;
  *)
        AC_MSG_RESULT(Unsupported operating system: ${host})
	no_libcap=yes
	ostype=unknown
        ;;
esac  

######
#
# Build paths and variables
#
######     

dnl Create the relevant Build variables

if test -n $BUILD_OS ; then
	AC_DEFINE_UNQUOTED([BUILD_DATE],"`date -u "+%Y-%m-%d %H:%M:%S"` UTC",[The date of this build])
	AC_DEFINE_UNQUOTED([BUILD_OS],"`${UNAME} -s`",[Operating System we are building on])
	AC_DEFINE_UNQUOTED([BUILD_MACHINE],"`${UNAME} -m`",[Machine we are building on])
	AC_DEFINE_UNQUOTED([BUILD_HOSTNAME],"`${UNAME} -n`",[Hostname of our Box])
	AC_DEFINE_UNQUOTED([BUILD_KERNEL],"`${UNAME} -r`",[Kernel version of this build])
	AC_SUBST([BUILD_DATE])
	AC_SUBST([BUILD_OS])
	AC_SUBST([BUILD_MACHINE])
	AC_SUBST([BUILD_HOSTNAME])
	AC_SUBST([BUILD_KERNEL])
fi

dnl check for host compiler when cross compiling
if test "$host" = "$build"; then
	HOST_CC="${CC}"
else
	HOST_CC="${HOST_CC-gcc}"
fi
AC_CHECK_PROG(have_host_cc, ${HOST_CC}, yes, no)
if test "$have_host_cc" = "no"; then
	AC_MSG_ERROR(No valid host compiler set with HOST_CC)
fi
AC_SUBST(HOST_CC)

dnl ##########################
dnl #
dnl # Installation directories
dnl #
dnl ##########################

# ==========================================================================
# Directory layouts are defined in separate layout profiles:
# --------------------------------------------------------------------------
#  profile        filename              description
# --------------------------------------------------------------------------
#  freebsd        layout.freebsd        follow convention used by FreeBSD ports system
#  freebsd-romfs  layout.freebsd-romfs  FreeBSD embedded devices with ROM filesystems
#  openbsd        layout.openbsd        follow convention used by OpenBSD ports system
#  fhs            layout.fhs            follow Filesystem Hierarchy Standard (FHS)
#  lsb            layout.lsb            follow Linux Standards Base (LSB) standard
#  lsb-romfs      layout.lsb-romfs      Linux embedded devices with ROM filesystems
#  singledir      built-in              install everything under a single directory
# --------------------------------------------------------------------------
#
# The default directory layout is OS dependent:
# --------------------------------------------------------------------------
#  host OS      default profile
# --------------------------------------------------------------------------
#  FreeBSD      freebsd
#  OpenBSD      openbsd
#  Linux        lsb
#  Solaris      singledir, using prefix /opt/csw/callweaver
# --------------------------------------------------------------------------
#
# Directory layout profiles contain a list of key=value pairs, *without* any
# spaces or tabs, one pair per line. The following keys are defined:
#  cwprefix, cwexecdir, cwutilsdir, cwconfdir, cwlibdir,
#  cwvardir, cwlogdir, cwrundir, cwspooldir, cwdatadir,
#  cwmandir and  cwdocdir
#
# Values are the full pathnames for the respective directories *without* any
# quotation marks. Any lines with a leading # character and lines containing
# pairs with invalid keys are ignored.
# ==========================================================================

# ----------------------------------------------------------
# Function isavail_directory_layout()
# ----------------------------------------------------------
# synopsis:
#   test whether a profile is available for a given layout
#
# input parameter:
#   $1 - name of the layout to be checked
#
# return values:
#   0 - no profile is available for the layout
#   1 - profile is available for the layout
# ----------------------------------------------------------
isavail_directory_layout() {
# return 0 if no parameter is passed to this function
   if test -z "$1"; then
        return 0;
   fi
# test parameter against each profile name found
   profilelist=`ls -1 ${srcdir}/layout.*`
   for file in $profilelist;
        do
             if test "${srcdir}/layout.$1" = "$file"; then
		return 1;
	     fi
        done
# parameter not found in the list
   return 0;
}
# END isavail_directory_layout

# ----------------------------------------------------------
# List of valid install directory variables separated by LF
# ----------------------------------------------------------
cwdirs=`${CAT} << EOF
cwprefix
cwexecdir
cwutilsdir
cwconfdir
cwlibdir
cwvardir
cwlogdir
cwrundir
cwspooldir
cwdatadir
cwmandir
cwdocdir
cwmoddir
cwogidir
EOF`
# END list

# ----------------------------------------------------------
# Function is_cw_dir()
# ----------------------------------------------------------
# synopsis:
#   test whether a given name is a valid install directory
#   variable as defined in the variable cwdirs
#
# input parameter:
#   $1 - name to be checked
#
# return values:
#   0 - input name is not an install directory variable
#   1 - input name is an install directory variable
# ----------------------------------------------------------
is_cw_dir() {
# return 0 if no parameter is passed to this function
   if test -z "$1"; then
        return 0;
   fi
# test parameter against each variable name in cwdirs
   for var in $cwdirs;
   do
        if test $1 = $var; then
             return 1;
        fi
   done
# parameter not found in cwdirs
   return 0;
}
# END is_cw_dirs

# ----------------------------------------------------------
# Function load_directory_profile()
# ----------------------------------------------------------
# synopsis:
#   load a directory profile from disk and assign install
#   directory variables from values defined in the profile
#
# input parameter:
#   $1 - name of directory profile to load
#
# return values:
#   0 - error, profile could not be found or read
#   1 - profile successfully loaded
# ----------------------------------------------------------
load_directory_profile() {
# return 0 if no parameter is passed to this function
   if test -z "$1"; then
        return 0;
   fi
#
# for future use - extract the help string from the profile
#   helpstr=`${CAT} ${srcdir}/layout.$1 | ${GREP} "^helpstring=*" | ${SED} -e 's/^helpstring=//'`
#   helpstr=`echo $helpstr | ${SED} -e 's/^"//' | ${SED} -e 's/"$//'`
#
# read all directory variables from the profile
#   profile=`${CAT} ${srcdir}/layout.$1 | ${SED} -e 's/#.*//' -e 's/\s*=\s*/=/' -e 's/\\\\ /|/g' -e 's/^\s*//' -e 's/\s.*//' | ${GREP} "^cw.*="`
   profile=`${CAT} ${srcdir}/layout.$1 | ${SED} -e 's/#.*//' -e 's/[[:blank:]]*=[[:blank:]]*/=/' -e 's/\\\\ /|/g' -e 's/^[[:blank:]]*//' -e 's/[[:blank:]].*//' | ${GREP} "^cw.*="`

   for line in $profile;
   do
        key=`echo "$line" | ${SED} -e 's/=.*$//'`;
        value=`echo "$line" | ${SED} -e 's/^[[a-z]]*=[['\''"]]*//' -e 's/[['\''"]]$//' -e 's/|/ /g'`;
        is_cw_dir ${key}
        if test $? -ne 1; then
             echo "ignoring '${key}' in ${srcdir}/layout.$1 because it is not a valid install directory variable" >&2;
        else
             eval ${key}="'${value}'"
        fi
   done
   return 1;
}
# END load_directory_profile

# define commandline parameter --with-directory-layout
AC_ARG_WITH([directory-layout],
	[AC_HELP_STRING([--with-directory-layout],[Specify an installation directory layout, either a pre-defined or a user-defined profile. Pre-defined profiles are fhs, freebsd, freebsd-romfs, lsb, lsb-romfs, openbsd and singledir. The default is OS dependent.])],,[with_directory_layout="automatic"])

# use OS specific default if no layout was specified
if test "${with_directory_layout}" = "automatic"; then
	AC_MSG_NOTICE([Directory layout will be set automatically...])
	case "${host_os}" in
	     freebsd*)
		# use FreeBSD profile in file layout.freebsd
		with_directory_layout="freebsd"
		;;
	     openbsd*)
		# use OpenBSD profile in file layout.openbsd
		with_directory_layout="openbsd"
		;;
	     linux*)
		# use LSB profile in file layout.lsb
		with_directory_layout="lsb"
		;;
	     solaris*)
		# use built-in SINGLEDIR profile
		with_directory_layout="singledir"
		;;
	     *)
		# use built-in SINGLEDIR profile
		with_directory_layout="singledir"
		;;
	esac
fi

# built-in SINGLEDIR profile ...
if test "${with_directory_layout}" = "singledir"; then
    AC_MSG_NOTICE([Selected directory layout is SINGLEDIR])
    # override default prefix
    if test "${ostype}" = "solaris"; then
	ac_default_prefix='/opt/csw/callweaver'
    else
	ac_default_prefix='/opt/callweaver'
    fi
    # set directory variables
    cwexecdir='${prefix}/sbin'
    cwutilsdir='${prefix}/bin'
    cwconfdir='${prefix}/etc'
    cwlibdir='${prefix}/lib'
    cwvardir='${prefix}/var'
    cwlogdir='${prefix}/var/log'
    cwrundir='${prefix}/var/run'
    cwspooldir='${prefix}/var/spool'
    cwdatadir='${prefix}/var/data'
    cwmandir='${prefix}/share/man'
    cwdocdir='${prefix}/doc'
else

# all other profiles ...
# check availability of selected layout, abort if not found
isavail_directory_layout ${with_directory_layout}
    if test $? -ne 1; then
	AC_MSG_ERROR([Unknown directory layout '${with_directory_layout}'])
    fi

# set install dir variables from selected profile
    layout_upper=`echo ${with_directory_layout} | ${TR} '[a-z]' '[A-Z]'`
    AC_MSG_NOTICE([Selected directory layout is ${layout_upper}])
    load_directory_profile ${with_directory_layout}
    if test -n "${cwprefix}"; then
	ac_default_prefix=${cwprefix}
    fi
fi

# directory for the main executable
AC_SUBST([cwexecdir])

# directory for commandline utilities
AC_SUBST([cwutilsdir])

# directory for configuration files
AC_SUBST([cwconfdir])

# path to master configuration file
if test -z "${cwconffile}"; then
    cwconffile='${cwconfdir}/callweaver.conf'
fi
AC_SUBST([cwconffile])

# directory for libraries
AC_SUBST([cwlibdir])

# directory for modules
if test -z "${cwmoddir}"; then
	cwmoddir='${cwlibdir}/modules'
fi
AC_SUBST([cwmoddir])

# directory for ICD modules
if test -z "${cwicdmoddir}"; then
	cwicdmoddir='${cwmoddir}/icd'
fi
AC_SUBST([cwicdmoddir])

# directory for mutable data
AC_SUBST([cwvardir])

# directory for database file of internal db
if test -z "${cwdbdir}"; then
	cwdbdir='${cwvardir}'
fi
AC_SUBST([cwdbdir])

# path to database file of internal db
if test -z "${cwdbfile}"; then
	cwdbfile='callweaverdb'
fi
AC_SUBST([cwdbfile])

# directory for temporary files
if test -z "${cwtmpdir}"; then
	cwtmpdir='${cwvardir}/tmp'
fi
AC_SUBST([cwtmpdir])

# directory for pid file and console socket
AC_SUBST([cwrundir])

# path to pid file
if test -z "${cwpidfile}"; then
	cwpidfile='${cwrundir}/callweaver.pid'
fi
AC_SUBST([cwpidfile])

# path to console socket
if test -z "${cwsocketfile}"; then
	cwsocketfile='${cwrundir}/callweaver.ctl'
fi
AC_SUBST([cwsocketfile])

# directory for log files
AC_SUBST([cwlogdir])

# directory for spool files
AC_SUBST([cwspooldir])

# directory for immutable data
AC_SUBST([cwdatadir])

# directory for man pages
AC_SUBST([cwmandir])

# directory for license, credits and read-me files
AC_SUBST([cwdocdir])

# directory for crypto keys
if test -z "${cwkeydir}"; then
	cwkeydir='${cwdatadir}/keys'
fi
AC_SUBST([cwkeydir])

# directory for sqlite database file
if test -z "${cwsqlitedir}"; then
	cwsqlitedir='${cwdbdir}'
fi
AC_SUBST([cwsqlitedir])

# directory for OGI scripts
if test -z "${cwogidir}"; then
	cwogidir='${cwdatadir}/ogi'
fi
AC_SUBST([cwogidir])

# directory for sound files
if test -z "${cwsoundsdir}"; then
	cwsoundsdir='${cwdatadir}/sounds'
fi
AC_SUBST([cwsoundsdir])

# directory for images
if test -z "${cwimagesdir}"; then
	cwimagesdir='${cwdatadir}/images'
fi
AC_SUBST([cwimagesdir])

# directory for on-hold music files
if test -z "${cwmohdir}"; then
	cwmohdir='${cwdatadir}/moh'
fi
AC_SUBST([cwmohdir])

# include directory
if test -z "${cwincludedir}"; then
	cwincludedir='${includedir}/callweaver'
fi
AC_SUBST([cwincludedir])

######
#
# Check for headers, structures, typedefs, libs etc which are not related to specific add-on packages
#
######
AC_ARG_ENABLE([builtin-sqlite3],
        [AC_HELP_STRING([--enable-builtin-sqlite3],[Enables the builtin thread-safe copy of SQLite library (default=yes)])],[builtin_sqlite3="$enableval"],[builtin_sqlite3="yes"])


dnl check for required libraries (also for add-ons)
dnl needed to move check for sqlite threadsafe here since it needs to check if we have a threadsafe sqlite3 BEFORE we check for libpthread)
if test "${builtin_sqlite3}" = "no"; then
	AC_CHECK_SQLITE3_THREADSAFE
fi
AC_CHECK_LIB([c], [main])
AC_CHECK_LIB([db], [main])
dnl # AC_CHECK_LIB([expat], [main]) # why would we need this one ?
AC_CHECK_LIB([ib], [main])
AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([ncurses], [main],,[AC_CHECK_LIB([curses],[main])])
AC_CHECK_LIB([readline],[main],,[AC_MSG_ERROR(readline is required to compile CallWeaver.)])
AC_CHECK_LIB([newt], [main],[AC_DEFINE([HAVE_LIBNEWT],["yes"],[yes there is newt])])
AM_CONDITIONAL([USE_NEWT],[test -n "${HAVE_LIBNEWT}"])
AC_CHECK_LIB([nsl], [main])
AC_CHECK_LIB([ossaudio], [main])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([resolv], [main])
AC_CHECK_LIB([rt], [main])
AC_CHECK_LIB([socket], [main])
AC_CHECK_LIB([t], [main])
# AC_CHECK_LIB([termcap], [main])


dnl checks for library functions
AC_FUNC_ALLOCA
AC_CHECK_FUNC(usleep, [TARGET_CFLAGS="$TARGET_CFLAGS -DHAVE_USLEEP=1"])
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
CW_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRCOLL
AC_FUNC_STRFTIME
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit bzero dup2 endpwent floor ftruncate getcwd gethostbyname gethostname gettimeofday clock_getres inet_ntoa isascii localtime_r memchr memmove memset mkdir munmap pow putenv re_comp regcomp rint select setenv socket sqrt strsep strcasecmp strchr strcspn strdup strerror strncasecmp strndup strrchr strspn strstr strtol strtoq unsetenv utime vasprintf]) 
AC_CHECK_FUNCS([daemon])

# Check if asctime_r() takes three arguments.
AC_MSG_CHECKING([if asctime_r() takes three arguments])
AC_TRY_COMPILE([
#include <stdio.h>
#include <time.h>
], [
	struct tm tm;
	char buf [r256];
	
	asctime_r (&tm, buf, sizeof (buf));
], [
	AC_DEFINE(ASCTIME_R_WITH_THREE_ARGS)
        AC_DEFINE([ASCTIME_R_WITH_THREE_ARGS],[],[asctime_r needs 3 arguments])
	AC_MSG_RESULT(yes)
], [
	AC_MSG_RESULT(no)
])

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([netinet/in.h])
dnl This does not work currently .. some bug in cygwin autoconf
dnl AC_CHECK_HEADERS([w32api/windows.h])
dnl AC_CHECK_HEADERS([w32api/winsock2.h],[],[],
dnl [#if HAVE_W32API_WINDOWS_H
dnl # include <w32api/windows.h>
dnl # endif
dnl ])
dnl AC_CHECK_HEADERS([w32api/ws2tcpip.h],[],[],
dnl [#if HAVE_W32API_WINSOCK2_H
dnl # include <w32api/windows.h>
dnl # include <w32api/winsock2.h>
dnl # endif
dnl ])
AC_CHECK_HEADER([dlfcn.h],[AM_CONDITIONAL([NEED_DLFCN_H],[true = yes])])
AC_CHECK_HEADERS([readline/readline.h readline/history.h],,[AC_MSG_ERROR(readline is required to compile CallWeaver.)])
AC_CHECK_HEADERS([glob.h])

dnl check structures
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_CHECK_MEMBERS([struct stat.st_blksize])  

dnl check compiler specifics
AC_C_INLINE
AC_C_CONST
AC_C_VOLATILE

dnl check types
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T  
AC_CHECK_TYPES([ptrdiff_t])  

dnl check declarations
AC_CHECK_DECLS(INET_ADDRSTRLEN,[],[],[#if HAVE_NETINET_IN_H
# include <netinet/in.h>
# endif
#if HAVE_W32API_WS32TCPIP_H
# include w32api/windows.h
# include w32api/winsock2.h  
# include w32api/ws2tcpip.h
#endif
])
AC_CHECK_DECLS(INET6_ADDRSTRLEN,[],[],[#if HAVE_NETINET_IN_H
# include <netinet/in.h>
# endif
#if HAVE_W32API_WS32TCPIP_H
# include w32api/windows.h
# include w32api/winsock2.h
# include w32api/ws2tcpip.h
#endif
])   
AC_CHECK_DECLS(__BEGIN_DECLS,[],AC_DEFINE(__BEGIN_DECLS,,[for stupid ast-db1]))
AC_CHECK_DECLS(__END_DECLS,[],AC_DEFINE(__END_DECLS,,[for stupid ast-db1])) 
# more type checks, horrible shit
if test "$tru64_types" = "yes"; then
        AC_CHECK_TYPE(u_int8_t, unsigned char)
        AC_CHECK_TYPE(u_int16_t, unsigned short)
        AC_CHECK_TYPE(u_int32_t, unsigned int)
else
        if test "$broken_types" = "yes" ; then
                AC_CHECK_TYPE(u_int8_t, unsigned char)
                AC_CHECK_TYPE(u_int16_t, unsigned short)
                AC_CHECK_TYPE(u_int32_t, unsigned long int)
        else
                AC_CHECK_TYPE(u_int8_t, uint8_t)
                AC_CHECK_TYPE(u_int16_t, uint16_t)
                AC_CHECK_TYPE(u_int32_t, uint32_t)
        fi
fi

AC_C_BIGENDIAN(AC_DEFINE([__BYTE_ORDER],__BIG_ENDIAN,[Big Endian]),AC_DEFINE([__BYTE_ORDER],__LITTLE_ENDIAN,[Little Endian]))
AC_DEFINE([__LITTLE_ENDIAN],1234,[for the places where it is not defined])
AC_DEFINE([__BIG_ENDIAN],4321,[for the places where it is not defined])

dnl ###########################################################################

if test "${builtin_sqlite3}" = "yes"; then
	SQLITE3_THREADSAFE_LIBS='${top_builddir}/sqlite3-embedded/libcwsqlite3.la -lpthread'
	SQLITE3_THREADSAFE_CFLAGS='-DTEMP_STORE=2 -DTHREADSAFE=1 -DSQLITE_OMIT_CURSOR -DNO_TCL -I${top_srcdir}/sqlite3-embedded -I${top_builddir}/sqlite3-embedded'
	case "${host}" in
	  *cygwin*)
	  	SQLITE3_THREADSAFE_CFLAGS="${SQLITE3_THREADSAFE_CFLAGS} -DOS_WIN"
		;;
	  *)
	  	SQLITE3_THREADSAFE_CFLAGS="${SQLITE3_THREADSAFE_CFLAGS} -DOS_UNIX"
		;;
	esac
	AC_SUBST([SQLITE3_THREADSAFE_LIBS])
	AC_SUBST([SQLITE3_THREADSAFE_CFLAGS])
	SQLITE3_SUBDIR=sqlite3-embedded
fi

AC_SUBST([SQLITE3_SUBDIR])

dnl ###########################################################################

AC_ARG_WITH([tiff-includes],
	[AC_HELP_STRING([--with-tiff-includes],[Location of libtiff include files.])],
	[CFLAGS="$CFLAGS -I${withval}"])

AC_ARG_WITH([tiff-libs],
	[AC_HELP_STRING([--with-tiff-libs],[Location of libtiff library files.])],
	[LDFLAGS="$LDFLAGS -L${withval}"])

AC_CHECK_HEADERS([tiffio.h],[HAVE_TIFFIO_H=1],[AC_MSG_ERROR(libtiff is required to compile CallWeaver.)])
AC_CHECK_LIB([tiff],[TIFFGetVersion],[HAVE_LIBTIFF=1],[AC_MSG_ERROR(libtiff is required to compile CallWeaver.)])

dnl ###########################################################################

if test "${force_generic_timers}" != "yes"; then
AC_CHECK_LIB([rt], [timer_create],[AC_DEFINE([HAVE_POSIX_TIMERS],[1],[yes there are real time libraries])],[AC_DEFINE([USE_GENERIC_TIMERS],[1],[forcing generic timers.])])
else
AC_DEFINE([USE_GENERIC_TIMERS],[1],[forcing generic timers according to the architecture.])
fi
dnl ###########################################################################

if test "${no_libcap}" != "yes"; then
AC_CHECK_HEADERS([sys/capability.h],[HAVE_PCAP_H=1],[AC_MSG_ERROR(libcap-dev is required to compile CallWeaver.)])
fi
dnl ###########################################################################

AC_CHECK_HEADERS([memcache.h],[memchached_headers="yes"])

if test "${memchached_headers}" = "yes"; then
	AC_CHECK_LIB([memcache], [mcMemSetup],[memchached_library="yes"])
fi

if test "${memchached_library}" = "yes"; then
	AC_DEFINE([HAVE_MEMCACHE],[1],[yes there are memchache headers and libraries])
fi
AM_CONDITIONAL([WANT_MEMCACHE],[test "${memchached_library}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([spandsp-includes],
	[AC_HELP_STRING([--with-spandsp-includes],[Location of SpanDSP include files.])],
	[CFLAGS="$CFLAGS -I${withval}"])

AC_ARG_WITH([spandsp-libs],
	[AC_HELP_STRING([--with-spandsp-libs],[Location of SpanDSP library files.])],
	[LDFLAGS="$LDFLAGS -L${withval}"])

AC_CHECK_HEADERS([spandsp.h],[HAVE_SPANDSP_H=1],[AC_MSG_ERROR(SpanDSP version 0.0.6pre12 or newer is required to compile CallWeaver.)])

AC_CHECK_HEADERS([spandsp/version.h],[HAVE_SPANDSP_VERSION_H=1],[AC_MSG_ERROR(SpanDSP does not appear to be new enough. You must have version 0.0.6pre12 or newer to compile CallWeaver.)],
[#ifdef HAVE_SPANDSP_H
#include <spandsp.h>
#endif
#include <spandsp/version.h>

#if SPANDSP_RELEASE_DATE < 20090502

#error
#endif
])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Optional stuff
dnl ###
dnl ###########################################################################
dnl ###########################################################################

dnl Optimize for host cpu version
AC_ARG_ENABLE(optimization,     
	[AC_HELP_STRING([--disable-optimization],[no detection or tuning flags for cpu version])],
	[OPTIMIZECPU="$enableval"],[OPTIMIZECPU="yes"])

dnl Enable SIP call limit. This is broken and should be fixed before release.
AC_ARG_ENABLE(sip-call-limit,
	[AC_HELP_STRING([--enable-sip-call-limit],[Enable broken SIP call limit (default=no)])],,[enable_sip_call_limit="no"])

if test "${enable_sip_call_limit}" = "yes"; then
   AC_DEFINE([ENABLE_SIP_CALL_LIMIT],[],[Enable broken SIP call limit])
fi
AM_CONDITIONAL([ENABLE_SIP_CALL_LIMIT],[test "${enable_sip_call_limit}" = "yes"])

dnl Enable SIP TCP/TLS support.
AC_ARG_ENABLE(sip-tcp-support,
	[AC_HELP_STRING([--enable-sip-tcp-support],[Enable SIP TCP/TLS support (default=no)])],,[enable_sip_tcp_support="no"])

if test "${enable_sip_tcp_support}" = "yes"; then
   AC_DEFINE([SIP_TCP_SUPPORT],[],[Enable SIP TCP/TLS support])
fi
AM_CONDITIONAL([WANT_SIP_TCP_SUPPORT],[test "${enable_sip_tcp_support}" = "yes"])

dnl Enable Cisco-style RTP payload type 2 handling for G.726 . This is a RFC violation for broken peers. Silence Bug should be fixed.
AC_ARG_ENABLE(rtp-payload-2,
	[AC_HELP_STRING([--enable-rtp-payload-2],[Enable Cisco-style RTP payload type 2 (default=no)])],,[enable_rtp_payload_2="no"])

if test "${enable_rtp_payload_2}" = "yes"; then
   AC_DEFINE([USE_DEPRECATED_G726],[],[Enable Cisco-style RTP payload type 2])
fi
AM_CONDITIONAL([USE_DEPRECATED_G726],[test "${enable_rtp_payload_2}" = "yes"])

dnl Enable debugging
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],[build with debug information])],,[enable_debug="no"])

if test "${enable_debug}" = "yes"; then
   AC_DEFINE([DEBUG],[],[Enable extra debugging.])
fi
AM_CONDITIONAL([WANT_DEBUG],[test "${enable_debug}" = "yes"])

dnl Enable debugging
AC_ARG_ENABLE(debug-scheduler,
	[AC_HELP_STRING([--enable-debug-scheduler],[build with scheduler debug information])],,[enable_debug_scheduler="no"])

if test "${enable_debug_scheduler}" = "yes"; then
   AC_DEFINE([DEBUG_SCHEDULER],[],[Enable scheduler debugging.])
fi

dnl Enable debugging
AC_ARG_ENABLE(stack-backtraces,
	[AC_HELP_STRING([--enable-stack-backtraces],[build with stack backtraces])],,[enable_stack_backtraces="no"])

if test "${enable_stack_backtraces}" = "yes"; then
   AC_DEFINE([STACK_BACKTRACES],[],[Enable generation of stack backtraces.])
fi

dnl
AC_ARG_ENABLE(do-crash,
	[AC_HELP_STRING([--enable-do-crash],[enable DO_CRASH])],,[enable_do_crash="no"])

if test "${enable_do_crash}" = "yes"; then
   AC_DEFINE([DO_CRASH],[],[Enable DO_CRASH.])
fi

dnl Enable frame tracing
AC_ARG_ENABLE(frame-tracing,
        [AC_HELP_STRING([--enable-frame-tracing],[Build with frame tracing.])],,[enable_frame_tracing="no"])

if test "${enable_frame_tracing}" = "yes"; then
   AC_DEFINE([TRACE_FRAMES],[],[Build with frame tracing.])
fi

dnl Enable valgrind shit
AC_ARG_ENABLE(valgrind,
        [AC_HELP_STRING([--enable-valgrind],[Build with valgrind information])],,[enable_valgrind="no"])

if test "${enable_valgrind}" = "yes"; then
   AC_DEFINE([VALGRIND],[],[Build with valgrind information.])
fi

dnl Enable profiling
AC_ARG_ENABLE(profiling,
	[AC_HELP_STRING([--enable-profiling],[build with profiling support])],,[enable_profiling="no"])

if test "${enable_profiling}" = "yes"; then
   AC_DEFINE([PROFILING],[],[Build with profiling support.])
fi

dnl Low memory support
AC_ARG_ENABLE(low_memory,
	[AC_HELP_STRING([--enable-low_memory],[enable low_memory support])],,[enable_low_memory="no"])

if test "${enable_low_memory}" = "yes"; then
   AC_DEFINE([LOW_MEMORY],[],[Low memory support.])
fi

dnl set user and group
cwrunuser="callweaver"
cwrungroup="callweaver"

AC_ARG_WITH(callweaver-user,
	[AC_HELP_STRING([--with-callweaver-user=USER],[Run as USER (default=callweaver)])],,[with_callweaver_user="callweaver"])

if test "${with_callweaver_user}" != "no" -a "${with_callweaver_user}" != "yes"; then
	cwrunuser="$with_callweaver_user"
fi

AC_ARG_WITH(callweaver-group,
	[AC_HELP_STRING([--with-callweaver-group=GROUP],[Run as GROUP (default=callweaver)])],,[with_callweaver_group="callweaver"])

if test "${with_callweaver_group}" != "no" -a "${with_callweaver_group}" != "yes"; then
	cwrungroup="$with_callweaver_group"
fi
AC_SUBST(cwrunuser)
AC_SUBST(cwrungroup)  

dnl ###########################################################################

AC_ARG_ENABLE([jabber],
	[AC_HELP_STRING([--enable-jabber],[Enable jabber support. (default=no)])],,[enable_jabber="no"])

if test "${enable_jabber}" = "yes"; then
   PKG_CHECK_MODULES([LOUDMOUTH],
	[loudmouth-1.0 >= 1.0.0],
	[HAVE_LOUDMOUTH=1],
	[AC_MSG_WARN([loudmouth library not found ... jabber related features (res_jabber, app_icd) will not be built])])
fi

dnl ###########################################################################

AC_ARG_ENABLE(javascript,
        [AC_HELP_STRING([--enable-javascript],[Enable the use of javascript (default=no)])],,
	[enable_javascript="no"])

if test "${enable_javascript}" = "yes"; then
   AC_CHECK_LIB([js],[main],[HAVE_LIBJS=1])

   AC_CHECK_HEADERS([jstypes.h],[HAVE_JSSCRIPT_H=1],,
[#define XP_UNIX
])
   AC_CHECK_HEADERS([js/jstypes.h],[HAVE_JS_JSSCRIPT_H=1],,
[#define XP_UNIX
])

   if test -n "$HAVE_LIBJS" -a \( -n "${HAVE_JSSCRIPT_H}" -o -n "${HAVE_JS_JSSCRIPT_H}" \); then
      JS_CFLAGS="-DXP_UNIX -DSVR4 -DSYSV -D_BSD_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R -DX86_LINUX  -DDEBUG -DDEBUG_root -DJS_THREADSAFE -D_GNU_SOURCE"
      if test -n "${HAVE_JS_JSSCRIPT_H}"; then
	 JS_CFLAGS="${JSCFLAGS} -I/usr/include/js"
      fi
      JS_LDFLAGS="-ljs"
      AC_SUBST(JS_CFLAGS)
      AC_SUBST(JS_LDFLAGS)
   fi

   AC_PATH_PROG([NSPRCONFIG], [nspr-config])

   if test -n "${NSPRCONFIG}"; then
      HAVE_NSPR=1
      NSPR_CFLAGS=`${NSPRCONFIG} --cflags`
      NSPR_LDFLAGS=`${NSPRCONFIG} --libs`
      AC_SUBST(NSPR_CFLAGS)
      AC_SUBST(NSPR_LDFLAGS)
   else
      PKG_CHECK_MODULES([NSPR],
	[mozilla-nspr],
	[HAVE_NSPR=1])
      AC_SUBST(NSPR_CFLAGS)
      AC_SUBST(NSPR_LDFLAGS)
   fi
fi

dnl ###########################################################################

#AC_ARG_ENABLE([odbc],
#	[AC_HELP_STRING([--enable-odbc],[Enable support for ODBC (default=no)])],,[enable_odbc="no"])

#if test "${enable_odbc}" = "yes"; then
#   AC_CHECK_LIB([odbc], [main], [HAVE_LIBODBC=1])

#   if test -n "${HAVE_LIBODBC}"; then
#      ODBC_CFLAGS=""
#      ODBC_LIBS="-lodbc"
#   fi
#fi

#AC_SUBST([ODBC_CFLAGS])
#AC_SUBST([ODBC_LIBS])

dnl ###########################################################################

AC_ARG_ENABLE([postgresql],
	[AC_HELP_STRING([--enable-postgresql],[Enable support for PostgreSQL. (default=no)])],,[enable_postgresql="no"])

if test "${enable_postgresql}" = "yes"; then
   if test "${with_pgsql_inc+set}" != set; then
      if test "${with_pgsql_lib+set}" != set; then
	 if test "${pg_prefix+set}" != set; then
	    with_pgsql_inc="/usr/include/postgresql/"
	    with_pgsql_lib="/usr/lib/"
	 fi
      fi
   fi
   CT_CHECK_POSTGRES_DB
fi

dnl ###########################################################################

AC_ARG_ENABLE([mysql],
	[AC_HELP_STRING([--enable-mysql],[Enable support for MySQL. (default=no)])],,[enable_mysql="no"])

if test "${enable_mysql}" = "yes"; then
   AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no, $PATH)
   if test "$MYSQL_CONFIG" = "no"; then
      AC_MSG_ERROR([You have specified --enable-mysql, but MySQL is not installed.])
   else
      MYSQL_LIBS=`$MYSQL_CONFIG --libs`
      MYSQL_CFLAGS=`$MYSQL_CONFIG --include`

      AC_SUBST([MYSQL_LIBS])
      AC_SUBST([MYSQL_CFLAGS])
   fi
fi

dnl ###########################################################################

AC_ARG_ENABLE([srtp],
	[AC_HELP_STRING([--enable-srtp],[Enable support for SRTP.])],,[enable_srtp="no"])

if test "${enable_srtp}" = "yes"; then
   AC_CHECK_HEADER([srtp/srtp.h],,[enable_srtp="no"])
   AC_CHECK_LIB([srtp],[srtp_init],,[enable_srtp="no"])
fi

if test "${enable_srtp}" = "yes"; then
   AC_DEFINE([ENABLE_SRTP],,[Enable support for SRTP.])
fi
AM_CONDITIONAL([WANT_SRTP],[test "${enable_srtp}" = "yes"])

dnl ##########################################################################

AC_ARG_WITH([ssl],
       [AC_HELP_STRING([--with-ssl=PATH],[Location of OpenSSL installation])],,[withval=""])

CHECK_SSL


dnl ##########################################################################

AC_ARG_ENABLE([udpfromto],
       [AC_HELP_STRING([--enable-udpfromto],[Enable extended UDP support for multi-homed systems (default=no)])],,
       [enable_udpfromto="no"])
 
if test "${enable_udpfromto}" = "yes" ; then
   #
   # IP_PKTINFO (Linux)
   #
   AC_CHECK_DECL([IP_PKTINFO],[HAVE_IP_PKTINFO=1],,
      [#ifdef HAVE_NETINET_IN_H
       #include <netinet/in.h>
       #endif
      ])
   if test "${HAVE_IP_PKTINFO}" = "yes" ; then
      AC_DEFINE([HAVE_IP_PKTINFO],,[Needed for recv-/sendfromto on Linux])
   fi
 
   #
   # IP_RECVDSTADDR (*BSD)
   #
   AC_CHECK_DECL([IP_RECVDSTADDR],[HAVE_IP_RECVDSTADDR=1],,
      [#ifdef HAVE_NETINET_IN_H
       #include <netinet/in.h>
       #endif
      ])
   #
   # IP_SENDSRCADDR (*BSD)
   #
   AC_CHECK_DECL([IP_SENDSRCADDR],[HAVE_IP_SENDSRCADDR=1],,
      [#ifdef HAVE_NETINET_IN_H
       #include <netinet/in.h>
       #endif
      ])
   if test -n "${HAVE_IP_RECVDSTADDR}" -a -n "${HAVE_IP_SENDSRCADDR}"; then
      AC_DEFINE([HAVE_IP_RECVDSTADDR],,[Needed for recv-/sendfromto on *BSD])
      AC_DEFINE([HAVE_IP_SENDSRCADDR],,[Needed for recv-/sendfromto on *BSD])
   fi
fi     

dnl ##########################################################################

AC_ARG_ENABLE([visdn],
	[AC_HELP_STRING([--enable-visdn],[Enable support for vISDN (default=no)])],,[enable_visdn="no"])

if test "${enable_visdn}" = "yes"; then
    AC_CHECK_HEADERS([linux/visdn/router.h], [HAVE_LINUX_VISDN_ROUTER_H=1])
    AC_CHECK_LIB([q931], [main], [HAVE_LIBQ931=1])
fi

dnl ##########################################################################

AC_ARG_ENABLE([zaptel],
	[AC_HELP_STRING([--enable-zaptel],[Enable support for Zaptel (default=yes)])],,[enable_zaptel="yes"])

if test "${enable_zaptel}" = "yes"; then
   AC_CHECK_HEADERS([zaptel/zaptel.h], [ZAPTEL_H="\"zaptel/zaptel.h\""])
   if test -z "${ZAPTEL_H}"; then
      AC_CHECK_HEADERS([zaptel.h], [ZAPTEL_H="\"zaptel.h\""])
   fi
   if test -z "${ZAPTEL_H}"; then
      AC_CHECK_HEADERS([linux/zaptel.h], [ZAPTEL_H="\"linux/zaptel.h\""])
   fi

   AC_CHECK_HEADERS([zaptel/tonezone.h], [TONEZONE_H="\"zaptel/tonezone.h\""])
   if test -z "${TONEZONE_H}"; then
      AC_CHECK_HEADERS([tonezone.h], [TONEZONE_H="\"tonezone.h\""])
   fi

   AC_DEFINE_UNQUOTED([ZAPTEL_H], [${ZAPTEL_H}], [Location of zaptel.h header file])
   AC_DEFINE_UNQUOTED([TONEZONE_H], [${TONEZONE_H}], [Location of tonezone.h header file])

   if test -n "${ZAPTEL_H}"; then
     AC_CHECK_TYPES(struct zt_radio_stat,[ZAPTEL_V12_OR_HIGHER=1],,[#include ${ZAPTEL_H}])

     if test "${ZAPTEL_V12_OR_HIGHER}" = "1"; then
       AC_MSG_NOTICE([Zaptel appears to be version 1.2 or higher -- ok.])
       AC_CHECK_LIB([tonezone], [main], [HAVE_LIBTONEZONE=1])
       AC_CHECK_LIB([pri], [main], [HAVE_LIBPRI=1])
     else
       AC_MSG_WARN([Zaptel appears to be older than version 1.2 -- Zaptel support will be disabled.])
       enable_zaptel="no"
     fi
   else
     AC_MSG_WARN([Zaptel could not be found  -- Zaptel support will be disabled.])
     enable_zaptel="no"
   fi
fi

AM_CONDITIONAL([HAVE_LIBPRI], [test -n "${HAVE_LIBPRI}"])

dnl ###########################################################################

dnl ###########################################################################
dnl ###
dnl ### Apps
dnl ###
dnl ###########################################################################
dnl ###########################################################################

dnl ###########################################################################

AC_ARG_WITH([app_adsiprog],
	[AC_HELP_STRING([--with-app_adsiprog],[Build and install app_adsiprog (default=yes)])],,
	[with_app_adsiprog="yes"])

AM_CONDITIONAL([WANT_APP_ADSIPROG],[test "${with_app_adsiprog}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_amd],
        [AC_HELP_STRING([--with-app_amd],[Build and install app_amd (default=yes)])],,
        [with_app_amd="yes"])

AM_CONDITIONAL([WANT_APP_AMD],[test "${with_app_amd}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_authenticate],
	[AC_HELP_STRING([--with-app_authenticate],[Build and install app_authenticate (default=yes)])],,
	[with_app_authenticate="yes"])

AM_CONDITIONAL([WANT_APP_AUTHENTICATE],[test "${with_app_authenticate}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_backgrounddetect],
	[AC_HELP_STRING([--with-app_backgrounddetect],[Build and install app_backgrounddetect (default=yes)])],,
	[with_app_backgrounddetect="yes"])

AM_CONDITIONAL([WANT_APP_BACKGROUNDDETECT],[test "${with_app_backgrounddetect}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_backticks],
	[AC_HELP_STRING([--with-app_backticks],[Build and install app_backticks (default=no)])],,
	[with_app_backticks="no"])

AM_CONDITIONAL([WANT_APP_BACKTICKS],[test "${with_app_backticks}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_cdr],
	[AC_HELP_STRING([--with-app_cdr],[Build and install app_cdr (default=yes)])],,
	[with_app_cdr="yes"])

AM_CONDITIONAL([WANT_APP_CDR],[test "${with_app_cdr}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_changrab],
	[AC_HELP_STRING([--with-app_changrab],[Build and install app_changrab (default=yes)])],,
	[with_app_changrab="yes"])

AM_CONDITIONAL([WANT_APP_CHANGRAB],[test "${with_app_changrab}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_chanisavail],
	[AC_HELP_STRING([--with-app_chanisavail],[Build and install app_chanisavail (default=yes)])],,
	[with_app_chanisavail="yes"])

AM_CONDITIONAL([WANT_APP_CHANISAVAIL],[test "${with_app_chanisavail}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_chanspy],
	[AC_HELP_STRING([--with-app_chanspy],[Build and install app_chanspy (default=yes)])],,
	[with_app_chanspy="yes"])

AM_CONDITIONAL([WANT_APP_CHANSPY],[test "${with_app_chanspy}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_controlplayback],
	[AC_HELP_STRING([--with-app_controlplayback],[Build and install app_controlplayback (default=yes)])],,
	[with_app_controlplayback="yes"])

AM_CONDITIONAL([WANT_APP_CONTROLPLAYBACK],[test "${with_app_controlplayback}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_db],
	[AC_HELP_STRING([--with-app_db],[Build and install app_db (default=yes)])],,
	[with_app_db="yes"])

AM_CONDITIONAL([WANT_APP_DB],[test "${with_app_db}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_devstate],
	[AC_HELP_STRING([--with-app_devstate],[Build and install app_devstate (default=yes)])],,
	[with_app_devstate="yes"])

AM_CONDITIONAL([WANT_APP_DEVSTATE],[test "${with_app_devstate}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_setcallerpres],
	[AC_HELP_STRING([--with-app_setcallerpres],[Build and install app_setcallerpres (default=yes)])],,
	[with_app_setcallerpres="yes"])

AM_CONDITIONAL([WANT_APP_SETCALLERPRES],[test "${with_app_setcallerpres}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_dial],
	[AC_HELP_STRING([--with-app_dial],[Build and install app_dial (default=yes)])],,
	[with_app_dial="yes"])

AM_CONDITIONAL([WANT_APP_DIAL],[test "${with_app_dial}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_dictate],
	[AC_HELP_STRING([--with-app_dictate],[Build and install app_dictate (default=yes)])],,
	[with_app_dictate="yes"])

AM_CONDITIONAL([WANT_APP_DICTATE],[test "${with_app_dictate}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_directed_pickup],
	[AC_HELP_STRING([--with-app_directed_pickup],[Build and install app_directed_pickup (default=yes)])],,
	[with_app_directed_pickup="yes"])

AM_CONDITIONAL([WANT_APP_DIRECTED_PICKUP],[test "${with_app_directed_pickup}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_directory],
	[AC_HELP_STRING([--with-app_directory],[Build and install app_directory (default=yes)])],,
	[with_app_directory="yes"])

AM_CONDITIONAL([WANT_APP_DIRECTORY],[test "${with_app_directory}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_disa],
	[AC_HELP_STRING([--with-app_disa],[Build and install app_disa (default=yes)])],,
	[with_app_disa="yes"])

AM_CONDITIONAL([WANT_APP_DISA],[test "${with_app_disa}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_dtmftotext],
	[AC_HELP_STRING([--with-app_dtmftotext],[Build and install app_dtmftotext (default=no)])],,
	[with_app_dtmftotext="no"])
 
AM_CONDITIONAL([WANT_APP_DTMFTOTEXT],[test "${with_app_dtmftotext}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_dumpchan],
	[AC_HELP_STRING([--with-app_dumpchan],[Build and install app_dumpchan (default=yes)])],,
	[with_app_dumpchan="yes"])

AM_CONDITIONAL([WANT_APP_DUMPCHAN],[test "${with_app_dumpchan}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_echo],
	[AC_HELP_STRING([--with-app_echo],[Build and install app_echo (default=yes)])],,
	[with_app_echo="yes"])

AM_CONDITIONAL([WANT_APP_ECHO],[test "${with_app_echo}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_enumlookup],
	[AC_HELP_STRING([--with-app_enumlookup],[Build and install app_enumlookup (default=yes)])],,
	[with_app_enumlookup="yes"])

AM_CONDITIONAL([WANT_APP_ENUMLOOKUP],[test "${with_app_enumlookup}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_eval],
	[AC_HELP_STRING([--with-app_eval],[Build and install app_eval (default=yes)])],,
	[with_app_eval="yes"])

AM_CONDITIONAL([WANT_APP_EVAL],[test "${with_app_eval}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_exec],
	[AC_HELP_STRING([--with-app_exec],[Build and install app_exec (default=yes)])],,
	[with_app_exec="yes"])

AM_CONDITIONAL([WANT_APP_EXEC],[test "${with_app_exec}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_faxdetect],
	[AC_HELP_STRING([--with-app_faxdetect],[Build and install app_faxdetect (default=yes)])],,
	[with_app_faxdetect="yes"])

AM_CONDITIONAL([WANT_APP_FAXDETECT], [test "${with_app_faxdetect}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_flash],
	[AC_HELP_STRING([--with-app_flash],[Build and install app_flash (default=yes)])],,
	[with_app_flash="yes"])

AM_CONDITIONAL([WANT_APP_FLASH], [test "${enable_zaptel}" = "yes" -a "${with_app_flash}" = "yes" -a -n "${ZAPTEL_H}"])

dnl ###########################################################################

AC_ARG_WITH([app_forkcdr],
	[AC_HELP_STRING([--with-app_forkcdr],[Build and install app_forkcdr (default=yes)])],,
	[with_app_forkcdr="yes"])

AM_CONDITIONAL([WANT_APP_FORKCDR],[test "${with_app_forkcdr}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_getcpeid],
	[AC_HELP_STRING([--with-app_getcpeid],[Build and install app_getcpeid (default=yes)])],,
	[with_app_getcpeid="yes"])

AM_CONDITIONAL([WANT_APP_GETCPEID],[test "${with_app_getcpeid}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_getdevstate],
	[AC_HELP_STRING([--with-app_getdevstate],[Build and install app_getdevstate (default=yes)])],,
	[with_app_getdevstate="yes"])

AM_CONDITIONAL([WANT_APP_GETDEVSTATE],[test "${with_app_getdevstate}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_getextstate],
	[AC_HELP_STRING([--with-app_getextstate],[Build and install app_getextstate (default=yes)])],,
	[with_app_getextstate="yes"])

AM_CONDITIONAL([WANT_APP_GETEXTSTATE],[test "${with_app_getextstate}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_groupcount],
	[AC_HELP_STRING([--with-app_groupcount],[Build and install app_groupcount (default=yes)])],,
	[with_app_groupcount="yes"])

AM_CONDITIONAL([WANT_APP_GROUPCOUNT],[test "${with_app_groupcount}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_hasnewvoicemail],
	[AC_HELP_STRING([--with-app_hasnewvoicemail],[Build and install app_hasnewvoicemail (default=yes)])],,
	[with_app_hasnewvoicemail="yes"])

AM_CONDITIONAL([WANT_APP_HASNEWVOICEMAIL],[test "${with_app_hasnewvoicemail}" = "yes"])

dnl ###########################################################################

#AC_ARG_WITH([app_lcr],
#        [AC_HELP_STRING([--with-app_lcr],[Build and install app_lcr, requires OpenDBX (default=no)])],,
#        [with_app_lcr="no"])

#AC_CHECK_HEADERS([opendbx.h],[HAVE_OPENDBX_H=1])
#AC_CHECK_LIB([opendbx],[odbx_init],[HAVE_LIBOPENDBX=1])

#AM_CONDITIONAL([WANT_APP_LCR],[test "${with_app_lcr}" = "yes" -a -n "${HAVE_OPENDBX_H}" -a -n "${HAVE_LIBOPENDBX}"])

dnl ###########################################################################

AC_ARG_WITH([app_ldap],
	[AC_HELP_STRING([--with-app_ldap],[Build and install app_ldap (default=no)])],,
	[with_app_ldap="no"])

AM_CONDITIONAL([WANT_APP_LDAP],[test "${with_app_ldap}" = "yes"])

if test "${with_app_ldap}" = "yes"; then
   AC_CHECK_LIB(ldap, ldap_open, [], [
		      echo "LDAP libraries not found. Install openldap-dev"
		      exit -1
		      ])
   AC_CHECK_LIB(lber, ber_alloc, [], [
		      echo "BER libraries not found. Install openldap-dev"
		      exit -1
		      ])
fi

dnl ###########################################################################

AC_ARG_WITH([app_lookupblacklist],
	[AC_HELP_STRING([--with-app_lookupblacklist],[Build and install app_lookupblacklist (default=yes)])],,
	[with_app_lookupblacklist="yes"])

AM_CONDITIONAL([WANT_APP_LOOKUPBLACKLIST],[test "${with_app_lookupblacklist}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_lookupcidname],
	[AC_HELP_STRING([--with-app_lookupcidname],[Build and install app_lookupcidname (default=yes)])],,
	[with_app_lookupcidname="yes"])

AM_CONDITIONAL([WANT_APP_LOOKUPCIDNAME],[test "${with_app_lookupcidname}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_meetme],
	[AC_HELP_STRING([--with-app_meetme],[Build and install app_meetme (default=yes)])],,
	[with_app_meetme="yes"])

AM_CONDITIONAL([WANT_APP_MEETME], [test "${enable_zaptel}" = "yes" -a "${with_app_meetme}" = "yes" -a -n "${ZAPTEL_H}"])

dnl ###########################################################################

AC_ARG_WITH([app_milliwatt],
	[AC_HELP_STRING([--with-app_milliwatt],[Build and install app_milliwatt (default=yes)])],,
	[with_app_milliwatt="yes"])

AM_CONDITIONAL([WANT_APP_MILLIWATT],[test "${with_app_milliwatt}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_muxmon],
	[AC_HELP_STRING([--with-app_muxmon],[Build and install app_muxmon (default=yes)])],,
	[with_app_muxmon="yes"])

AM_CONDITIONAL([WANT_APP_MUXMON],[test "${with_app_muxmon}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_osplookup],
	[AC_HELP_STRING([--with-app_osplookup],[Build and install app_osplookup (default=no)])],,
	[with_app_osplookup="no"])

AM_CONDITIONAL([WANT_APP_OSPLOOKUP],[test "${with_app_osplookup}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_page],
        [AC_HELP_STRING([--with-app_page],[Build and install app_page (default=no)])],,
        [with_app_age="no"])

AM_CONDITIONAL([WANT_APP_PAGE],[test "${with_app_page}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_pipe],
   [AC_HELP_STRING([--with-app_pipe],[Build and install app_pipe (default=yes)])],,
   [with_app_pipe="yes"])

AM_CONDITIONAL([WANT_APP_PIPE],[test "${with_app_pipe}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_playback],
	[AC_HELP_STRING([--with-app_playback],[Build and install app_playback (default=yes)])],,
	[with_app_playback="yes"])

AM_CONDITIONAL([WANT_APP_PLAYBACK],[test "${with_app_playback}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_privacy],
	[AC_HELP_STRING([--with-app_privacy],[Build and install app_privacy (default=yes)])],,
	[with_app_privacy="yes"])

AM_CONDITIONAL([WANT_APP_PRIVACY],[test "${with_app_privacy}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_proc],
	[AC_HELP_STRING([--with-app_proc],[Build and install app_proc (default=yes)])],,
	[with_app_proc="yes"])

AM_CONDITIONAL([WANT_APP_PROC],[test "${with_app_proc}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_queue],
	[AC_HELP_STRING([--with-app_queue],[Build and install app_queue (default=yes)])],,
	[with_app_queue="yes"])

AM_CONDITIONAL([WANT_APP_QUEUE],[test "${with_app_queue}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_random],
	[AC_HELP_STRING([--with-app_random],[Build and install app_random (default=yes)])],,
	[with_app_random="yes"])

AM_CONDITIONAL([WANT_APP_RANDOM],[test "${with_app_random}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_read],
	[AC_HELP_STRING([--with-app_read],[Build and install app_read (default=yes)])],,
	[with_app_read="yes"])

AM_CONDITIONAL([WANT_APP_READ],[test "${with_app_read}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_record],
	[AC_HELP_STRING([--with-app_record],[Build and install app_record (default=yes)])],,
	[with_app_record="yes"])

AM_CONDITIONAL([WANT_APP_RECORD],[test "${with_app_record}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_rxfax],
	[AC_HELP_STRING([--with-app_rxfax],[Build and install app_rxfax (default=yes)])],,
	[with_app_rxfax="yes"])

AM_CONDITIONAL([WANT_APP_RXFAX],
	[test "${with_app_rxfax}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_sayunixtime],
	[AC_HELP_STRING([--with-app_sayunixtime],[Build and install app_sayunixtime (default=yes)])],,
	[with_app_sayunixtime="yes"])

AM_CONDITIONAL([WANT_APP_SAYUNIXTIME],[test "${with_app_sayunixtime}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_senddtmf],
	[AC_HELP_STRING([--with-app_senddtmf],[Build and install app_senddtmf (default=yes)])],,
	[with_app_senddtmf="yes"])

AM_CONDITIONAL([WANT_APP_SENDDTMF],[test "${with_app_senddtmf}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_sendtext],
	[AC_HELP_STRING([--with-app_sendtext],[Build and install app_sendtext (default=yes)])],,
	[with_app_sendtext="yes"])

AM_CONDITIONAL([WANT_APP_SENDTEXT],[test "${with_app_sendtext}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_setcdruserfield],
	[AC_HELP_STRING([--with-app_setcdruserfield],[Build and install app_setcdruserfield (default=yes)])],,
	[with_app_setcdruserfield="yes"])

AM_CONDITIONAL([WANT_APP_SETCDRUSERFIELD],[test "${with_app_setcdruserfield}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_setrdnis],
	[AC_HELP_STRING([--with-app_setrdnis],[Build and install app_setrdnis (default=yes)])],,
	[with_app_setrdnis="yes"])

AM_CONDITIONAL([WANT_APP_SETRDNIS],[test "${with_app_setrdnis}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_settransfercapability],
	[AC_HELP_STRING([--with-app_settransfercapability],[Build and install app_settransfercapability (default=yes)])],,
	[with_app_settransfercapability="yes"])

AM_CONDITIONAL([WANT_APP_SETTRANSFERCAPABILITY],[test "${with_app_settransfercapability}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_sms],
	[AC_HELP_STRING([--with-app_sms],[Build and install app_sms (default=yes)])],,
	[with_app_sms="yes"])

if test "${with_app_sms}"; then
   AC_CHECK_HEADERS([popt.h], [HAVE_POPT_H=1])
   AC_CHECK_LIB([popt], [poptStrerror], [HAVE_LIBPOPT=1])
fi

AM_CONDITIONAL([WANT_SMSQ],[test -n "${HAVE_POPT_H}" -a -n "${HAVE_LIBPOPT}"])
AM_CONDITIONAL([WANT_APP_SMS],[test "${with_app_sms}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_softhangup],
	[AC_HELP_STRING([--with-app_softhangup],[Build and install app_softhangup (default=yes)])],,
	[with_app_softhangup="yes"])

AM_CONDITIONAL([WANT_APP_SOFTHANGUP],[test "${with_app_softhangup}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_sql_mysql],
        [AC_HELP_STRING([--with-app_sql_mysql],[Build and install app_sql_mysql (default=yes)])],,
        [with_app_sql_mysql="yes"])
		
AM_CONDITIONAL([WANT_APP_SQL_MYSQL],[test "${enable_mysql}" = "yes" -a "$with_app_sql_mysql" = "yes"])
		
dnl ###########################################################################

AC_ARG_WITH([app_sql_postgres],
        [AC_HELP_STRING([--with-app_sql_postgres],[Build and install app_sql_postgres (default=yes)])],,
        [with_app_sql_postgres="yes"])
		
AM_CONDITIONAL([WANT_APP_SQL_POSTGRES],[test "${enable_postgresql}" = "yes" -a "$with_app_sql_postgres" = "yes"])
		
dnl ###########################################################################

AC_ARG_WITH([app_stack],
	[AC_HELP_STRING([--with-app_stack],[Build and install app_stack (default=yes)])],,
	[with_app_stack="yes"])

AM_CONDITIONAL([WANT_APP_STACK],[test "${with_app_stack}" = "yes"])

dnl ###########################################################################
		
AC_ARG_WITH([app_system],
	[AC_HELP_STRING([--with-app_system],[Build and install app_system (default=yes)])],,
	[with_app_system="yes"])

AM_CONDITIONAL([WANT_APP_SYSTEM],[test "${with_app_system}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_t38gateway],
	[AC_HELP_STRING([--with-app_t38gateway],[Build and install app_t38gateway (default=yes)])],,
	[with_app_t38gateway="yes"])

AM_CONDITIONAL([WANT_APP_T38GATEWAY],
	[test "${with_app_t38gateway}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_transfer],
	[AC_HELP_STRING([--with-app_transfer],[Build and install app_transfer (default=yes)])],,
	[with_app_transfer="yes"])

AM_CONDITIONAL([WANT_APP_TRANSFER],[test "${with_app_transfer}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_txfax],
	[AC_HELP_STRING([--with-app_txfax],[Build and install app_txfax (default=yes)])],,
	[with_app_txfax="yes"])

AM_CONDITIONAL([WANT_APP_TXFAX],
	[test "${with_app_txfax}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_userevent],
	[AC_HELP_STRING([--with-app_userevent],[Build and install app_userevent (default=yes)])],,
	[with_app_userevent="yes"])

AM_CONDITIONAL([WANT_APP_USEREVENT],[test "${with_app_userevent}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_v110],
   [AC_HELP_STRING([--with-app_v110],[Build and install app_v110 [Requires chan_misdn] (default=no)])],,
   [with_app_v110="no"])

AM_CONDITIONAL([WANT_APP_V110],[test "${with_app_v110}" = "yes" -a "${with_chan_misdn}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_valetparking],
	[AC_HELP_STRING([--with-app_valetparking],[Build and install app_valetparking (default=no)])],,
	[with_app_valetparking="no"])

AM_CONDITIONAL([WANT_APP_VALETPARKING],[test "${with_app_valetparking}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_verbose],
	[AC_HELP_STRING([--with-app_verbose],[Build and install app_verbose (default=yes)])],,
	[with_app_verbose="yes"])

AM_CONDITIONAL([WANT_APP_VERBOSE],[test "${with_app_verbose}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_visdn_ppp],
	[AC_HELP_STRING([--with-app_visdn_ppp],[Build and install app_visdn_ppp (default=yes)])],,
	[with_app_visdn_ppp="yes"])

AM_CONDITIONAL([WANT_APP_VISDN_PPP],[test "${with_app_visdn_ppp}" = "yes" -a "${enable_visdn}" -a -n "${HAVE_VISDN_VISDN_H}" -a -n "${HAVE_LIBQ931}"])

dnl ###########################################################################

AC_ARG_WITH([app_voicemail],
	[AC_HELP_STRING([--with-app_voicemail],[Build and install app_voicemail (default=yes)])],,
	[with_app_voicemail="yes"])

AM_CONDITIONAL([WANT_APP_VOICEMAIL],[test "${with_app_voicemail}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_waitforring],
	[AC_HELP_STRING([--with-app_waitforring],[Build and install app_waitforring (default=yes)])],,
	[with_app_waitforring="yes"])

AM_CONDITIONAL([WANT_APP_WAITFORRING],[test "${with_app_waitforring}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_waitfordigits],
	[AC_HELP_STRING([--with-app_waitfordigits],[Build and install app_waitfordigits (default=yes)])],,
	[with_app_waitfordigits="yes"])

AM_CONDITIONAL([WANT_APP_WAITFORDIGITS],[test "${with_app_waitfordigits}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_waitforsilence],
	[AC_HELP_STRING([--with-app_waitforsilence],[Build and install app_waitforsilence (default=yes)])],,
	[with_app_waitforsilence="yes"])

AM_CONDITIONAL([WANT_APP_WAITFORSILENCE],[test "${with_app_waitforsilence}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_while],
	[AC_HELP_STRING([--with-app_while],[Build and install app_while (default=yes)])],,
	[with_app_while="yes"])

AM_CONDITIONAL([WANT_APP_WHILE],[test "${with_app_while}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([app_zapateller],
	[AC_HELP_STRING([--with-app_zapateller],[Build and install app_zapateller (default=yes)])],,
	[with_app_zapateller="yes"])

AM_CONDITIONAL([WANT_APP_ZAPATELLER],[test "${with_app_zapateller}" = "yes"])


dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Channels
dnl ###
dnl ###########################################################################
dnl ###########################################################################


AC_ARG_WITH([chan_alsa],
        [AC_HELP_STRING([--with-chan_alsa],[Build and install chan_alsa (default=no)])],,
        [with_chan_alsa="no"])

if test "${with_chan_alsa}" = "yes"; then
   PKG_CHECK_MODULES([ALSA], [alsa], [HAVE_ALSA=1])
fi

AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)

AM_CONDITIONAL([WANT_CHAN_ALSA],[test "${with_chan_alsa}" = "yes" -a -n "${HAVE_ALSA}"])

AC_ARG_WITH([chan_agent],
	[AC_HELP_STRING([--with-chan_agent],[Build and install chan_agent (default=yes)])],,[with_chan_agent="yes"])

AM_CONDITIONAL([WANT_CHAN_AGENT],[test "${with_chan_agent}" = "yes"])

dnl ###########################################################################

MISDN_CFLAGS="-I/usr/include/ -I/usr/include/i4lnet -I/usr/include/mISDNuser"
MISDN_LDFLAGS=

AC_ARG_WITH([misdn-includes],
	[AC_HELP_STRING([--with-misdn-includes],[Location of mISDN include files.])],
	[MISDN_CFLAGS="-I${withval}"])

AC_ARG_WITH([misdn-libs],
	[AC_HELP_STRING([--with-misdn-libs],[Location of mISDN library files.])],
	[MISDN_LDFLAGS="-L${withval}"])

AC_SUBST(MISDN_CFLAGS)
AC_SUBST(MISDN_LDFLAGS)

AC_ARG_WITH([chan_misdn],
	[AC_HELP_STRING([--with-chan_misdn],[Build and install chan_misdn (default=no)])],,[with_chan_misdn="no"])

AM_CONDITIONAL([WANT_CHAN_MISDN],[test "${with_chan_misdn}" = "yes"])

dnl ###########################################################################
	
AC_ARG_WITH([chan_capi],
	[AC_HELP_STRING([--with-chan_capi],[Build and install chan_capi (default=no)])],,[with_chan_capi="no"])

if test "${with_chan_capi}" = "yes" ; then
   AC_CHECK_HEADERS([capi20.h], [HAVE_CAPI20_H=1])
   AC_CHECK_HEADERS([capiutils.h],[HAVE_CAPIUTILS_H=1])

	if test -n "${HAVE_CAPI20_H}" -a -n "${HAVE_CAPIUTILS_H}"
	then
		WANT_CHAN_CAPI="yes"
	fi
fi

AM_CONDITIONAL([WANT_CHAN_CAPI],[test "${with_chan_capi}" = "yes" -a -n "${HAVE_CAPI20_H}" -a -n "${HAVE_CAPIUTILS_H}"])

dnl ###########################################################################

AC_ARG_WITH([chan_fax],
	[AC_HELP_STRING([--with-chan_fax],[Build and install chan_fax (default=no)])],,[want_chan_fax="no"])

if test "${with_chan_fax}" = "yes"; then
   CHAN_FAX_LIBS="-lspandsp -ltiff"
   AC_CHECK_HEADERS([pty.h], [HAVE_PTY_H=1])
   AC_CHECK_LIB([util], [openpty], [HAVE_OPENPTY=1 CHAN_FAX_LIBS="${CHAN_FAX_LIBS} -lutil"],
	[AC_CHECK_LIB([c], [openpty], [HAVE_OPENPTY=1])])
fi
AC_SUBST(CHAN_FAX_LIBS)

AM_CONDITIONAL([WANT_CHAN_FAX],
	[test "${with_chan_fax}" = "yes" -a -n "${HAVE_OPENPTY}"])

dnl ###########################################################################

AC_ARG_WITH([chan_features],
	[AC_HELP_STRING([--with-chan_features],[Build and install chan_features (default=yes)])],,[with_chan_features="yes"])

AM_CONDITIONAL([WANT_CHAN_FEATURES],[test "${with_chan_features}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([chan_iax2],
	[AC_HELP_STRING([--with-chan_iax2],[Build and install chan_iax2 (default=yes)])],,[with_chan_iax2="yes"])

AM_CONDITIONAL([WANT_CHAN_IAX2],[test "${with_chan_iax2}" = "yes"])

dnl ###########################################################################

AC_ARG_ENABLE([iax-trunking],
        [AC_HELP_STRING([--enable-iax-trunking],[Enables the IAX trunking feature (default=no)])],,[enable_iax_trunking="no"])

if test "${enable_iax_trunking}" = "yes"; then
   AC_DEFINE([IAX_TRUNKING],,[Enable IAX trunking feature])
fi


dnl ###########################################################################
if test "${enable_iax_trunking}" = "yes"; then
   AC_DEFINE([IAX_TRUNKING],,[Enable IAX trunking feature])
   fi
   
AC_ARG_WITH([chan_local],
	[AC_HELP_STRING([--with-chan_local],[Build and install chan_local (default=yes)])],,[with_chan_local="yes"])

AM_CONDITIONAL([WANT_CHAN_LOCAL],[test "${with_chan_local}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([chan_mgcp],
	[AC_HELP_STRING([--with-chan_mgcp],[Build and install chan_mgcp (default=yes)])],,[with_chan_mgcp="yes"])

AM_CONDITIONAL([WANT_CHAN_MGCP],[test "${with_chan_mgcp}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([chan_sip],
	[AC_HELP_STRING([--with-chan_sip],[Build and install chan_sip (default=yes)])],,[with_chan_sip="yes"])

AM_CONDITIONAL([WANT_CHAN_SIP],[test "${with_chan_sip}" = "yes"])

#AC_ARG_ENABLE([t38],
#	[AC_HELP_STRING([--enable-t38],[Enable T.38 support (default=yes)])],,[enable_t38="yes"])

#if test "x${enable_t38}" != "xno"; then
#	AC_DEFINE([T38_SUPPORT],[1],[chan_sip T.38 support])
#fi

dnl ###########################################################################

AC_ARG_WITH([chan_sccp],
	[AC_HELP_STRING([--with-chan_sccp],[Build and install chan_sccp (default=no)])],,[with_chan_sccp="no"])

AM_CONDITIONAL([WANT_CHAN_SCCP],[test "${with_chan_sccp}" = "yes"])

AC_ARG_ENABLE([sccp-park],
	[AC_HELP_STRING([--disable-sccp-park],[Disable sccp call parking support (default=enabled)])],,[enable_sccp_park="yes"])

if test "x${enable_sccp_park}" != "xno"; then
	AC_DEFINE([CS_SCCP_PARK],[1],[chan_sccp call parking support])
fi

AC_ARG_ENABLE([sccp-pickup],
	[AC_HELP_STRING([--disable-sccp-pickup],[Disable sccp call pickup support (default=enabled)])],,[enable_sccp_pickup="yes"])

if test "x${enable_sccp_pickup}" != "xno"; then
	AC_DEFINE([CS_SCCP_PICKUP],[1],[chan_sccp call pickup support])
fi

dnl ###########################################################################

AC_ARG_WITH(chan_unicall,
	[AC_HELP_STRING([--with-chan_unicall],[Build and install chan_unicall (default=no).])],,[with_chan_unicall="no"])

if test "${with_chan_unicall}" = "yes"; then
   AC_CHECK_LIB([unicall], [main], [HAVE_LIBUNICALL=1])
fi

AM_CONDITIONAL([WANT_CHAN_UNICALL],[test "${with_chan_unicall}" = "yes" -a -n "${HAVE_LIBUNICALL}"])

dnl ###########################################################################

AC_ARG_WITH([chan_visdn],
	[AC_HELP_STRING([--with-chan_visdn],[Build and install chan_visdn (default=no)])],,[with_chan_visdn="no"])

AM_CONDITIONAL([WANT_CHAN_VISDN],[test "${with_chan_visdn}" = "yes" -a "${enable_visdn}" -a -n "${HAVE_VISDN_VISDN_H}" -a -n "${HAVE_LIBQ931}"])

dnl ###########################################################################

AC_ARG_WITH([chan_woomera],
	[AC_HELP_STRING([--with-chan_woomera],[Build and install chan_woomera (default=yes)])],,
	[with_chan_woomera="yes"])

AM_CONDITIONAL([WANT_CHAN_WOOMERA], [test "${with_chan_woomera}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([chan_bluetooth],
	[AC_HELP_STRING([--with-chan_bluetooth],[Build and install chan_bluetooth (default=no)])],,
	[with_chan_bluetooth="no"])

if test "${with_chan_bluetooth}" = "yes" ; then
	AC_CHECK_HEADERS([bluetooth/bluetooth.h], [HAVE_BLUETOOTH_H=1])
	AC_CHECK_LIB([bluetooth], [main], [HAVE_LIBBLUETOOTH=1])
fi


AM_CONDITIONAL([WANT_CHAN_BLUETOOTH], [test "${with_chan_bluetooth}" = "yes" -a -n "${HAVE_BLUETOOTH_H}" -a -n "${HAVE_LIBBLUETOOTH}"])

dnl ###########################################################################

AC_ARG_WITH([chan_zap],
	[AC_HELP_STRING([--with-chan_zap],[Build and install chan_zap (default=yes) (requires --enable-zaptel)])],,
	[with_chan_zap="yes"])

AM_CONDITIONAL([WANT_CHAN_ZAP], [test "${enable_zaptel}" = "yes" -a "${with_chan_zap}" = "yes" -a -n "${HAVE_LIBTONEZONE}"])

dnl ###########################################################################

AC_ARG_WITH([app_icd],
	[AC_HELP_STRING([--with-app_icd],[Build and install app_icd (default=no)])],,
	[with_app_icd="no"])

AM_CONDITIONAL([WANT_APP_ICD],[test "${with_app_icd}" = "yes"])

#app_icd = "no"
#AC_ARG_WITH(app_icd,
#	[ --with-app_icd=no			use app_icd, a full blown inteligent call distribution system (default=no) *** HIGHLY EXPERIMENTAL FOR THE MOMENT ****],,[with_app_icd="no"])
#
#

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### CDR
dnl ###
dnl ###########################################################################
dnl ###########################################################################

#AC_ARG_WITH(cdr_odbc,
#	[AC_HELP_STRING([--with-cdr_odbc],[Build and install cdr_odbc (default=no) (requires --enable-odbc])],,[with_cdr_odbc="no"])

#AM_CONDITIONAL([WANT_CDR_ODBC],[test "${enable_odbc}" = "yes" -a "${with_cdr_odbc}" = "yes" -a -n "${HAVE_LIBODBC}"])

dnl ###########################################################################

AC_ARG_WITH(cdr_manager,
        [AC_HELP_STRING([--with-cdr_manager],[Build and install cdr_manager (default=yes)])],,[with_cdr_manager="yes"])

AM_CONDITIONAL([WANT_CDR_MANAGER],[test "${with_cdr_manager}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(cdr_mysql,
	[AC_HELP_STRING([--with-cdr_mysql],[Build and install cdr_mysql (default=no) (requires --enable-mysql])],,[with_cdr_mysql="no"])

AM_CONDITIONAL([WANT_CDR_MYSQL],[test "${enable_mysql}" = "yes" -a "$with_cdr_mysql" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(cdr_pgsql,
	[AC_HELP_STRING([--with-cdr_pgsql],[Build and install cdr_pgsql (default=no) (requires --enable-postgresql])],,[with_cdr_pgsql="no"])

AM_CONDITIONAL([WANT_CDR_PGSQL],[test "${enable_postgresql}" = "yes" -a "$with_cdr_pgsql" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(cdr_pgsql_custom,
        [AC_HELP_STRING([--with-cdr_pgsql_custom],[Build and install cdr_pgsql_custom (default=no) (requires --enable-postgresql)])],,[with_cdr_pgsql_custom="no"])

AM_CONDITIONAL([WANT_CDR_PGSQL_CUSTOM],[test "${enable_postgresql}" = "yes" -a "$with_cdr_pgsql_custom" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(cdr_sqlite3_custom,
        [AC_HELP_STRING([--with-cdr_sqlite3_custom],[Build and install cdr_sqlite3_custom (default=no)])],,[with_cdr_sqlite3_custom="no"])

AM_CONDITIONAL([WANT_CDR_SQLITE3_CUSTOM],[test "${with_cdr_sqlite3_custom}" = "yes"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Codecs
dnl ###
dnl ###########################################################################
dnl ###########################################################################

AC_ARG_WITH([codec_speex],
	[AC_HELP_STRING([--with-codec_speex],[Build and install codec_speex (default=yes)])],,[with_codec_speex="yes"])

if test "${with_codec_speex}" = "yes"; then
   AC_CHECK_LIB([speex], [main], [HAVE_LIBSPEEX=1],[AC_MSG_ERROR(Cannot find Speex library.)])
   
   AC_CHECK_HEADER([speex/speex.h],[HAVE_SPEEX_H=1 AC_SUBST([SPEEX_CFLAGS],[-I/usr/include/speex])],[AC_MSG_ERROR(Cannot find header files for Speex.)])
fi

AM_CONDITIONAL([WANT_CODEC_SPEEX],[test "${with_codec_speex}" = "yes" -a -n "${HAVE_LIBSPEEX}" -a -n "${HAVE_SPEEX_H}"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Formats
dnl ###
dnl ###########################################################################
dnl ###########################################################################

AC_ARG_WITH([format_jpeg],
        [AC_HELP_STRING([--with-format_jpeg],[Build and install format_jpeg (default=yes)])],,
        [with_format_jpeg="yes"])

AM_CONDITIONAL([WANT_FORMAT_JPEG],[test "${with_format_jpeg}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([format_oggvorbis],
        [AC_HELP_STRING([--with-format_oggvorbis],[Build and install format_oggvorbis (default=yes)])],,
        [with_format_oggvorbis="yes"])

if test "${with_format_oggvorbis}" = "yes"; then
	AC_CHECK_LIB([ogg], [main], [HAVE_LIBOGG=1 OGG_LIBS="-logg"])
	AC_CHECK_LIB([vorbis], [main], [HAVE_LIBVORBIS=1 VORBIS_LIBS="-lvorbis"])
	AC_CHECK_LIB([vorbisenc], [main], [HAVE_LIBVORBISENC=1 VORBIS_LIBS="${VORBIS_LIBS} -lvorbisenc"])

	AC_CHECK_HEADER([ogg/ogg.h], [HAVE_OGG_H=1])
	AC_CHECK_HEADER([vorbis/vorbisenc.h], [HAVE_VORBIS_H=1])

	if test -n "${HAVE_LIBVORBIS}" -a -n "${HAVE_LIBVORBISENC}" -a -n "${HAVE_LIBOGG}"; then
		AC_SUBST([VORBIS_LIBS], [${VORBIS_LIBS}])
		AC_SUBST([OGG_LIBS], [${OGG_LIBS}])
	fi
fi

AM_CONDITIONAL([WANT_FORMAT_OGGVORBIS],
	[test "${with_format_oggvorbis}" = "yes" -a -n "${HAVE_LIBVORBIS}" -a -n "${HAVE_LIBVORBISENC}" -a -n "${HAVE_LIBOGG}" -a -n "${HAVE_OGG_H}" -a -n "${HAVE_VORBIS_H}"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Functions
dnl ###
dnl ###########################################################################
dnl ###########################################################################

AC_ARG_WITH([func_callerid],
        [AC_HELP_STRING([--with-func_callerid],[Build and install func_callerid (default=yes)])],,
        [with_func_callerid="yes"])

AM_CONDITIONAL([WANT_FUNC_CALLERID],[test "${with_func_callerid}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_config],
        [AC_HELP_STRING([--with-func_config],[Build and install func_config (default=yes)])],,
        [with_func_config="yes"])

AM_CONDITIONAL([WANT_FUNC_CONFIG],[test "${with_func_config}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_cdr],
        [AC_HELP_STRING([--with-func_cdr],[Build and install func_cdr (default=yes)])],,
        [with_func_cdr="yes"])

AM_CONDITIONAL([WANT_FUNC_CDR],[test "${with_func_cdr}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_db],
        [AC_HELP_STRING([--with-func_db],[Build and install func_db (default=yes)])],,
        [with_func_db="yes"])

AM_CONDITIONAL([WANT_FUNC_DB],[test "${with_func_db}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_enum],
        [AC_HELP_STRING([--with-func_enum],[Build and install func_enum (default=yes)])],,
        [with_func_enum="yes"])

AM_CONDITIONAL([WANT_FUNC_ENUM],[test "${with_func_enum}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_env],
        [AC_HELP_STRING([--with-func_env],[Build and install func_env (default=yes)])],,
        [with_func_env="yes"])

AM_CONDITIONAL([WANT_FUNC_ENV],[test "${with_func_env}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_fileexists],
        [AC_HELP_STRING([--with-func_fileexists],[Build and install func_fileexists (default=yes)])],,
        [with_func_fileexists="yes"])

AM_CONDITIONAL([WANT_FUNC_FILEEXISTS],[test "${with_func_fileexists}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_groupcount],
        [AC_HELP_STRING([--with-func_groupcount],[Build and install func_groupcount (default=yes)])],,
        [with_func_groupcount="yes"])

AM_CONDITIONAL([WANT_FUNC_GROUPCOUNT],[test "${with_func_groupcount}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_language],
        [AC_HELP_STRING([--with-func_language],[Build and install func_language (default=yes)])],,
        [with_func_language="yes"])

AM_CONDITIONAL([WANT_FUNC_LANGUAGE],[test "${with_func_language}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_logic],
        [AC_HELP_STRING([--with-func_logic],[Build and install func_logic (default=yes)])],,
        [with_func_logic="yes"])

AM_CONDITIONAL([WANT_FUNC_LOGIC],[test "${with_func_logic}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_math],
        [AC_HELP_STRING([--with-func_math],[Build and install func_math (default=yes)])],,
        [with_func_math="yes"])

AM_CONDITIONAL([WANT_FUNC_MATH],[test "${with_func_math}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_md5],
        [AC_HELP_STRING([--with-func_md5],[Build and install func_md5 (default=yes)])],,
        [with_func_md5="yes"])

AM_CONDITIONAL([WANT_FUNC_MD5],[test "${with_func_md5}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_moh],
        [AC_HELP_STRING([--with-func_moh],[Build and install func_moh (default=yes)])],,
        [with_func_moh="yes"])

AM_CONDITIONAL([WANT_FUNC_MOH],[test "${with_func_moh}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_strings],
        [AC_HELP_STRING([--with-func_strings],[Build and install func_strings (default=yes)])],,
        [with_func_strings="yes"])

AM_CONDITIONAL([WANT_FUNC_STRINGS],[test "${with_func_strings}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_timeout],
        [AC_HELP_STRING([--with-func_timeout],[Build and install func_timeout (default=yes)])],,
        [with_func_timeout="yes"])

AM_CONDITIONAL([WANT_FUNC_TIMEOUT],[test "${with_func_timeout}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([func_uri],
        [AC_HELP_STRING([--with-func_uri],[Build and install func_uri (default=yes)])],,
        [with_func_uri="yes"])

AM_CONDITIONAL([WANT_FUNC_URI],[test "${with_func_uri}" = "yes"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### PBX
dnl ###
dnl ###########################################################################
dnl ###########################################################################

AC_ARG_WITH([pbx_dundi],
	[AC_HELP_STRING([--with-pbx_dundi],[Build and install pbx_dundi (default=yes)])],,
	[with_pbx_dundi="yes"])

if test "${with_pbx_dundi}" = "yes"; then
   AC_CHECK_LIB([z], [main], [HAVE_LIBZ=1])
fi
   
AM_CONDITIONAL([WANT_PBX_DUNDI],[test "${with_pbx_dundi}" = "yes" -a -n "${HAVE_LIBZ}"])

dnl ###########################################################################

AC_ARG_WITH([pbx_realtime],
	[AC_HELP_STRING([--with-pbx_realtime],[Build and install pbx_realtime (default=yes)])],,
	[with_pbx_realtime="yes"])

AM_CONDITIONAL([WANT_PBX_REALTIME],[test "${with_pbx_realtime}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([pbx_ael],
	[AC_HELP_STRING([--with-pbx_ael],[Build and install pbx_ael (Experimental) (default=no)])],,
	[with_pbx_ael="no"])

AM_CONDITIONAL([WANT_PBX_AEL],[test "${with_pbx_ael}" = "yes"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Resources
dnl ###
dnl ###########################################################################
dnl ###########################################################################

#AC_ARG_WITH([res_config_odbc],
#        [AC_HELP_STRING([--with-res_config_odbc],[Build and install res_config_odbc (default=no) (requires --enable-odbc and --with-res_odbc)])],,[with_res_config_odbc="no"])

#AM_CONDITIONAL([WANT_RES_CONFIG_ODBC],[test "${enable_odbc}" = "yes" -a "${with_res_config_odbc}" = "yes" -a "${with_res_odbc}" = "yes" -a -n "${HAVE_LIBODBC}"])

dnl ###########################################################################

AC_ARG_WITH([res_config_curl],
        [AC_HELP_STRING([--with-res_config_curl],[Build and install res_config_curl (default=no)])],,[with_res_config_curl="no"])

AM_CONDITIONAL([WANT_RES_CONFIG_CURL],[test "${with_res_config_curl}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([res_snmp],
        [AC_HELP_STRING([--with-res_snmp],[Build and install res_snmp (default=no)])],,[with_res_snmp="no"])

AM_CONDITIONAL([WANT_RES_SNMP],[test "${with_res_snmp}" = "yes"])

if test "${with_res_snmp}" = "yes"; then
   CHECK_SNMP
fi

dnl ###########################################################################

AC_ARG_WITH(res_config_mysql,
	[AC_HELP_STRING([--with-res_config_mysql],[Build and install res_config_mysql (default=no) (requires --enable-mysql])],,[with_res_config_mysql="no"])

AM_CONDITIONAL([WANT_RES_CONFIG_MYSQL],[test "${enable_mysql}" = "yes" -a "${with_res_config_mysql}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(res_config_pgsql,
	[AC_HELP_STRING([--with-res_config_pgsql],[Build and install res_config_pgsql (default=no) (requires --enable-postgresql])],,[with_res_config_pgsql="no"])

AM_CONDITIONAL([WANT_RES_CONFIG_PGSQL],[test "${enable_postgresql}" = "yes" -a "${with_res_config_pgsql}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH([res_jabber],
	[AC_HELP_STRING([--with-res_jabber],[Build and install res_jabber (default=no) (requires --enable-jabber])],,[with_res_jabber="no"])

AM_CONDITIONAL([WANT_RES_JABBER],[test "${enable_jabber}" = "yes" -a "${with_res_jabber}" = "yes" -a -n "${HAVE_LOUDMOUTH}"])

dnl ###########################################################################

AC_ARG_WITH([res_js],
	[AC_HELP_STRING([--with-res_js],[Build and install res_js (default=no) (requires --enable-javascript)])],,[with_res_js="no"])

AM_CONDITIONAL([WANT_RES_JS],[test "${enable_javascript}" = "yes" -a "${with_res_js}" = "yes" -a -n "${HAVE_LIBJS}" -a \( -n "${HAVE_JSSCRIPT_H}" -o -n "${HAVE_JS_JSSCRIPT_H}" \) -a -n "${HAVE_NSPR}"])  

dnl ###########################################################################

#AC_ARG_WITH([res_odbc],
#	[AC_HELP_STRING([--with-res_odbc],[Build and install res_odbc (default=no) (requires --enable-odbc)])],,
#	[with_res_odbc="no"])

#AM_CONDITIONAL([WANT_RES_ODBC],[test "${enable_odbc}" = "yes" -a "${with_res_odbc}" = "yes" -a -n "${HAVE_LIBODBC}"])

dnl ###########################################################################

AC_ARG_WITH([debug_malloc],
	[AC_HELP_STRING([--with-debug_malloc],[Enable debug malloc (default=no)])],,[with_debug_malloc="no"])

if test "$with_debug_malloc" = "yes" ; then
    AC_DEFINE([__CW_DEBUG_MALLOC], [1], [We want malloc debug ...])
fi

dnl ###########################################################################

dnl res_osp ... open settlement protocol handling
dnl look for Transnexus OSP Toolkit (stolen from the OpenH323 configure.ac)

dnl MSWIN_DISPLAY    transnexus_osp,Transnexus OSP Toolkit
dnl MSWIN_CHECK_FILE transnexus_osp,include/osp/ospcryptowrap.h,H323_TRANSNEXUS_OSP=1
dnl MSWIN_DIR_SYMBOL transnexus_osp,H323_TRANSNEXUS_OSP_DIR
dnl MSWIN_CHECK_DIR  transnexus_osp,lib\
dnl MSWIN_CHECK_DIR  transnexus_osp,pthread\lib
dnl MSWIN_CHECK_DIR  transnexus_osp,pthread\include

AC_ARG_ENABLE(transnexusosp,
       [AC_HELP_STRING([--disable-transnexusosp],[disable support for the Transnexus OSP toolkit])],,[enable_transnexusosp="no"])

if test "${enable_transnexusosp}z" = "z" ; then
  enable_transnexusosp="yes"
fi

if test "x$enable_transnexusosp" = "xno" ; then
  AC_MSG_NOTICE(Disabling support for the Transnexus OSP toolkit)
else
  AC_CHECK_HEADERS(osp/osp.h, HAS_TRANSNEXUS_OSP=1)
  if test "${HAS_TRANSNEXUS_OSP}x" = "1x" ; then
    AC_CHECK_LIB(osp, OSPPInit, HAS_TRANSNEXUS_OSP=1, HAS_TRANSNEXUS_OSP=, -lssl)
    if test "${HAS_TRANSNEXUS_OSP}x" = "1x" ; then
      TRANSNEXUS_OSP=1
      AC_DEFINE([TRANSNEXUS_OSP],"1",[yes we have osp ...])
      OSP_LIBS="-losp"
      OSP_INCLUDE="#include <osp/osp.h>"
    else
      unset ac_cv_lib_osp_OSPPInit
      AC_CHECK_LIB(osp, OSPPInit, HAS_TRANSNEXUS_OSP=1, HAS_TRANSNEXUS_OSP=, -lssl)
      if test "${HAS_TRANSNEXUS_OSP}x" = "x" ; then
         unset TRANSNEXUS_OSP
      else
        TRANSNEXUS_OSP=1
        AC_DEFINE([TRANSNEXUS_OSP],"1",[yes we have OSP])
        OSP_CFLAGS="-I/usr/local/include"
        OSP_LDLIBS="${LDLIBS} -losp"
        OSP_INCLUDE='#include "/usr/local/include/osp/osp.h"'
      fi
    fi
  fi
fi

AC_SUBST(TRANSNEXUS_OSP)
AC_SUBST(OSP_LIBS)
AC_SUBST(OSP_INCLUDE)
AC_SUBST(OSP_CFLAGS)
AM_CONDITIONAL([WANT_OSP],[test -n "${TRANSNEXUS_OSP}"])

dnl ###########################################################################

AC_ARG_WITH(res_sqlite,
        [AC_HELP_STRING([--with-res_sqlite],[Build and install res_sqlite (default=no)])],,[with_res_sqlite="no"])

AM_CONDITIONAL([WANT_RES_SQLITE],[test "${with_res_sqlite}" = "yes"])

dnl ###########################################################################

AC_ARG_WITH(efence,
        [AC_HELP_STRING([--with-efence],[Build with electric fence (default=no)])],,[with_efence="no"])

if test "x${with_efence}" = "xyes" ; then
  AC_MSG_CHECKING([for efence])
  AC_CHECK_LIB(efence,EF_Print,
    AC_MSG_RESULT([ok])
    LIBS="$LIBS -lefence",
    AC_MSG_RESULT([library missing])
  )
fi

dnl ###########################################################################

AC_PATH_PROG( PKGCONFIG, pkg-config, no, $PATH:/usr/bin:/usr/local/bin)

if test "$PKGCONFIG" != "no" ; then
    AC_MSG_CHECKING([sndfile with pkg-config])

    $PKGCONFIG --exists sndfile
    SNDFILE_EXISTS=$?

    if test "$SNDFILE_EXISTS" -eq "0"; then
	SNDFILE_LIBS=`$PKGCONFIG --libs-only-l sndfile 2>/dev/null`
	SNDFILE_LDFLAGS=`$PKGCONFIG --libs-only-L sndfile 2>/dev/null`
	SNDFILE_CFLAGS=`$PKGCONFIG --cflags-only-I sndfile 2>/dev/null`

	AC_DEFINE([HAVE_SNDFILE],[1],[sndfile library avalaible])

        AC_MSG_RESULT([found])
        AC_SUBST(SNDFILE_LIBS)
        AC_SUBST(SNDFILE_LDFLAGS)
        AC_SUBST(SNDFILE_CFLAGS)
    else
        AC_MSG_RESULT([no])
    fi
fi

AM_CONDITIONAL([WANT_FILESTREAM_SNDFILE],[test "$SNDFILE_EXISTS" -eq "0"])

dnl ###########################################################################
dnl ###########################################################################
dnl ###
dnl ### Documentation
dnl ###
dnl ###########################################################################
dnl ###########################################################################

# yes yes yes, we know that the GNU coding standards say that we should only
# be using the --with parameter prefix for external packages but we are using it
# nevertheless because it makes more sense from a plain good English point of view.

AC_ARG_WITH([man_pages],
        [AC_HELP_STRING([--with-man-pages],[Install man pages (default=yes)])],,[with_man_pages="yes"])

AM_CONDITIONAL([WANT_MAN_PAGES],[test "${with_man_pages}" = "yes"])

AC_ARG_WITH([readme],
        [AC_HELP_STRING([--with-readme],[Install README file (default=yes)])],,[with_readme="yes"])

AM_CONDITIONAL([WANT_README],[test "${with_readme}" = "yes"])

AC_ARG_WITH([license],
        [AC_HELP_STRING([--with-license],[Install LICENSE file (default=yes)])],,[with_license="yes"])

AM_CONDITIONAL([WANT_LICENSE],[test "${with_license}" = "yes"])

AC_ARG_WITH([credits],
        [AC_HELP_STRING([--with-credits],[Install CREDITS (default=yes)])],,[with_credits="yes"])

AM_CONDITIONAL([WANT_CREDITS],[test "${with_credits}" = "yes"])

AC_ARG_WITH([sgml_docs],
        [AC_HELP_STRING([--with-sgml-docs],[Install SGML documentation (default=no)])],,[with_sgml_docs="no"])

AM_CONDITIONAL([WANT_SGML_DOCS],[test "${with_sgml_docs}" = "yes"])


AC_SUBST(SVN)
AC_SUBST(GREP)
AC_SUBST(GROUPADD)
AC_SUBST(SOLINK)
AC_SUBST(CWLINK)
AC_SUBST(CPU_OPTIONS)
AC_SUBST(all_debug)
AC_SUBST(DEBUG_THREADS)
AC_SUBST(DEBUG_MALLOC)
AC_SUBST(LOW_MEMORY)
AC_SUBST(prefix)

AC_SUBST_FILE(substitute)
substitute=${srcdir}/substitute.mak

AM_CONFIG_HEADER(include/confdefs.h)  

######
#
# Define in which subdirectories we have to run configure as well
#
######
AC_CONFIG_SUBDIRS(libltdl)

AC_OUTPUT([Makefile
		 include/Makefile
		 include/callweaver/Makefile
		 libs/Makefile
                 ogi/Makefile
                 apps/Makefile
                 apps/icd/Makefile
		 apps/nconference/Makefile
                 cdr/Makefile
                 channels/Makefile
                 channels/iax2/Makefile
                 channels/sccp/Makefile
                 channels/fax/Makefile
                 channels/misdn/Makefile
                 codecs/Makefile
 		 corelib/Makefile
		 corelib/version.sh
 		 corelib/jitterbuffer/Makefile
                 formats/Makefile
                 funcs/Makefile
                 pbx/Makefile
                 res/Makefile
                 stdtime/Makefile
                 utils/Makefile
		 configs/Makefile
 		 ogi/ogi-test.ogi
		 ogi/fastogi-test
		 keys/Makefile
])

if test "${builtin_sqlite3}" = "yes"; then
	AC_OUTPUT([	 sqlite3-embedded/Makefile
			 sqlite3-embedded/mkopcodec.awk
			 sqlite3-embedded/mkopcodeh.awk
	])
fi


echo ""
echo "   ************************"
echo "   *   Configure status   *"
echo "   ************************"
echo ""

echo "Loudmouth/Jabber Support:  ${HAVE_LIBLOUDMOUTH}"
if test "x${HAVE_LIBLOUDMOUTH}" = "xyes" ; then
        echo "Loudmouth CFLAGS: ${LOUDMOUTH_CFLAGS}"
        echo "Loudmouth LIBS:     ${LOUDMOUTH_LIBS}"
fi     

echo "CAPI Channel module: ${WANT_CHAN_CAPI}"

echo ""

echo "CFLAGS: 		$CFLAGS"
echo "LIBS: 		$LIBS"
echo "LDFLAGS: 		$LDFLAGS"
echo "CPU_OPTIONS: 	${CPU_OPTIONS}"
echo "BUILD_OS: 	${BUILD_OS}"
echo "BUILD_MACHINE: 	${BUILD_MACHINE}"
echo "BUILD_HOST: 	${BUILD_HOST}"
echo ""
echo "prefix:           ${prefix}"
echo "exec_prefix:      ${exec_prefix}"
echo ""
echo "bindir:           ${bindir}"
echo "sbindir:          ${sbindir}"
echo "libdir:           ${libdir}"
echo "sysconfdir:       ${sysconfdir}"
echo "localstatedir:    ${localstatedir}"
echo "datadir:          ${datadir}"
echo "includedir:       ${includedir}"
echo ""
echo "directory-layout: ${with_directory_layout}"
echo ""
echo "cwexecdir:      ${cwexecdir}"
echo "cwutilsdir:     ${cwutilsdir}"
echo "cwconfdir:      ${cwconfdir}"
echo "cwconffile:     ${cwconffile}"
echo "cwlibdir:       ${cwlibdir}"
echo "cwmoddir:       ${cwmoddir}"
echo "cwvardir:       ${cwvardir}"
echo "cwdbdir:        ${cwdbdir}"
echo "cwdbfile:       ${cwdbfile}"
echo "cwtmpdir:       ${cwtmpdir}"
echo "cwrundir:       ${cwrundir}"
echo "cwpidfile:      ${cwpidfile}"
echo "cwsocketfile:   ${cwsocketfile}"
echo "cwlogdir:       ${cwlogdir}"
echo "cwspooldir:     ${cwspooldir}"
echo "cwdatadir:      ${cwdatadir}"
echo "cwmandir:       ${cwmandir}"
echo "cwdocdir:       ${cwdocdir}"
echo "cwkeydir:       ${cwkeydir}"
echo "cwsqlitedir:    ${cwsqlitedir}"
echo "cwogidir:       ${cwogidir}"
echo "cwsoundsdir:    ${cwsoundsdir}"
echo "cwimagesdir:    ${cwimagesdir}"
echo "cwmohdir:       ${cwmohdir}"
echo "cwincludedir:   ${cwincludedir}"
#
# END configure
