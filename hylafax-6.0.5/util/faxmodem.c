/*	$Id$ */
/*
 * Copyright (c) 1995-1996 Sam Leffler
 * Copyright (c) 1995-1996 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "config.h"
#include "port.h"

static void
fatal(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (isatty(fileno(stderr))) {
        vfprintf(stderr, fmt, ap);
        putc('\n', stderr);
    } else {
        vsyslog(LOG_ERR, fmt, ap);
    }
    va_end(ap);
    exit(-1);
}


#define	BIT(i)	(1<<(i))

int fxmin(int a, int b)          { return (a < b) ? a : b; }

const char OPAREN = '(';
const char CPAREN = ')';
const char COMMA = ',';
const char SPACE = ' ';

/*
 * Parse a Class 2 parameter range string.  This is very
 * forgiving because modem vendors do not exactly follow
 * the syntax specified in the "standard".  Try looking
 * at some of the responses given by rev ~4.04 of the
 * ZyXEL firmware (for example)!
 *
 * NB: We accept alphanumeric items but don't return them
 *     in the parsed range so that modems like the ZyXEL 2864
 *     that indicate they support ``Class Z'' are handled.
 */
int
vparseRange(const char* cp, int masked, int nargs, ... )
{
    int b = 1;
    va_list ap;
    va_start(ap, nargs);
    while (nargs-- > 0) {
	char matchc;
	int acceptList;
	int mask;

	while (cp[0] == SPACE)
	    cp++;
	if (cp[0] == OPAREN) {				/* (<items>) */
	    matchc = CPAREN;
	    acceptList = 1;
	    cp++;
	} else if (isalnum(cp[0])) {			/* <item> */
	    matchc = COMMA;
	    acceptList = (nargs == 0);
	} else {					/* skip to comma */
	    b = 0;
	    break;
	}
	mask = 0;
	while (cp[0] && cp[0] != matchc) {
	    int v;
	    int r;

	    if (cp[0] == SPACE) {			/* ignore white space */
		cp++;
		continue;
	    }
	    if (!isalnum(cp[0])) {
		b = 0;
		goto done;
	    }
	    if (isdigit(cp[0])) {
		v = 0;
		do {
		    v = v*10 + (cp[0] - '0');
		} while (isdigit((++cp)[0]));
	    } else {
		v = -1;					/* XXX skip item below */
		while (isalnum((++cp)[0]))
		    ;
	    }
	    r = v;
	    if (cp[0] == '-') {				/* <low>-<high> */
		cp++;
		if (!isdigit(cp[0])) {
		    b = 0;
		    goto done;
		}
		r = 0;
		do {
		    r = r*10 + (cp[0] - '0');
		} while (isdigit((++cp)[0]));
	    } else if (cp[0] == '.') {			/* <d.b> */
		cp++;
		while (isdigit(cp[0]))			/* XXX */
		    cp++;
		v++, r++;				/* XXX 2.0 -> 3 */
	    }
	    if (v != -1) {			/* expand range or list */
		if ((BIT(nargs) & masked) == BIT(nargs)) {
		    /*
		     * These are pre-masked values. T.32 Table 21 gives valid
		     * values as: 00, 01, 02, 04, 08, 10, 20, 40 (hex).
		     *
		     * Some modems may say "(00-7F)" when what's meant is
		     * "(00-40)" or simply "(7F)".
		     */
		    if (v == 00 && r == 127)
			v = r = 127;
		    if (v == r)
			mask = v;
		    else {
			r = fxmin(r, 64);	/* clamp to valid range */
			mask = 0;
			for (; v <= r; v++)
			    if (v == 0 || v == 1 || v == 2 || v == 4 || v == 8 || v == 16 || v == 32 || v == 64)
				mask += v;
		    }
		}

		else {
			r = fxmin(r, 31);	/* clamp to valid range */
			for (; v <= r; v++)
			    mask |= 1<<v;
		}
	    }
	    if (acceptList && cp[0] == COMMA)		/* (<item>,<item>...) */
		cp++;
	}
	*va_arg(ap, int*) = mask;
	if (cp[0] == matchc)
	    cp++;
	if (matchc == CPAREN && cp[0] == COMMA)
	    cp++;
    }
done:
    va_end(ap);
    return (b);
}

/*
 * Class 2 Fax Modem Definitions.
 */
#define	VR_ALL	(BIT(7)-1)
#define	BR_ALL	(BIT(6)-1)	/* don't encode > 14.4 Kbps */
#define	WD_ALL	(BIT(5)-1)
#define	LN_ALL	(BIT(3)-1)
#define	DF_ALL	(BIT(4)-1)
#define EC_DISABLE 0
#define	EC_ALL	(BIT(5)-1)
#define	BF_ALL	(BIT(2)-1)
#define	ST_ALL	(BIT(8)-1)

/*
 * Parse a Class 2 parameter specification and
 * return a string with the encoded information.
 * This should encode identically as does
 * Class2Params::encodeCaps().
 */
int
parseCapabilities(const char* cp, u_int* caps)
{
    /* we are limited to 32 bits, thus BR_ALL is restricted above */
    int vr, br, wd, ln, df, ec, bf, st;
    
    /*
     * reversed, count-down style bitmask: used to tell vparseRange
     * which capabilities are 'true' bitmask (not just max bit);
     * right now they are vr and bf (ie: vr is 7th position considering
     * the list in reversed order and base 0)
     */
    int masked = (1 << 7) + (1 << 1);
    
    if (vparseRange(cp, masked, 8, &vr,&br,&wd,&ln,&df,&ec,&bf,&st)) {
	*caps = (vr&VR_ALL)
	     | ((br&BR_ALL)<<8)
	     | ((wd&WD_ALL)<<14)
	     | ((ln&LN_ALL)<<19)
	     | ((df&DF_ALL)<<22)
	     | ((ec&EC_DISABLE)<<26)
	     | (((ec&EC_ALL) != 0 ? 1 : 0)<<27)
	     | ((bf&BF_ALL)<<28)
	     | ((st&ST_ALL)<<30)
	     ;
	return (1);
    } else
	return (0);
}
#undef	BIT

extern	int cvtFacility(const char*, int*);

int
main(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int fifo, c;
    char* spooldir = FAX_SPOOLDIR;
    u_int caps;
    char canpoll = 'P';				/* can poll by default */
    char devname[80];
    int cmdlen, pdevlen;
    char cmd[80];
    const char* usage = "[-c fax-capabilities] [-p] [-P] [-u priority] [-q queue-dir]";
    char* cp;
    int facility = LOG_DAEMON;
    int priority = -1;

    (void) cvtFacility(LOG_FAX, &facility);
    openlog(argv[0], LOG_PID|LOG_ODELAY, facility);
    /*
     * Setup defaults:
     *
     *	(vr),(br),(wd),(ln),(df),(ec),(bf),(st)
     * where,
     *	vr	vertical resolution
     *	br	bit rate
     *	wd	page width
     *	ln	page length
     *	df	data compression
     *	ec	error correction
     *	bf	binary file transfer
     *	st	scan time/line
     */
    parseCapabilities("(0,1),(0-3),(0-4),(0-2),(0),(0),(0),(0-7)", &caps);
    while ((c = getopt(argc, argv, "c:q:u:pP")) != -1)
	switch (c) {
	case 'c':
	    if (!parseCapabilities(optarg, &caps))
		fatal("Syntax error parsing Class 2 capabilities string \"%s\"",
		    optarg);
	    break;
	case 'p':
	case 'P':
	    canpoll = c;
	    break;
	case 'q':
	    spooldir = optarg;
	    break;
	case 'u':
	    priority = atoi(optarg);
	    break;
	case '?':
	    fatal("usage: %s %s modem", argv[0], usage);
	    /*NOTREACHED*/
	}
    if (optind != argc-1) {
        fatal("Missing modem device.\nusage: %s %s modem", argv[0], usage);
    }
    pdevlen = strlen(_PATH_DEV);
    if (strncmp(argv[optind], _PATH_DEV, pdevlen) == 0) {
        if (strlen(argv[optind] + pdevlen) < sizeof(devname)) {
            strcpy(devname, argv[optind] + pdevlen);
        } else {
            fatal("Argument is too long: %s", argv[optind] + pdevlen);
        }
    } else {
        if (strlen(argv[optind]) < sizeof(devname)) {
            strcpy(devname, argv[optind]);
        } else {
            fatal("Argument is too long: %s", argv[optind]);
        }
    }
    for (cp = devname; (cp = strchr(cp, '/')); *cp++ = '_')
	;
    if (chdir(spooldir) < 0) {
        fatal("%s: chdir: %s", spooldir, strerror(errno));
    }
    fifo = open(FAX_FIFO, O_WRONLY|O_NDELAY);
    if (fifo < 0) {
        fatal("%s: open: %s", FAX_FIFO, strerror(errno));
    }
    if (priority != -1) {
        cmdlen = snprintf(cmd, sizeof(cmd), "+%s:R%c%08x:%x", devname, canpoll, caps, priority);
    } else {
        cmdlen = snprintf(cmd, sizeof(cmd), "+%s:R%c%08x", devname, canpoll, caps);
    }
    if (cmdlen < 0 || cmdlen > sizeof(cmd) || write(fifo, cmd, cmdlen) != cmdlen) {
        fatal("%s: FIFO write failed for command (%s)",
            argv[0], strerror(errno));
    }
    (void) close(fifo);
    return 0;
}
