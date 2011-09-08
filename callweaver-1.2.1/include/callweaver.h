/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * General Definitions for CallWeaver top level program
 * 
 * Copyright (C) 1999-2005, Mark Spencer
 *
 * Mark Spencer <markster@digium.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

/*! \file
 * \brief CallWeaver main include file. File version handling, generic pbx functions.
*/
#if !defined(_CALLWEAVER_H)
#define _CALLWEAVER_H

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#if !defined(FALSE)
#define FALSE 0
#endif
#if !defined(TRUE)
#define TRUE (!FALSE)
#endif

#define DEFAULT_LANGUAGE "en"

#define DEFAULT_SAMPLE_RATE 8000
#define DEFAULT_SAMPLES_PER_MS  ((DEFAULT_SAMPLE_RATE)/1000)

#define CW_CONFIG_MAX_PATH 255


/* provided in callweaver.c */
extern int callweaver_main(int argc, char *argv[]);
extern char cw_config_CW_CONFIG_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_CONFIG_FILE[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_MODULE_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_SPOOL_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_MONITOR_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_VAR_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_LOG_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_OGI_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_DB[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_DB_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_KEY_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_PID[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_SOCKET[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_RUN_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_CTL_PERMISSIONS[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_CTL_OWNER[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_CTL_GROUP[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_CTL[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_SYSTEM_NAME[20];
extern char cw_config_CW_SOUNDS_DIR[CW_CONFIG_MAX_PATH];
extern char cw_config_CW_ALLOW_SPAGHETTI_CODE[20];

/* Provided by version.c */
extern const char cw_version_string[];

/* Provided by callweaver.c */
extern int cw_set_priority(int);
/* Provided by module.c */
extern int load_modules(const int preload_only);
/* Provided by pbx.c */
extern int load_pbx(void);
/* Provided by logger.c */
extern int init_logger(void);
extern void close_logger(void);
/* Provided by frame.c */
extern int init_framer(void);
/* Provided by logger.c */
extern int reload_logger(int);
/* Provided by term.c */
extern int term_init(void);
/* Provided by db.c */
extern int cwdb_init(void);
extern int cwdb_shutdown(void);
/* Provided by channel.c */
extern void cw_channels_init(void);
/* Provided by dnsmgr.c */
extern int dnsmgr_init(void);
extern void dnsmgr_reload(void);

/*!
 * \brief Register the version of a source code file with the core.
 * \param file the source file name
 * \param version the version string (typically a CVS revision keyword string)
 * \return nothing
 *
 * This function should not be called directly, but instead the
 * CALLWEAVER_FILE_VERSION macro should be used to register a file with the core.
 */
void cw_register_file_version(const char *file, const char *version);

/*!
 * \brief Unregister a source code file from the core.
 * \param file the source file name
 * \return nothing
 *
 * This function should not be called directly, but instead the
 * CALLWEAVER_FILE_VERSION macro should be used to automatically unregister
 * the file when the module is unloaded.
 */
void cw_unregister_file_version(const char *file);

/*!
 * \brief Register/unregister a source code file with the core.
 * \param file the source file name
 * \param version the version string (typically a CVS revision keyword string)
 *
 * This macro will place a file-scope constructor and destructor into the
 * source of the module using it; this will cause the version of this file
 * to registered with the CallWeaver core (and unregistered) at the appropriate
 * times.
 *
 * Example:
 *
 * \code
 * CALLWEAVER_FILE_VERSION("\$HeadURL\$", "\$Revision\$")
 * \endcode
 *
 * \note The dollar signs above have been protected with backslashes to keep
 * CVS from modifying them in this file; under normal circumstances they would
 * not be present and CVS would expand the Revision keyword into the file's
 * revision number.
 */
#if defined(__GNUC__) && !defined(LOW_MEMORY)
#define CALLWEAVER_FILE_VERSION(file, version) \
	static void __attribute__((constructor)) __register_file_version(void) \
	{ \
		cw_register_file_version(file, version); \
	} \
	static void __attribute__((destructor)) __unregister_file_version(void) \
	{ \
		cw_unregister_file_version(file); \
	}
#elif !defined(LOW_MEMORY) /* ! __GNUC__  && ! LOW_MEMORY*/
#define CALLWEAVER_FILE_VERSION(file, x) static const char __file_version[] = x;
#else /* LOW_MEMORY */
#define CALLWEAVER_FILE_VERSION(file, x)
#endif /* __GNUC__ */

#if defined(__CW_DEBUG_MALLOC)  &&  !defined(_CALLWEAVER_CALLWEAVER_MM_H)
#include "callweaver/callweaver_mm.h"
#endif

#endif
