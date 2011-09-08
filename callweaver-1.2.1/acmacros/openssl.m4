#
# Check SSL
#
AC_DEFUN([CHECK_SSL], [

    for dir in $withval /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr /usr/sfw; do
        ssldir="$dir"
        if test -f "$dir/include/openssl/ssl.h"; then
            found_ssl="yes";
            SSL_CFLAGS="$CFLAGS -I$ssldir/include/openssl -DHAVE_SSL";
            SSL_CXXFLAGS="$CXXFLAGS -I$ssldir/include/openssl -DHAVE_SSL";
            break;
        fi
        if test -f "$dir/include/ssl.h"; then
            found_ssl="yes";
            SSL_CFLAGS="-I$ssldir/include/ -DHAVE_SSL";
            SSL_CXXFLAGS="-I$ssldir/include/ -DHAVE_SSL";
            break
        fi
    done
    if test x_$found_ssl != x_yes; then
        AC_MSG_ERROR(Cannot find ssl libraries - these are required. Please install the openssl-devel package)
    else
        printf "OpenSSL found in $ssldir\n";
        SSL_LIBS="-L$ssldir/lib -lssl -lcrypto";
        HAVE_SSL=yes
    fi
    AC_SUBST([HAVE_SSL])
    AC_SUBST([SSL_LIBS])
    AC_SUBST([SSL_CFLAGS])

])dnl
