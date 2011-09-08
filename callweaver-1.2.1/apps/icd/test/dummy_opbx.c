#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "callweaver/cli.h"
#include "callweaver/lock.h"
#include "callweaver/logger.h"
#include "callweaver/icd/icd_types.h"

struct opbx_channel { int x; };
struct opbx_cdr { int x; };
struct opbx_frame { int x; };
typedef struct opbx_cdr opbx_cdrbe;
int option_verbose;
struct opbx_variable { int x; };
struct opbx_config { int x; };
struct opbx_app { int x; };

int icd_debug;

int icd_verbose;

icd_config_registry *app_icd_config_registry;


void opbx_log(int level, const char *file, int line, const char *function, const char *fmt, ...) {
    va_list ap;

    fprintf(stdout, "File %s, Line %d (%s): ", file, line, function);
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void opbx_verbose(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
}


void opbx_cli(int fd, char *fmt, ...) {
}

int opbx_hangup(struct opbx_channel *chan) {
    return 0;
}

char opbx_waitstream(struct opbx_channel *c, char *breakon) {
    return '\0';
}

int opbx_streamfile(struct opbx_channel *c, char *filename, char *preflang) {
    return 0;
}

void opbx_moh_stop(struct opbx_channel *chan) {
}

int opbx_moh_start(struct opbx_channel *chan, char *class) {
    return 0;
}

char opbx_waitfordigit(struct opbx_channel *c, int ms) {
    return '\0';
}

int opbx_autoservice_stop(struct opbx_channel *chan) {
    return 0;
}

int opbx_park_call(struct opbx_channel *chan, struct opbx_channel *host, int timeout, int *extout) {
    return 0;
}

char *ast_parking_ext(void) {
    return NULL;
}

int opbx_write(struct opbx_channel *chan, struct opbx_frame *frame) {
    return 0;
}

void opbx_cdr_free(struct opbx_cdr *cdr) {
}

int opbx_cdr_init(struct opbx_cdr *cdr, struct opbx_channel *chan) {
    return 0;
}

int opbx_cdr_setcid(struct opbx_cdr *cdr, struct opbx_channel *chan) {
    return 0;
}

int opbx_cdr_register(char *name, char *desc, opbx_cdrbe be) {
    return 0;
}

void opbx_cdr_unregister(char *name) {
}

void opbx_cdr_start(struct opbx_cdr *cdr) {
}

void opbx_cdr_answer(struct opbx_cdr *cdr) {
}

void opbx_cdr_busy(struct opbx_cdr *cdr) {
}

void opbx_cdr_failed(struct opbx_cdr *cdr) {
}

int opbx_cdr_disposition(struct opbx_cdr *cdr, int cause) {
    return 0;
}

void opbx_cdr_end(struct opbx_cdr *cdr) {
}

void opbx_cdr_post(struct opbx_cdr *cdr) {
}

void opbx_cdr_setdestchan(struct opbx_cdr *cdr, char *chan) {
}

void opbx_cdr_setapp(struct opbx_cdr *cdr, char *app, char *data) {
}

int opbx_cdr_amaflags2int(char *flag) {
    return 0;
}

char *ast_cdr_disp2str(int disposition) {
    return  NULL;
}

void opbx_cdr_reset(struct opbx_cdr *cdr, int post) {
}

char *ast_cdr_flags2str(int flags) {
    return  NULL;
}

int opbx_cdr_setaccount(struct opbx_channel *chan, char *account) {
    return 0;
}

int opbx_cdr_update(struct opbx_channel *chan) {
    return 0;
}

void opbx_fr_free(struct opbx_frame *fr) {
}

struct opbx_cdr *ast_cdr_alloc(void) {
    return NULL;
}

struct opbx_frame *ast_read(struct opbx_channel *chan) {
    return NULL;
}

int opbx_waitfor(struct opbx_channel *chan, int ms) {
    return 0;
}

int opbx_call(struct opbx_channel *chan, char *addr, int timeout) {
    return 0;
}

struct opbx_channel *ast_request(char *type, int format, void *data) {
    return NULL;
}

void opbx_set_callerid(struct opbx_channel *chan, char *callerid, int  anitoo) {
}

int opbx_stopstream(struct opbx_channel *c) {
    return 0;
}

int opbx_async_goto(struct opbx_channel *chan, char *context, char *exten, int priority, int needlock) {
    return 0;
}

int opbx_channel_setoption(struct opbx_channel *channel, int option, void *data, int datalen, int block) {
    return 0;
}

int opbx_answer(struct opbx_channel *chan) {
    return 0;
}

int opbx_channel_bridge(struct opbx_channel *c0, struct opbx_channel *c1, int flags, struct opbx_frame **fo, struct opbx_channel **rc) {
    return 0;
}

int opbx_indicate(struct opbx_channel *chan, int condition) {
    return 0;
}

int opbx_autoservice_start(struct opbx_channel *chan) {
    return 0;
}

int opbx_matchmore_extension(struct opbx_channel *c, char *context, char *exten, int priority, char *callerid) {
    return 0;
}

int opbx_exists_extension(struct opbx_channel *c, char *context, char *exten, int priority, char *callerid) {
    return 0;
}

int opbx_softhangup(struct opbx_channel *chan, int cause) {
    return 0;
}

int opbx_app_getdata(struct opbx_channel *c, char *prompt, char *s, int maxlen, int timeout) {
    return 0;
}

int opbx_best_codec(int fmts) {
    return 0;
}

int opbx_set_read_format(struct opbx_channel *chan, int format) {
    return 0;
}

int opbx_set_write_format(struct opbx_channel *chan, int format) {
    return 0;
}

int opbx_channel_make_compatible(struct opbx_channel *c0, struct opbx_channel *c1) {
    return 0;
}


int opbx_cli_register ( struct opbx_cli_entry * e ) { 
    return 0; 
}

int opbx_register_application( char * name, int(* execute)(struct opbx_channel *, char **, int), char * synopsis, char * syntax, char * description) {
    return 0;
}

int opbx_unregister_application(void *app) {
    return 0;
}


int opbx_true(char *val) {
    return 0;
}

void opbx_update_use_count(void) {
}

int opbx_safe_sleep(struct opbx_channel *chan, int ms) {
    return 0;
}

char *ast_category_browse(struct opbx_config *config, char *prev) {
    return NULL;
}

struct opbx_variable *ast_variable_browse(struct opbx_config *config, char *category) {
    return NULL;
}


void opbx_destroy(struct opbx_config *config) {
}

struct opbx_config *ast_load(char *configfile) {
    return NULL;
}

int opbx_say_digits(struct opbx_channel *chan, int num, char *ints, char *lang) {
    return 0;
}

struct opbx_channel *ast_channel_alloc(int needalertpipe) {
    return NULL;
}

int opbx_channel_masquerade(struct opbx_channel *original, struct opbx_channel *clone) {
    return 0;
}

void opbx_deactivate_generator(struct opbx_channel *chan) {
}

int opbx_say_number(struct opbx_channel *chan, int num, char *ints, char *lang, char *options) {
    return 0;
}

int opbx_check_hangup(struct opbx_channel *chan) {
    return 0;
}

struct opbx_app *pbx_findapp(char *app) {
 return NULL;
} 

int pbx_exec(struct opbx_channel *c, struct opbx_app *app, void *data) {
 return 0;
}
 
char *ast_state2str(int state) {
 return NULL;
}
 
struct opbx_channel *opbx_waitfor_nandfds(struct opbx_channel **c, int n, int *fds, int nfds, int *exception,int *outfd, int *ms) {
	return NULL;
}

int opbx_cli_unregister(struct opbx_cli_entry *e) {
    return 0;
}
