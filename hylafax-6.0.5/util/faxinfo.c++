/*	$Id$ */
/*
* Copyright (c) 1990-1996 Sam Leffler
* Copyright (c) 1991-1996 Silicon Graphics, Inc.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "tiffio.h"

#include "PageSize.h"
#include "Class2Params.h"
#include "CallID.h"
#include "Sys.h"

#include "port.h"

extern	const char* fmtTime(time_t t);

/*
 * This tests whether the tif file is a "fax" image.
 * Traditional fax images are MH, MR, MMR, but some can
 * be JPEG, JBIG, and possibly others.  So we use
 * TIFFTAG_FAXRECVPARAMS as a "fax" image identifier,
 * and if it's not there, then we resort to traditional
 * tactics.
 */
static bool
isFAXImage(TIFF* tif)
{
#ifdef TIFFTAG_FAXRECVPARAMS
    uint32 v;
    if (TIFFGetField(tif, TIFFTAG_FAXRECVPARAMS, &v) && v != 0)
	return (true);
#endif
    uint16 w;
    if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &w) && w != 1)
	return (false);
    if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &w) && w != 1)
	return (false);
    if (!TIFFGetField(tif, TIFFTAG_COMPRESSION, &w) ||
      (w != COMPRESSION_CCITTFAX3 && w != COMPRESSION_CCITTFAX4))
	return (false);
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &w) ||
      (w != PHOTOMETRIC_MINISWHITE && w != PHOTOMETRIC_MINISBLACK))
	return (false);
    return (true);
}

static void
sanitize(fxStr& s)
{
    for(u_int i = 0; i < s.length(); i++) {
        if (!isascii(s[i]) || !isprint(s[i])) s[i] = '?';
    }
}

static void
usage (const char* app)
{
    printf(_("usage: %s [-n] [-S fmt] [-s fmt] [-e fmt] [-E fmt] [-D]\n\n"), app);
    printf(_("\t-n\tPrint FAX filename\n"));
    printf(_("\t-C d\tQuoted CSV-style output with <d> as the deliminator\n"));
    printf(_("\t-c d\tCSV-style output with <d> as the deliminator\n"));
    printf(_("\t-r\traw format - values outputed with no names\n"));

    printf(_("  Raw format options:\n"));
    printf(_("\t-S fmt\tUse fmt for the fax start format\n"));
    printf(_("\t-s fmt\tUse fmt for the field start format\n"));
    printf(_("\t-e fmt\tUse fmt for the field end format\n"));
    printf(_("\t-E fmt\tUse fmt for the fax end format\n"));
}

static const char*
escapedString (const char*src)
{
    char* res;
    int len;
    len = strlen(src);
    res = (char*)malloc(len+1);
    if (res)
    {
	char* dst = res;
	for (int i = 0; i <= len; i++)
	{
	    switch (src[i])
	    {
	    	case '\\':
		    switch (src[++i])
		    {
			case 'n':	*dst++ = '\n';	break;
			case 'r':	*dst++ = '\r';	break;
			case 't':	*dst++ = '\t';	break;
			default:
			    *dst++ = src[i];
		    }
		    break;
		case '%':
		    *dst++ = src[i];
		    switch (src[++i])
		    {
			case 'n':	*dst++ = '%';	break;
			default:
			    *dst++ = src[i];
		    }
		    break;

		default:
		    *dst++ = src[i];
	    }
	}
    } else
    {
    	exit(ENOMEM);
    }
    return res;
}

static const char* faxStart = "%s:\n";
static const char* fieldStart = "%10s: ";
static const char* fieldEnd = "\n";
static const char* faxEnd = "";

static void 
printStart (const char* filename)
{
    printf(faxStart, filename);
}

static void
printField (const char* val_fmt, const char* name, ...)
{
    printf(fieldStart, name);
    va_list ap;
    va_start(ap,name);
    vprintf(val_fmt, ap);
    va_end(ap);
    printf(fieldEnd, name);
}

static void
printEnd (const char* filename)
{
    printf(faxEnd, filename);
}

int
main(int argc, char** argv)
{
    const char* appName = argv[0];
    bool debug = false;
    bool baseName = false;
    int c;

    while ((c = getopt(argc, argv, "C:c:rnbS:s:e:E:D")) != -1)
	switch (c) {
	    case '?':
	    	usage(appName);
		return 0;
	    case 'C':
		faxStart = "\"%s\"";
		fieldStart = escapedString(fxStr::format("%%0.0s%s\"", optarg));
		fieldEnd = "\"";
		faxEnd = "\n";
		break;
	    case 'c':
		faxStart = "%s";
		fieldStart = escapedString(fxStr::format("%%0.0s%s", optarg));
		fieldEnd = "";
		faxEnd = "\n";
		break;
	    case 'r':
		faxStart = "";
		fieldStart = "%0.0s";
		fieldEnd = "\n";
		faxEnd = "";
		break;
	    case 'n':
		faxStart = "";
		break;
	    case 'b':
	    	baseName = true;
		break;
	    case 'S':
		faxStart = escapedString(optarg);
	    	break;
	    case 's':
		fieldStart = escapedString(optarg);
	    	break;
	    case 'e':
		fieldEnd = escapedString(optarg);
	    	break;
	    case 'E':
		faxEnd = escapedString(optarg);
	    	break;
	    case 'D':
	    	debug = true;
		break;
	}

    if (debug)
	fprintf(stderr, "faxStart='%s'\nfieldStart='%s'\nfieldEnd='%s'\nfaxEnd'%s'\n",
		faxStart,fieldStart,fieldEnd,faxEnd);

    while (optind < argc) {
	const char* name = argv[optind];

	if (baseName) {
	    const char* r = strrchr(name, '/');
	    if (r)
		name = r+1;
	}

	printStart(name);
	TIFFSetErrorHandler(NULL);
	TIFFSetWarningHandler(NULL);
	TIFF* tif = TIFFOpen(argv[optind], "r");
	if (tif == NULL) {
	    printf(_("Could not open %s; either not TIFF or corrupted.\n"),
		    argv[optind]);
	    return (1);
	}
	bool ok = isFAXImage(tif);
	if (!ok) {
	    printf(_("Does not look like a facsimile?\n"));
	    return (1);
	}

	Class2Params params;
	params.jp = 0;
	uint32 v;
	float vres = 3.85;					// XXX default
	float hres = 8.03;
#ifdef TIFFTAG_FAXRECVPARAMS
	if (TIFFGetField(tif, TIFFTAG_FAXRECVPARAMS, &v)) {
	    params.decode((u_int) v);			// page transfer params
	    // inch & metric resolutions overlap and are distinguished by yres
	    TIFFGetField(tif, TIFFTAG_YRESOLUTION, &vres);
	    switch ((u_int) vres) {
		case 100:
		    params.vr = VR_200X100;
		    break;
		case 200:
		    params.vr = VR_200X200;
		    break;
		case 400:
		    params.vr = VR_200X400;
		    break;
		case 300:
		    params.vr = VR_300X300;
		    break;
	    }
	} else {
#endif
	uint16 compression = 0;
	(void) TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
	if (compression == COMPRESSION_JBIG) {
	    params.df = DF_JBIG;
	} else if (compression == COMPRESSION_JPEG) {
	    params.df = 0;
	    params.jp = JP_COLOR;
	} else if (compression == COMPRESSION_CCITTFAX4) {
	    params.df = DF_2DMMR;
	} else {
	    uint32 g3opts = 0;
	    TIFFGetField(tif, TIFFTAG_GROUP3OPTIONS, &g3opts);
	    params.df = (g3opts&GROUP3OPT_2DENCODING ? DF_2DMR : DF_1DMH);
	}
	if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &vres)) {
	    uint16 resunit = RESUNIT_INCH;			// TIFF spec default
	    TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
	    if (resunit == RESUNIT_INCH)
		vres /= 25.4;
	    if (resunit == RESUNIT_NONE)
		vres /= 720.0;				// postscript units ?
	    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &hres)) {
		if (resunit == RESUNIT_INCH)
		    hres /= 25.4;
		if (resunit == RESUNIT_NONE)
		    hres /= 720.0;				// postscript units ?
	    }
	}
	params.setRes((u_int) hres, (u_int) vres);		// resolution
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &v);
	params.setPageWidthInPixels((u_int) v);		// page width
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &v);
	params.setPageLengthInMM((u_int)(v / vres));	// page length
#ifdef TIFFTAG_FAXRECVPARAMS
	}
#endif
	char* cp;
#ifdef TIFFTAG_FAXDCS
	if (TIFFGetField(tif, TIFFTAG_FAXDCS, &cp) && strncmp(cp, "00 00 00", 8) != 0) {
	    // cannot trust br from faxdcs as V.34-Fax does not provide it there
	    u_int brhold = params.br;
	    fxStr faxdcs(cp);
	    sanitize(faxdcs);
	    params.asciiDecode((const char*) faxdcs);	// params per Table 2/T.30
	    params.setFromDCS(params);
	    params.br = brhold;
	}
#endif
	fxStr sender = "";
	CallID callid;
	if (TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &cp)) {
	    while (cp[0] != '\0' && cp[0] != '\n') {	// sender
		sender.append(cp[0]);
		cp++;
	    }
	    sanitize(sender);
	    u_int i = 0;
	    while (cp[0] == '\n') {
		cp++;
		callid.resize(i+1);
		while (cp[0] != '\0' && cp[0] != '\n') {
		    callid[i].append(cp[0]);
		    cp++;
		}
		sanitize(callid[i]);
		i++;
	    }
	} else
	    sender = _("<unknown>");
	printField("%s", _("Sender"), (const char*) sender);
#ifdef TIFFTAG_FAXSUBADDRESS
	if (TIFFGetField(tif, TIFFTAG_FAXSUBADDRESS, &cp)) {
	    fxStr subaddr(cp);
	    sanitize(subaddr);
	    printField("%s", _("SubAddr"), (const char*) subaddr);
	}
#endif
	fxStr date;
	if (TIFFGetField(tif, TIFFTAG_DATETIME, &cp)) {	// time received
	    date = cp;
	    sanitize(date);
	} else {
	    struct stat sb;
	    fstat(TIFFFileno(tif), &sb);
	    char buf[80];
	    strftime(buf, sizeof (buf),
		"%Y:%m:%d %H:%M:%S %Z", localtime(&sb.st_mtime));
	    date = buf;
	}
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &v);
	float h = v / (params.vr == VR_NORMAL ? 3.85 : 
	    params.vr == VR_200X100 ? 3.94 : 
	    params.vr == VR_FINE ? 7.7 :
	    params.vr == VR_200X200 ? 7.87 : 
	    params.vr == VR_R8 ? 15.4 : 
	    params.vr == VR_200X400 ? 15.75 : 
	    params.vr == VR_300X300 ? 11.81 : 15.4);
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &v);
	float w = v / (params.vr == VR_NORMAL ? 8.0 : 
	    params.vr == VR_200X100 ? 8.00 : 
	    params.vr == VR_FINE ? 8.00 :
	    params.vr == VR_200X200 ? 8.00 : 
	    params.vr == VR_R8 ? 8.00 : 
	    params.vr == VR_200X400 ? 8.00 : 
	    params.vr == VR_300X300 ? 12.01 : 16.01);
	time_t time = 0;
	u_int npages = 0;					// page count
	do {
	    npages++;
#ifdef TIFFTAG_FAXRECVTIME
	    if (TIFFGetField(tif, TIFFTAG_FAXRECVTIME, &v))
		time += v;
#endif
	} while (TIFFReadDirectory(tif));
	TIFFClose(tif);

	printField("%u", _("Pages"), npages);
	if (params.vr == VR_NORMAL)
	    printField(_("Normal"), _("Quality"));
	else if (params.vr == VR_FINE)
	    printField(_("Fine"), _("Quality"));
	else if (params.vr == VR_R8)
	    printField(_("Superfine"), _("Quality"));
	else if (params.vr == VR_R16)
	    printField(_("Hyperfine"), _("Quality"));
	else
	    printField(_("%u lines/inch"), _("Quality"), params.verticalRes());
	PageSizeInfo* info = PageSizeInfo::getPageSizeBySize(w, h);
	if (info)
	    printField("%s", _("Page"), info->name());
	else
	    printField(_("%u by %u"), _("Page"), params.pageWidth(), (u_int) h);
	delete info;
	printField("%s", _("Received"), (const char*) date);
	printField("%s", _("TimeToRecv"), time == 0 ? _("<unknown>") : fmtTime(time));
	printField("%s", _("SignalRate"), params.bitRateName());
	printField("%s", _("DataFormat"), params.dataFormatName());
	printField("%s", _("ErrCorrect"), params.ec == EC_DISABLE ? _("No") : _("Yes"));
	for (u_int i = 0; i < callid.size(); i++) {
	    // formatting will mess up if i gets bigger than one digit
	    fxStr fmt(fxStr::format(_("CallID%u"), i+1));
	    printField("%s", (const char*)fmt, (const char*) callid.id(i));
	}
	printEnd(name);
	optind++;
    }
    return (0);
}
