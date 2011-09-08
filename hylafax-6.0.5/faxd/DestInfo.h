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
#ifndef _DestInfo_
#define	_DestInfo_
/*
 * Destination Information Support.
 */
#include "QLink.h"
#include "FaxMachineInfo.h"
#include "Dictionary.h"

class Batch;
class fxStr;

/*
 * This structure is used to create a dictionary indexed by
 * canonical destination phone number.  All jobs to a destination
 * are referenced here with jobs blocked due to concurrency
 * limitations queued here.  The FaxMachineInfo data structure
 * common to all jobs going to the same destination is kept
 * here to permit shared access and updates without going to
 * the associated file.  Note that certain interfaces are needed
 * because the data structure only exists as a member of a
 * dictionary (e.g. explicit mechanisms for reading and updating
 * the external file). 
 */ 
class DestInfo 
  : public QLink
{
private:
    u_short		activeCount;	// count of active jobs to destination
    FaxMachineInfo	info;		// remote machine capabilities and such
    Batch*		running;	// jobs to dest being processed
public:
    QLink		readyQ;		// list of ready jobs
    QLink		sleepQ;		// list of sleeping jobs

    DestInfo();
    DestInfo(const DestInfo& other);
    ~DestInfo();

    u_int getActiveCount() const;		// return count of active jobs
    u_int getSleepCount() const;		// return count of active jobs
    u_int getReadyCount() const;		// return count of ready jobs

    bool isEmpty() const;		// true if any jobs referenced

    bool isActive(Batch&) const;	// true if job is considered active
    void active(Batch&);			// set job active to destination
    void done(Batch&);			// remove job from active set

    FaxMachineInfo& getInfo(const fxStr& number);
    void updateConfig();		// write info file if necessary
};


fxDECLARE_StrKeyDictionary(DestInfoDict, DestInfo)
#endif /* _DestInfo_ */
