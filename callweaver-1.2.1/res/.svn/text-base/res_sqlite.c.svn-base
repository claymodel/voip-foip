/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * SQLite Resource
 * 
 * Copyright (C) 2004, Anthony Minessale II
 *
 * Anthony Minessale II <anthmct@cylynx.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include <sqliteInt.h>

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/cli.h"
#include "callweaver/module.h"
#include "callweaver/utils.h"
#include "callweaver/config.h"
#include "callweaver.h"

/* When you change the DATE_FORMAT, be sure to change the CHAR(19) below to something else */
#define DATE_FORMAT "%Y-%m-%d %T"
#define MYMAX_RESULTS 20
#define ARRAY_LEN 256
#define ARRAY_SIZE 256
#define COPY_KEYS 1
#define DONT_COPY_KEYS 0
#define	 CDR_STRING_SIZE 128
#define	 MAX_REG 10
#define	 CDR_ELEM 19
#define LOG_UNIQUEID
#define LOG_USERFIELD

static char *create_dialplan_sql = "CREATE TABLE dialplan(context varchar(255), exten varchar(255), pri int, app varchar(255), data varchar(255));";
static char *create_cdr_sql = 
"CREATE TABLE cw_cdr (\n"
"   acctid      INTEGER PRIMARY KEY,\n"
"   clid        VARCHAR(80),\n"
"   src     VARCHAR(80),\n"
"   dst     VARCHAR(80),\n"
"   dcontext    VARCHAR(80),\n"
"   channel     VARCHAR(80),\n"
"   dstchannel  VARCHAR(80),\n"
"   lastapp     VARCHAR(80),\n"
"   lastdata    VARCHAR(80),\n"
"   start       CHAR(19),\n"
"   answer      CHAR(19),\n"
"   end     CHAR(19),\n"
"   duration    INTEGER,\n"
"   billsec     INTEGER,\n"
"   disposition INTEGER,\n"
"   amaflags    INTEGER,\n"
"   accountcode VARCHAR(20),\n"
"   uniqueid   VARCHAR(32),\n"
"   userfield  VARCHAR(255)\n"
");\n";


static char *create_config_sql = 
"create table cw_config (\n"
"						 id integer primary key,\n"
"						 cat_metric int not null default 0,\n"
"						 var_metric int not null default 0,\n"
"						 commented int not null default 0,\n"
"						 filename varchar(128) not null,\n"
"						 category varchar(128) not null default 'default',\n"
"						 var_name varchar(128) not null,\n"
"						 var_val varchar(128) not null\n"
"						 );\n\n"
"CREATE INDEX cw_config_index_1 ON cw_config(filename);\n"
"CREATE INDEX cw_config_index_2 ON cw_config(filename,category);\n"
"CREATE INDEX cw_config_index_3 ON cw_config(filename,category,var_name);";




static char *cdr_name = "cdr_res_sqlite";
static int has_cdr=-1;
static int has_config=-1;
static int has_switch=-1;
static int has_cli=0;
static char vh = 'h';
static int default_timeout = 300;
static int do_reload=1;

static char cdr_table[ARRAY_SIZE];
static char cdr_dbfile[ARRAY_SIZE];

static char config_table[ARRAY_SIZE];
static char config_dbfile[ARRAY_SIZE];

static char switch_table[ARRAY_SIZE];
static char switch_dbfile[ARRAY_SIZE];

typedef struct extension_cache extension_cache;
typedef struct switch_config switch_config;

static int exist_callback(void *pArg, int argc, char **argv, char **columnNames);

struct extension_cache {
	time_t expires;
	char exten[ARRAY_SIZE];
	char context[ARRAY_SIZE];
	char app_name[ARRAY_SIZE][ARRAY_LEN];
	char app_data[ARRAY_SIZE][ARRAY_LEN];
	int argc;
};

struct switch_config {
	int timeout;
	int fd;
	int seeheads;
};

static char *desc = "SQLite Resource Module";
static char default_dbfile[ARRAY_SIZE] = {"/usr/local/callweaver/sqlite/callweaver.db"};
static char clidb[ARRAY_SIZE] = {"/usr/local/callweaver/sqlite/callweaver.db"};

static Hash extens;



static const char *tdesc = "SQLite SQL Interface";

static void *app;
static const char *name = "SQL";
static const char *synopsis = "SQL(\"[sql statement]\"[, dbname])\n" 
"[if it's a select it will auto-vivify chan vars matching the selected column names.]\n";
static const char *syntax = "SQL(\"[sql statement]\"[, dbname])";


static void pick_path(char *dbname,char *buf, size_t size) {

	memset(buf,0,size);
	if (strchr(dbname,'/')) {
		strncpy(buf,dbname,size);
	}
	else {
		snprintf(buf,size,"%s/%s.db",cw_config_CW_DB_DIR,dbname);
	}
}

static sqlite3 *open_db(char *filename) {
	sqlite3 *db;
	char path[ARRAY_SIZE];
	
	pick_path(filename,path,ARRAY_SIZE);
	if (sqlite3_open(path,&db)) {
		cw_log(LOG_WARNING,"SQL ERR [%s]\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		db=NULL;
	}
	return db;
}



STANDARD_LOCAL_USER;

LOCAL_USER_DECL;




static int app_callback(void *pArg, int argc, char **argv, char **columnNames){
	int x=0;
	struct cw_channel *chan = pArg;

	if (chan) {
		for(x=0;x<argc;x++)
			pbx_builtin_setvar_helper(chan,columnNames[x],argv[x]);
	}
	return 0;
}

static int sqlite_execapp(struct cw_channel *chan, int argc,char **argv)
{
	struct localuser *u;
	char *errmsg;
	sqlite3 *db;
	int res=0;

	if (argc < 1 || !argv[0][0]) {
		cw_log(LOG_WARNING, "sql requires an argument (sql)\n");
		return -1;
	}

	db = open_db(argc > 1 && argv[1][0] ? argv[1] : default_dbfile);
	if (!db)
		return -1;

	LOCAL_USER_ADD(u);
	/* Do our thing here */


	sqlite3_exec(
				db,
				argv[0],
				app_callback,
				chan,
				&errmsg
				);

	if (errmsg) {
		cw_log(LOG_WARNING,"SQL ERR [%s]\n",errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
	}


	LOCAL_USER_REMOVE(u);
	sqlite3_close(db);
	return res;
}




CW_MUTEX_DEFINE_STATIC(switch_lock);
CW_MUTEX_DEFINE_STATIC(cdr_lock);


static int cli_callback(void *pArg, int argc, char **argv, char **columnNames){
	int fd=0;
	int x=0;
	switch_config *config = pArg;

	if (config) {
		fd = config->fd;
	}
	else
		return -1;

	if (vh == 'v') {
		cw_cli(fd,"\n");
		for(x=0;x<argc;x++)
			cw_cli(fd,"%s=%s\n",columnNames[x],argv[x]);
		cw_cli(fd,"\n");
	}
	else {
		if (!config->seeheads) {
			cw_cli(fd,",");
			for(x=0;x<argc;x++)
				cw_cli(fd,"%s,",columnNames[x]);
			config->seeheads = 1;
			cw_cli(fd,"\n");
		}
		cw_cli(fd,",");
		for(x=0;x<argc;x++)
			cw_cli(fd,"%s,",argv[x]);
		cw_cli(fd,"\n");
	}

	return 0;
}

static int sqlite_cli(int fd, int argc, char *argv[]) {
	char *errmsg = NULL;
	char sqlbuf[1024];
	int x = 0;
	int start = 0;
	switch_config config;
	extension_cache *cache;
	memset(&config,0,sizeof(switch_config));
	sqlite3 *db=NULL;
	HashElem *elem;
	char path[ARRAY_SIZE];
	char *sql = NULL;

	if (has_switch) {
		if (argv[1] && !strcmp(argv[1],"switchtable")) {
			if (argv[2]) {
				cw_mutex_lock(&switch_lock);
				strncpy(switch_table,argv[2],ARRAY_SIZE);
				cw_mutex_unlock(&switch_lock);
			}
			cw_cli(fd,"\nswitch table is %s\n\n",switch_table);
			return 0;
		}
		else if (argv[1] && !strcmp(argv[1],"switchdb")) {
			if (argv[2]) {
				cw_mutex_lock(&switch_lock);
				pick_path(argv[2],switch_dbfile,ARRAY_SIZE);
				cw_mutex_unlock(&switch_lock);
			}
			cw_cli(fd,"\nswitch db is %s\n\n",switch_dbfile);
			return 0;
		}
	}


	if (has_cdr) {
		if (argv[1] && !strcmp(argv[1],"cdrtable")) {
			if (argv[2]) {
				cw_mutex_lock(&cdr_lock);
				strncpy(cdr_table,argv[2],ARRAY_SIZE);
				cw_mutex_unlock(&cdr_lock);
			}
			cw_cli(fd,"\ncdr table is %s\n\n",cdr_table);
			return 0;
		}
		else if (argv[1] && !strcmp(argv[1],"cdrdb")) {
			if (argv[2]) {
				cw_mutex_lock(&cdr_lock);
				pick_path(argv[2],cdr_dbfile,ARRAY_SIZE);
				cw_mutex_unlock(&cdr_lock);
			}
			cw_cli(fd,"\ncdr db is %s\n\n",cdr_dbfile);
			return 0;
		}
	}


	if (!strcmp(argv[0],"sql")) {
		start = 1;
		if (argv[1] && !strcmp(argv[1],"cacheall")) {


			config.timeout = argv[3] ? atoi(argv[3]) : default_timeout;
			if (!config.timeout)
				config.timeout = default_timeout;
			if (!config.timeout)
				config.timeout = 300;
			db = open_db(clidb);

			if((sql = sqlite3_mprintf("select *,context from %q order by context,exten,pri",argv[2] ? argv[2] : switch_table))) {
				sqlite3_exec(
							 db,
							 sql,
							 exist_callback,
							 &config,
							 &errmsg
							 );
				if (errmsg) {
					cw_log(LOG_WARNING,"SQL ERR [%s]\n",errmsg);
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			
				sqlite3_free(sql);
				sql = NULL;

			} else {
				cw_cli(fd,"\nmalloc failed, good luck\n");
			}
			sqlite3_close(db);
			return 0;
		}
		else if (argv[1] && (!strcmp(argv[1],"use") || !strcmp(argv[1],"db"))) {
			if (argv[2]) {
				cw_mutex_lock(&switch_lock);
				pick_path(argv[2],path,ARRAY_SIZE);
				strncpy(clidb,path,ARRAY_SIZE);
				cw_mutex_unlock(&switch_lock);
			}
			cw_cli(fd,"\nnow using %s\n\n",clidb);
			return 0;
		}
		else if (argv[1] && !strcmp(argv[1],"cachetimeout")) {
			if (argv[2]) {
				cw_mutex_lock(&switch_lock);
				default_timeout = atoi(argv[2]);
				if (!default_timeout)
					default_timeout = 300;
				cw_mutex_unlock(&switch_lock);
			}
			cw_cli(fd,"\ncachetimeout is %d\n\n",default_timeout);
			return 0;
		}
		else if (argv[1] && !strcmp(argv[1],"vh")) {
			if (argv[2]) {
				cw_mutex_lock(&switch_lock);
				vh = argv[2][0] == 'v' ? 'v' : 'h';
				cw_mutex_unlock(&switch_lock);
			}
			cw_cli(fd,"\nvh is %c\n\n",vh);
			return 0;
		}
		else if (argv[1] && !strcmp(argv[1],"clearcache")) {
			cw_mutex_lock(&switch_lock);
			elem = extens.first;
			while ( elem ){
				HashElem *next_elem = elem->next;
				cache = elem->data;
				cw_cli(fd,"OK Erasing %s@%s\n",cache->exten,cache->context);
				free(cache);
				cache = NULL;
				elem = next_elem;
			}
			sqlite3HashClear(&extens);
			sqlite3HashInit(&extens,SQLITE_HASH_STRING,COPY_KEYS);
			cw_mutex_unlock(&switch_lock);
			cw_cli(fd,"\nOK. Cache Clear!\n\n");
			return 0;
		}

	}

	if (argc > start) {
		memset(sqlbuf,0,1024);
		for(x=start;x<argc;x++) {
			strncat(sqlbuf,argv[x],1024);
			strncat(sqlbuf," ",1024);
		}
		config.fd = fd;
		config.seeheads = 0;
		cw_cli(fd,"\n\n");
		cw_mutex_lock(&switch_lock);
		if ((db = open_db(clidb))) {
			sqlite3_exec(
						 db,
						 sqlbuf,
						 cli_callback,
						 &config,
						 &errmsg
						 );
			cw_mutex_unlock(&switch_lock);
			if (errmsg) {
				cw_cli(fd,"ERROR: '%s'\n[%s]\n",errmsg,sqlbuf);
				sqlite3_free(errmsg);
				errmsg = NULL;
			}
			sqlite3_close(db);
		} else {
			cw_cli(fd,"ERROR OPEINING DB.\n");
			return -1;
		}

	} else {
		cw_cli(fd,"ERROR! NO SQL?\n");
		return -1;
	}
	cw_cli(fd,"\n\n");

	return 0;
}


static int exist_callback(void *pArg, int argc, char **argv, char **columnNames){
	extension_cache *cache;
	char key[ARRAY_SIZE];
	int pri;
	time_t now;
	int needs_add=0;
	switch_config *config=NULL;
	int timeout=0;
	char *context;


	if (pArg) {
		config = pArg;
		timeout = config->timeout;
	}

	if (!timeout)
		timeout = default_timeout;

	context = argv[5] ? argv[5] : argv[0];
	if (!strcmp(context,"_any_")) {
		cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch: magic extension: %s not cached\n",argv[1]);
		return 0;
	}

	snprintf(key, ARRAY_SIZE, "%s.%s", argv[1], context);
	time(&now);
	pri = atoi(argv[2]);


	cache = (extension_cache *) sqlite3HashFind(&extens,key,strlen(key));

	if (! cache) {
		cache = malloc(sizeof(extension_cache));
		needs_add = 1;
	}

	if (needs_add || cache->expires < now) {
		memset(cache,0,sizeof(extension_cache));
		strncpy(cache->context,argv[0],sizeof(cache->context));
		strncpy(cache->exten,argv[1],sizeof(cache->exten));
	}
	cache->expires = now + timeout;

	if (config && ! config->seeheads) {
		cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch: extension %s@%s will expire in %d seconds\n",argv[1],context,timeout);
		config->seeheads = 1;
	}

	strncpy(cache->app_name[pri], argv[3], sizeof(cache->app_name[pri]));
	strncpy(cache->app_data[pri], argv[4], sizeof(cache->app_data[pri]));
	
	if (needs_add) {
		sqlite3HashInsert(&extens, key, strlen(key), cache);
	}
	return 0;
}




static int SQLiteSwitch_exists(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	int res = 0; 
	char key[ARRAY_SIZE];
	extension_cache *cache;
	time_t now;
	char *errmsg = NULL;
	char databuf[ARRAY_SIZE];
	char *table,*tmp,*filename=NULL;
	int expires=0;
	switch_config config;
	sqlite3 *db=NULL;
	char *sql = NULL;

	memset(&config,0,sizeof(switch_config));
	memset(databuf,0,ARRAY_SIZE);
	strncpy(databuf,data,ARRAY_SIZE);
	if (!strlen(databuf))
		strncpy(databuf,switch_table,ARRAY_SIZE);

	table = databuf;
	tmp = strchr(table,':');
	if (tmp) {
		*tmp = '\0';
		tmp++;
		expires = atoi(tmp);
		filename = strchr(tmp,':');
		if (filename) {
			*filename = '\0';
			filename++;
		}
	}
	if (!expires)
		expires = default_timeout;

	config.timeout = expires;
	if (!filename)
		filename = switch_dbfile;


	//cw_log(LOG_NOTICE, "SQLiteSwitch_exists %d: con: %s, exten: %s, pri: %d, cid: %s, data: %s\n", res, context, exten, priority, callerid ? callerid : "<unknown>", data);
	time(&now);
	snprintf(key, ARRAY_SIZE, "%s.%s", exten, context);

	cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch_exists lookup [%s]: ",key);
	cache = (extension_cache *) sqlite3HashFind(&extens,key,strlen(key));
	cw_verbose("%s\n",cache ? "match" : "fail");



	if (! cache || (cache && cache->expires < now) ) {
		if ((db = open_db(filename))) {

		
			if((sql = sqlite3_mprintf("select *,'%q' as real_context from %q where (context='%q' or context='_any_') and exten='%q' order by context,exten,pri",
								  context,
								  table,
								  context,
								  exten
									  ))) {
				sqlite3_exec(
							 db,
							 sql,
							 exist_callback,
							 &config,
							 &errmsg
							 );
				if (errmsg) {
					cw_log(LOG_WARNING,"SQL ERR [%s]\n",errmsg);
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			} else {
				cw_log(LOG_WARNING,"malloc failed, good luck!\n");
			}
			sqlite3_close(db);
			if (sql) {
				sqlite3_free(sql);
				sql = NULL;
			}
			cache = (extension_cache *) sqlite3HashFind(&extens,key,strlen(key));
		}  else {
			cw_log(LOG_WARNING,"ERROR OPEINING DB.\n");
			return -1;
		}

	}
	
	if (cache) {
		cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch_exists lookup [%s,%d]: ",key,priority);
		res = strlen(cache->app_name[priority]) ? 1 : 0;
		cw_verbose("%s\n",res ? "match" : "fail");
	}

	return res;
}

static int SQLiteSwitch_canmatch(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{

	int res = 1; 

	//cw_log(LOG_NOTICE, "SQLiteSwitch_canmatch %d: con: %s, exten: %s, pri: %d, cid: %s, data: %s\n", res, context, exten, priority, callerid ? callerid : "<unknown>", data);


	return res;

}


static int SQLiteSwitch_exec(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{
	struct cw_app *app;
	int res = 0;
	extension_cache *cache;
	char key[ARRAY_SIZE];
	time_t now;
	char app_data[1024];
	

	time(&now);
	//cw_log(LOG_NOTICE, "SQLiteSwitch_exec: con: %s, exten: %s, pri: %d, cid: %s, data: %s\n", context, exten, priority, callerid ? callerid : "<unknown>", data);

	snprintf(key, ARRAY_SIZE, "%s.%s", exten, context);

	cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch_exec lookup [%s]: ",key);
	cache = (extension_cache *) sqlite3HashFind(&extens,key,strlen(key));
	cw_verbose("%s\n",cache ? "match" : "fail");

	if (cache) {
		app = pbx_findapp(cache->app_name[priority]);
		if (app) {
			pbx_substitute_variables_helper(chan, cache->app_data[priority], app_data, sizeof(app_data));
			cw_verbose(VERBOSE_PREFIX_2 "SQLiteSwitch_exec: call app %s(%s)\n",cache->app_name[priority],app_data);
			res = pbx_exec(chan, app, app_data);
		}
		else {
			cw_log(LOG_WARNING, "application %s not registered\n",cache->app_name[priority]);
			res = -1;
		}
	}
		
	return res;
}


static int SQLiteSwitch_matchmore(struct cw_channel *chan, const char *context, const char *exten, int priority, const char *callerid, const char *data)
{

	int res = 0; 

	//cw_log(LOG_NOTICE, "SQLiteSwitch_matchmore %d: con: %s, exten: %s, pri: %d, cid: %s, data: %s\n", res, context, exten, priority, callerid ? callerid : "<unknown>", data);

	return res;

}


static struct cw_switch sqlite_switch = 
	{
		name:			"SQLite",
		description:	"Read The Dialplan From The SQLite Database",
		exists:			SQLiteSwitch_exists,
		canmatch:		SQLiteSwitch_canmatch,
		exec:			SQLiteSwitch_exec,
		matchmore:		SQLiteSwitch_matchmore,
	};

static char scmd[] = "SQLite command\n";
static char shelp[] = "\n\nsql [sql statement]\n"
"sql cacheall\n"
"sql clearcache\n"
"sql vh [v|h]\n"
"sql dptable [table name]\n"
"sql use [dbname]\n"
"\n"
"[select][insert][update][delete][begin][rollback][create][drop] <...>\n";


static struct cw_cli_entry	 cli_sqlite1 = { { "select", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite2 = { { "insert", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite3 = { { "update", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite4 = { { "delete", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite5 = { { "begin", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite6 = { { "rollback", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite7 = { { "sql", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite8 = { { "create", NULL }, sqlite_cli, scmd, shelp};
static struct cw_cli_entry	 cli_sqlite9 = { { "drop", NULL }, sqlite_cli, scmd, shelp};

static struct cw_config *config_sqlite(const char *database, const char *table, const char *file, struct cw_config *cfg)
{
	struct cw_variable *new_v, *v;
	struct cw_category *cur_cat;
	char ttable[ARRAY_SIZE];
	int configured = 0, res = 0;
	sqlite3_stmt *stmt = NULL;;
	int cat_metric=0, last_cat_metric=0;
	char sql[ARRAY_SIZE];
	char last[ARRAY_SIZE] = "";
	char path[ARRAY_SIZE];
	sqlite3 *db;
	int running=0;
	int i=0;
	struct cw_config *config;


	/* if the data was not passed from extconfig.conf then get it from sqlite.conf */
	if(!database || cw_strlen_zero(database)) {
		if (!file || !strcmp (file, "res_sqlite.conf"))
			return NULL; // cant configure myself with myself !

		config = cw_config_load ("res_sqlite.conf");
		if (config) {
			for (v = cw_variable_browse (config, "config"); v; v = v->next) {
				if (!strcmp (v->name, "table")) {
					strncpy (ttable, v->value, sizeof (ttable));
					configured++;
				} else if (!strcmp (v->name, "dbfile")) {
					pick_path(v->value,path,ARRAY_SIZE);
					configured++;
				}
			}
			cw_config_destroy (config);
		}
	} else {
		pick_path((char *)database,path,ARRAY_SIZE);
		strncpy (ttable, table, sizeof (ttable));
		configured = 2;
	}

	if (configured < 2)
		return NULL;


	db=open_db(path);
	if (!db)
		return NULL;

	sprintf (sql, "select * from %s where filename='%s' and commented=0 order by filename,cat_metric desc,var_metric asc,id", ttable, file);
	res = sqlite3_prepare(db,sql,0,&stmt,0);

	if (res) {
		cw_log (LOG_WARNING, "SQL select error [%s]!\n[%s]\n\n",sqlite3_errmsg(db), sql);
		return NULL;
	}

	cur_cat = cw_config_get_current_category(cfg);

	/*
	  0 id int
	  1 cat_metric int not null default 0,
	  2 var_metric int not null default 0,
	  3 commented int not null default 0,
	  4 filename varchar(128) not null,
	  5 category varchar(128) not null default 'default',
	  6 var_name varchar(128) not null,
	  7 var_val varchar(128) not null
	*/
	
	running = 1;
	while (running) {
		res = sqlite3_step(stmt);
		running = 1;
		switch( res ){
		case SQLITE_DONE:	running = 0 ; break;
		case SQLITE_BUSY:	sleep(2); running = 2; break ;
		case SQLITE_ERROR:	running = 0; break;
		case SQLITE_MISUSE: running = 0; break;
		case SQLITE_ROW:	
		default: 
			break;
		}	
		if (!running)
			break;
		
		if (running == 2)
			continue;

		if(option_verbose > 4)
			for(i=0;i<8;i++){
				cw_verbose(VERBOSE_PREFIX_3"SQLite Config: %d=%s\n",i,sqlite3_column_text(stmt,i));
			}
		
		if (strcmp (last, sqlite3_column_text(stmt,5)) || last_cat_metric != cat_metric) {
			cur_cat = cw_category_new((char *)sqlite3_column_text(stmt,5));
			if (!cur_cat) {
				cw_log(LOG_WARNING, "Out of memory!\n");
				break;
			}
			strcpy (last, sqlite3_column_text(stmt,5));
			last_cat_metric	= cat_metric;
			cw_category_append(cfg, cur_cat);
		}
		new_v = cw_variable_new ((char *)sqlite3_column_text(stmt,6), (char *)sqlite3_column_text(stmt,7));
		cw_variable_append(cur_cat, new_v);
	}
	
	if ((sqlite3_finalize(stmt))) 
		cw_log(LOG_ERROR,"ERROR: %s\n",sqlite3_errmsg(db));

	return cfg;
}


static int sqlite_log(struct cw_cdr *cdr)
{
	int res = 0;
	char *zErr = 0;
	struct tm tm;
	time_t t;
	char startstr[80], answerstr[80], endstr[80];
	int count;
	sqlite3 *db;
	char *sql = NULL;

	db = open_db(cdr_dbfile);
	if (!db)
		return -1;

	t = cdr->start.tv_sec;
	localtime_r(&t, &tm);
	strftime(startstr, sizeof(startstr), DATE_FORMAT, &tm);

	t = cdr->answer.tv_sec;
	localtime_r(&t, &tm);
	strftime(answerstr, sizeof(answerstr), DATE_FORMAT, &tm);

	t = cdr->end.tv_sec;
	localtime_r(&t, &tm);
	strftime(endstr, sizeof(endstr), DATE_FORMAT, &tm);

	for(count=0; count<10; count++) {
		if((sql = sqlite3_mprintf(
							  "INSERT INTO %q ("
							  "clid,src,dst,dcontext,"
							  "channel,dstchannel,lastapp,lastdata, "
							  "start,answer,end,"
							  "duration,billsec,disposition,amaflags, "
							  "accountcode"
#ifdef LOG_UNIQUEID
							  ",uniqueid"
#endif
#ifdef LOG_USERFIELD
							  ",userfield"
#endif
							  ") VALUES ("
							  "'%q', '%q', '%q', '%q', "
							  "'%q', '%q', '%q', '%q', "
							  "'%q', '%q', '%q', "
							  "%d, %d, %d, %d, "
							  "'%q'"
#ifdef LOG_UNIQUEID
							  ",'%q'"
#endif
#ifdef LOG_USERFIELD
							  ",'%q'"
#endif
							  ")",cdr_table,
							  cdr->clid, cdr->src, cdr->dst, cdr->dcontext,
							  cdr->channel, cdr->dstchannel, cdr->lastapp, cdr->lastdata,
							  startstr, answerstr, endstr,
							  cdr->duration, cdr->billsec, cdr->disposition, cdr->amaflags,
							  cdr->accountcode
#ifdef LOG_UNIQUEID
							  ,cdr->uniqueid
#endif
#ifdef LOG_USERFIELD
							  ,cdr->userfield
#endif
							  ))) {

			res = sqlite3_exec(db,
							   sql
							   ,NULL,
							   NULL,
							   &zErr
							   );
		} else {
			cw_log(LOG_ERROR, "malloc failed, good luck!\n");
			break;
		}


		if (res != SQLITE_BUSY && res != SQLITE_LOCKED)
			break;
		usleep(200);
	}
	if (sql) {
		sqlite3_free(sql);
		sql = NULL;
	}
	if (zErr) {
		cw_log(LOG_ERROR, "cdr_sqlite: %s\n", zErr);

		cw_log(LOG_ERROR,
				"INSERT INTO %s ("
				"clid,src,dst,dcontext,"
				"channel,dstchannel,lastapp,lastdata, "
				"start,answer,end,"
				"duration,billsec,disposition,amaflags, "
				"accountcode"
#ifdef LOG_UNIQUEID
				",uniqueid"
#endif
#ifdef LOG_USERFIELD
				",userfield"
#endif
				") VALUES ("
				"'%s', '%s', '%s', '%s', "
				"'%s', '%s', '%s', '%s', "
				"'%s', '%s', '%s', "
				"%d, %d, %d, %d, "
				"'%s'"
#ifdef LOG_UNIQUEID
				",'%s'"
#endif
#ifdef LOG_USERFIELD
				",'%s'"
#endif
				")\n",cdr_table,
				cdr->clid, cdr->src, cdr->dst, cdr->dcontext,
				cdr->channel, cdr->dstchannel, cdr->lastapp, cdr->lastdata,
				startstr, answerstr, endstr,
				cdr->duration, cdr->billsec, cdr->disposition, cdr->amaflags,
				cdr->accountcode
#				ifdef LOG_UNIQUEID
				,cdr->uniqueid
#endif
#ifdef LOG_USERFIELD
				,cdr->userfield
#endif
				);

		free(zErr);
	}

	sqlite3_close(db);
	db=NULL;
	return res;
}


static void check_table_exists(char *dbfile, char *test_sql, char *create_sql) {
	sqlite3 *db;
	char *errmsg;

	if((db = open_db(dbfile))) {
		if(test_sql) {
			sqlite3_exec(
						 db,
						 test_sql,
						 NULL,
						 NULL,
						 &errmsg
						 );

			if (errmsg) {
				cw_log(LOG_WARNING,"SQL ERR [%s]\n[%s]\nAuto Repairing!\n",errmsg,test_sql);
				sqlite3_free(errmsg);
				errmsg = NULL;
				sqlite3_exec(
							 db,
							 create_sql,
							 NULL,
							 NULL,
							 &errmsg
							 );
				if (errmsg) {
					cw_log(LOG_WARNING,"SQL ERR [%s]\n[%s]\n",errmsg,create_sql);
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			}
			sqlite3_close(db);
		}
	}

}

static int load_config(int hard) {

	struct cw_config *config;
	struct cw_variable *v;
	char *sql;
	

	config = cw_config_load ("res_sqlite.conf");
	if (config) {
		for (v = cw_variable_browse (config, "general"); v; v = v->next) {
			if (!strcmp (v->name, "reload")) {
				do_reload = cw_true(v->value);
			}
		}
		if (!hard)
			if (!do_reload) {
				cw_verbose(VERBOSE_PREFIX_2 "RES SQLite Skipping Reload set reload => yes in [general]\n");
				return 0;
			}
		
		cw_verbose(VERBOSE_PREFIX_2 "RES SQLite Loading Defaults\n");
		has_cdr=-1;
		has_switch=-1;
		has_config=-1;
		has_cli=0;
		
		
		for (v = cw_variable_browse (config, "cdr"); v; v = v->next) {
			if (!strcmp (v->name, "table")) {
				strncpy (cdr_table, v->value, sizeof (cdr_table));
				has_cdr++;
			} else if (!strcmp (v->name, "dbfile")) {
				pick_path(v->value,cdr_dbfile,ARRAY_SIZE);
				has_cdr++;
			}
		}

		for (v = cw_variable_browse (config, "config"); v; v = v->next) {
			if (!strcmp (v->name, "table")) {
				strncpy (config_table, v->value, sizeof (cdr_table));
				has_config++;
			} else if (!strcmp (v->name, "dbfile")) {
				pick_path(v->value,config_dbfile,ARRAY_SIZE);
				has_config++;
			}
		}

		for (v = cw_variable_browse (config, "switch"); v; v = v->next) {
			if (!strcmp (v->name, "table")) {
				strncpy (switch_table, v->value, sizeof (cdr_table));
				has_switch++;
			} else if (!strcmp (v->name, "dbfile")) {
				pick_path(v->value,switch_dbfile,ARRAY_SIZE);
				has_switch++;
			}
		}

		for (v = cw_variable_browse (config, "cli"); v; v = v->next) {
			if (!strcmp (v->name, "dbfile")) {
				pick_path(v->value,clidb,ARRAY_SIZE);
				has_cli++;
			}
		}
		
		for (v = cw_variable_browse (config, "default"); v; v = v->next) {
			if (!strcmp (v->name, "dbfile")) {
				pick_path(v->value,default_dbfile,ARRAY_SIZE);
				has_cli++;
			}
		}

		cw_config_destroy (config);
	}




	if(has_cdr > 0) {
		if((sql = sqlite3_mprintf("select count(*) from %q limit 1",cdr_table))) {
			check_table_exists(cdr_dbfile,sql,create_cdr_sql);
			sqlite3_free(sql);
			sql = NULL;
		}
	}

	if(has_config > 0) {
		if((sql = sqlite3_mprintf("select count(*) from %q limit 1",config_table))) {
			check_table_exists(config_dbfile,sql,create_config_sql);
			sqlite3_free(sql);
			sql = NULL;
		}
	}

	if(has_switch > 0) {
		if((sql = sqlite3_mprintf("select count(*) from %q limit 1",switch_table))) {
			check_table_exists(switch_dbfile,sql,create_dialplan_sql);
			sqlite3_free(sql);
			sql = NULL;
		}
	}

	return 0;
}


int reload(void) {
	return load_config(0);
}

static struct cw_config_engine sqlite_engine = {
	.name = "sqlite",
	.load_func = config_sqlite


};

int load_module(void)
{
	int res = 0;
	do_reload = 1;
	load_config(1);
	do_reload = 0;

	cw_config_engine_register(&sqlite_engine);
	cw_verbose(VERBOSE_PREFIX_2 "SQLite Config Handler Registered.\n");


	if (has_cdr) {
		cw_verbose(VERBOSE_PREFIX_2 "Loading SQLite CDR\n");
		res = cw_cdr_register(cdr_name, "RES SQLite CDR", sqlite_log);
	}
	else 
		cw_verbose(VERBOSE_PREFIX_2 "SQLite CDR Disabled\n");



	app = cw_register_application(name, sqlite_execapp, synopsis, syntax, tdesc);


	if (has_switch) {
		if (cw_register_switch(&sqlite_switch))
			cw_log(LOG_ERROR, "Unable to register SQLite Switch\n");
		else {
			sqlite3HashInit(&extens,SQLITE_HASH_STRING,COPY_KEYS);
			cw_verbose(VERBOSE_PREFIX_2 "Registered SQLite Switch\n");
		}
	}
	else 
		cw_verbose(VERBOSE_PREFIX_2 "Sqlite Switch Disabled\n");

	if (has_cli) {
		cw_verbose(VERBOSE_PREFIX_2 "Activating SQLite CLI Command Set.\n");
		cw_cli_register(&cli_sqlite1); 
		cw_cli_register(&cli_sqlite2);	 
		cw_cli_register(&cli_sqlite3);	 
		cw_cli_register(&cli_sqlite4);	 
		cw_cli_register(&cli_sqlite5);	 
		cw_cli_register(&cli_sqlite6);
		cw_cli_register(&cli_sqlite7);
		cw_cli_register(&cli_sqlite8);
		cw_cli_register(&cli_sqlite9);
	}
	else 
		cw_verbose(VERBOSE_PREFIX_2 "SQLite CLI Command Set Not Configured.\n");

	return res;
}



int unload_module(void)
{
	int res = 0;
	do_reload = 1;

	cw_config_engine_deregister(&sqlite_engine);
	if (has_cdr) {
		cw_cdr_unregister(cdr_name);
		cw_verbose(VERBOSE_PREFIX_2 "SQLite CDR Disabled\n");
	}

	if (has_cli) {
		cw_verbose(VERBOSE_PREFIX_2 "SQLite CLI Disabled\n");
		cw_cli_unregister(&cli_sqlite1);  
		cw_cli_unregister(&cli_sqlite2);
		cw_cli_unregister(&cli_sqlite3);  
		cw_cli_unregister(&cli_sqlite4);  
		cw_cli_unregister(&cli_sqlite5);  
		cw_cli_unregister(&cli_sqlite6);  
		cw_cli_unregister(&cli_sqlite7);  
		cw_cli_unregister(&cli_sqlite8);  
		cw_cli_unregister(&cli_sqlite9);  
	}
		
	cw_unregister_application(app);




	if (has_switch) {
		cw_verbose(VERBOSE_PREFIX_2 "SQLite Switch Disabled\n");
		cw_unregister_switch(&sqlite_switch);
		sqlite3HashClear(&extens);
	}
	
	return res;
}

char *description(void)
{
	return desc;
}

int usecount(void)
{
	int res=0;
	STANDARD_USECOUNT(res);
	return res;
}

