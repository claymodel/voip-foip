AC_DEFUN([AX_SYS_PERLSHARPBANG],[dnl

   AC_PATH_PROG(PERLINTERP,perl,perl)
   ac_cv_path_perlinterp="$PERLINTERP"
   _sHpB='#!'

   AC_ARG_WITH(perl-shebang,
                AC_HELP_STRING([--with-perl-shebang],
           [override what perl thinks is the way for the kernel to start it (seldom needed)]dnl
                           ),
                [opt_perl_shebang="$withval"]dnl
                            ,dnl
                [opt_perl_shebang="not_set"]dnl
    )dnl

   AC_CACHE_CHECK([whether explicit instead of detected sharpbang is to be used],
                   ax_cv_opt_perl_shebang,
                  [ case "$opt_perl_shebang" in
                      not_set  ) ax_cv_opt_perl_shebang=''
                               ;;
                         *     )
        ax_cv_opt_perl_shebang=`echo "$opt_perl_shebang" | sed -e's|^#!\s*\(.*\)$|\1|'`
                    esac
                  ]dnl
    )dnl

   if test "A$ax_cv_opt_perl_shebang" != "A"
     then
       ac_cv_sys_kernshrpbang_perl="$ax_cv_opt_perl_shebang"
       PERL_SHEBANG="$ac_cv_sys_kernshrpbang_perl"
       AC_SUBST(PERL_SHEBANG)dnl
       AC_MSG_NOTICE([OK - PERL_SHEBANG is $_sHpB$PERL_SHEBANG.])

# Automatic detection of sharpbang formula starts here
     else dnl

   _somian_shbangperl=`$PERLINTERP -V:startperl`
   negclass="[[^']]"; dnl
# must leave this comment:  m4 will remove the outer brackets for us, heheh
   AC_CACHE_CHECK([for kernel sharpbang invocation to start perl],
                  ac_cv_sys_kernshrpbang_perl,
        [_somian_kspb_perl=`echo "$_somian_shbangperl" | sed -ne"s|.*='\($negclass*\)';$|\1|p"`
        if test "x$_somian_kspb_perl" = x
          then _somian_ksbp_warn_empty='durnit'
          else
          case "A$_somian_kspb_perl" in
                 A#!*perl* )
           ac_cv_sys_kernshrpbang_perl=`echo "$_somian_kspb_perl" | sed -e's|#!\(.*\)$|\1|'`
                        ;;
                     A*    )  _somian_ksbp_warn_defau='trouble'
                              ac_cv_sys_kernshrpbang_perl="$PERLINTERP"
          esac
        fi
])dnl Done with testing sharpbang

# The above prints Checking ... result message to user.
   PERL_SHEBANG="$ac_cv_sys_kernshrpbang_perl"
   AC_SUBST(PERL_SHEBANG)
    if test A${_somian_ksbp_warn_empty+set} = Aset
      then   AC_MSG_WARN([dnl
In last check, doing $PERLINTERP -V:startperl yielded empty result! That should not happen.])
    fi
# Inform user after printing result value
    if test A${_somian_ksbp_warn_defau+set} = Aset
      then AC_MSG_NOTICE([Maybe Not good -])
           AC_MSG_WARN([dnl
In last check perl's Config query did not work so we bunted: $_sHpB$PERLINTERP])
      else AC_MSG_NOTICE([OK Good result - ])
           AC_MSG_NOTICE([dnl
In last check we got a proper-looking answer from perl's Config: $_somian_shbangperl])
dnl Done with user info messages
    fi
dnl Outer loop checked for user override term here
  fi dnl

])dnl
