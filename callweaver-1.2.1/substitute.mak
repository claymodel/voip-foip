substitute = $(SED) \
	-e 's,@prefix\@,${prefix},g' \
	-e 's,@exec_prefix\@,${exec_prefix},g' \
	-e 's,@bindir\@,${bindir},g' \
	-e 's,@sbindir\@,${sbindir},g' \
	-e 's,@libdir\@,${libdir},g' \
	-e 's,@sysconfdir\@,${sysconfdir},g' \
	-e 's,@localstatedir\@,${localstatedir},g' \
	-e 's,@datadir\@,${datadir},g' \
	-e 's,@cwexecdir\@,${cwexecdir},g' \
	-e 's,@cwutilsdir\@,${cwutilsdir},g' \
	-e 's,@cwconfdir\@,${cwconfdir},g' \
	-e 's,@cwconffile\@,${cwconffile},g' \
	-e 's,@cwlibdir\@,${cwlibdir},g' \
	-e 's,@cwmoddir\@,${cwmoddir},g' \
	-e 's,@cwvardir\@,${cwvardir},g' \
	-e 's,@cwvardir\@,${cwvardir},g' \
	-e 's,@cwdbdir\@,${cwdbdir},g' \
	-e 's,@cwdbfile\@,${cwdbfile},g' \
	-e 's,@cwtmpdir\@,${cwtmpdir},g' \
	-e 's,@cwrundir\@,${cwrundir},g' \
	-e 's,@cwpidfile\@,${cwpidfile},g' \
	-e 's,@cwsocketfile\@,${cwsocketfile},g' \
	-e 's,@cwlogdir\@,${cwlogdir},g' \
	-e 's,@cwspooldir\@,${cwspooldir},g' \
	-e 's,@cwmohdir\@,${cwmohdir},g' \
	-e 's,@cwdatadir\@,${cwdatadir},g' \
	-e 's,@cwmandir\@,${cwmandir},g' \
	-e 's,@cwdocdir\@,${cwdocdir},g' \
	-e 's,@cwkeydir\@,${cwkeydir},g' \
	-e 's,@cwsqlitedir\@,${cwsqlitedir},g' \
	-e 's,@cwogidir\@,${cwogidir},g' \
	-e 's,@cwsoundsdir\@,${cwsoundsdir},g' \
	-e 's,@cwimagesdir\@,${cwimagesdir},g' \
	-e 's,@cwincludedir\@,${cwincludedir},g' \
	-e 's,@cwrunuser\@,${cwrunuser},g' \
	-e 's,@cwrungroup\@,${cwrungroup},g'
