AUTOMAKE_OPTS 		= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES = format_au.la format_g723_1.la format_g726.la	\
format_g729.la format_gsm.la format_h263.la format_pcm.la		\
format_pcm_alaw.la format_sln.la format_wav.la format_wav_gsm.la

format_au_la_SOURCES 		= format_au.c
format_au_la_LDFLAGS            = -module -avoid-version -no-undefined
format_au_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la  

format_g723_1_la_SOURCES 	= format_g723_1.c
format_g723_1_la_LDFLAGS        = -module -avoid-version -no-undefined
format_g723_1_la_LIBADD           = ${top_builddir}/corelib/libcallweaver.la  

format_g726_la_SOURCES 		= format_g726.c
format_g726_la_LDFLAGS          = -module -avoid-version -no-undefined
format_g726_la_LIBADD           = ${top_builddir}/corelib/libcallweaver.la  

format_g729_la_SOURCES 		= format_g729.c
format_g729_la_LDFLAGS          = -module -avoid-version -no-undefined
format_g729_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la

format_gsm_la_SOURCES 		= format_gsm.c
format_gsm_la_LDFLAGS           = -module -avoid-version -no-undefined
format_gsm_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la 

format_h263_la_SOURCES 		= format_h263.c
format_h263_la_LDFLAGS          = -module -avoid-version -no-undefined
format_h263_la_LIBADD           = ${top_builddir}/corelib/libcallweaver.la  

if WANT_FORMAT_JPEG
cwmod_LTLIBRARIES		+= format_jpeg.la
format_jpeg_la_SOURCES 		= format_jpeg.c
format_jpeg_la_LDFLAGS          = -module -avoid-version -no-undefined
format_jpeg_la_LIBADD           = ${top_builddir}/corelib/libcallweaver.la  
endif WANT_FORMAT_JPEG

if WANT_FORMAT_OGGVORBIS
cwmod_LTLIBRARIES		+= format_ogg_vorbis.la
format_ogg_vorbis_la_SOURCES 	= format_ogg_vorbis.c
format_ogg_vorbis_la_LDFLAGS     = -module -avoid-version -no-undefined
format_ogg_vorbis_la_LIBADD      = ${top_builddir}/corelib/libcallweaver.la @OGG_LIBS@ @VORBIS_LIBS@
endif WANT_FORMAT_OGGVORBIS

format_pcm_alaw_la_SOURCES 	= format_pcm_alaw.c
format_pcm_alaw_la_LDFLAGS      = -module -avoid-version -no-undefined
format_pcm_alaw_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la  

format_pcm_la_SOURCES 		= format_pcm.c
format_pcm_la_LDFLAGS           = -module -avoid-version -no-undefined
format_pcm_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la  

format_sln_la_SOURCES 		= format_sln.c
format_sln_la_LDFLAGS           = -module -avoid-version -no-undefined
format_sln_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la  

format_wav_gsm_la_SOURCES 	= format_wav_gsm.c msgsm.h
format_wav_gsm_la_LDFLAGS       = -module -avoid-version -no-undefined
format_wav_gsm_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la  

format_wav_la_SOURCES 		= format_wav.c
format_wav_la_LDFLAGS           = -module -avoid-version -no-undefined
format_wav_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la  

INCLUDES = -I$(top_srcdir)/include
