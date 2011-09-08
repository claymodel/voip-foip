ASTERISK_SOURCE=/usr/src/asterisk
AST_MODULES_DIR=/usr/lib/asterisk/modules

DEBUG=-g
DEFINE=-Iinclude -I$(ASTERISK_SOURCE)/include -D_REENTRANT -D_GNU_SOURCE  -O6  -DCRYPTO -fPIC -DBUSYDETECT_MARTIN -DNEW_PRI_HANGUP
CFLAG= -march=i586 -pipe -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations $(DEBUG)

FLS+=chan_sccp
FLS+=sccp_actions
FLS+=sccp_channel
FLS+=sccp_device
FLS+=sccp_line
FLS+=sccp_helper
FLS+=sccp_pbx
FLS+=sccp_cli
FLS+=sccp_softkeys
FLS+=sccp_sched
FLS+=sccp_socket

CFLAGS+=$(DEFINE) $(CFLAG) -I$(ASTERISK_SOURCE)
HEADERS=$(shell for i in $(FLS) ; do echo $$i.h ; done)
OBJECTS=$(shell for i in $(FLS) ; do echo .tmp/$$i.o ; done)
SOURCES=$(shell for i in $(FLS) ; do echo $$i.c ; done)

chan_sccp.so: .tmp $(OBJECTS) $(HEADERS) $(SOURCES)
	@echo "Linking chan_sccp.so"
	@$(CC) -shared -Xlinker -x -o chan_sccp.so $(OBJECTS)

.tmp/%.o: $(HEADERS)
	@printf "Now compiling .... %-15s\t%s lines \n" $*.c "`wc -l $*.c | cut -b4-8`"
	@$(CC) -I/usr/include/ $(CFLAGS) -I$(ASTERISK_SOURCE) $(DEFINE) -c $*.c -o .tmp/$*.o

install: chan_sccp.so
	rm -f $(AST_MODULES_DIR)/chan_sccp.so
	cp chan_sccp.so $(AST_MODULES_DIR)
	if ! [ -f /etc/asterisk/sccp.conf ]; then cp conf/sccp.conf /etc/asterisk/; fi

clean:
	rm -rf chan_sccp.so .tmp

Makefile:

MEEP=chan_sccp.o
MOO=chan_sccp.c	

.tmp/chan_sccp.o:     chan_sccp.c
.tmp/sccp_actions.o:  sccp_actions.c
.tmp/sccp_channel.o:  sccp_channel.c
.tmp/sccp_device.o:   sccp_device.c
.tmp/sccp_helper.o:   sccp_helper.c
.tmp/sccp_pbx.o:      sccp_pbx.c
.tmp/sccp_cli.o:      sccp_cli.c
.tmp/sccp_line.o:     sccp_line.c
.tmp/sccp_softkeys.o: sccp_softkeys.c
.tmp/sccp_sched.o:    sccp_sched.c
.tmp/sccp_socket.o:   sccp_socket.c

.tmp:
	@mkdir -p .tmp
