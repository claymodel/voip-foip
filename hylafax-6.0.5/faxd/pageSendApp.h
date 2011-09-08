/*	$Id$ */
/*
 * Copyright (c) 1994-1996 Sam Leffler
 * Copyright (c) 1994-1996 Silicon Graphics, Inc.
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
#ifndef _pageSendApp_
#define	_pageSendApp_
/*
 * IXO/UCP Send Page App.
 */
#include "faxApp.h"
#include "ModemServer.h"
#include "FaxTrace.h"

class UUCPLock;
class fxStackBuffer;

class pageSendApp : public ModemServer, public faxApp {
public:
    struct stringtag {
	const char*	 name;
	fxStr pageSendApp::* p;
	const char*	 def;		// NULL is shorthand for ""
    };
    struct numbertag {
	const char*	 name;
	u_int pageSendApp::*p;
	u_int		 def;
    };
private:
// runtime state
    bool	ready;			// modem ready for use
    UUCPLock*	modemLock;		// uucp lockfile handle
    time_t	connTime;		// time connected to peer

    fxStr	pagerSetupCmds;		// pager-specific modem setup commands
    fxStr	pagerTTYParity;		// parity setting for tty
    u_int	pagerMaxMsgLength;	// default max message text length

    fxStr	ixoService;		// IXO protocol service string
    fxStr	ixoDeviceID;		// IXO protocol device id string
    u_int	ixoMaxUnknown;		// max unknown responses to permit
    u_int	ixoIDProbe;		// time between probes for ID= string
    u_int	ixoIDTimeout;		// timeout waiting for initial ID=
    u_int	ixoLoginRetries;	// # times to retry login procedure
    u_int	ixoLoginTimeout;	// timeout on login procedure
    u_int	ixoGATimeout;		// timeout waiting for go-ahead message
    u_int	ixoXmitRetries;		// # times to retry sending msg block
    u_int	ixoXmitTimeout;		// timeout waiting for xmit response
    u_int	ixoAckTimeout;		// timeout waiting ro final ack/nak

    fxStr	batchid;		// Batch ID, for session logs

    static pageSendApp* _instance;

    static stringtag strings[];
    static stringtag atcmds[];
    static numbertag numbers[];

// configuration support
    void	setupConfig();
    void	resetConfig();
    bool	setConfigItem(const char* tag, const char* value);
    u_int	getConfigParity(const char* value) const;
// page transmission support (independent of paging protocol)
    void 	sendPage(FaxRequest&, FaxMachineInfo&, u_int&);
    void 	sendPage(FaxRequest&, FaxMachineInfo&, const fxStr&, 
			const fxStr&, u_int&);	
// IXO transmission support
    void	sendIxoPage(FaxRequest&, FaxMachineInfo&, const fxStr&, Status& result, u_int&);
    bool	sendPagerMsg(FaxRequest&, FaxItem&, const fxStr&, Status& result);
    u_int	getResponse(fxStackBuffer& buf, long secs);
    bool	prepareMsg(FaxRequest&, FaxMachineInfo&, fxStr&);
    bool	pagePrologue(FaxRequest&, const FaxMachineInfo&, Status& result);
    bool	pageGoAhead(FaxRequest&, const FaxMachineInfo&, Status& result);
    bool	pageEpilogue(FaxRequest&, const FaxMachineInfo&, Status& result);
    void	sendFailed(FaxRequest&, FaxSendStatus, const Status&, u_int = 0);
    void	notifyPageSent(FaxRequest& req, u_int fi);
    time_t	getConnectTime() const;
// UCP transmission support
    void	sendUcpPage(FaxRequest&, FaxMachineInfo&, const fxStr&, Status& result);
    bool	sendUcpMsg(FaxRequest&, FaxItem&, const fxStr&, Status& result,
		    FaxMachineInfo&);
// modem handling
    bool	canLockModem();
    bool	lockModem();
    void	unlockModem();
    bool	setupModem(bool isSend);
// notification interfaces used by ModemServer
    void	notifyModemReady();
    void	notifyModemWedged();
// miscellaneous
    bool	putModem(const void* data, int n, long ms = 0);
    void	traceResponse(const fxStackBuffer& buf);
    void	traceIXO(const char* fmt ...);
    void	traceIXOCom(const char* dir, const u_char* data, u_int cc);
public:
    pageSendApp(const fxStr& device, const fxStr& devID);
    ~pageSendApp();

    static pageSendApp& instance();

    void	initialize(int argc, char** argv);
    void	open();
    void	close();

    FaxSendStatus send(const char** filename, int num);

    bool	isReady() const;
};
inline bool pageSendApp::isReady() const 	{ return ready; }
#endif
