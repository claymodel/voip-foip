/*	$Id$ */
/*
 * Copyright (c) 1990-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
 * HylaFAX is a trademark of Silicon Graphics
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
#include "Sys.h"
#include "FaxRequest.h"
#include "StackBuffer.h"
#include "class2.h"
#include "config.h"
#include "hash.h"			// pre-calculated hash values

#include <ctype.h>
#include <errno.h>


/*
 * HylaFAX job request file handling.
 */

extern void vlogError(const char* fmt, va_list ap);
extern void logError(const char* fmt ...);

FaxRequest::FaxRequest(const fxStr& qf, int f) : qfile(qf)
{
    reset();
    fd = f;
}

void
FaxRequest::reset(void)
{
    tts = 0;
    killtime = 0;
    retrytime = 0;
    state = 0;
    status = send_retry;
    pri = (u_short) -1;
    usrpri = FAX_DEFPRIORITY;
    pagewidth = pagelength = resolution = 0;
    npages = totpages = nskip = skippages = ncover = coverpages= 0;
    ntries = ndials = 0;
    minbr = BR_2400;
    desiredbr = BR_33600;
    desiredst = ST_0MS;
    desiredec = EC_ENABLE256;
    desireddf = DF_2DMMR;
    desiredtl = 0;
    totdials = 0, maxdials = (u_short) FAX_REDIALS;
    tottries = 0, maxtries = (u_short) FAX_RETRIES;
    useccover = true;
    usexvres = false;
    pagechop = chop_default;
    chopthreshold = -1;
    csi = fxStr::null;
    nsf = fxStr::null;
    notify = no_notice;
    jobtype = "facsimile";		// for compatibility w/ old clients
}

FaxRequest::~FaxRequest()
{
    if (fd != -1)
	Sys::close(fd);
}

#define	N(a)		(sizeof (a) / sizeof (a[0]))

FaxRequest::stringval FaxRequest::strvals[] = {
    { "external",	&FaxRequest::external },
    { "number",		&FaxRequest::number },
    { "mailaddr",	&FaxRequest::mailaddr },
    { "sender",		&FaxRequest::sender },
    { "jobid",		&FaxRequest::jobid },
    { "jobtag",		&FaxRequest::jobtag },
    { "pagehandling",	&FaxRequest::pagehandling },
    { "modem",		&FaxRequest::modem },
    { "faxnumber",	&FaxRequest::faxnumber },
    { "tsi",		&FaxRequest::tsi },
    { "receiver",	&FaxRequest::receiver },
    { "company",	&FaxRequest::company },
    { "location",	&FaxRequest::location },
    { "voice",		&FaxRequest::voice },
    { "fromcompany",	&FaxRequest::fromcompany },
    { "fromlocation",	&FaxRequest::fromlocation },
    { "fromvoice",	&FaxRequest::fromvoice },
    { "regarding",	&FaxRequest::regarding },
    { "comments",	&FaxRequest::comments },
    { "cover",		&FaxRequest::cover },
    { "client",		&FaxRequest::client },
    { "owner",		&FaxRequest::owner },
    { "groupid",	&FaxRequest::groupid },
    { "signalrate",	&FaxRequest::sigrate },
    { "dataformat",	&FaxRequest::df },
    { "jobtype",	&FaxRequest::jobtype },
    { "tagline",	&FaxRequest::tagline },
    { "subaddr",	&FaxRequest::subaddr },
    { "passwd",		&FaxRequest::passwd },
    { "doneop",		&FaxRequest::doneop },
    { "commid",		&FaxRequest::commid },
    { "csi",		&FaxRequest::csi },
    { "nsf",		&FaxRequest::nsf },
    { "pagerange",	&FaxRequest::pagerange },
};
FaxRequest::shortval FaxRequest::shortvals[] = {
    { "state",		&FaxRequest::state },
    { "npages",		&FaxRequest::npages },
    { "totpages",	&FaxRequest::totpages },
    { "nskip",		&FaxRequest::nskip },
    { "skippages",	&FaxRequest::skippages },
    { "ncover",		&FaxRequest::ncover },
    { "coverpages",	&FaxRequest::coverpages },
    { "ntries",		&FaxRequest::ntries },
    { "ndials",		&FaxRequest::ndials },
    { "totdials",	&FaxRequest::totdials },
    { "maxdials",	&FaxRequest::maxdials },
    { "tottries",	&FaxRequest::tottries },
    { "maxtries",	&FaxRequest::maxtries },
    { "pagewidth",	&FaxRequest::pagewidth },
    { "resolution",	&FaxRequest::resolution },
    { "pagelength",	&FaxRequest::pagelength },
    { "priority",	&FaxRequest::usrpri },
    { "schedpri",	&FaxRequest::pri },
    { "minbr",		&FaxRequest::minbr },
    { "desiredbr",	&FaxRequest::desiredbr },
    { "desiredst",	&FaxRequest::desiredst },
    { "desiredec",	&FaxRequest::desiredec },
    { "desireddf",	&FaxRequest::desireddf },
    { "desiredtl",	&FaxRequest::desiredtl },
    { "useccover",	&FaxRequest::useccover },
    { "usexvres",	&FaxRequest::usexvres },
};
const char* FaxRequest::opNames[18] = {
    "fax",
    "tiff",
    "!tiff",
    "pdf",
    "!pdf",
    "postscript",
    "!postscript",
    "pcl",
    "!pcl",
    "data",
    "!data",
    "poll",
    "page",
    "!page",
    "uucp",
    "15", "16", "17"
};
/*
 * These are also used in util/notify.sh.in
 */
const char* FaxRequest::notifyVals[4] = {
    "none",			// no_notice
    "when done",		// when_done
    "when requeued",		// when_requeued
    "when done+requeued"	// when_done|when_requeued
};
const char* FaxRequest::chopVals[4] = {
    "default",			// chop_default
    "none",			// chop_none
    "all",			// chop_all
    "last"			// chop_last
};

/*
 * Parse the contents of a job description file.
 */
bool
FaxRequest::readQFile(bool& rejectJob)
{
    fxStr statusstring;
    u_int statuscode = 999;
    rejectJob = false;
    lineno = 0;
    lseek(fd, 0L, SEEK_SET);			// XXX should only for re-read
    /*
     * Read the file contents in with one read.  If the
     * file is too large to fit in the buffer allocated
     * on the stack then dynamically allocate one.  The
     * 2K size was chosen based on statistics; most files
     * are less than 1K in size.
     *
     * NB: we don't mmap the file because we're going to
     *     modify its contents in memory during parsing
     *     (plus it's not clear that mmap is a win for
     *     such a small file).
     */
    struct stat sb;
    Sys::fstat(fd, sb);
    if (sb.st_size < 2) {
	error("Corrupted file (too small)");
	return (false);
    }
    char stackbuf[2048];
    char* buf = stackbuf;
    char* bp = buf;
    if (sb.st_size > (off_t)sizeof(stackbuf)-1)	// extra byte for terminating \0
	bp = buf = new char[sb.st_size+1];
    if (Sys::read(fd, bp, (u_int) sb.st_size) != sb.st_size) {
	error("Read error: %s", strerror(errno));
	if (buf != stackbuf)
	    delete [] buf;
	return (false);
    }
    /*
     * We set status to send_nobatch, just incase whatever is
     * writing the QFile hasn't.  This probaby only happens if
     * and old faxsend copy is being used that doesn't understand
     * the status field, but we'll try and handle that gracefully,
     * in a way which will cause least trouble.
     */
    status = send_nobatch;

    /*
     * Force \n-termination of the last line in the
     * file.  This simplifies the logic below by always
     * being able to look for \n-termination and not
     * worry about running off the end of the buffer.
     */
    char* ep = bp+sb.st_size;
    if (ep[-1] != '\n')
	ep[0] = '\n';
    do {
	lineno++;
	/*
	 * Collect command identifier and calculate hash.
	 * The hash value is used to identify the command
	 * using a set of pre-calculated values (see the
	 * mkhash program).  Note that other strings might
	 * hash to valid hash values; we don't care or check
	 * for this because we know the client-server protocol
	 * process only writes valid entries in the file.
	 */
	const char* cmd = bp;
	HASH_DECLARE(hash);
	for (; *bp != ':' && *bp != '\n'; bp++)
	{
	    HASH_ITERATE(hash, *bp);
	}
	if (*bp != ':') {			// invalid, skip line
	    error("Syntax error, missing ':' on line %u", (u_int) lineno);
	    while (*bp++ != '\n')
		;
	    continue;
	}
	HASH_FINISH(hash);

	*bp++ = '\0';				// null-terminate cmd
	/*
	 * Collect the parameter value.
	 */
	while (*bp == ' ')			// skip leading white space
	    bp++;
	char* tag = bp;
	while (*bp != '\n')
	    bp++;
	*bp++ = '\0';				// null-terminate tag
	// logError("%s[%u]: %s", cmd, hash, tag);
	switch (hash) {
	case H_EXTERNAL:	external = tag; break;
	case H_NUMBER:		number = tag; break;
	case H_MAILADDR:	mailaddr = tag; break;
	case H_SENDER:		sender = tag; break;
	case H_JOBID:		jobid = tag; break;
	case H_JOBTAG:		jobtag = tag; break;
	case H_COMMID:		commid = tag; break;
	case H_PAGEHANDLING:	pagehandling = tag; break;
	case H_MODEM:		modem = tag; break;
	case H_FAXNUMBER:	faxnumber = tag; break;
	case H_TSI:		tsi = tag; break;
	case H_CSI:		csi = tag; break;
	case H_RECEIVER:	receiver = tag; break;
	case H_COMPANY:		company = tag; break;
	case H_LOCATION:	location = tag; break;
	case H_VOICE:		voice = tag; break;
	case H_FROMLOCATION:	fromlocation = tag; break;
	case H_FROMVOICE:	fromvoice = tag; break;
	case H_REGARDING:	regarding = tag; break;
	case H_COVER:		cover = tag; break;
	case H_CLIENT:		client = tag; break;
	case H_OWNER:		owner = tag; break;
	case H_GROUPID:		groupid = tag; break;
	case H_SIGNALRATE:	sigrate = tag; break;
	case H_DATAFORMAT:	df = tag; break;
	case H_JOBTYPE:		jobtype = tag; break;
	case H_TAGLINE:		tagline = tag; break;
	case H_TOTTRIES:	tottries = atoi(tag); break;
	case H_SUBADDR:		subaddr = tag; break;
	case H_PASSWD:		passwd = tag; break;
	case H_STATE:		state = tag[0] - '0'; break;
	case H_COMMENTS:	comments = tag; break;
	case H_NPAGES:		npages = atoi(tag); break;
	case H_FROMCOMPANY:	fromcompany = tag; break;
	case H_TOTPAGES:	totpages = atoi(tag); break;
	case H_NTRIES:		ntries = atoi(tag); break;
	case H_MAXTRIES:	maxtries = atoi(tag); break;
	case H_NDIALS:		ndials = atoi(tag); break;
	case H_TOTDIALS:	totdials = atoi(tag); break;
	case H_MAXDIALS:	maxdials = atoi(tag); break;
	case H_PAGEWIDTH:	pagewidth = atoi(tag); break;
	case H_RESOLUTION:	resolution = atoi(tag); break;
	case H_PAGELENGTH:	pagelength = atoi(tag); break;
	case H_PRIORITY:	usrpri = atoi(tag); break;
	case H_SCHEDPRI:	pri = atoi(tag); break;
	case H_NSKIP:		nskip = atoi(tag); break;
	case H_SKIPPAGES:	skippages = atoi(tag); break;
	case H_DESIREDBR:	desiredbr = atoi(tag); break;
	case H_DESIREDST:	desiredst = tag[0] - '0'; break;
	case H_DESIREDEC:	desiredec = tag[0] - '0'; break;
	case H_DESIREDDF:	desireddf = tag[0] - '0'; break;
	case H_DESIREDTL:	desiredtl = tag[0] - '0'; break;
	case H_USECCOVER:	useccover = tag[0] - '0'; break;
	case H_USEXVRES:	usexvres = tag[0] - '0'; break;
	case H_TTS:		tts = atoi(tag); break;
	case H_KILLTIME:	killtime = atoi(tag); break;
	case H_RETRYTIME:	retrytime = atoi(tag); break;
	case H_NOTIFY:		checkNotifyValue(tag); break;
	case H_PAGECHOP:	checkChopValue(tag); break;
	case H_CHOPTHRESHOLD:	chopthreshold = atof(tag); break;
	case H_PAGERANGE:	pagerange = tag; break;
	case H_NSF:		nsf = tag; break;
	case H_STATUSCODE:	statuscode = atoi(tag); break;
	case H_DONEOP:		doneop = tag; break;
	case H_RETURNED:	status = (FaxSendStatus) atoi(tag); break;
	case H_MINBR:		minbr = atoi(tag); break;
	case H_NCOVER:		ncover = atoi(tag); break;
	case H_COVERPAGES:	coverpages = atoi(tag); break;

	case H_STATUS:
	    /*
	     * Check for multi-line status strings.
	     */
	    if (bp-tag > 1 && bp[-2] == '\\') {
		*--bp = '\n';		// put back original \n
		do {
		    lineno++, bp++;
		    while (*bp != '\n')
			bp++;
		} while (*bp == '\n' && bp > tag && bp[-1] == '\\');
		*bp++ = '\0';
	    }
	    statusstring = tag;
	    break;


	case H_POLL:		addItem(send_poll, tag); break;
	case H_FAX:		addItem(send_fax, tag); break;
	case H_PDF:
	    if (cmd[0] == '!')
		addItem(send_pdf_saved, tag);
	    else
		addItem(send_pdf, tag, rejectJob);
	    break;
	case H_TIFF:
	    if (cmd[0] == '!')
		addItem(send_tiff_saved, tag);
	    else
		addItem(send_tiff, tag, rejectJob);
	    break;
	case H_POSTSCRIPT:
	    if (cmd[0] == '!')
		addItem(send_postscript_saved, tag);
	    else
		addItem(send_postscript, tag, rejectJob);
	    break;
	case H_PCL:
	    if (cmd[0] == '!')
		addItem(send_pcl_saved, tag);
	    else
		addItem(send_pcl, tag, rejectJob);
	    break;
	case H_DATA:
	    if (cmd[0] == '!')
		addItem(send_data_saved, tag);
	    else
		addItem(send_data, tag, rejectJob);
	    break;
	case H_PAGE:
	    if (cmd[0] == '!')
		addItem(send_page_saved, tag);
	    else
		addItem(send_page, tag);
	    break;
	default:
	    error("Unknown field %s[%u]: %s", cmd, hash, tag);
	}
    } while (bp < ep);
    if (pri == (u_short) -1)
	pri = usrpri;
    if (tts == 0)	// distinguish ``now'' from unset
	tts = Sys::now();
    /*
     * Validate certain items that are assumed to have
     * ``suitable values'' by higher-level code (i.e.
     * the scheduler).
     */
    if (state < state_suspended || state > state_failed) {
	error("Invalid scheduler state %u in job request", state);
	rejectJob = true;
    }
#define	isNull(s)	((s).length() == 0)
    if (isNull(number) || isNull(mailaddr) || isNull(sender) || isNull(jobid)
     || isNull(modem)  || isNull(client)   || isNull(owner)) {
	rejectJob = true;
	error("Null or missing %s in job request",
	    isNull(number)   ? "number" :
	    isNull(mailaddr) ? "mailaddr" :
	    isNull(sender)   ? "sender" :
	    isNull(jobid)    ? "jobid" :
	    isNull(modem)    ? "modem" :
	    isNull(client)   ? "client" :
			       "owner"
	);
    }

    /*
     * We try to default it to something sane if it wasn't in
     * the qfile already
     */
    if (statuscode == 999 && statusstring.length() == 0)
	    statuscode = 0;

    result = Status(statuscode, "%s", (const char*)statusstring);
    if (minbr > BR_33600)	minbr = BR_33600;
    if (desiredbr > BR_33600)	desiredbr = BR_33600;
    if (desiredst > ST_40MS)	desiredst = ST_40MS;
    if (desiredec > EC_ECLFULL)	desiredec = EC_ECLFULL;
    if (desireddf > DF_2DMMR)	desireddf = DF_2DMMR;
    if (buf != stackbuf)			// dynamically allocated buffer
	delete [] buf;
    return (true);
}

/*
 * Re-read a job description file.
 *
 * Note that a file that has been written *should*
 * have every entry set in it and so re-reading it
 * will cause all fields to be written.  This means
 * that we should not need to reset the state to the
 * default values assigned when the instance is created;
 * except for the items array which is dynamically
 * allocated and appended to.  If you don't believe
 * this, enable the code notdef'd out below.
 */
bool
FaxRequest::reReadQFile(bool& rejectJob)
{
#ifdef notdef
    reset();					// non-string items
    for (int i = N(strvals)-1; i >= 0; i--)	// string stuff
	(*this).*strvals[i].p = "";
#endif
    items.resize(0);				// document/polling requests
    return (readQFile(rejectJob));
}

#define	DUMP(fp, vals, fmt, cast) {					\
    for (u_int i = 0; i < N(vals); i++)					\
	sb.fput(fmt, vals[i].name, cast((*this).*vals[i].p));		\
}

/*
 * Maybe write to temp and rename instead of
 * updating in-place.  The files are so small
 * however that we're unlikely to ever get ENOSPC
 * when updating an existing file.  Also by using
 * an existing file we avoid allocating a new
 * disk block each time, instead overwriting the
 * already allocated one.  This can be meaningful
 * on a busy server given the potential number of
 * times we update the q file.
 */
void
FaxRequest::writeQFile()
{
    fxStackBuffer sb;
    sb.fput("tts:%u\n", tts);
    sb.fput("killtime:%u\n", killtime);
    sb.fput("retrytime:%u\n", retrytime);

    DUMP(fp, shortvals,	"%s:%d\n", (int));
    DUMP(fp, strvals,	"%s:%s\n", (const char*));
    /*
     * Escape unprotected \n's with \\.
     */
    sb.put("status:");
    const char* cp = result.string();
    const char* sp = cp;
    while (*cp) {
	if (*cp == '\n' && cp[-1] != '\\') {
	   sb.put(sp, cp-sp);
	   sb.put('\\');
	   sp = cp;
	}
	cp++;
    }
    sb.put(sp, cp-sp); sb.put('\n');
    sb.fput("statuscode:%d\n", result.value());
    sb.fput("returned:%d\n", status);
    sb.fput("notify:%s\n", notifyVals[notify&3]);
    sb.fput("pagechop:%s\n", chopVals[pagechop&3]);
    sb.fput("chopthreshold:%g\n", chopthreshold);
    for (u_int i = 0; i < items.length(); i++) {
	const FaxItem& fitem = items[i];
	sb.fput("%s:%u:%s:%s\n"
	    , opNames[fitem.op&15]
	    , fitem.dirnum
	    , (const char*) fitem.addr
	    , (const char*) fitem.item
	);
    }
    lseek(fd, 0L, SEEK_SET);
    Sys::write(fd, sb, sb.getLength());
    (void) ftruncate(fd, sb.getLength());
    // XXX maybe should fsync, but not especially portable
}

/*
 * Return the base document name given a
 * per-job document name (either with a
 * jobid suffix or, if a cover page, with
 * a ``.cover'' suffix).
 */
fxStr
FaxRequest::mkbasedoc(const fxStr& file)
{
    fxStr doc(file);
    u_int l = doc.nextR(doc.length(), '.');
    doc.resize(l-1);
    return (doc);
}

/*
 * Check if a document that is about to be removed from
 * the job request was converted from another.  If so,
 * rename the source document according to convention
 * so that all references to the document point to the
 * same file when everything has been sent.  This has the
 * effect of decrementing the link count on the source
 * file and permits us to use the link count as a reference
 * use count for releasing imaged documents (see the
 * large comments explaining this in the scheduler).
 */
void
FaxRequest::renameSaved(u_int fi)
{
    if (fi > 0 && items[fi-1].isSavedOp()) {
	FaxItem& src = items[fi-1];
	fxStr basedoc = mkbasedoc(src.item);
	if (Sys::rename(src.item, basedoc) < 0) {
	    logError("Unable to rename transmitted document %s: %s",
		(const char*) src.item, strerror(errno));
	}
	// Posix rename will succeed without doing anything if the
	// source and destination files are hard linked
	Sys::unlink(src.item);		// just remove it
	src.item = basedoc;		// change job reference
    }
}

/*
 * Does the specified document (assumed to be send_fax)
 * appear to have any potential source references?
 */
bool
FaxRequest::isUnreferenced(u_int fi)
{
    if (fi > 0 && items[fi-1].isSavedOp()) {
	struct stat sb;
	if (Sys::stat(mkbasedoc(items[fi-1].item), sb) < 0 ||
	  sb.st_nlink == 1)
	    return (true);
    }
    return (false);
}

static bool
hasDotDot(const char* pathname)
{
    const char* cp = pathname;
    while (cp) {
	if (cp[0] == '.')		// NB: good enough
	    return (true);
	if ((cp = strchr(cp, '/')))
	    cp++;
    }
    return (false);
}

bool
FaxRequest::checkDocument(const char* pathname)
{
    /*
     * Scan full pathname to disallow access to
     * files outside the spooling hiearchy.
     */
    if (pathname[0] == '/' || hasDotDot(pathname)) {
	error("Invalid document file \"%s\"", pathname);
	return (false);
    }
    int fd = Sys::open(pathname, 0);
    if (fd == -1) {
	error("Can not access document file \"%s\": %s",
	    pathname, strerror(errno));
	return (false);
    }
    Sys::close(fd);
    return (true);
}

/*
 * Add a request entry that does not require checking
 * the document pathname to make sure that it is valid.
 */
void
FaxRequest::addItem(FaxSendOp op, char* tag)
{
    char* cp = tag;
    while (*cp && *cp != ':')
	cp++;
    int dirnum;
    if (*cp == ':') {			// directory index
	dirnum = atoi(tag);
	tag = ++cp;
    } else
	dirnum = 0;
    while (*cp && *cp != ':')
	cp++;
    if (*cp == ':')			// address info
	*cp++ = '\0';
    else
	cp = tag, tag = "";
    items.append(FaxItem(op, dirnum, tag, cp));
}

/*
 * Add a request entry and verify the document is valid.
 */
void
FaxRequest::addItem(FaxSendOp op, char* tag, bool& rejectJob)
{
    char* cp = tag;
    while (*cp && *cp != ':')
	cp++;
    int dirnum;
    if (*cp == ':') {			// directory index
	dirnum = atoi(tag);
	tag = ++cp;
    } else
	dirnum = 0;
    while (*cp && *cp != ':')
	cp++;
    if (*cp == ':')			// address info
	*cp++ = '\0';
    else
	cp = tag, tag = "";
    if (!checkDocument(cp)) {
    	error("Rejected document in corrupt job request");
	rejectJob = true;
    }
    else
	items.append(FaxItem(op, dirnum, tag, cp));
}

bool
FaxRequest::isStrCmd(const char* cmd, u_int& ix)
{
    for (int i = N(strvals)-1; i >= 0; i--)
	if (strcmp(strvals[i].name, cmd) == 0) {
	    ix = i;
	    return (true);
	}
    return (false);
}

bool
FaxRequest::isShortCmd(const char* cmd, u_int& ix)
{
    for (int i = N(shortvals)-1; i >= 0; i--)
	if (strcmp(shortvals[i].name, cmd) == 0) {
	    ix = i;
	    return (true);
	}
    return (false);
}

void
FaxRequest::checkNotifyValue(const char* tag)
{
    for (int i = N(notifyVals)-1; i >= 0; i--)
	 if (strcmp(notifyVals[i], tag) == 0) {
	    notify = i;
	    return;
	}
    error("Invalid notify value \"%s\"", tag);
}

void
FaxRequest::checkChopValue(const char* tag)
{
    for (int i = N(chopVals)-1; i >= 0; i--)
	 if (strcmp(chopVals[i], tag) == 0) {
	    pagechop = i;
	    return;
	}
    error("Invalid pagechop value \"%s\"", tag);
}

u_int
FaxRequest::findItem(FaxSendOp op, u_int ix) const
{
    while (ix < items.length()) {
	if (items[ix].op == op)
	    return (ix);
	ix++;
    }
    return fx_invalidArrayIndex;
}

void
FaxRequest::insertFax(u_int ix, const fxStr& file)
{
    items.insert(FaxItem(send_fax, 0, fxStr::null, file), ix);
}

void
FaxRequest::error(const char* fmt0 ...)
{
    fxStr fmt = fxStr::format("%s: line %u: %s", (const char*) qfile, (u_int) lineno, fmt0);
    va_list ap;
    va_start(ap, fmt0);
    vlogError(fmt, ap);
    va_end(ap);
}
