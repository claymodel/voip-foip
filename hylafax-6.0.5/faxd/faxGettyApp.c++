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
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <math.h>
#include <limits.h>
#include <sys/file.h>
#include <grp.h>
#include <unistd.h>

#include "Sys.h"

#include "Dispatcher.h"

#include "FaxRecvInfo.h"
#include "FaxMachineInfo.h"
#include "FaxAcctInfo.h"
#include "faxGettyApp.h"
#include "UUCPLock.h"
#include "Getty.h"
#include "REArray.h"
#include "BoolArray.h"
#include "config.h"

/*
 * HylaFAX Spooling and Command Agent.
 */
AnswerTimeoutHandler::AnswerTimeoutHandler() {}
AnswerTimeoutHandler::~AnswerTimeoutHandler() {}
void AnswerTimeoutHandler::timerExpired(long, long)
    { faxGettyApp::instance().answerCleanup(); }

const fxStr faxGettyApp::recvDir	= FAX_RECVDIR;

faxGettyApp* faxGettyApp::_instance = NULL;

faxGettyApp::faxGettyApp(const fxStr& devName, const fxStr& devID)
    : FaxServer(devName, devID)
{
    devfifo = -1;
    modemLock = NULL;
    setupConfig();

    fxAssert(_instance == NULL, "Cannot create multiple faxGettyApp instances");
    _instance = this;
}

faxGettyApp::~faxGettyApp()
{
    delete modemLock;
}

faxGettyApp& faxGettyApp::instance() { return *_instance; }

void
faxGettyApp::initialize(int argc, char** argv)
{
    FaxServer::initialize(argc, argv);
    faxApp::initialize(argc, argv);

    for (GetoptIter iter(argc, argv, getOpts()); iter.notDone(); iter++)
	switch (iter.option()) {
	case 'c':			// set configuration parameter
	    readConfigItem(iter.optArg());
	    break;
	case 'D':			// detach process from tty
	    detachFromTTY();
	    break;
	}
    modemLock = getUUCPLock(getModemDevice());
    /*
     * Notify queuer that modem exists and what the
     * phone number is.  This is used by the queuer
     * when processing other messages we sent
     * later--such as when a document is received.
     */
    sendModemStatus("N" | getModemNumber());
}

void
faxGettyApp::open()
{
    traceServer("OPEN %s  %s"
        , (const char*) getModemDevice()
        , HYLAFAX_VERSION
    );
    faxApp::open();
    FaxServer::open();
}

void
faxGettyApp::close()
{
    if (isRunning()) {
	sendModemStatus("D");
	traceServer("CLOSE " | getModemDevice());
	faxApp::close();
	FaxServer::close();
    }
}

bool faxGettyApp::lockModem()		{ return modemLock->lock(); }
void faxGettyApp::unlockModem()		{ modemLock->unlock(); }
bool faxGettyApp::canLockModem()	{ return modemLock->check(); }
bool faxGettyApp::isModemLocked()	{ return modemLock->isLocked(); }

bool
faxGettyApp::setupModem(bool isSend)
{
    /*
     * Reread the configuration file if it has been
     * changed.  We do this before each setup operation
     * since we are a long-running process and it should
     * not be necessary to restart the process to have
     * config file changes take effect.
     */
    if (updateConfig(getConfigFile()))
	sendModemStatus("N" | getModemNumber());
    if (FaxServer::setupModem(false) && ModemServer::readyModem()) {
	/*
	 * Setup modem for receiving.
	 */
	FaxModem* modem = (FaxModem*) ModemServer::getModem();
	modem->setupReceive();
	/*
	 * If the server is configured, listen for incoming calls.
	 */
	setRingsBeforeAnswer(ringsBeforeAnswer);
	return (true);
    } else
	return (false);
}

/*
 * Discard any handle on the modem.
 */
void
faxGettyApp::discardModem(bool dropDTR)
{
    int fd = getModemFd();
    if (fd >= 0) {
	if (Dispatcher::instance().handler(fd, Dispatcher::ReadMask))
	    Dispatcher::instance().unlink(fd);
    }
    FaxServer::discardModem(dropDTR);
}

/*
 * Begin listening for ring status messages from the modem.
 * The modem is locked and we mark it as busy to the queuer
 * (just as if we were answering a call).  We listen for one
 * ring status message and, if successful, keep the modem
 * locked and change state so that further input will be
 * consumed.
 */
void
faxGettyApp::listenBegin()
{
    if (lockModem()) {
	changeState(LISTENING);
	sendModemStatus("B");
	ringsHeard = 0;
	listenForRing();
    } else if (state != SENDING && state != ANSWERING && state != RECEIVING) {
	/*
	 * The modem is in use to call out, or some other process
	 * has grabbed the lock to handle the incoming call. 
	 * Discard our handle on the modem and change to LOCKWAIT
	 * state where we wait for the modem to come available again.
	 */
	traceServer("ANSWER: Can not lock modem device");
	discardModem(false);
	changeState(LOCKWAIT, pollLockWait);
	sendModemStatus("U");
    }
}

/*
 * Consume a ring status message from the modem.  We assume
 * the modem is locked and that input is present.  If we don't
 * see a ring then we exit LISTENING state and reset our
 * handle on the modem.  If the number of rings received
 * matches ringsBeforeAnswer then we answer the phone.
 */
void
faxGettyApp::listenForRing()
{
    Dispatcher::instance().stopTimer(&answerHandler);

    bool again;

    do {
        CallType ctype = ClassModem::CALLTYPE_UNKNOWN;
	CallID callid(idConfig.length());
        again = false;

        if (modemWaitForRings(ringsHeard, ctype, callid)) {
	    if (! callid.isEmpty()) {
		received_callid = callid;	// CNID is only sent once.  Store it
						// for answering after later RINGs.
		fxStr send;
		for (u_int i = 0; i < received_callid.size(); i++) {
		    if (i) send.append(',');
		    send.append("\"" | received_callid[i] | "\"");
		    traceServer("ANSWER: Call ID %d \"%s\"", i+1, received_callid.id(i));
		}
		faxApp::sendModemStatus(getModemDeviceID(), "C%s", (const char*) send);
	    }
	    ++ringsHeard;
            if (dringOn.length() && ctype == ClassModem::CALLTYPE_UNKNOWN) --ringsHeard;
             // distinctive ring failed to identify - get another ring
            
	    /* DID modems may only signal a call with DID data - no RING */
	    bool done = false;
	    for (u_int i = 0; i < callid.size(); i++) {
		if ((u_int) idConfig[i].answerlength > 0 && 
		    callid[i].length() >= (u_int) idConfig[i].answerlength) {
		    done = true;
		    break;
		}
	    }
	    if (done || ringsBeforeAnswer && (ringsHeard >= ringsBeforeAnswer))
		answerPhone(ClassModem::ANSTYPE_ANY, ctype, received_callid);
	    else if (isModemInput())
	        again = true;
	    else
	        // NB: 10 second timeout should be plenty
	        Dispatcher::instance().startTimer(10, 0, &answerHandler);
        } else
	    answerCleanup();
    } while (again);
}

/*
 * Handle a user request to answer the phone.
 * Note that this can come in when we are in
 * many different states.
 */
void
faxGettyApp::answerPhoneCmd(AnswerType atype, const char* dialnumber)
{
    CallType ctype = ClassModem::CALLTYPE_UNKNOWN;
    if (state == LISTENING) {
	/*
	 * We were listening to rings when an answer command
	 * was received.  The modem is already locked so just
	 * cancel any timeout and answer the call.
	 */
	Dispatcher::instance().stopTimer(&answerHandler);
	answerPhone(atype, ctype, received_callid, dialnumber);
    } else if (lockModem()) {
	/*
	 * The modem is ours, notifier the queuer and answer.
	 */
	sendModemStatus("B");
	answerPhone(atype, ctype, received_callid, dialnumber);
    } else if (state != ANSWERING && state != RECEIVING) {
	/*
	 * The modem is in use to call out, or some other process
	 * has grabbed the lock to handle the incoming call. 
	 * Discard our handle on the modem and change to LOCKWAIT
	 * state where we wait for the modem to come available again.
	 */
	discardModem(false);
	changeState(LOCKWAIT, pollLockWait);
	sendModemStatus("U");
    }
}

/*
 * Answer the telephone in response to data from the modem
 * (e.g. a "RING" message) or an explicit command from the
 * user (sending an "ANSWER" command through the FIFO).
 */
void
faxGettyApp::answerPhone(AnswerType atype, CallType ctype, const CallID& callid, const char* dialnumber)
{
    changeState(ANSWERING);
    beginSession(FAXNumber);
    sendModemStatus("I" | getCommID());

    FaxAcctInfo ai;
    ai.user = "fax";
    ai.commid = getCommID();
    ai.start = Sys::now();
    ai.device = getModemDeviceID();
    ai.dest = getModemNumber();
    ai.callid = callid;
    ai.npages = 0;
    ai.params = 0;
    ai.csi = "";
    ai.jobid = "";
    ai.jobtag = "";
    ai.owner = "";
    /*
     * Answer the phone according to atype.  If this is
     * ``any'', then pick a more specific type.  If
     * adaptive-answer is enabled and ``any'' is requested,
     * then rotate through the set of possibilities.
     */
    bool callResolved;
    bool advanceRotary = true;
    Status eresult;

    fxStr callid_formatted = "";

    /*
     * We default to accepting all calls.
     * If the DynamicConfig set's RejectCall to true, then we
     * will reject it.
     */
    rejectCall = false;

    for (u_int i = 0; i < callid.size(); i++)
	callid_formatted.append(quote | callid.id(i) | enquote);

    if (callid_formatted.length())
    	traceProtocol("CallID:%s", (const char*) callid_formatted);

    if (dynamicConfig.length()) {
	fxStr cmd(dynamicConfig | quote | getModemDevice() | enquote | callid_formatted);
	traceServer("DynamicConfig: %s", (const char*)cmd);
	fxStr localid = "";
	int pipefd[2], status;
	char line[1024];
	pipe(pipefd);
	pid_t pid = fork();
	switch (pid) {
	    case -1:
		eresult = Status(305, "Could not fork for local ID");
		logError("%s", eresult.string());
		Sys::close(pipefd[0]);
		Sys::close(pipefd[1]);
		break;
	    case  0:
		dup2(pipefd[1], STDOUT_FILENO);
		Sys::close(pipefd[0]);
		Sys::close(pipefd[1]);
		execl("/bin/sh", "sh", "-c", (const char*) cmd, (char*) NULL);
		sleep(1);
		_exit(1);
	    default:
		Sys::close(pipefd[1]);
		{
		    FILE* fd = fdopen(pipefd[0], "r");
		    while (fgets(line, sizeof (line)-1, fd)){
			line[strlen(line)-1]='\0';		// Nuke \n at end of line
			(void) readConfigItem(line);
		    }
		    Sys::waitpid(pid, status);
		    if (status != 0)
		    {
			eresult = Status(306, "Bad exit status %#o for \'%s\'", status, (const char*) cmd);
			logError("%s", eresult.string());
		    }
		    // modem settings may have changed...
		    FaxModem* modem = (FaxModem*) ModemServer::getModem();
		    modem->pokeConfig(false);
		}
		Sys::close(pipefd[0]);
		break;
	}
    }

    if (rejectCall)
    {
	/*
	 * Call was rejected based on Caller ID information.
	 */
	eresult = Status(308, "ANSWER: CALL REJECTED");
	traceServer("%s", eresult.string());
	callResolved = false;
	advanceRotary = false;
    } else {
	if (ctype != ClassModem::CALLTYPE_UNKNOWN) {
	    /*
	     * Distinctive ring or other means has already identified
	     * the type of call.  If we're to answer the call in a
	     * different way, then treat this as an error and don't
	     * answer the phone.  Otherwise answer according to the
	     * deduced call type.
	     */
	    if (atype != ClassModem::ANSTYPE_ANY && ctype != atype) {
	        eresult = Status(309, "ANSWER: Call deduced as %s,"
		     " but told to answer as %s; call ignored",
		     ClassModem::callTypes[ctype],
		     ClassModem::answerTypes[atype]);
		traceServer("%s", eresult.string());
		callResolved = false;
		advanceRotary = false;
	    } else {
		// NB: answer based on ctype, not atype
		if (!(noAnswerVoice && ctype == ClassModem::CALLTYPE_VOICE)) 
		    ctype = modemAnswerCall(ctype, eresult, dialnumber);
		callResolved = processCall(ctype, eresult, callid);
	    }
	} else if (atype == ClassModem::ANSTYPE_ANY) {
	    /*
	     * Normal operation; answer according to the settings
	     * for the rotary and adaptive answer capabilities.
	     */
	    int r = answerRotor;
	    do {
		callResolved = answerCall(answerRotary[r], ctype, eresult, callid, dialnumber);
		r = (r+1) % answerRotorSize;
	    } while (!callResolved && adaptiveAnswer && r != answerRotor);
	} else {
	    /*
	     * Answer for a specific type of call but w/o
	     * any existing call type information such as
	     * distinctive ring.
	     */
	    callResolved = answerCall(atype, ctype, eresult, callid, dialnumber);
	}
    }
    if (eresult.value() != 0)
	traceProtocol("%s", eresult.string());
    /*
     * Call resolved.  If we were able to recognize the call
     * type and setup a session, then reset the answer rotary
     * state if there is a bias toward a specific answer type.
     * Otherwise, if the call failed, advance the rotor to
     * the next answer type in preparation for the next call.
     */
    if (callResolved) {
	if (answerBias != (u_int) -1)
	    answerRotor = answerBias;
    } else if (advanceRotary) {
	if (adaptiveAnswer)
	    answerRotor = 0;
	else
	    answerRotor = (answerRotor+1) % answerRotorSize;
    }
    sendModemStatus("I");
    endSession();

    ai.status = eresult.string();
    ai.duration = Sys::now() - ai.start;
    ai.conntime = ai.duration;
    if (logCalls && !ai.record("CALL"))
	logError("Error writing CALL accounting record, dest=%s",
	    (const char*) ai.dest);


    answerCleanup();
}

/*
 * Cleanup after answering a call or listening for ring status
 * messages from the modem (when ringsBeforeAnswer is zero).
 *
 * If we still have a handle on the modem, then force a hangup
 * and discard the handle.  We do this explicitly because some
 * modems are impossible to safely hangup in the event of a
 * problem.  Forcing a close on the device so that the modem
 * will see DTR drop (hopefully) should clean up any bad state
 * its in.  We then immediately try to setup the modem again
 * so that we can be ready to answer incoming calls again.
 *
 * NB: the modem may have been discarded if a child process
 *     was invoked to handle the inbound call.
 */
void
faxGettyApp::answerCleanup()
{
    if (modemReady()) {
	modemHangup();
	discardModem(true);
    }

    for (u_int i = 0; i < received_callid.size(); i++)
	received_callid[i].resize(0);

    bool isSetup;
    if (isModemLocked() || lockModem()) {
	isSetup = setupModem(false);
	unlockModem();
    } else
	isSetup = false;
    if (isSetup)
	changeState(RUNNING, pollLockWait);
    else
	changeState(MODEMWAIT, pollModemWait);
}

/*
 * Answer a call according to the specified answer type
 * and return the deduced call type.  If we are to use an
 * external application to deduce and possibly handle the
 * call then we do it here and return the call type it
 * comes up with.  Otherwise we use whatever deduction
 * the modem layer arrives at as the call type.
 */
bool
faxGettyApp::answerCall(AnswerType atype, CallType& ctype, Status& eresult, const CallID& callid, const char* dialnumber)
{
    bool callResolved;
    if (atype == ClassModem::ANSTYPE_EXTERN) {
	if (egettyArgs != "") {
	    /*
	     * Use an external application to deduce and possibly
	     * handle the call.  We invoke the egetty application
	     * and interpret the exit status as the deduced call type.
	     * If the call has not been handled in the subprocess
	     * then we take action based on the returned call type.
	     */
	    ctype = runGetty("EXTERN GETTY", OSnewEGetty,
		egettyArgs, eresult, lockExternCalls, callid, true);
	    if (ctype == ClassModem::CALLTYPE_DONE)	// NB: call completed
		return (true);
	    if (ctype != ClassModem::CALLTYPE_ERROR)
		modemAnswerCallCmd(ctype);
	} else
	    eresult = Status(310, "External getty use is not permitted");
    } else
	ctype = modemAnswerCall(atype, eresult, dialnumber);
    callResolved = processCall(ctype, eresult, callid);
    return (callResolved);
}

/*
 * Process an inbound call after the phone's been answered.
 * Calls may either be handled within the process or through
 * an external application.  Fax calls are handled internally.
 * Other types of calls are handled with external apps.  The
 * modem is conditioned for service, the process is started
 * with the open file descriptor passed on stdin+stdout+stderr,
 * and the local handle on the modem is discarded so that SIGHUP
 * is delivered to the subprocess (group) on last close.  This
 * process waits for the subprocess to terminate, at which time
 * it removes the modem lock file and let's faxgetty continue on
 * to recondition the modem for incoming calls (if configured).
 */
bool
faxGettyApp::processCall(CallType ctype, Status& eresult, const CallID& callid)
{
    bool callHandled = false;

    /*
     * First of - turn of Dispatcher
     */
    int fd = getModemFd();
    if (fd >= 0) {
	if (Dispatcher::instance().handler(fd, Dispatcher::ReadMask))
	    Dispatcher::instance().unlink(fd);
    }
    switch (ctype) {
    case ClassModem::CALLTYPE_FAX:
	traceServer("ANSWER: FAX CONNECTION  DEVICE '%s'"
	    , (const char*) getModemDevice()
	);
	changeState(RECEIVING);
	sendRecvStatus(getModemDeviceID(), "B");
	callHandled = recvFax(callid, eresult);
	sendRecvStatus(getModemDeviceID(), "E");
	break;
    case ClassModem::CALLTYPE_DATA:
	traceServer("ANSWER: DATA CONNECTION");
	if (gettyArgs != "") {
	    sendModemStatus("d");
	    runGetty("GETTY", OSnewGetty, gettyArgs, eresult, lockDataCalls, callid);
	    sendModemStatus("e");
	} else
	    traceServer("ANSWER: Data connections are not permitted");
	callHandled = true;
	break;
    case ClassModem::CALLTYPE_VOICE:
	traceServer("ANSWER: VOICE CONNECTION");
	if (vgettyArgs != "") {
	    sendModemStatus("v");
	    runGetty("VGETTY", OSnewVGetty, vgettyArgs, eresult, lockVoiceCalls, callid);
	    sendModemStatus("w");
	} else
	    traceServer("ANSWER: Voice connections are not permitted");
	callHandled = true;
	break;
    case ClassModem::CALLTYPE_ERROR:
	traceServer("ANSWER: %s", eresult.string());
	break;
    }
    return (callHandled);
}

/*
 * Run a getty subprocess and wait for it to terminate.
 * The speed parameter is passed to use in establishing
 * a login session.
 */
CallType
faxGettyApp::runGetty(
    const char* what,
    Getty* (*newgetty)(const fxStr&, const fxStr&),
    const char* args,
    Status& eresult,
    bool keepLock,
    const CallID& callid,
    bool keepModem
)
{
    fxStr prefix(_PATH_DEV);
    fxStr dev(getModemDevice());
    if (dev.head(prefix.length()) == prefix)
	dev.remove(0, prefix.length());
    Getty* getty = (*newgetty)(dev, fxStr::format("%u", getModemRate()));
    if (getty == NULL) {
	eresult = Status(311, "%s: could not create", what);
	return (ClassModem::CALLTYPE_ERROR);
    }

    getty->setupArgv(args, callid);

    /*
     * The getty process should not inherit the lock file.
     * Remove it here before the fork so that our state is
     * correct (so further unlock calls will do nothing).
     * Note that we remove the lock here because apps such
     * as ppp and slip that install their own lock cannot
     * cope with finding a lock in place (even if it has
     * their pid in it).  This creates a potential window
     * during which outbound jobs might grab the modem
     * since they won't find a lock file in place.
     */
    if (!keepLock)
	unlockModem();
    bool parentIsInit = (getppid() == 1);
    pid_t pid = fork();
    if (pid == -1) {
	eresult = Status(312, "%s: can not fork: %s", what, strerror(errno));
	delete getty;
	return (ClassModem::CALLTYPE_ERROR);
    }
    if (pid == 0) {			// child, start getty session
	setProcessPriority(BASE);	// remove any high priority
	if (keepLock)
	    /*
	     * The getty process should inherit the lock file.
	     * Force the UUCP lock owner so that apps find their
	     * own pid in the lock file.  Otherwise they abort
	     * thinking some other process already has control
	     * of the modem.  Note that doing this creates a
	     * potential window for stale lock removal between
	     * the time the login process terminates and the
	     * parent retakes ownership of the lock file (see below).
	     */
	    modemLock->setOwner(getpid());
	{
	  setpwent();

	  uid_t uid = getuid();
	  gid_t gid = getgid();
          uid_t euid = geteuid();
          gid_t egid = getegid();
	  if (setegid(gid) < 0)
	      traceServer("runGetty::setegid: %m");
	  if (seteuid(uid) < 0)
	      traceServer("runGetty::seteuid (child): %m");
          const struct passwd *pwd = getpwuid(euid);
          if (initgroups(pwd->pw_name, egid) != 0)
              traceServer("runGetty::initgroups: %m");

	  endpwent();
	}
	getty->run(getModemFd(), parentIsInit);
	_exit(127);
	/*NOTREACHED*/
    }
    traceServer("%s: START \"%s\", pid %lu", what,
	(const char*) getty->getCmdLine(), (u_long) pid);
    getty->setPID(pid);
    /*
     * Purge existing modem state because the getty+login
     * processe will change everything and because we must
     * close the descriptor so that the getty will get
     * SIGHUP on last close.
     */
    if (!keepModem)
	discardModem(false);
    changeState(GETTYWAIT);
    int status;
    getty->wait(status, true);		// wait for getty/login work to complete
    if ( status > 1280 ) { // codes returned larger than 1280 are undefined and must be an error    
        status = 1024;
        eresult = Status(313, "ERROR: Unknown status");
    }
    /*
     * Retake ownership of the modem.  Note that there's
     * a race in here (another process could come along
     * and purge the lock file thinking it was stale because
     * the pid is for the process that just terminated);
     * the only way to avoid it is to use a real locking
     * mechanism (e.g. flock on the lock file).
     */
    if (keepLock)
	modemLock->setOwner(0);		// NB: 0 =>'s use setup pid
    uid_t euid = geteuid();
    if (seteuid(getuid()) < 0)		// Getty::hangup assumes euid is root
	 traceServer("runGetty: seteuid (parent): %m");
    getty->hangup();			// cleanup getty-related stuff
    seteuid(euid);
    traceServer("%s: exit status %#o", what, status);
    delete getty;
    return (CallType)((status >> 8) & 0xff);
}

/*
 * Set the number of rings to wait before answering
 * the telephone.  If there is a modem setup, then
 * configure the dispatcher to reflect whether or not
 * we need to listen for data from the modem (the RING
 * status messages).
 */
void
faxGettyApp::setRingsBeforeAnswer(int rings)
{
    ringsBeforeAnswer = rings;
    if (modemReady()) {
	int fd = getModemFd();
	Dispatcher::instance().link(fd, Dispatcher::ReadMask, this);
	/*
	 * Systems that have a tty driver based on SVR4 streams
	 * frequently don't implement select properly in that if
	 * output is collected by another process (e.g. cu, tip,
	 * kermit) quickly we do not reliably get woken up.  On
	 * these systems however the close on the tty usually
	 * causes us to wakeup from select for an exceptional
	 * condition on the descriptor--and this is good enough
	 * for us to do the work we need.
	 */
	Dispatcher::instance().link(fd, Dispatcher::ExceptMask, this);
    }
}

/*
 * Notification handlers.
 */

/*
 * Handle notification that the modem device has become
 * available again after a period of being unavailable.
 */
void
faxGettyApp::notifyModemReady()
{
    (void) faxApp::sendModemStatus(getModemDeviceID(),
	readyState | getModemCapabilities() | ":%02x", modemPriority);
}

/*
 * Handle notification that the modem device looks to
 * be in a state that requires operator intervention.
 */
void
faxGettyApp::notifyModemWedged()
{
    if (!sendModemStatus("W"))
	logError("MODEM %s appears to be wedged",
	    (const char*) getModemDevice());
    close();
}

void
faxGettyApp::notifyRecvBegun(FaxRecvInfo& ri)
{
    (void) sendRecvStatus(getModemDeviceID(), "S%s", (const char*) ri.encode());

    FaxServer::notifyRecvBegun(ri);
}

/*
 * Handle notification that a page has been received.
 */
void
faxGettyApp::notifyPageRecvd(TIFF* tif, FaxRecvInfo& ri, int ppm)
{
    (void) sendRecvStatus(getModemDeviceID(), "P%s", (const char*) ri.encode());

    FaxServer::notifyPageRecvd(tif, ri, ppm);

    // XXX fill in
}

/*
 * Handle notification that a document has been received.
 */
void
faxGettyApp::notifyDocumentRecvd(FaxRecvInfo& ri)
{
    (void) sendRecvStatus(getModemDeviceID(), "D%s", (const char*) ri.encode());

    FaxServer::notifyDocumentRecvd(ri);

    FaxAcctInfo ai;
    ai.user = "fax";
    ai.commid = getCommID();
    ai.duration = (time_t) ri.time;
    ai.start = Sys::now() - ai.duration;
    ai.conntime = ai.duration;
    ai.device = getModemDeviceID();
    ai.dest = getModemNumber();
    ai.csi = ri.sender;
    ai.npages = ri.npages;
    ai.params = ri.params.encode();
    ai.status = ri.reason;
    ai.jobid = ri.qfile;
    ai.jobtag = "";
    ai.callid = ri.callid;
    ai.owner = "";
    ri.params.asciiEncode(ai.faxdcs);
    if (!ai.record("RECV"))
	logError("Error writing RECV accounting record, dest=%s",
	    (const char*) ai.dest);
}

/*
 * Handle notification that a document has been received.
 */
void
faxGettyApp::notifyRecvDone(FaxRecvInfo& ri)
{
    FaxServer::notifyRecvDone(ri);

    fxStr callid_formatted;
    for (u_int i = 0; i < ri.callid.size(); i++) {
	callid_formatted.append(quote | ri.callid.id(i) | enquote);
    }
    // hand to delivery/notification command
    fxStr cmd(faxRcvdCmd
	| quote |             ri.qfile	| enquote
	| quote |   getModemDeviceID()	| enquote
	| quote |            ri.commid	| enquote
	| quote |            ri.reason	| enquote
	| callid_formatted);
    traceServer("RECV FAX: %s", (const char*) cmd);
    setProcessPriority(BASE);			// lower priority
    runCmd(cmd, true, this);
    setProcessPriority(state);			// restore priority
}

/*
 * Send a modem status message to the central queuer process.
 */
bool
faxGettyApp::sendModemStatus(const char* msg)
{
    return faxApp::sendModemStatus(getModemDeviceID(), msg);
}

/*
 * FIFO-related support.
 */

/*
 * Open the requisite FIFO special files.
 */
void
faxGettyApp::openFIFOs()
{
    devfifo = openFIFO(fifoName | "." | getModemDeviceID(), 0600, true);
    Dispatcher::instance().link(devfifo, Dispatcher::ReadMask, this);
}

void
faxGettyApp::closeFIFOs()
{
    Sys::close(devfifo), devfifo = -1;
}

/*
 * Respond to input on the specified file descriptor.
 */
int
faxGettyApp::inputReady(int fd)
{
    if (fd == devfifo)
	return FIFOInput(fd);
    else {
	if (state == LISTENING)
	    listenForRing();
	else
	    listenBegin();
	return (0);
    }
}

/*
 * Process a message received through a FIFO.
 */
void
faxGettyApp::FIFOMessage(const char* cp)
{
    switch (cp[0]) {
    case 'A':				// answer the phone
	traceServer("ANSWER %s", cp[1] != '\0' ? cp+1 : "any");
	if (cp[1] != '\0' && !streq(cp+1, "any")) {
	    if (streq(cp+1, "fax"))
		answerPhoneCmd(ClassModem::ANSTYPE_FAX);
	    else if (streq(cp+1, "data"))
		answerPhoneCmd(ClassModem::ANSTYPE_DATA);
	    else if (streq(cp+1, "voice"))
		answerPhoneCmd(ClassModem::ANSTYPE_VOICE);
	    else if (streq(cp+1, "extern"))
		answerPhoneCmd(ClassModem::ANSTYPE_EXTERN);
	    else if (strncmp(cp+1, "dial", 4) == 0) {
		/*   
		 * In order to accomodate some so-called "polling" servers which
		 * do not follow spec in that we have to dial in, but begin the
		 * poll with ATA and follow fax reception protocol rather than
		 * polling protocol.  (Which essentially requires us to not
		 * produce typical calling CNG beeps, but rather a CED squelch.)
		 * We must terminate the dialstring with a semicolon, which
		 * should instruct the modem to return "OK" after dialing and not
		 * produce CNG, at which time we would follow with ATA.
		 */
		const char* dialnumber = cp+5;		// usually must end with ";"
		answerPhoneCmd(ClassModem::ANSTYPE_DIAL, dialnumber);
	    }
	} else
	    answerPhoneCmd(answerRotary[0]);
	break;
    case 'C':				// configuration control
	traceServer("CONFIG \"%s\"", cp+1);
	readConfigItem(cp+1);
	break;
    case 'H':				// HELLO from queuer
	traceServer("HELLO");
	sendModemStatus("N" | getModemNumber());
	if (state == FaxServer::RUNNING)
	    notifyModemReady();		// sends capabilities also
	break;
    case 'Q':				// quit
	traceServer("QUIT");
	close();
	break;
    case 'S':				// set modem ready state
	traceServer("STATE \"%s\"", cp+1);
	setConfigItem("modemreadystate", cp+1);
	break;
    case 'L':				// set modem ready state
	traceServer("LOCKWAIT");
	discardModem(false);
	changeState(LOCKWAIT, pollLockWait);
	break;
    case 'Z':				// abort send/receive
	FaxServer::abortSession(Status(301, "Receive aborted due to operator intervention"));
	break;
    default:
	faxApp::FIFOMessage(cp);
	break;
    }
}

/*
 * Miscellaneous stuff.
 */

/*
 * Configuration support.
 */

void
faxGettyApp::resetConfig()
{
    FaxServer::resetConfig();
    setupConfig();
}

#define	N(a)	(sizeof (a) / sizeof (a[0]))

faxGettyApp::stringtag faxGettyApp::strings[] = {
{ "gettyargs",		&faxGettyApp::gettyArgs },
{ "vgettyargs",		&faxGettyApp::vgettyArgs },
{ "egettyargs",		&faxGettyApp::egettyArgs },
{ "faxrcvdcmd",		&faxGettyApp::faxRcvdCmd,	FAX_FAXRCVDCMD },
{ "dynamicconfig",	&faxGettyApp::dynamicConfig },
};
faxGettyApp::numbertag faxGettyApp::numbers[] = {
{ "answerbias",		&faxGettyApp::answerBias,	(u_int) -1 },
};
faxGettyApp::booltag faxGettyApp::booleans[] = {
{ "adaptiveanswer",	&faxGettyApp::adaptiveAnswer,	false },
{ "lockdatacalls",	&faxGettyApp::lockDataCalls,	true },
{ "lockvoicecalls",	&faxGettyApp::lockVoiceCalls,	true },
{ "lockexterncalls",	&faxGettyApp::lockExternCalls,	true },
{ "logcalls",		&faxGettyApp::logCalls,		true },
{ "rejectcall",		&faxGettyApp::rejectCall,	false },
};

void
faxGettyApp::setupConfig()
{
    int i;

    for (i = N(strings)-1; i >= 0; i--)
	(*this).*strings[i].p = (strings[i].def ? strings[i].def : "");
    for (i = N(numbers)-1; i >= 0; i--)
	(*this).*numbers[i].p = numbers[i].def;
    for (i = N(booleans)-1; i >= 0; i--)
	(*this).*booleans[i].p = booleans[i].def;
    readyState = "R";			// default is really ready
    modemPriority = 255;		// default is lowest priority
    ringsBeforeAnswer = 0;		// default is not to answer phone
    setAnswerRotary("any");		// answer calls as ``any''
}

bool
faxGettyApp::setConfigItem(const char* tag, const char* value)
{
    u_int ix;
    if (findTag(tag, (const tags*) strings, N(strings), ix)) {
	(*this).*strings[ix].p = value;
    } else if (findTag(tag, (const tags*)numbers, N(numbers), ix)) {
	(*this).*numbers[ix].p = getNumber(value);
	switch (ix) {
	case 0: answerBias = fxmin(answerBias, (u_int) 2); break;
	}
    } else if (findTag(tag, (const tags*)booleans, N(booleans), ix)) {
	(*this).*booleans[ix].p = getBoolean(value);
    } else if (streq(tag, "ringsbeforeanswer")) {
	setRingsBeforeAnswer(getNumber(value));
    } else if (streq(tag, "answerrotary")) {
	setAnswerRotary(value);
    } else if (streq(tag, "modempriority")) {
	u_int p = getNumber(value);
	if (p != modemPriority) {
	    modemPriority = p;
	    if (state == FaxServer::RUNNING)
		notifyModemReady();
	}
    } else if (streq(tag, "modemreadystate")) {
	if (readyState != value) {
	    readyState = value;
	    if (state == FaxServer::RUNNING)
		notifyModemReady();
	}
    } else
	return (FaxServer::setConfigItem(tag, value));
    return (true);
}
#undef	N

/*
 * Process an answer rotary spec string.
 */
void
faxGettyApp::setAnswerRotary(const fxStr& value)
{
    u_int i;
    u_int l = 0;
    for (i = 0; i < 3 && l < value.length(); i++) {
	fxStr type(value.token(l, " \t"));
	type.raisecase();
	if (type == "FAX")
	    answerRotary[i] = ClassModem::ANSTYPE_FAX;
	else if (type == "DATA")
	    answerRotary[i] = ClassModem::ANSTYPE_DATA;
	else if (type == "VOICE")
	    answerRotary[i] = ClassModem::ANSTYPE_VOICE;
	else if (type == "EXTERN")
	    answerRotary[i] = ClassModem::ANSTYPE_EXTERN;
	else {
	    if (type != "ANY")
		configError("Unknown answer type \"%s\"", (const char*) type);
	    answerRotary[i] = ClassModem::ANSTYPE_ANY;
	}
    }
    if (i == 0)				// void string
	answerRotary[i++] = ClassModem::ANSTYPE_ANY;
    answerRotor = 0;
    answerRotorSize = i;
}

static void
usage(const char* appName)
{
    faxApp::fatal("usage: %s [-c config-item] [-q queue-dir] [-Dpx] modem-device", appName);
}

static void
sigCleanup(int s)
{
    logError("CAUGHT SIGNAL %d", s);
    faxGettyApp::instance().close();
    _exit(-1);
}

int
main(int argc, char** argv)
{
    faxApp::setupLogging("FaxGetty");

    fxStr appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');

    faxGettyApp::setupPermissions();

    faxApp::setOpts("c:q:Ddpx");		// p+x are for ModemServer

    fxStr queueDir(FAX_SPOOLDIR);
    fxStr device;
    GetoptIter iter(argc, argv, faxApp::getOpts());
    for (; iter.notDone(); iter++)
	switch (iter.option()) {
	case 'q': queueDir = iter.optArg(); break;
	case '?': usage(appName);
	}
    if (device == "") {
	device = iter.getArg();
	if (device == "")
	    usage(appName);
    }
    if (device[0] != '/')				// for getty
	device.insert(_PATH_DEV);
    if (Sys::chdir(queueDir) < 0)
	faxApp::fatal(queueDir | ": Can not change directory");

    faxGettyApp* app = new faxGettyApp(device, faxApp::devToID(device));

    signal(SIGTERM, fxSIGHANDLER(sigCleanup));
    signal(SIGINT, fxSIGHANDLER(sigCleanup));

    app->initialize(argc, argv);
    app->open();
    while (app->isRunning())
	Dispatcher::instance().dispatch();
    app->close();
    delete app;

    return 0;
}
