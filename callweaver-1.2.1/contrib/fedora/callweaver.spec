%define snap 1984

%bcond_without misdn
%bcond_without zaptel

%bcond_without	fedora

Name:		callweaver
Version:	1.2
Release:	2.rc1.svn%{snap}%{?dist}
Summary:	The Truly Open Source PBX

Group:		Applications/Internet
License:	GPL
URL:		http://www.callweaver.org/
# svn co -r %{snap} svn://svn.callweaver.org/callweaver/trunk callweaver
# tar cvfz callweaver-r%{snap}.tar.gz callweaver
Source0:	callweaver-r%{snap}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	spandsp-devel >= 0.0.3-1.pre24
BuildRequires:	%{?snap:libtool automake autoconf} zlib-devel
BuildRequires:	fedora-usermgmt-devel bluez-libs-devel openldap-devel
BuildRequires:	libjpeg-devel loudmouth-devel nspr-devel js-devel ncurses-devel
BuildRequires:	unixODBC-devel openssl-devel speex-devel alsa-lib-devel
BuildRequires:	isdn4k-utils-devel libcap-devel sqlite-devel mysql-devel
BuildRequires:	postgresql-devel libedit-devel %{?with_misdn:mISDN-devel}
BuildRequires:	popt %{?with_zaptel:zaptel-devel libpri-devel}

Requires:	/sbin/chkconfig
%{?FE_USERADD_REQ}

%description
CallWeaver is an Open Source PBX and telephony development platform that
can both replace a conventional PBX and act as a platform for developing
custom telephony applications for delivering dynamic content over a
telephone similarly to how one can deliver dynamic content through a
web browser using CGI and a web server.

CallWeaver talks to a variety of telephony hardware including BRI, PRI,
POTS, Bluetooth headsets and IP telephony clients using SIP and IAX
protocols protocol (e.g. ekiga or kphone).  For more information and a
current list of supported hardware, see www.callweaver.org.


%package devel
Group:		Applications/Internet
Summary:	Development package for %{name}
Requires:	%{name} = %{version}-%{release}

%description devel
Use this package for developing and building modules for CallWeaver


%package postgresql
Group:		Applications/Internet
Summary:	PostgreSQL support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description postgresql
This package contains modules for CallWeaver which make use of PostgreSQL.

%package mysql
Group:		Applications/Internet
Summary:	MySQL support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description mysql
This package contains modules for CallWeaver which make use of MySQL.

%package odbc
Group:		Applications/Internet
Summary:	ODBC support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description odbc
This package contains modules for CallWeaver which make use of ODBC.

%package ldap
Group:		Applications/Internet
Summary:	LDAP support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description ldap
This package contains modules for CallWeaver which make use of LDAP.

%package bluetooth
Group:		Applications/Internet
Summary:	Bluetooth channel driver for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description bluetooth
This package contains a Bluetooth channel driver for CallWeaver, which
allows Bluetooth headsets and telephones to be used for communication.

%package capi
Group:		Applications/Internet
Summary:	CAPI channel driver for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description capi
This package contains a CAPI (Common ISDN API) channel driver for
CallWeaver, allowing CAPI devices to be used for making and receiving calls.

%package misdn
Group:		Applications/Internet
Summary:	mISDN channel driver for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description misdn
This package contains the mISDN channel driver for CallWeaver. mISDN is
the replacement modular ISDN stack for Linux, intended to be merged
into the 2.6 kernel.

%package zaptel
Group:		Applications/Internet
Summary:	Zaptel modules for CallWeaver
Requires:	%{name} = %{version}-%{release}
Requires:	zaptel >= 1.4.0-1

%description zaptel
This package contains the Zap channel driver and MeetMe application
for CallWeaver. Zaptel is Digium's telephony driver package.

%package jabber
Group:		Applications/Internet
Summary:	Jabber support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description jabber
This package contains Jabber protocol support for CallWeaver.

%package javascript
Group:		Applications/Internet
Summary:	JavaScript support for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description javascript
This package contains JavaScript support for CallWeaver.

%package alsa
Group:		Applications/Internet
Summary:	ALSA channel driver for CallWeaver
Requires:	%{name} = %{version}-%{release}

%description alsa
This package contains an ALSA console driver for CallWeaver, which allows
the local sound devices to be used for making and receiving calls.

%package ogi
Group:		Applications/Internet
Summary:	CallWeaver Gateway Interface
Requires:	%{name} = %{version}-%{release}

%description ogi
This package contains files support for the CallWeaver Gateway Interface; a
convenient interface between CallWeaver and external scripts or programs.


%prep
%setup -q -n callweaver

%build
%if 0%{?snap}
./bootstrap.sh
%endif
# res_sqlite seems to use internal functions of sqlite3 which don't 
# even _exist_ in current versions. Disable it until it's fixed.

%configure --with-directory-layout=lsb --with-chan_bluetooth \
	   --with-chan_fax --with-chan_capi --with-chan_alsa --with-app_ldap \
	   --disable-zaptel --enable-t38 --enable-postgresql --with-cdr-pgsql \
	   --with-res_config_pqsql --with-cdr-odbc --with-res_config_odbc \
	   --with-perl-shebang='#! /usr/bin/perl' --disable-builtin-sqlite3 \
	   --enable-javascript --with-res_js --enable-fast-install \
	   %{?with_misdn:--with-chan_misdn} \
	   %{?with_zaptel:--enable-zaptel} \
	   --enable-jabber --with-res_jabber \
	   --enable-mysql --with-res_config_mysql --with-cdr_mysql
	   

# Poxy fscking libtool is _such_ a pile of crap...
sed -i 's/^CC="gcc"/CC="gcc -Wl,--as-needed"/' libtool

# Poxy fscking autocrap isn't much better
sed -i 's:^/usr/bin/perl:#! /usr/bin/perl:' ogi/fastogi-test ogi/ogi-test.ogi

make %{?_smp_mflags} CCLD="gcc -Wl,--as-needed"

mkdir doc2
mv doc/README.{misdn,chan_capi,res_jabber,odbcstorage} doc2

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT/%{_libdir}/callweaver/modules/*.la
rm -f $RPM_BUILD_ROOT/%{_libdir}/callweaver/*.a
rm -f $RPM_BUILD_ROOT/%{_libdir}/callweaver/*.la
mkdir -p $RPM_BUILD_ROOT%{_initrddir}
install -m0755 contrib/fedora/callweaver $RPM_BUILD_ROOT%{_initrddir}/callweaver
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d
install -m0644 contrib/fedora/callweaver.logrotate $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/callweaver

mv $RPM_BUILD_ROOT/%{_datadir}/callweaver/ogi/eogi-*test $RPM_BUILD_ROOT/%{_sbindir}

# More autocrap insanity. We can't just move confdefs.h into the callweaver/ subdir
# because then autocrap will add that subdir to the compiler's include path and
# many things break. So let's just clean up after it.
sed -i 's:"confdefs.h":<callweaver/confdefs.h>:' $RPM_BUILD_ROOT/%{_includedir}/callweaver/*.h
install -m0644 include/confdefs.h $RPM_BUILD_ROOT/%{_includedir}/callweaver/confdefs.h

%clean
rm -rf $RPM_BUILD_ROOT

%pre
%__fe_groupadd 30 -r callweaver &>/dev/null || :
%__fe_useradd  30 -r -s /sbin/nologin -d %{_sysconfdir}/callweaver -M \
                    -c 'CallWeaver user' -g callweaver callweaver &>/dev/null || :
%post
/sbin/chkconfig --add callweaver

%preun
test "$1" != 0 || /sbin/chkconfig --del callweaver

%postun
%__fe_userdel  callweaver &>/dev/null || :
%__fe_groupdel callweaver &>/dev/null || :

%pre zaptel
%{_sbindir}/usermod -G `id -G callweaver | tr " " , `,zaptel callweaver

%pre misdn
%{_sbindir}/usermod -G `id -G callweaver | tr " " , `,misdn callweaver

%files
%defattr(-,root,root,-)
%doc COPYING CREDITS LICENSE BUGS AUTHORS SECURITY README HARDWARE
%doc doc/README.* doc/*.txt doc/*.html sample.call ChangeLog
%doc BUGS InstallGuide.txt
%config(noreplace) %{_sysconfdir}/logrotate.d/callweaver
%{_initrddir}/callweaver
%{_sbindir}/callweaver
%{_bindir}/smsq
%{_bindir}/streamplayer
%dir %{_libdir}/callweaver
%{_libdir}/callweaver/lib*.so.*
%dir %{_libdir}/callweaver/modules
%{_libdir}/callweaver/modules/*.so
%{_mandir}/man8/callweaver.8.gz
%dir %{_datadir}/callweaver
%dir %attr(0755,callweaver,callweaver) %{_sysconfdir}/callweaver
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/*
%attr(2755,callweaver,callweaver) %{_localstatedir}/spool/callweaver
%attr(0755,callweaver,callweaver) %{_localstatedir}/log/callweaver
%attr(0755,callweaver,callweaver) %{_localstatedir}/run/callweaver
# Unneeded
%exclude %{_sysconfdir}/callweaver/cdr_tds.conf
# Separately packaged
%exclude %{_libdir}/callweaver/modules/*pgsql.so
%exclude %{_libdir}/callweaver/modules/*mysql.so
%exclude %{_libdir}/callweaver/modules/app_sql_postgres.so
%exclude %{_libdir}/callweaver/modules/app_ldap.so
%exclude %{_libdir}/callweaver/modules/cdr_odbc.so
%exclude %{_libdir}/callweaver/modules/chan_bluetooth.so
%exclude %{_libdir}/callweaver/modules/res_jabber.so
%exclude %{_libdir}/callweaver/modules/res_js.so
%exclude %{_libdir}/callweaver/modules/chan_alsa.so
%exclude %{_libdir}/callweaver/modules/res_ogi.so
%exclude %{_libdir}/callweaver/modules/chan_capi.so
%exclude %{_sysconfdir}/callweaver/cdr_pgsql.conf
%exclude %{_sysconfdir}/callweaver/*mysql.conf
%exclude %{_sysconfdir}/callweaver/cdr_odbc.conf
%exclude %{_sysconfdir}/callweaver/chan_bluetooth.conf
%exclude %{_sysconfdir}/callweaver/res_jabber.conf
%exclude %{_sysconfdir}/callweaver/alsa.conf
%exclude %{_sysconfdir}/callweaver/capi.conf
%if 0%{?with_misdn:1}
%exclude %{_libdir}/callweaver/modules/chan_misdn.so
%exclude %{_sysconfdir}/callweaver/misdn.conf
%endif
%if 0%{?with_zaptel:1}
%exclude %{_libdir}/callweaver/modules/chan_zap.so
%exclude %{_libdir}/callweaver/modules/app_meetme.so
%exclude %{_libdir}/callweaver/modules/app_flash.so
%exclude %{_sysconfdir}/callweaver/zapata.conf
%exclude %{_sysconfdir}/callweaver/meetme.conf
%endif

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/callweaver
%{_includedir}/callweaver/*.h
%{_libdir}/callweaver/lib*.so

%files postgresql
%{_libdir}/callweaver/modules/*pgsql.so
%{_libdir}/callweaver/modules/app_sql_postgres.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/cdr_pgsql.conf

%files mysql
%{_libdir}/callweaver/modules/*mysql.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/*mysql.conf

%files odbc
%{_libdir}/callweaver/modules/cdr_odbc.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/cdr_odbc.conf
%doc doc2/README.odbcstorage

%files ldap
%{_libdir}/callweaver/modules/app_ldap.so

%files bluetooth
%{_libdir}/callweaver/modules/chan_bluetooth.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/chan_bluetooth.conf

%files capi
%{_libdir}/callweaver/modules/chan_capi.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/capi.conf
%doc doc2/README.chan_capi

%if 0%{?with_misdn:1}
%files misdn
%{_libdir}/callweaver/modules/chan_misdn.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/misdn.conf
%doc doc2/README.misdn
%endif

%if 0%{?with_zaptel:1}
%files zaptel
%{_libdir}/callweaver/modules/chan_zap.so
%{_libdir}/callweaver/modules/app_meetme.so
%{_libdir}/callweaver/modules/app_flash.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/zapata.conf
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/meetme.conf
%endif

%files jabber
%{_libdir}/callweaver/modules/res_jabber.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/res_jabber.conf
%doc doc2/README.res_jabber

%files javascript
%{_libdir}/callweaver/modules/res_js.so

%files alsa
%{_libdir}/callweaver/modules/chan_alsa.so
%config(noreplace) %attr(0644,callweaver,callweaver) %{_sysconfdir}/callweaver/alsa.conf

%files ogi
%{_libdir}/callweaver/modules/res_ogi.so
%dir %attr(0755,root,root) %{_datadir}/callweaver/ogi
%{_datadir}/callweaver/ogi/*
%{_sbindir}/eogi*

%changelog
* Wed Oct 18 2006 David Woodhouse <dwmw2@infradead.org> 1.2-2.rc1.svn1984
- Build with mISDN and MySQL support

* Thu Oct  5 2006 David Woodhouse <dwmw2@infradead.org> 1.2-1.rc1.svn1979
- Initial build
