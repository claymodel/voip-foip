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
#include "MsgFmt.h"
#include "MIMEState.h"
#include "StackBuffer.h"
#include "StrArray.h"
#include "Sys.h"
#include "SendFaxClient.h"
#include "NLS.h"
#include "config.h"

#include <ctype.h>
#include <errno.h>
#if HAS_LOCALE
extern "C" {
#include <locale.h>
}
#endif

class MySendFaxClient : public SendFaxClient {
public:
    MySendFaxClient();
    ~MySendFaxClient();

    void setup(bool);
    bool setConfigItem(const char* tag, const char* value);
    void fatal(const char* fmt ...);
};
MySendFaxClient::MySendFaxClient() {}
MySendFaxClient::~MySendFaxClient() {}
void
MySendFaxClient::setup(bool b)
{
    resetConfig();
    readConfig(FAX_SYSCONF);
    readConfig(FAX_LIBDATA "/sendfax.conf");
    readConfig(FAX_LIBDATA "/faxmail.conf");
    readConfig(FAX_USERCONF);
    setVerbose(b);
    FaxClient::setVerbose(b);
}
bool MySendFaxClient::setConfigItem(const char* tag, const char* value)
    { return SendFaxClient::setConfigItem(tag, value); }

class faxMailApp : public TextFormat, public MsgFmt {
private:
    bool	markDiscarded;		// mark MIME parts not handled
    bool	withinFile;		// between beginFile() & endFile()
    bool	empty;			// No acutall formatted output
    bool	debugLeaveTmp;		// Leave tmp files alone for debuging
    fxStr	mimeConverters;		// pathname to MIME converter scripts
    fxStr	mailProlog;		// site-specific prologue definitions
    fxStr	clientProlog;		// client-specific prologue info
    fxStr  	pageSize;       // record specified page size
    fxStr	msgDivider;		// digest message divider
    fxStrArray	tmps;			// temp files
    fxStr	tmpDir;			// directory for temp files

    fxStr	mimeid;			// For identifing mime parts uniquely

    MySendFaxClient* client;		// for direct delivery
    SendFaxJob*	job;			// reference to outbound job
    fxStr	coverTempl;		// coverpage template file
    fxStr	mailUser;		// user ID for contacting server
    fxStr	notify;			// notification request
    bool	autoCoverPage;		// make cover page for direct delivery
    bool	formatEnvHeaders;	// format envelope headers
    bool	trimText;		// trim text parts

    void formatMIME(FILE* fd, MIMEState& mime, MsgFmt& msg);
    bool formatText(FILE* fd, MIMEState& mime);
    void formatMultipart(FILE* fd, MIMEState& mime, MsgFmt& msg);
    void formatMessage(FILE* fd, MIMEState& mime, MsgFmt& msg);
    void formatApplication(FILE* fd, MIMEState& mime);
    void formatDiscarded(MIMEState& mime);
    void formatAttachment (FILE* fd, MIMEState& mime);
    bool formatWithExternal(FILE* fd, const fxStr& app, MIMEState& mime);

    void emitClientPrologue(FILE*);

    void discardPart(FILE* fd, MIMEState& mime);
    bool copyPart(FILE* fd, MIMEState& mime, fxStr& tmpFile);
    bool runConverter(const fxStr& app, const fxStr& input, const fxStr& output,
		MIMEState& mime);
    void copyFile(FILE* fd, const char* filename);

    void resetConfig();
    void setupConfig();
    bool setConfigItem(const char* tag, const char* value);

    void usage(void);
public:
    faxMailApp();
    ~faxMailApp();

    void run(int argc, char** argv);

    void setVerbose(bool b)		{ verbose = b; }
    void setBoldFont(const char* cp)	{ boldFont = cp; }
    void setItalicFont(const char* cp)	{ italicFont = cp; }
};

faxMailApp::faxMailApp()
  : tmpDir(_PATH_TMP "/faxmail.XXXXXX")
{
    client = NULL;

    faxMailApp::setupConfig();		// NB: virtual
    setTitle("HylaFAX-Mail");

    if (Sys::mkdtemp(&tmpDir[0]) == NULL)
	fxFatal(_("Cannot create temp directory %s"), (const char*) tmpDir);
}

faxMailApp::~faxMailApp()
{
    delete client;
    if (! debugLeaveTmp)
    {
	for (u_int i = 0, n = tmps.length(); i < n; i++)
	    Sys::unlink(tmps[i]);
	Sys::rmdir(tmpDir);
    }
}

void
faxMailApp::run(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int c;

    resetConfig();
    readConfig(FAX_SYSCONF);
    readConfig(FAX_LIBDATA "/faxmail.conf");
    readConfig(FAX_USERCONF);

    while ((c = Sys::getopt(argc, argv, "12b:cC:df:H:i:M:nNp:rRs:t:Tu:vW:")) != -1)
	switch (c) {
	case '1': case '2':		// format in 1 or 2 columns
	    setNumberOfColumns(c - '0');
	    break;
	case 'b':			// bold font for headers
	    setBoldFont(optarg);
	    break;
	case 'c':			// truncate lines
	    setLineWrapping(false);
	    break;
	case 'C':			// specify cover template to use
	    coverTempl = optarg;
	    break;
	case 'd':			// kept for backwards compatiblity
	    // XXX: Should we warn this isn't needed?
	    break;
	case 'f':			// default font for text body
	    setTextFont(optarg);
	    break;
	case 'H':			// page height
	    setPageHeight(atof(optarg));
	    break;
	case 'i':			// italic font for headers
	    setItalicFont(optarg);
	    break;
	case 'M':			// page margins
	    setPageMargins(optarg);
	    break;
	case 'n':			// suppress cover page
	    autoCoverPage = false;
	    break;
	case 'N':
	    formatEnvHeaders = false;
	    break;
	case 'p':			// point size
	    setTextPointSize(TextFormat::inch(optarg));
	    break;
	case 'r':			// rotate page (landscape)
	    setPageOrientation(TextFormat::LANDSCAPE);
	    break;
	case 'R':			// don't rotate page (portrait)
	    setPageOrientation(TextFormat::PORTRAIT);
	    break;
	case 's':			// page size
        pageSize = optarg;
	    setPageSize(pageSize);
	    break;
	case 't':
	    notify = optarg;
	    break;
	case 'T':			// suppress formatting MIME text parts
	    trimText = true;
	    break;
	case 'u':			// mail/login user
	    mailUser = optarg;
	    break;
	case 'v':			// trace work
	    setVerbose(true);
	    break;
	case 'W':			// page width
	    setPageWidth(atof(optarg));
	    break;
	case '?':
	    usage();
	    /*NOTREACHED*/
	}

    MIMEState mime("text", "plain");
    parseHeaders(stdin, mime.lineno);	// collect top-level headers

    if (!getPageHeaders())		// narrow top+bottom margins
	setPageMargins("t=0.35in,b=0.25in");
    else
	setPageMargins("t=0.6in,b=0.25in");


    /*
     * Setup to submit the formatted facsimile directly
     * to a server.
     */
    client = new MySendFaxClient;
    client->setup(verbose);

    /*
     * Setup the destination (dialstring and
     * optionally a receipient).  Dialing stuff
     * given on the command line is replaced by
     * information in the envelope so that strings
     * that contain characters that are invalid
     * email addresses can be specified.
     */
    job = &client->addJob();
    if (optind < argc) {			// specified on command line
	fxStr dest(argv[optind]);		// [person@]number
	u_int l = dest.next(0, '@');
	if (l != dest.length()) {
	    job->setCoverName(dest.head(l));
	    dest.cut(0, l+1);
	}
	job->setDialString(dest);
    }
    const fxStr* s;
    if ((s = findHeader("x-fax-dialstring")))  // dialstring in envelope
	job->setDialString(*s);
    if (job->getDialString() == "")
	fxFatal(_("No Destination/Dialstring specified"));

    /*
     * Establish the sender's identity.
     */
    if (optind+1 < argc) {
	client->setFromIdentity(argv[optind+1]);
    } else if ((s = findHeader("from"))) {
	client->setFromIdentity(*s);
    } else {
	fxFatal(_("No From/Sender identity specified"));
    }

    if (pageSize != "")
        job->setPageSize(pageSize);
    if (notify != "")
	job->setNotification((const char*)notify);

    /*
     * Scan envelope for any meta-headers that
     * control how job submission is to be done.
     */
    job->setAutoCoverPage(autoCoverPage);
    for (u_int i = 0, n = fields.length();  i < n; i++) {
	const fxStr& field = fields[i];
	if (strncasecmp(field, "x-fax-", 6) != 0)
	    continue;
	fxStr tag(field.tail(field.length() - 6));
	tag.lowercase();
	if (job->setConfigItem(tag, MsgFmt::headers[i]))
	    ;
	else if (client->setConfigItem(tag, MsgFmt::headers[i]))
	    ;
    }

    /*
     * If a cover page is desired fill in any info
     * from the envelope that might be useful.
     */
    if (job->getAutoCoverPage()) {
	if (coverTempl.length())
	    job->setCoverTemplate(coverTempl);
	/*
	 * If nothing has been specified for a
	 * regarding field on the cover page and
	 * a subject line exists, use that info.
	 */
	if (job->getCoverRegarding() == "" && (s = findHeader("subject"))) {
	    fxStr subj(*s);
	    while (subj.length() > 3 && strncasecmp(subj, "Re:", 3) == 0)
		subj.remove(0, subj.skip(3, " \t"));
	    job->setCoverRegarding(subj);
	}
	/*
	 * Likewise for the receipient name.
	 */
	if (job->getCoverName() == "" &&
		((s = findHeader("x-fax-to")) || (s = findHeader("to")))) {
	    /*
	     * Try to extract a user name from the to information.
	     */
	    fxStr to(*s);
	    u_int l = to.next(0, '<');
	    u_int tl = to.length();
	    if (l == tl) {
		l = to.next(0, '(');
		if (l != tl)		// joe@foobar (Joe Schmo)
		    l++, to = to.token(l, ')');
		else {			// joe@foobar
		    l = to.next(0, '@');
		    if (l != tl)
			to = to.head(l);
		}
	    } else {			// Joe Schmo <joe@foobar>
		to = to.head(l);
	    }
	    // strip any leading&trailing white space
	    to.remove(0, to.skip(0, " \t"));
	    to.resize(to.skipR(to.length(), " \t"));

	    /*
	     * Remove matched quoting characters, recursively
	     * If they don't match, we'll assume they are part of
	     * the larger string, and not "quoting" marks
	     * The string to must have at least 2 chars for us to
	     * have anything to work on.
	     */
	    for (;to.length() > 1;)
	    {
		int i;
		/* We have 3 sets of quoting chars to look for */
		const char* remove[] = { "\"\"", "''","()" };
		for (i = 0; i < 3; i++)
		    if (to[0] == remove[i][0] &&
			    to[to.length()-1] == remove[i][1])
		    {
			    to.remove(0,1);
			    to.resize(to.length()-1);
			    break;
		    }
		if (i == 3)		// Nothing found, don't repeat
			break;
	    }
	    job->setCoverName(to);
	}
    }

    /*
     * Redirect formatted output to a temp
     * file and setup delivery of the file.
     */
    fxStr file = tmpDir | "/mail.ps";
    int fd = Sys::open(file, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR);
    if (fd < 0) {
	fxFatal("Cannot create temp file %s", (const char*) file);
    }
    tmps.append(file);
    client->addFile(file);
    beginFormatting(fdopen(fd, "w"));

    /*
     * empty tracks if we've actually put any real content
     * into our self (MsgFmt/TextFormat), instead of just the boiler-plate
     * PostScript.  If we're still empty at the end, we don't submit
     * the file we've been making.
     */
    empty = true;

    mimeid="part";

    const fxStr* version = findHeader("MIME-Version");

    if (version && stripComments(*version) == "1.0") {
	if (verbose)
	    fprintf(stderr, _("faxmail: This is a MIME message\n"));
	beginFile();
	withinFile = true;
	// We only format top-level headers if they are
	// wanted
	if (formatEnvHeaders && formatHeaders (*this) )
	    empty = false;
	formatMIME(stdin, mime, *this);	// parse MIME format
        if (withinFile) endFile();
	withinFile = false;
    } else {
	if (verbose)
	    fprintf(stderr, _("faxmail: This is not a MIME message\n"));
	beginFile();
	withinFile = true;
	// We only format top-level headers if they are
	// wanted
	if (formatEnvHeaders && formatHeaders(*this) )
	    empty = false;;
	if (formatText(stdin, mime))	// treat body as text/plain
	    empty = false;
        if (withinFile) endFile();
	withinFile = false;
    }

    endFormatting();

    /*
     * If our "formatted" file is empty, remove it.
     * We know it's the first file the client has.
     */
    if (empty)
	client->removeFile(0);

    if (client) {			// complete direct delivery
	bool status = false;
	fxStr emsg;
	const char* user = mailUser;
	if (user[0] == '\0')		// null user =>'s use real uid
	    user = NULL;
	if (client->callServer(emsg)) {
	    status = client->login(user, emsg)
		  && client->prepareForJobSubmissions(emsg)
		  && client->submitJobs(emsg);
	    client->hangupServer();
	}
	if (!status)
	    fxFatal(_("unable to process message:\n\t%s"), (const char*) emsg);
    }
}

/*
 * Emit PostScript prologue stuff defined in
 * system-wide prologue file and possibly
 * supplied in a MIME-encoded body part.
 */
void
faxMailApp::emitClientPrologue(FILE* fd)
{
    if (mailProlog != "")		// site-specific prologue
	copyFile(fd, mailProlog);
    if (clientProlog != "")		// copy client-specific prologue
	copyFile(fd, clientProlog);
}

/*
 * Parse MIME headers and dispatch to the appropriate
 * formatter based on the content-type information.
 */
void
faxMailApp::formatMIME(FILE* fd, MIMEState& mime, MsgFmt& msg)
{
    fxStr emsg;

    if (mime.parse(msg, emsg)) {
	if (verbose)
	    mime.trace(stderr);
	// XXX anything but us-ascii is treated as ISO-8859-1
	setISO8859(mime.getCharset() != CS_USASCII);

	/*
	 * Check first for any external script/command to
	 * use in converting the body part.
	 * If something is present, then we just decode the
	 * body part into a temporary file and hand it to
	 * the script.  Otherwise we fallback on some builtin
	 * body part handlers that process the most common
	 * content types we expect to encounter.
	 */
	const fxStr& type = mime.getType();
	fxStr app = mimeConverters | "/" | type | "/" | mime.getSubType();

	if (Sys::access(app, X_OK) >= 0)
		formatWithExternal(fd, app, mime);
	else if (type == "text" && mime.getSubType() == "plain") {
	    if ( formatText(fd, mime) )
		empty = false;
	} else if (type == "application")
	    formatApplication(fd, mime);
	else if (type == "multipart")
	    formatMultipart(fd, mime, msg);
	else if (type == "message")
	    formatMessage(fd, mime, msg);
	else
	    formatAttachment(fd, mime);
    } else
	error("%s", (const char*) emsg);
}

/*
 * Format a text part.
 */
bool
faxMailApp::formatText(FILE* fd, MIMEState& mime)
{
    fxStackBuffer buf;
    bool trim = trimText;
    u_int lines = 0;

    while (mime.getLine(fd, buf))
    {
	if (trim)
	    trim = (buf.getLength() == 0 || buf[0] == 0xA);
	if (! trim)
	{
	    format(buf, buf.getLength());	// NB: treat as text/plain
	    lines++;
	}
    }
    return lines > 0;
}

/*
 * Format a multi-part part.
 */
void
faxMailApp::formatMultipart(FILE* fd, MIMEState& mime, MsgFmt& msg)
{
    fxStr tmp(mimeid);
    int count = 0;
    discardPart(fd, mime);			// prologue
    if (!mime.isLastPart()) {
	bool last = false;
	while (! last) {
	    int c = getc(fd);
	    if (c == EOF) {
		error(_("Badly formatted MIME; premature EOF"));
		break;
	    }
	    ungetc(c, fd);			// push back read ahead

	    count++;
	    mimeid=fxStr::format("%s.%d", (const char*)tmp, count);
	    MsgFmt bodyHdrs(msg);		// parse any headers
	    bodyHdrs.parseHeaders(fd, mime.lineno);

	    MIMEState bodyMime(mime);		// state for sub-part
	    formatMIME(fd, bodyMime, bodyHdrs);
	    last = bodyMime.isLastPart();
	}
    }
    mimeid=tmp;
}

/*
 * Format a message part.
 */
void
faxMailApp::formatMessage(FILE* fd, MIMEState& mime, MsgFmt& msg)
{
    if (mime.getSubType() == "rfc822") {	// discard anything else
						// 980316 - mic: new MsgFmt 
	MsgFmt bodyHdrs(msg);            
	bodyHdrs.parseHeaders(fd, mime.lineno);
	/*
	 * Calculate the amount of space required to format
	 * the message headers and any required inter-message
	 * divider mark.  If there isn't enough space to put
	 * the headers and one line of text together on the
	 * same column then break and start a new column.
	 */
	const char* divider = NULL;
	if (mime.isParent("multipart", "digest") && msgDivider != "") {
	    /*
	     * XXX, hack.  Avoid inserting a digest divider when
	     * the sub-part is a faxmail prologue or related part.
	     */
	    const fxStr* s = bodyHdrs.findHeader("Content-Type");
	    if (!s || !strneq(*s, "application/x-faxmail", 21))
		divider = msgDivider;
	}
	u_int nl = bodyHdrs.headerCount()	// header lines
	    + (bodyHdrs.headerCount() > 0)	// blank line following header
	    + (divider != NULL)			// digest divider
	    + 1;				// 1st text line of message
	reserveVSpace(nl*getTextLineHeight());
	if (divider) {				// emit digest divider
	    beginLine();
		fprintf(getOutputFile(), " %s ", divider);
	    endLine();
	}
	if (nl > 0 )
	    if (bodyHdrs.formatHeaders(*this) )	// emit formatted headers
		empty = false;

	MIMEState subMime(mime);
	formatMIME(fd, subMime, bodyHdrs);	// format message body
    } else {
	discardPart(fd, mime);
	formatDiscarded(mime);
    }
}

/*
 * Format an application part.
 */
void
faxMailApp::formatApplication(FILE* fd, MIMEState& mime)
{
    if (mime.getSubType() == "x-faxmail-inline-postscript") {	// copy PS straight thru
	if (withinFile) endFile();
	withinFile = false;
	FILE* fout = getOutputFile();
	fxStackBuffer buf;
	while (mime.getLine(fd, buf))
	    (void) fwrite((const char*) buf, buf.getLength(), 1, fout);
	if (!withinFile) beginFile();
	withinFile = true;
    } else if (mime.getSubType() == "x-faxmail-prolog") {
	copyPart(fd, mime, clientProlog);	// save client PS prologue
    } else {
	formatAttachment(fd, mime);
    }
}

/*
 * Format a MIME part using an external conversion script.
 * The output of this script is submitted as a document
 */
bool
faxMailApp::formatWithExternal (FILE* fd, const fxStr& app, MIMEState& mime)
{
    if (verbose)
	fprintf(stderr, _("CONVERT: run %s\n"), (const char*) app);

    fxStr tmp = tmpDir | "/" | mimeid;
    tmps.append(tmp);

    if (copyPart(fd, mime, tmp)) {
	fxStr output = tmp | ".formatted";
	tmps.append(output);

	if (runConverter(app, tmp, output, mime) ) {
	    struct stat sb;
	    Sys::stat(output, sb);
	    if (sb.st_size > 0)
		client->addFile(output);
	}
    }

    return false;
}

/*
 * Mark a discarded part if configured.
 */
void
faxMailApp::formatDiscarded(MIMEState& mime)
{
    if (markDiscarded) {
	fxStackBuffer buf;
	buf.put("-----------------------------\n");
	if (mime.getDescription() != "")
	    buf.fput(_("DISCARDED %s (%s/%s) GOES HERE\n")
		, (const char*) mime.getDescription()
		, (const char*) mime.getType()
		, (const char*) mime.getSubType()
	    );
	else
	    buf.fput(_("DISCARDED %s/%s GOES HERE\n")
		, (const char*) mime.getType()
		, (const char*) mime.getSubType()
	    );
	format((const char*)buf, buf.getLength());
	empty = false;
    }
    if (mime.getDescription() != "")
	fprintf(stderr, _("DISCARDED: %s (%s/%s)\n"),
	    (const char*) mime.getDescription(),
	    (const char*) mime.getType(),
	    (const char*) mime.getSubType());
    else
	fprintf(stderr, _("DISCARDED: %s/%s\n"),
	    (const char*) mime.getType(),
	    (const char*) mime.getSubType());
}

/*
 * Format an unknown message part - we rely on
 * typerules to do this for us
 */
void
faxMailApp::formatAttachment(FILE* fd, MIMEState& mime)
{
    fxStr tmp = tmpDir | "/" | mimeid;
    tmps.append(tmp);

    if (copyPart(fd, mime, tmp))
	client->addFile(tmp);
}


/*
 * Discard input data up to the next boundary marker.
 */
void
faxMailApp::discardPart(FILE* fd, MIMEState& mime)
{
    fxStackBuffer buf;
    while (mime.getLine(fd, buf))
	;					// discard input data
}

/*
 * Copy input data up to the next boundary marker.
 * The data is stored in a temporary file that is
 * either created or, if passed in, appended to.
 * The latter is used, for example, to accumulate
 * client-specified prologue data.
 */
bool
faxMailApp::copyPart(FILE* fd, MIMEState& mime, fxStr& tmpFile)
{
    int ftmp;
    if (tmpFile == "") {
        const char* templ = _PATH_TMP "/faxmail.XXXXXX";
        char* buff = new char[strlen(templ) + 1];
        strcpy(buff, templ);
        ftmp = Sys::mkstemp(buff);
        tmpFile = buff;
        delete[] buff;
        tmps.append(tmpFile);
    } else {
        ftmp = Sys::open(tmpFile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR | S_IWUSR);
    }
    if (ftmp >= 0) {
        /*
        if (!Sys::isRegularFile(tmpFile)) {
            error(_("%s: is not a regular file"), (const char*) tmpFile);
            return(false);
        }
        */
        fxStackBuffer buf;
        bool ok = true;
        while (mime.getLine(fd, buf) && ok) {
	        ok = ((u_int) Sys::write(ftmp, buf, buf.getLength()) == buf.getLength());
        }
        if (ok) {
            Sys::close(ftmp);
            return (true);
        }
        error(_("%s: write error: %s"), (const char*) tmpFile, strerror(errno));
        Sys::close(ftmp);
    } else {
	    error(_("%s: Can not create temporary file"), (const char*) tmpFile);
    }
    discardPart(fd, mime);
    return (false);
}

/*
 * Run an external converter program.
 */
bool
faxMailApp::runConverter(const fxStr& app, const fxStr& input, const fxStr& output,
	    MIMEState& mime)
{
    int fd = Sys::open(output, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR);
    if (fd < 0)
	fxFatal(_("Couldn't open output file: %s"), (const char*)output);

    const char* av[3];
    av[0] = strrchr(app, '/');
    if (av[0] == NULL)
	av[0] = app;
    // XXX should probably pass in MIME state like charset
    av[1] = input;
    av[2] = NULL;
    pid_t pid = fork();
    switch (pid) {
    case -1:				// error
	close(fd);
	error(_("Error converting %s/%s; could not fork subprocess: %s")
	    , (const char*) mime.getType()
	    , (const char*) mime.getSubType()
	    , strerror(errno)
	);
	break;
    case 0:				// child, exec command
	dup2(fd, STDOUT_FILENO);
	close(fd);
	Sys::execv(app, (char* const*) av);
	_exit(-1);
	/*NOTREACHED*/
    default:
	int status;
	close(fd);
	if (Sys::waitpid(pid, status) == pid && status == 0)
	    return (true);
	error(_("Error converting %s/%s; command was \"%s %s\"; exit status %x")
	    , (const char*) mime.getType()
	    , (const char*) mime.getSubType()
	    , (const char*) app
	    , (const char*) input
	    , status
	);
	break;
    }
    return (false);
}

/*
 * Copy the contents of the specified file to
 * the output stream.
 */
void
faxMailApp::copyFile(FILE* fd, const char* filename)
{
    int fp = Sys::open(filename, O_RDONLY);
    if (fp >= 0) {
	char buf[16*1024];
	u_int cc;
	while ((cc = Sys::read(fp, buf, sizeof (buf))) > 0)
	    (void) fwrite(buf, cc, 1, fd);
	Sys::close(fp);
    }
}

/*
 * Configuration support.
 */

void
faxMailApp::setupConfig()
{
    markDiscarded = true;
    mimeConverters = FAX_LIBEXEC "/faxmail";
    mailProlog = FAX_LIBDATA "/faxmail.ps";
    debugLeaveTmp = false;
    msgDivider = "";
    pageSize = "";
    mailUser = "";			// default to real uid
    notify = "";
    coverTempl = "";
    autoCoverPage = true;		// a la sendfax
    formatEnvHeaders = true;		// format envelop headers by default
    trimText = false;			// don't trim leading CR from text parts by default

    setPageHeaders(false);		// disable normal page headers
    setNumberOfColumns(1);		// 1 input page per output page

    setLineWrapping(true);
    setISO8859(true);

    MsgFmt::setupConfig();
}

void
faxMailApp::resetConfig()
{
    TextFormat::resetConfig();
    setupConfig();
}

#undef streq
#define	streq(a,b)	(strcasecmp(a,b)==0)

bool
faxMailApp::setConfigItem(const char* tag, const char* value)
{
    if (streq(tag, "markdiscarded"))
	markDiscarded = getBoolean(value);
    else if (streq(tag, "autocoverpage"))
	autoCoverPage = getBoolean(value);
    else if (streq(tag, "formatenvheaders"))
	formatEnvHeaders = getBoolean(value);
    else if (streq(tag, "trimtext"))
	trimText = getBoolean(value);
    else if (streq(tag, "mimeconverters"))
	mimeConverters = value;
    else if (streq(tag, "prologfile"))
	mailProlog = value;
    else if (streq(tag, "digestdivider"))
	msgDivider = value;
    else if (streq(tag, "mailuser"))
	mailUser = value;
    else if (streq(tag, "debugleavetmp"))
	debugLeaveTmp = getBoolean(value);
    else if (MsgFmt::setConfigItem(tag, value))
	;
    else if (TextFormat::setConfigItem(tag, value))
	;
    else
	return (false);
    return (true);
}
#undef streq

#include <signal.h>

static	faxMailApp* app = NULL;

static void
cleanup()
{
    faxMailApp* a = app;
    app = NULL;
    delete a;
}

static void
sigDone(int)
{
    cleanup();
    exit(-1);
}

int
main(int argc, char** argv)
{
#ifdef LC_CTYPE
    setlocale(LC_CTYPE, "");			// for <ctype.h> calls
#endif
    NLS::Setup("hylafax-client");

    app = new faxMailApp;

    app->run(argc, argv);
    signal(SIGHUP, fxSIGHANDLER(SIG_IGN));
    signal(SIGINT, fxSIGHANDLER(SIG_IGN));
    signal(SIGTERM, fxSIGHANDLER(SIG_IGN));
    cleanup();
    return (0);
}

static void
vfatal(FILE* fd, const char* fmt, va_list ap)
{
    fprintf(stderr, "faxmail: ");
    vfprintf(fd, fmt, ap);
    va_end(ap);
    fputs(".\n", fd);
    sigDone(0);
}

void
MySendFaxClient::fatal(const char* fmt ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfatal(stderr, fmt, ap);
    /*NOTTEACHED*/
}

void
fxFatal(const char* fmt ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfatal(stderr, fmt, ap);
    /*NOTREACHED*/
}

void
faxMailApp::usage()
{
    fxFatal(_("usage: faxmail"
	" [-b boldfont]"
	" [-H pageheight]"
	" [-i italicfont]"
	" [-f textfont]"
	" [-p pointsize]"
	" [-s pagesize]"
	" [-W pagewidth]"
	" [-M margins]"
	" [-C covertemplate]"
	" [-t notify]"
	" [-u user]"
	" [-12cnNrRTv]")
    );
}
