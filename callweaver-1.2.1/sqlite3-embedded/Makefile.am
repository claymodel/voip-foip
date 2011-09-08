AUTOMAKE_OPTS		= gnu foreign

DEFS += -include $(top_builddir)/include/confdefs.h

cwlib_LTLIBRARIES	= libcwsqlite3.la

libcwsqlite3_la_SOURCES = \
  src/alter.c \
  src/analyze.c \
  src/attach.c \
  src/auth.c \
  src/btree.c \
  src/btree.h \
  src/build.c \
  src/callback.c \
  src/complete.c \
  src/date.c \
  src/delete.c \
  src/expr.c \
  src/func.c \
  src/hash.c \
  src/hash.h \
  src/insert.c \
  src/legacy.c \
  src/loadext.c \
  src/main.c \
  src/os.c \
  src/os_unix.c \
  src/os_win.c \
  src/os_os2.c \
  src/pager.c \
  src/pager.h \
  src/pragma.c \
  src/prepare.c \
  src/printf.c \
  src/random.c \
  src/select.c \
  src/shell.c \
  src/sqlite.h.in \
  src/sqliteInt.h \
  src/table.c \
  src/tclsqlite.c \
  src/tokenize.c \
  src/trigger.c \
  src/utf.c \
  src/update.c \
  src/util.c \
  src/vacuum.c \
  src/vdbe.c \
  src/vdbe.h \
  src/vdbeapi.c \
  src/vdbeaux.c \
  src/vdbefifo.c \
  src/vdbemem.c \
  src/vdbeInt.h \
  src/vtab.c \
  src/where.c

nodist_libcwsqlite3_la_SOURCES =  sqlite3.h opcodes.c opcodes.h parse.c parse.h 

libcwsqlite3_la_CFLAGS  = $(AM_CFLAGS) @SQLITE3_THREADSAFE_CFLAGS@ -I$(srcdir)/src
libcwsqlite3_la_LIBADD  = -lpthread
libcwsqlite3_la_LDFLAGS = -no-undefined -version-info "8:6:8"

EXTRA_DIST = src/parse.y tool/lempar.c
BUILT_SOURCES = opcodes.c opcodes.h parse.c parse.h keywordhash.h sqlite3.h
CLEANFILES = parse.out parse.c parse.h opcodes.c opcodes.h keywordhash.h sqlite3.h \
	    lempar.c parse.h.temp parse.y

opcodes.c: opcodes.h mkopcodec.awk
	sort -n -b -k 2 opcodes.h | ${AWK} -f mkopcodec.awk > opcodes.c

opcodes.h: parse.h src/vdbe.c mkopcodeh.awk
	cat parse.h $(srcdir)/src/vdbe.c | ${AWK} -f mkopcodeh.awk > opcodes.h

parse.c: sqlite3.h parse.y lempar.c lemon
	./lemon parse.y
	mv parse.h parse.h.temp

parse.h: parse.c
	awk -f addopcodes.awk parse.h.temp >parse.h

lempar.c: tool/lempar.c
	cp $(srcdir)/tool/lempar.c .

parse.y: src/parse.y
	cp $(srcdir)/src/parse.y .

keywordhash.h: mkkeywordhash tool/mkkeywordhash.c
	./mkkeywordhash > keywordhash.h

sqlite3.h:  src/sqlite.h.in
	${SED} -e s/--VERS--/3.3.8/ $(srcdir)/src/sqlite.h.in | \
	${SED} -e s/--VERSION-NUMBER--/3003008/ > sqlite3.h

lemon: tool/lemon.c
	$(HOST_CC) -o $@ $<
	
mkkeywordhash: tool/mkkeywordhash.c
	$(HOST_CC) -o $@ $<


noinst_PROGRAMS = lemon mkkeywordhash

