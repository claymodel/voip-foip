#!/usr/bin/env sh
echo ""
echo "CallWeaver bootstrap script to generate autotools build environment"

############
##
## Functions
##
############
debug ()
{
	# Outputs debug statments if DEBUG var is set
	if [ ! -z "$DEBUG" ]; then
		echo "DEBUG: $1"
	fi
}
version_compare()
{
	# Checks a command is found and the version is high enough
	PROGRAM=$1
	MAJOR=$2
	MINOR=$3
	MICRO=$4
	test -z "$MAJOR" && MAJOR=0
	test -z "$MINOR" && MINOR=0
	test -z "$MICRO" && MICRO=0

	debug "Checking $PROGRAM >= $MAJOR.$MINOR.$MICRO"

	WHICH_PATH=`whereis which | cut -f2 -d' '`
	COMMAND=`$WHICH_PATH $PROGRAM`
	if [ -z $COMMAND ]; then
		echo "$PROGRAM-$MAJOR.$MINOR.$MICRO is required and was not found."
		return 1
	else
		debug "Found $COMMAND"
	fi

	INS_VER=`$COMMAND --version | head -1 | sed 's/[^0-9]*//' | cut -d' ' -f1`
	INS_MAJOR=`echo $INS_VER | cut -d. -f1 | sed s/[a-zA-Z\-].*//g`
	INS_MINOR=`echo $INS_VER | cut -d. -f2 | sed s/[a-zA-Z\-].*//g`
	INS_MICRO=`echo $INS_VER | cut -d. -f3 | sed s/[a-zA-Z\-].*//g`
	test -z "$INS_MAJOR" && INS_MAJOR=0
	test -z "$INS_MINOR" && INS_MINOR=0
	test -z "$INS_MICRO" && INS_MICRO=0
	debug "Installed version: $INS_VER"

	if [ "$INS_MAJOR" -gt "$MAJOR" ]; then
		debug "MAJOR: $INS_MAJOR > $MAJOR"
		return 0
	elif [ "$INS_MAJOR" -eq "$MAJOR" ]; then
		debug "MAJOR: $INS_MAJOR = $MAJOR"
		if [ "$INS_MINOR" -gt "$MINOR" ]; then
			debug "MINOR: $INS_MINOR > $MINOR"
			return 0
		elif [ "$INS_MINOR" -eq "$MINOR" ]; then
			if [ "$INS_MICRO" -ge "$MICRO" ]; then
				debug "MICRO: $INS_MICRO >= $MICRO"
				return 0
			else
				debug "MICRO: $INS_MICRO < $MICRO"
			fi
		else
			debug "MINOR: $INS_MINOR < $MINOR"
		fi
	else
		debug "MAJOR: $INS_MAJOR < $MAJOR"
	fi

	echo "You have the wrong version of $PROGRAM. The minimum required version is $MAJOR.$MINOR.$MICRO"
	echo "    and the version installed is $INS_MAJOR.$INS_MINOR.$INS_MICRO ($COMMAND)."
	return 1
}

UNAME=`uname`

###################
##
## FreeBSD Reminder
##
###################

if [ "x$UNAME" = "xFreeBSD" ]; then
    echo ""
    echo "******************************************"
    echo "***              NOTICE                ***"
    echo "******************************************"
    echo ""
    echo "Libtool appears to be having difficulties"
    echo "on FreeBSD. Please use the following"
    echo "workaround to bootstrap on FreeBSD."
    echo ""
    echo "cd /usr/local/share/aclocal19"
    echo "ln -s ../aclocal/libtool15.m4 ."
    echo "ln -s ../aclocal/ltdl15.m4 ."
    echo ""
    echo "******************************************"
fi

##################
##
## NetBSD reminder
##
##################

if [ "x$UNAME" = "xNetBSD" ]; then
    echo ""
    echo "Please remember to run gmake instead of make on NetBSD"
    echo ""
fi

################
##
## Version Check
##
################

echo ""
echo "checking versions of required software ..."
if [ "x$UNAME" = "xFreeBSD" ]; then
version_compare libtoolize 1 5 10 || exit 1
version_compare automake19 1 9 3 || exit 1
version_compare autoconf259 2 59 || exit 1
ACLOCAL=aclocal19
AUTOHEADER=autoheader259
AUTOMAKE=automake19
AUTOCONF=autoconf259
else
version_compare libtoolize 1 5 6 || exit 1
version_compare automake 1 9 2 || exit 1
version_compare autoconf 2 59 || exit 1
ACLOCAL=aclocal
AUTOHEADER=autoheader
AUTOMAKE=automake
AUTOCONF=autoconf
fi
echo "version check completed."

echo ""
echo "the following steps will take some time, please be patient ..."
echo "running libtoolize ..."
libtoolize --copy --force --ltdl
#NetBSD seems to need this file writable
chmod u+w libltdl/configure

echo "running aclocal ..."
$ACLOCAL -I acmacros
echo "running autoheader ..."
$AUTOHEADER --force
echo "running automake ..."
$AUTOMAKE --copy --add-missing
echo "running autoconf ..."
$AUTOCONF --force

cd libltdl
echo "running aclocal in libltdl ..."
$ACLOCAL
echo "running aclocal in libltdl ..."
$AUTOMAKE --copy --add-missing
echo "running autoheader in libltdl ..."
$AUTOHEADER --force
echo "running autoconf in libltdl ..."
$AUTOCONF --force
cd ..

if test ! -f Makefile.in; then
    echo "ERROR: failed to generate Makefile.in"
    exit 1;
fi

#pushd sqlite3-embedded
#libtoolize --copy --force
#$ACLOCAL
#$AUTOCONF --force
#popd

chmod ug+x debian/rules

#################################
##
## POST-PROCESSING OF Makefile.in
##
#################################

echo ""
echo "post-processing Makefile.in for additional targets ..."

echo "adding 'install-man-pages' and 'install-docs'"
mk="\$(MAKE) \$(AM_MAKEFLAGS)"
#echo "mk='$mk'"
mkman="$mk install-man-pages"
#echo "mkman='$mkman'"
mkdoc="$mk install-docs"
#echo "mkdoc='$mkdoc'"

# The following sed regex only worked on Linux, it failed on BSD ...
#sedex="s/^\t[ \t]*\$(MAKE)[ \t]*\$(AM_MAKEFLAGS)[ \t]*install-data-hook$/i\\\t$mkman\n\t$mkdoc"

# The following sed regex works on BSD, it needs to be tested  on Linux ...
sedex="/^[ 	]*\$(MAKE)[ 	]*\$(AM_MAKEFLAGS)[ 	]*install-data-hook$/{
i\\
\	${mkman}\\
\	${mkdoc}
}"

#echo "sedex='$sedex'"
sed -e "$sedex" Makefile.in > Makefile.temp1

echo "adding 'uninstall-man-pages' and 'uninstall-docs'"
sedex="s/uninstall-utilitySCRIPTS$/& uninstall-man-pages uninstall-docs/g"

#echo "sedex='$sedex'"
sed -e "$sedex" Makefile.temp1 > Makefile.temp2

mv -f Makefile.temp2 Makefile.in
rm Makefile.temp1

##################
##
## Notify and exit
##
##################

echo ""
echo "bootstrap complete."

# END OF FILE
