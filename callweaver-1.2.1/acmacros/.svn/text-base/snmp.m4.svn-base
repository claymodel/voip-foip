#
# Check for libnetsnmp
#
AC_DEFUN([CHECK_SNMP],
[AC_MSG_CHECKING(if net_snmp is wanted)
# AC_ARG_WITH(snmp,
# [  --with-net_snmp enable snmp [will check /usr /usr/local ]
# ],
#[   AC_MSG_RESULT(yes)
    for dir in $withval /usr/local /usr ; do
        snmpdir="$dir"

	if test -f "$dir/include/net-snmp/net-snmp-config.h"; then
		if test -f "$dir/include/net-snmp/net-snmp-includes.h"; then
			if test -f "$dir/include/net-snmp/agent/net-snmp-agent-includes.h"; then
				found_snmp="yes";
				break;
			fi
		fi
        fi
    done
    if test x_$found_snmp != x_yes; then
        AC_MSG_ERROR(Cannot find snmp libraries - these are required. Please install the libsnmp-dev package)
    else
        printf "Net-SNMP found in $snmpdir\n";
        SNMP_LIBS="-L$snmpdir/lib -lsnmp -lcrypto";
        HAVE_SNMP=yes
    fi
    AC_SUBST([HAVE_SNMP])
#],
#[
#    AC_MSG_RESULT(no)
#])
])dnl
