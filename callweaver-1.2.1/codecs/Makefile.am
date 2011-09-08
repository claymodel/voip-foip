AUTOMAKE_OPTS		= gnu foreign

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES	= codec_lpc10.la codec_dvi_adpcm.la codec_oki_adpcm.la \
			  codec_ulaw.la codec_alaw.la codec_a_mu.la \
			  codec_g722.la codec_g722_16k_8k.la codec_g726.la \
			  codec_gsm.la

codec_gsm_la_SOURCES	= codec_gsm.c
codec_gsm_la_CFLAGS     = $(AM_CFLAGS)
codec_gsm_la_LIBADD		= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
codec_gsm_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_lpc10_la_SOURCES	= codec_lpc10.c
codec_lpc10_la_CFLAGS   = $(AM_CFLAGS) -I${top_srcdir}/libs/lpc10
codec_lpc10_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la -lm
codec_lpc10_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_dvi_adpcm_la_SOURCES	= codec_dvi_adpcm.c
codec_dvi_adpcm_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_dvi_adpcm_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la 
codec_oki_adpcm_la_SOURCES	= codec_oki_adpcm.c
codec_oki_adpcm_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_oki_adpcm_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la 
codec_ulaw_la_SOURCES	= codec_ulaw.c
codec_ulaw_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_ulaw_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la 
codec_alaw_la_SOURCES	= codec_alaw.c
codec_alaw_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_alaw_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la 
codec_a_mu_la_SOURCES	= codec_a_mu.c
codec_a_mu_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_a_mu_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la 
codec_g722_la_SOURCES   = codec_g722.c
codec_g722_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_g722_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
codec_g722_16k_8k_la_SOURCES   = codec_g722_16k_8k.c
codec_g722_16k_8k_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_g722_16k_8k_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
codec_g726_la_SOURCES   = codec_g726.c
codec_g726_la_LDFLAGS	= -module -avoid-version -no-undefined
codec_g726_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la

if WANT_CODEC_SPEEX
cwmod_LTLIBRARIES	+= codec_speex.la
codec_speex_la_SOURCES	= codec_speex.c
codec_speex_la_LIBADD	= -lspeex ${top_builddir}/corelib/libcallweaver.la
codec_speex_la_CFLAGS	= $(AM_CFLAGS) @SPEEX_CFLAGS@
codec_speex_la_LDFLAGS  = -module -avoid-version -no-undefined
endif WANT_CODEC_SPEEX

INCLUDES = -I$(top_srcdir)/include
