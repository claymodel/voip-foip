AUTOMAKE_OPTS			= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES		= 
SUBDIRS 			= nconference

# We don't want ICD in 1.2 before it's stable
#
#if WANT_APP_ICD
#SUBDIRS += icd
#endif WANT_APP_ICD


if WANT_APP_ADSIPROG
cwmod_LTLIBRARIES			+= app_adsiprog.la
app_adsiprog_la_SOURCES			= app_adsiprog.c
app_adsiprog_la_LDFLAGS			= -module -avoid-version -no-undefined
app_adsiprog_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_ADSIPROG

if WANT_APP_PIPE
cwmod_LTLIBRARIES        += app_pipe.la
app_pipe_la_SOURCES       = app_pipe.c
app_pipe_la_LDFLAGS       = -module -avoid-version -no-undefined
app_pipe_la_LIBADD        = ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_PIPE

if WANT_APP_AMD
cwmod_LTLIBRARIES			+= app_amd.la
app_amd_la_SOURCES			= app_amd.c
app_amd_la_LDFLAGS			= -module -avoid-version -no-undefined
app_amd_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_AMD

if WANT_APP_AUTHENTICATE
cwmod_LTLIBRARIES			+= app_authenticate.la
app_authenticate_la_SOURCES		= app_authenticate.c
app_authenticate_la_LDFLAGS		= -module -avoid-version -no-undefined
app_authenticate_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_AUTHENTICATE

if WANT_APP_BACKTICKS
cwmod_LTLIBRARIES			+= app_backticks.la
app_backticks_la_SOURCES		= app_backticks.c
app_backticks_la_LDFLAGS		= -module -avoid-version -no-undefined
app_backticks_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_BACKTICKS


if WANT_APP_CDR
cwmod_LTLIBRARIES			+= app_cdr.la
app_cdr_la_SOURCES			= app_cdr.c
app_cdr_la_LDFLAGS			= -module -avoid-version -no-undefined
app_cdr_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_CDR

if WANT_APP_CHANGRAB
cwmod_LTLIBRARIES			+= app_changrab.la
app_changrab_la_SOURCES                 = app_changrab.c
app_changrab_la_LDFLAGS                 = -module -avoid-version -no-undefined
app_changrab_la_LIBADD                  = ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_CHANGRAB

if WANT_APP_CHANISAVAIL
cwmod_LTLIBRARIES			+= app_chanisavail.la
app_chanisavail_la_SOURCES		= app_chanisavail.c
app_chanisavail_la_LDFLAGS		= -module -avoid-version -no-undefined
app_chanisavail_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_CHANISAVAIL

if WANT_APP_CHANSPY
cwmod_LTLIBRARIES			+= app_chanspy.la
app_chanspy_la_SOURCES			= app_chanspy.c
app_chanspy_la_LDFLAGS			= -module -avoid-version -no-undefined
app_chanspy_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_CHANSPY

if WANT_APP_CONTROLPLAYBACK
cwmod_LTLIBRARIES			+= app_controlplayback.la
app_controlplayback_la_SOURCES		= app_controlplayback.c
app_controlplayback_la_LDFLAGS		= -module -avoid-version -no-undefined
app_controlplayback_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_CONTROLPLAYBACK

if WANT_APP_DB
cwmod_LTLIBRARIES			+= app_db.la
app_db_la_SOURCES			= app_db.c
app_db_la_LDFLAGS			= -module -avoid-version -no-undefined
app_db_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DB

if WANT_APP_DEVSTATE
cwmod_LTLIBRARIES			+= app_devstate.la
app_devstate_la_SOURCES			= app_devstate.c
app_devstate_la_LDFLAGS			= -module -avoid-version -no-undefined
app_devstate_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DEVSTATE

if WANT_APP_DIAL
cwmod_LTLIBRARIES			+= app_dial.la
app_dial_la_SOURCES			= app_dial.c
app_dial_la_LDFLAGS			= -module -avoid-version -no-undefined
app_dial_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DIAL

if WANT_APP_DICTATE
cwmod_LTLIBRARIES			+= app_dictate.la
app_dictate_la_SOURCES			= app_dictate.c
app_dictate_la_LDFLAGS			= -module -avoid-version -no-undefined
app_dictate_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DICTATE

if WANT_APP_DIRECTED_PICKUP
cwmod_LTLIBRARIES			+= app_directed_pickup.la
app_directed_pickup_la_SOURCES		= app_directed_pickup.c
app_directed_pickup_la_LDFLAGS		= -module -avoid-version -no-undefined
app_directed_pickup_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DIRECTED_PICKUP

if WANT_APP_DIRECTORY
cwmod_LTLIBRARIES			+= app_directory.la
app_directory_la_SOURCES		= app_directory.c
app_directory_la_LDFLAGS		= -module -avoid-version -no-undefined
app_directory_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DIRECTORY

if WANT_APP_DISA
cwmod_LTLIBRARIES			+= app_disa.la
app_disa_la_SOURCES			= app_disa.c
app_disa_la_LDFLAGS			= -module -avoid-version -no-undefined
app_disa_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DISA

if WANT_APP_DUMPCHAN
cwmod_LTLIBRARIES			+= app_dumpchan.la
app_dumpchan_la_SOURCES			= app_dumpchan.c
app_dumpchan_la_LDFLAGS			= -module -avoid-version -no-undefined
app_dumpchan_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DUMPCHAN

if WANT_APP_ECHO
cwmod_LTLIBRARIES			+= app_echo.la
app_echo_la_SOURCES			= app_echo.c
app_echo_la_LDFLAGS			= -module -avoid-version -no-undefined
app_echo_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_ECHO

if WANT_APP_ENUMLOOKUP
cwmod_LTLIBRARIES			+= app_enumlookup.la
app_enumlookup_la_SOURCES		= app_enumlookup.c
app_enumlookup_la_LDFLAGS		= -module -avoid-version -no-undefined
app_enumlookup_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_ENUMLOOKUP

if WANT_APP_EVAL
cwmod_LTLIBRARIES			+= app_eval.la
app_eval_la_SOURCES			= app_eval.c
app_eval_la_LDFLAGS			= -module -avoid-version -no-undefined
app_eval_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_EVAL

if WANT_APP_EXEC
cwmod_LTLIBRARIES			+= app_exec.la
app_exec_la_SOURCES			= app_exec.c
app_exec_la_LDFLAGS			= -module -avoid-version -no-undefined
app_exec_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_EXEC

if WANT_APP_FLASH
cwmod_LTLIBRARIES			+= app_flash.la
app_flash_la_SOURCES			= app_flash.c
app_flash_la_LDFLAGS			= -module -avoid-version -no-undefined
app_flash_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_FLASH

if WANT_APP_FORKCDR
cwmod_LTLIBRARIES			+= app_forkcdr.la
app_forkcdr_la_SOURCES			= app_forkcdr.c
app_forkcdr_la_LDFLAGS			= -module -avoid-version -no-undefined
app_forkcdr_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_FORKCDR

if WANT_APP_GETCPEID
cwmod_LTLIBRARIES			+= app_getcpeid.la
app_getcpeid_la_SOURCES			= app_getcpeid.c
app_getcpeid_la_LDFLAGS			= -module -avoid-version -no-undefined
app_getcpeid_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_GETCPEID

if WANT_APP_GROUPCOUNT
cwmod_LTLIBRARIES			+= app_groupcount.la
app_groupcount_la_SOURCES		= app_groupcount.c
app_groupcount_la_LDFLAGS		= -module -avoid-version -no-undefined
app_groupcount_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_GROUPCOUNT

if WANT_APP_HASNEWVOICEMAIL
cwmod_LTLIBRARIES			+= app_hasnewvoicemail.la
app_hasnewvoicemail_la_SOURCES		= app_hasnewvoicemail.c
app_hasnewvoicemail_la_LDFLAGS		= -module -avoid-version -no-undefined
app_hasnewvoicemail_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_HASNEWVOICEMAIL

if WANT_APP_LOOKUPBLACKLIST
cwmod_LTLIBRARIES			+= app_lookupblacklist.la
app_lookupblacklist_la_SOURCES		= app_lookupblacklist.c
app_lookupblacklist_la_LDFLAGS		= -module -avoid-version -no-undefined
app_lookupblacklist_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_LOOKUPBLACKLIST

if WANT_APP_LOOKUPCIDNAME
cwmod_LTLIBRARIES			+= app_lookupcidname.la
app_lookupcidname_la_SOURCES		= app_lookupcidname.c
app_lookupcidname_la_LDFLAGS		= -module -avoid-version -no-undefined
app_lookupcidname_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_LOOKUPCIDNAME

if WANT_APP_PROC
cwmod_LTLIBRARIES			+= app_proc.la
app_proc_la_SOURCES			= app_proc.c
app_proc_la_LDFLAGS			= -module -avoid-version -no-undefined
app_proc_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_PROC

if WANT_APP_MILLIWATT
cwmod_LTLIBRARIES			+= app_milliwatt.la
app_milliwatt_la_SOURCES		= app_milliwatt.c
app_milliwatt_la_LDFLAGS		= -module -avoid-version -no-undefined
app_milliwatt_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_MILLIWATT

if WANT_APP_MEETME
cwmod_LTLIBRARIES			+= app_meetme.la
app_meetme_la_SOURCES			= app_meetme.c enter.h leave.h
app_meetme_la_LDFLAGS			= -module -avoid-version -no-undefined
app_meetme_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_MEETME

if WANT_APP_MUXMON
cwmod_LTLIBRARIES			+= app_muxmon.la
app_muxmon_la_SOURCES			= app_muxmon.c
app_muxmon_la_LDFLAGS			= -module -avoid-version -no-undefined
app_muxmon_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_MUXMON

if WANT_OSP
if WANT_APP_OSPLOOKUP
cwmod_LTLIBRARIES			+= app_osplookup.la
app_osplookup_la_SOURCES		= app_osplookup.c
app_osplookup_la_CFLAGS			= $(AM_CFLAGS) ${SSL_CFLAGS}
app_osplookup_la_LDFLAGS		= -module -avoid-version -no-undefined
app_osplookup_la_LIBADD			= ${SSL_LIBS} ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_OSPLOOKUP
endif WANT_OSP

if WANT_APP_PAGE
cwmod_LTLIBRARIES			+= app_page.la
app_page_la_SOURCES			= app_page.c
app_page_la_LDFLAGS			= -module -avoid-version -no-undefined
app_page_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_PAGE

if WANT_APP_PLAYBACK
cwmod_LTLIBRARIES			+= app_playback.la
app_playback_la_SOURCES			= app_playback.c
app_playback_la_LDFLAGS			= -module -avoid-version -no-undefined
app_playback_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_PLAYBACK

if WANT_APP_PRIVACY
cwmod_LTLIBRARIES			+= app_privacy.la
app_privacy_la_SOURCES			= app_privacy.c
app_privacy_la_LDFLAGS			= -module -avoid-version -no-undefined
app_privacy_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_PRIVACY

if WANT_APP_QUEUE
cwmod_LTLIBRARIES			+= app_queue.la
app_queue_la_SOURCES			= app_queue.c
app_queue_la_LDFLAGS			= -module -avoid-version -no-undefined
app_queue_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_QUEUE

if WANT_APP_RANDOM
cwmod_LTLIBRARIES			+= app_random.la
app_random_la_SOURCES			= app_random.c
app_random_la_LDFLAGS			= -module -avoid-version -no-undefined
app_random_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_RANDOM

if WANT_APP_READ
cwmod_LTLIBRARIES			+= app_read.la
app_read_la_SOURCES			= app_read.c
app_read_la_LDFLAGS			= -module -avoid-version -no-undefined
app_read_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_READ

if WANT_APP_RECORD
cwmod_LTLIBRARIES			+= app_record.la
app_record_la_SOURCES			= app_record.c
app_record_la_LDFLAGS			= -module -avoid-version -no-undefined
app_record_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_RECORD

if WANT_APP_RXFAX
cwmod_LTLIBRARIES			+= app_rxfax.la
app_rxfax_la_SOURCES			= app_rxfax.c
app_rxfax_la_LDFLAGS			= -module -avoid-version -no-undefined
app_rxfax_la_LIBADD			= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_RXFAX

if WANT_APP_SAYUNIXTIME
cwmod_LTLIBRARIES			+= app_sayunixtime.la
app_sayunixtime_la_SOURCES		= app_sayunixtime.c
app_sayunixtime_la_LDFLAGS		= -module -avoid-version -no-undefined
app_sayunixtime_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SAYUNIXTIME

if WANT_APP_SENDDTMF
cwmod_LTLIBRARIES			+= app_senddtmf.la
app_senddtmf_la_SOURCES			= app_senddtmf.c
app_senddtmf_la_LDFLAGS			= -module -avoid-version -no-undefined
app_senddtmf_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SENDDTMF

if WANT_APP_SENDTEXT
cwmod_LTLIBRARIES			+= app_sendtext.la
app_sendtext_la_SOURCES			= app_sendtext.c
app_sendtext_la_LDFLAGS			= -module -avoid-version -no-undefined
app_sendtext_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SENDTEXT

if WANT_APP_SETCDRUSERFIELD
cwmod_LTLIBRARIES			+= app_setcdruserfield.la
app_setcdruserfield_la_SOURCES		= app_setcdruserfield.c
app_setcdruserfield_la_LDFLAGS		= -module -avoid-version -no-undefined
app_setcdruserfield_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SETCDRUSERFIELD

if WANT_APP_SETCALLERPRES
cwmod_LTLIBRARIES			+= app_setcallerpres.la
app_setcallerpres_la_SOURCES		= app_setcallerpres.c
app_setcallerpres_la_LDFLAGS		= -module -avoid-version -no-undefined
app_setcallerpres_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SETCALLERPRES

if WANT_APP_SETRDNIS
cwmod_LTLIBRARIES			+= app_setrdnis.la
app_setrdnis_la_SOURCES			= app_setrdnis.c
app_setrdnis_la_LDFLAGS			= -module -avoid-version -no-undefined
app_setrdnis_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SETRDNIS

if WANT_APP_SETTRANSFERCAPABILITY
cwmod_LTLIBRARIES			+= app_settransfercapability.la
app_settransfercapability_la_SOURCES	= app_settransfercapability.c
app_settransfercapability_la_LDFLAGS	= -module -avoid-version -no-undefined
app_settransfercapability_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SETTRANSFERCAPABILITY

if WANT_APP_SOFTHANGUP
cwmod_LTLIBRARIES			+= app_softhangup.la
app_softhangup_la_SOURCES		= app_softhangup.c
app_softhangup_la_LDFLAGS		= -module -avoid-version -no-undefined
app_softhangup_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SOFTHANGUP

if WANT_APP_SQL_MYSQL
cwmod_LTLIBRARIES                     += app_sql_mysql.la
app_sql_mysql_la_SOURCES		= app_sql_mysql.c
app_sql_mysql_la_CFLAGS			= $(AM_CFLAGS) @MYSQL_CFLAGS@ 
app_sql_mysql_la_LDFLAGS		= -module -avoid-version -no-undefined
app_sql_mysql_la_LIBADD			= @MYSQL_LIBS@ ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SQL_MYSQL

if WANT_APP_SQL_POSTGRES
cwmod_LTLIBRARIES                     += app_sql_postgres.la
app_sql_postgres_la_SOURCES             = app_sql_postgres.c
app_sql_postgres_la_CFLAGS		= $(AM_CFLAGS) @PQINCPATH@ 
app_sql_postgres_la_LDFLAGS             = -module -avoid-version -no-undefined
app_sql_postgres_la_LIBADD              = @PQLIBPATH@ -lpq ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SQL_POSTGRES

if WANT_APP_STACK
cwmod_LTLIBRARIES			+= app_stack.la
app_stack_la_SOURCES			= app_stack.c
app_stack_la_LDFLAGS			= -module -avoid-version -no-undefined
app_stack_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_STACK

if WANT_APP_SYSTEM
cwmod_LTLIBRARIES			+= app_system.la
app_system_la_SOURCES			= app_system.c
app_system_la_LDFLAGS			= -module -avoid-version -no-undefined
app_system_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SYSTEM

if WANT_APP_BACKGROUNDDETECT
cwmod_LTLIBRARIES			+= app_backgrounddetect.la
app_backgrounddetect_la_SOURCES		= app_backgrounddetect.c
app_backgrounddetect_la_LDFLAGS		= -module -avoid-version -no-undefined
app_backgrounddetect_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_BACKGROUNDDETECT

if WANT_APP_T38GATEWAY
cwmod_LTLIBRARIES			+= app_t38gateway.la
app_t38gateway_la_SOURCES	= app_t38gateway.c
app_t38gateway_la_LDFLAGS	= -module -avoid-version -no-undefined
app_t38gateway_la_LIBADD	= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_T38GATEWAY

if WANT_APP_TRANSFER
cwmod_LTLIBRARIES			+= app_transfer.la
app_transfer_la_SOURCES			= app_transfer.c
app_transfer_la_LDFLAGS			= -module -avoid-version -no-undefined
app_transfer_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_TRANSFER

if WANT_APP_TXFAX
cwmod_LTLIBRARIES			+= app_txfax.la
app_txfax_la_SOURCES			= app_txfax.c
app_txfax_la_LDFLAGS			= -module -avoid-version -no-undefined
app_txfax_la_LIBADD			= -lspandsp -ltiff ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_TXFAX

if WANT_APP_USEREVENT
cwmod_LTLIBRARIES			+= app_userevent.la
app_userevent_la_SOURCES		= app_userevent.c
app_userevent_la_LDFLAGS		= -module -avoid-version -no-undefined
app_userevent_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_USEREVENT

if WANT_APP_VALETPARKING
cwmod_LTLIBRARIES			+= app_valetparking.la
app_valetparking_la_SOURCES		= app_valetparking.c
app_valetparking_la_LDFLAGS		= -module -avoid-version -no-undefined
app_valetparking_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_VALETPARKING

if WANT_APP_VISDN_PPP
cwmod_LTLIBRARIES			+= app_visdn_ppp.la
app_visdn_ppp_la_SOURCES		= app_visdn_ppp.c
app_visdn_ppp_la_LDFLAGS		= -module -avoid-version -no-undefined
app_visdn_ppp_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_VISDN_PPP

if WANT_APP_VERBOSE
cwmod_LTLIBRARIES			+= app_verbose.la
app_verbose_la_SOURCES			= app_verbose.c
app_verbose_la_LDFLAGS			= -module -avoid-version -no-undefined
app_verbose_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_VERBOSE

if WANT_APP_VOICEMAIL
cwmod_LTLIBRARIES			+= app_voicemail.la
app_voicemail_la_SOURCES		= app_voicemail.c
app_voicemail_la_LDFLAGS		= -module -avoid-version -no-undefined
app_voicemail_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_VOICEMAIL

if WANT_APP_WAITFORDIGITS
cwmod_LTLIBRARIES			+= app_waitfordigits.la
app_waitfordigits_la_SOURCES		= app_waitfordigits.c
app_waitfordigits_la_LDFLAGS		= -module -avoid-version -no-undefined
app_waitfordigits_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_WAITFORDIGITS

if WANT_APP_WAITFORSILENCE
cwmod_LTLIBRARIES			+= app_waitforsilence.la
app_waitforsilence_la_SOURCES		= app_waitforsilence.c
app_waitforsilence_la_LDFLAGS		= -module -avoid-version -no-undefined
app_waitforsilence_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_WAITFORSILENCE

if WANT_APP_WAITFORRING
cwmod_LTLIBRARIES			+= app_waitforring.la
app_waitforring_la_SOURCES		= app_waitforring.c
app_waitforring_la_LDFLAGS		= -module -avoid-version -no-undefined
app_waitforring_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_WAITFORRING

if WANT_APP_WHILE
cwmod_LTLIBRARIES			+= app_while.la
app_while_la_SOURCES			= app_while.c
app_while_la_LDFLAGS			= -module -avoid-version -no-undefined
app_while_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_WHILE

if WANT_APP_ZAPATELLER
cwmod_LTLIBRARIES			+= app_zapateller.la
app_zapateller_la_SOURCES		= app_zapateller.c
app_zapateller_la_LDFLAGS		= -module -avoid-version -no-undefined
app_zapateller_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_ZAPATELLER

if WANT_APP_LDAP
cwmod_LTLIBRARIES			+= app_ldap.la
app_ldap_la_SOURCES			= app_ldap.c
app_ldap_la_LDFLAGS			= -module -avoid-version -no-undefined
app_ldap_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la -lldap -llber
app_ldap_la_CFLAGS			= $(AM_CFLAGS) -DLDAP_DEPRECATED
endif WANT_APP_LDAP

if WANT_APP_GETDEVSTATE
cwmod_LTLIBRARIES			+= app_getdevstate.la
app_getdevstate_la_SOURCES		= app_getdevstate.c
app_getdevstate_la_LDFLAGS		= -module -avoid-version -no-undefined
app_getdevstate_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_GETDEVSTATE

if WANT_APP_GETEXTSTATE
cwmod_LTLIBRARIES			+= app_getextstate.la
app_getextstate_la_SOURCES		= app_getextstate.c
app_getextstate_la_LDFLAGS		= -module -avoid-version -no-undefined
app_getextstate_la_LIBADD		= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_GETEXTSTATE

if WANT_APP_SMS
cwmod_LTLIBRARIES			+= app_sms.la
app_sms_la_SOURCES			= app_sms.c
app_sms_la_LDFLAGS			= -module -avoid-version -no-undefined
app_sms_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_SMS

if WANT_APP_DTMFTOTEXT
cwmod_LTLIBRARIES			+= app_dtmftotext.la
app_dtmftotext_la_SOURCES	= app_dtmftotext.c
app_dtmftotext_la_LDFLAGS	= -module -avoid-version -no-undefined
app_dtmftotext_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_DTMFTOTEXT

if WANT_APP_V110
cwmod_LTLIBRARIES			+= app_v110.la
app_v110_la_SOURCES			= app_v110.c
app_v110_la_LDFLAGS			= -module -avoid-version -no-undefined
app_v110_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_V110

if WANT_APP_FAXDETECT
cwmod_LTLIBRARIES			+= app_faxdetect.la
app_faxdetect_la_SOURCES		= app_faxdetect.c
app_faxdetect_la_LDFLAGS		= -module -avoid-version -no-undefined
app_faxdetect_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la
endif WANT_APP_FAXDETECT

INCLUDES = -I$(top_srcdir)/include
