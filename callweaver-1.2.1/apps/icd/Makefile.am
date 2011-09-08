AUTOMAKE_OPTS			= gnu

DEFS += -include $(top_builddir)/include/confdefs.h

cwmod_LTLIBRARIES		= app_icd.la

app_icd_la_SOURCES		= app_icd.c icd_agent.c icd_bridge.c icd_caller.c icd_caller_list.c icd_command.c \
 	        icd_conference.c icd_config.c icd_customer.c icd_distributor.c icd_distributor_list.c \
	        icd_event.c icd_fieldset.c icd_list.c icd_listeners.c icd_member.c icd_member_list.c \
	        icd_metalist.c icd_queue.c voidhash.c \
 	    		icd_module_api.c icd_plugable_fn.c icd_plugable_fn_list.c
app_icd_la_LDFLAGS						= -module -avoid-version -no-undefined
app_icd_la_LIBADD						= ${top_builddir}/corelib/libcallweaver.la

cwicdmod_LTLIBRARIES	= icd_mod_agent_priority_groups.la icd_mod_event_cwmgr.la \
			icd_mod_event_cli.la icd_mod_external.la icd_mod_matchagent.la
			
icd_mod_agent_priority_groups_la_SOURCES	= icd_mod_agent_priority_groups.c
icd_mod_agent_priority_groups_la_LDFLAGS	= -module -avoid-version -no-undefined
icd_mod_agent_priority_groups_la_LIBADD	= ${top_builddir}/corelib/libcallweaver.la 

icd_mod_event_cwmgr_la_SOURCES 			= icd_mod_event_cwmgr.c
icd_mod_event_cwmgr_la_LDFLAGS			= -module -avoid-version -no-undefined
icd_mod_event_cwmgr_la_LIBADD			= ${top_builddir}/corelib/libcallweaver.la 

icd_mod_event_cli_la_SOURCES				= icd_mod_event_cli.c
icd_mod_event_cli_la_LDFLAGS				= -module -avoid-version -no-undefined
icd_mod_event_cli_la_LIBADD				= ${top_builddir}/corelib/libcallweaver.la 

icd_mod_external_la_SOURCES				= icd_mod_external.c
icd_mod_external_la_LDFLAGS				= -module -avoid-version -no-undefined
icd_mod_external_la_LIBADD				= ${top_builddir}/corelib/libcallweaver.la 

icd_mod_matchagent_la_SOURCES				= icd_mod_matchagent.c
icd_mod_matchagent_la_LDFLAGS				= -module -avoid-version -no-undefined
icd_mod_matchagent_la_LIBADD				= ${top_builddir}/corelib/libcallweaver.la 

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)

