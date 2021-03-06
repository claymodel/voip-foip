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
#ifndef _CLASS2ERSATZ_
#define	_CLASS2ERSATZ_
/*
 * Ersatz Class 2-style Modem Driver (SP-2388-A of August 30, 1991).
 */
#include "Class2.h"

class Class2ErsatzModem : public Class2Modem {
protected:
// transmission support
    bool	sendPage(TIFF* tif, u_int pageChop, bool coverpage);
    bool	pageDone(u_int ppm, u_int& ppr);

    bool	sendEOT();
    void	abortDataTransfer();
// miscellaneous
    virtual ATResponse atResponse(char* buf, long ms = 30*1000);
public:
    Class2ErsatzModem(FaxServer&, const ModemConfig&);
    virtual ~Class2ErsatzModem();
};
#endif /* _CLASS2ERSATZ_ */
