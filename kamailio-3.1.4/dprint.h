/*
 * $Id$
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief SIP-router core :: debug printing
 * @ingroup core
 * Module: @ref core
 */

#ifndef dprint_h
#define dprint_h

#include <assert.h>
#include <syslog.h>
#include <stdio.h> /* stderr, fprintf() */

#include "compiler_opt.h"
#include "cfg_core.h"


/** if defined the function name will also be logged. */
#ifdef NO_LOG_FUNC_NAME
#	undef LOG_FUNC_NAME
#else
/* by default log the function name */
#	define LOG_FUNC_NAME
#endif /* NO_LOG_FUNC_NAME */

/* C >= 99 has __func__, older gcc versions have __FUNCTION__ */
#if __STDC_VERSION__ < 199901L
#	if __GNUC__ >= 2 && defined __FUNCTION__
#		define _FUNC_NAME_ __FUNCTION__
#	else
#		define _FUNC_NAME_ ""
#		undef LOG_FUNC_NAME
#	endif
#else
#	define _FUNC_NAME_ __func__
#endif

#ifdef NO_DEBUG
#	ifdef MOD_NAME
#		define LOC_INFO		MOD_NAME ": "
#	else
#		define LOC_INFO		"<core>: "
#	endif
#else
#	define XCT2STR(i) #i
#	define CT2STR(l)  XCT2STR(l)
#
#	ifdef MOD_NAME
#		define LOC_INFO		MOD_NAME " [" __FILE__ ":" CT2STR(__LINE__) "]: "
#	else
#		define LOC_INFO		"<core> [" __FILE__ ":" CT2STR(__LINE__) "]: "
#	endif
#
#	ifdef NO_LOG
#		undef NO_LOG
#	endif
#endif /* NO_DEBUG */


/*
 * Log levels
 */
#define L_ALERT		-5
#define L_BUG		-4
#define L_CRIT2		-3  /* like L_CRIT, but adds prefix */
#define L_CRIT  	-2  /* no prefix added */
#define L_ERR   	-1
#define L_WARN   	0
#define L_NOTICE 	1
#define L_INFO   	2
#define L_DBG    	3

/** @brief This is the facility value used to indicate that the caller of the macro
 * did not override the facility. Value 0 (the defaul) is LOG_KERN on Linux
 */
#define DEFAULT_FACILITY 0

#define LOG_LEVEL2NAME(level)	(log_level_info[(level) - (L_ALERT)].name)
#define LOG2SYSLOG_LEVEL(level) \
	(log_level_info[(level) - (L_ALERT)].syslog_level)


/** @brief my_pid(), process_no are from pt.h but we cannot #include it here
   because of circular dependencies */
extern int process_no;
extern int my_pid();

/** @brief non-zero if logging to stderr instead to the syslog */
extern int log_stderr;

/** @brief maps log levels to their string name and corresponding syslog level */

struct log_level_info {
 	char *name;
	int syslog_level;
};

#define is_printable(level) (cfg_get(core, core_cfg, debug)>=(level))
extern struct log_level_info log_level_info[];
extern char *log_name;

#ifndef NO_SIG_DEBUG
/** @brief protection against "simultaneous" printing from signal handlers */
extern volatile int dprint_crit; 
#endif

int str2facility(char *s);
int log_facility_fixup(void *handle, str *gname, str *name, void **val);


/** @brief
 * General logging macros
 *
 * LOG_(level, prefix, fmt, ...) prints "printf"-formatted log message to
 * stderr (if `log_stderr' is non-zero) or to syslog.  Note that `fmt' must
 * be constant. `prefix' is added to the beginning of the message.
 *
 * LOG(level, fmt, ...) is same as LOG_() with LOC_INFO prefix.
 */
#ifdef NO_LOG

#	ifdef __SUNPRO_C
#		define LOG_(facility, level, prefix, fmt, ...)
#		define LOG(level, fmt, ...)
#		define LOG_FC(facility, level, fmt, ...)
#	else
#		define LOG_(facility, level, prefix, fmt, args...)
#		define LOG(level, fmt, args...)
#		define LOG_FC(facility, level, fmt, args...)
#	endif

#else

#	ifdef NO_SIG_DEBUG
#		define DPRINT_NON_CRIT		(1)
#		define DPRINT_CRIT_ENTER
#		define DPRINT_CRIT_EXIT
#	else
#		define DPRINT_NON_CRIT		(dprint_crit==0)
#		define DPRINT_CRIT_ENTER	(dprint_crit++)
#		define DPRINT_CRIT_EXIT		(dprint_crit--)
#	endif

#	ifdef __SUNPRO_C
#		define LOG_(facility, level, prefix, fmt, ...) \
			do { \
				if (unlikely(cfg_get(core, core_cfg, debug) >= (level) && \
						DPRINT_NON_CRIT)) { \
					DPRINT_CRIT_ENTER; \
					if (likely(((level) >= L_ALERT) && ((level) <= L_DBG))){ \
						if (unlikely(log_stderr)) { \
							fprintf(stderr, "%2d(%d) %s: %s" fmt, \
									process_no, my_pid(), \
									LOG_LEVEL2NAME(level), (prefix), \
									__VA_ARGS__); \
						} else { \
							syslog(LOG2SYSLOG_LEVEL(level) | \
								   (((facility) != DEFAULT_FACILITY) ? \
									(facility) : \
									cfg_get(core, core_cfg, log_facility)), \
									"%s: %s" fmt, LOG_LEVEL2NAME(level),\
									(prefix), __VA_ARGS__); \
						} \
					} else { \
						if (log_stderr) { \
							fprintf(stderr, "%2d(%d) %s" fmt, \
									process_no, my_pid(), \
									(prefix),  __VA_ARGS__); \
						} else { \
							if ((level)<L_ALERT) \
								syslog(LOG2SYSLOG_LEVEL(L_ALERT) | \
									   (((facility) != DEFAULT_FACILITY) ? \
										(facility) : \
										cfg_get(core, core_cfg, log_facility)),\
									   "%s" fmt, (prefix), __VA_ARGS__); \
							else \
								syslog(LOG2SYSLOG_LEVEL(L_DBG) | \
									   (((facility) != DEFAULT_FACILITY) ? \
										(facility) : \
										cfg_get(core, core_cfg, log_facility)),\
									   "%s" fmt, (prefix), __VA_ARGS__); \
						} \
					} \
					DPRINT_CRIT_EXIT; \
				} \
			} while(0)
			
#		ifdef LOG_FUNC_NAME
#			define LOG(level, fmt, ...) \
	LOG_(DEFAULT_FACILITY, (level), LOC_INFO, "%s(): " fmt,\
				_FUNC_NAME_, __VA_ARGS__)

#			define LOG_FC(facility, level, fmt, ...) \
	LOG_((facility), (level), LOC_INFO, "%s(): " fmt,\
				_FUNC_NAME_, __VA_ARGS__)
#		else /* LOG_FUNC_NAME */

#			define LOG(level, fmt, ...) \
	LOG_(DEFAULT_FACILITY, (level), LOC_INFO, fmt, __VA_ARGS__)

#			define LOG_FC(facility, level, fmt, ...) \
	LOG_((facility), (level), LOC_INFO, fmt, __VA_ARGS__)

#		endif /* LOG_FUNC_NAME */

#	else /* ! __SUNPRO_C */
#		define LOG_(facility, level, prefix, fmt, args...) \
			do { \
				if (cfg_get(core, core_cfg, debug) >= (level) && \
						DPRINT_NON_CRIT) { \
					DPRINT_CRIT_ENTER; \
					if (likely(((level) >= L_ALERT) && ((level) <= L_DBG))){ \
						if (unlikely(log_stderr)) { \
							fprintf(stderr, "%2d(%d) %s: %s" fmt, \
									process_no, my_pid(), \
									LOG_LEVEL2NAME(level), \
									(prefix) , ## args);\
						} else { \
							syslog(LOG2SYSLOG_LEVEL(level) |\
								   (((facility) != DEFAULT_FACILITY) ? \
									(facility) : \
									cfg_get(core, core_cfg, log_facility)), \
									"%s: %s" fmt, LOG_LEVEL2NAME(level),\
									(prefix) , ## args); \
						} \
					} else { \
						if (log_stderr) { \
							fprintf(stderr, "%2d(%d) %s" fmt, \
										process_no, my_pid(), \
										(prefix) , ## args); \
						} else { \
							if ((level)<L_ALERT) \
								syslog(LOG2SYSLOG_LEVEL(L_ALERT) | \
									   (((facility) != DEFAULT_FACILITY) ? \
										(facility) : \
										cfg_get(core, core_cfg, log_facility)),\
										"%s" fmt, (prefix) , ## args); \
							else \
								syslog(LOG2SYSLOG_LEVEL(L_DBG) | \
									   (((facility) != DEFAULT_FACILITY) ? \
										(facility) : \
										cfg_get(core, core_cfg, log_facility)),\
										"%s" fmt, (prefix) , ## args); \
						} \
					} \
					DPRINT_CRIT_EXIT; \
				} \
			} while(0)
			
#		ifdef LOG_FUNC_NAME
#			define LOG(level, fmt, args...) \
	LOG_(DEFAULT_FACILITY, (level), LOC_INFO, "%s(): " fmt ,\
			_FUNC_NAME_, ## args)

#			define LOG_FC(facility, level, fmt, args...) \
	LOG_((facility), (level), LOC_INFO, "%s(): " fmt , _FUNC_NAME_, ## args)

#		else /* LOG_FUNC_NAME */
#			define LOG(level, fmt, args...) \
	LOG_(DEFAULT_FACILITY, (level), LOC_INFO, fmt , ## args)
#			define LOG_FC(facility, level, fmt, args...) \
	LOG_((facility), (level), LOC_INFO, fmt , ## args)

#		endif /* LOG_FUNC_NAME */
#	endif /* __SUNPRO_C */
#endif /* NO_LOG */


/** @name SimpleLog
 * Simplier, prefered logging macros for constant log level
 */
/*@ { */
#ifdef __SUNPRO_C
#	define ALERT(...)  LOG(L_ALERT,  __VA_ARGS__)
#	define BUG(...)    LOG(L_BUG,   __VA_ARGS__)
#	define ERR(...)    LOG(L_ERR,    __VA_ARGS__)
#	define WARN(...)   LOG(L_WARN,   __VA_ARGS__)
#	define NOTICE(...) LOG(L_NOTICE, __VA_ARGS__)
#	define INFO(...)   LOG(L_INFO,   __VA_ARGS__)
#	define CRIT(...)    LOG(L_CRIT2,   __VA_ARGS__)

#	ifdef NO_DEBUG
#		define DBG(...)
#	else
#		define DBG(...)    LOG(L_DBG, __VA_ARGS__)
#	endif		
/*@ } */

/* obsolete, do not use */
#	define DEBUG(...) DBG(__VA_ARGS__)

#else /* ! __SUNPRO_C */
#	define ALERT(fmt, args...)  LOG(L_ALERT,  fmt , ## args)
#	define BUG(fmt, args...)    LOG(L_BUG,   fmt , ## args)
#	define ERR(fmt, args...)    LOG(L_ERR,    fmt , ## args)
#	define WARN(fmt, args...)   LOG(L_WARN,   fmt , ## args)
#	define NOTICE(fmt, args...) LOG(L_NOTICE, fmt , ## args)
#	define INFO(fmt, args...)   LOG(L_INFO,   fmt , ## args)
#	define CRIT(fmt, args...)   LOG(L_CRIT2,   fmt , ## args)

#	ifdef NO_DEBUG
#		define DBG(fmt, args...)
#	else
#		define DBG(fmt, args...)    LOG(L_DBG, fmt , ## args)
#	endif		

/* obsolete, do not use */
#	define DEBUG(fmt, args...) DBG(fmt , ## args)
		
#endif /* __SUNPRO_C */


/* kamailio/openser compatibility */

#define LM_GEN1 LOG
#define LM_GEN2 LOG_FC
#define LM_ALERT ALERT
#define LM_CRIT  CRIT
#define LM_ERR ERR
#define LM_WARN WARN
#define LM_NOTICE NOTICE
#define LM_INFO INFO
#define LM_DBG DEBUG

#endif /* !dprint_h */
