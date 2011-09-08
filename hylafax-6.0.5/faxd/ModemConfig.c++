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
#include "ModemConfig.h"

#include "t.30.h"
#include "config.h"
#include "tiffio.h"

#include <ctype.h>

#define	N(a)	(sizeof (a) / sizeof (a[0]))

ModemConfig::ModemConfig()
{
    setupConfig();
}

ModemConfig::~ModemConfig()
{
}

const fxStr&
ModemConfig::getFlowCmd(FlowControl f) const
{
    if (f == ClassModem::FLOW_RTSCTS)
	return (hardFlowCmd);
    else if (f == ClassModem::FLOW_XONXOFF)
	return (softFlowCmd);
    else if (f == ClassModem::FLOW_NONE)
	return (noFlowCmd);
    else
	return (fxStr::null);
}

/*
 * The following tables map configuration parameter names to
 * pointers to class ModemConfig members and provide default
 * values that are forced when the configuration is reset
 * prior to reading a configuration file.
 */

/*
 * Note that all of the Class 2/2.0 parameters except
 * Class2CQQueryCmd are initialized at the time the
 * modem is setup based on whether the modem is Class 2
 * or Class 2.0.
 */
static struct {
    const char*		 name;
    fxStr ModemConfig::* p;
    const char*		 def;		// NULL is shorthand for ""
} atcmds[] = {
{ "modemanswercmd",		&ModemConfig::answerAnyCmd,	"ATA" },
{ "modemansweranycmd",		&ModemConfig::answerAnyCmd },
{ "modemanswerfaxcmd",		&ModemConfig::answerFaxCmd },
{ "modemanswerdatacmd",		&ModemConfig::answerDataCmd },
{ "modemanswervoicecmd",	&ModemConfig::answerVoiceCmd },
{ "modemanswerdialcmd",		&ModemConfig::answerDialCmd },
{ "modemanswerfaxbegincmd",	&ModemConfig::answerFaxBeginCmd },
{ "modemanswerdatabegincmd",	&ModemConfig::answerDataBeginCmd },
{ "modemanswervoicebegincmd",	&ModemConfig::answerVoiceBeginCmd },
{ "modemringresponse",		&ModemConfig::ringResponse },
{ "modemresetcmds",		&ModemConfig::resetCmds },
{ "modemreadycmds",		&ModemConfig::readyCmds },
{ "modemdialcmd",		&ModemConfig::dialCmd,		"ATDT%s" },
{ "modemnoflowcmd",		&ModemConfig::noFlowCmd },
{ "modemsoftflowcmd",		&ModemConfig::softFlowCmd },
{ "modemhardflowcmd",		&ModemConfig::hardFlowCmd },
{ "modemsetupaacmd",		&ModemConfig::setupAACmd },
{ "modemsetupdtrcmd",		&ModemConfig::setupDTRCmd },
{ "modemsetupdcdcmd",		&ModemConfig::setupDCDCmd },
{ "modemnoautoanswercmd",	&ModemConfig::noAutoAnswerCmd,	"ATS0=0" },
{ "modemechooffcmd",		&ModemConfig::echoOffCmd,	"ATE0" },
{ "modemverboseresultscmd",	&ModemConfig::verboseResultsCmd,"ATV1" },
{ "modemresultcodescmd",	&ModemConfig::resultCodesCmd,	"ATQ0" },
{ "modemonhookcmd",		&ModemConfig::onHookCmd,	"ATH0" },
{ "modemsoftresetcmd",		&ModemConfig::softResetCmd,	"ATZ" },
{ "modemwaittimecmd",		&ModemConfig::waitTimeCmd,	"ATS7=60" },
{ "modemcommapausetimecmd",	&ModemConfig::pauseTimeCmd,	"ATS8=2" },
{ "modemmfrquerycmd",		&ModemConfig::mfrQueryCmd },
{ "modemmodelquerycmd",		&ModemConfig::modelQueryCmd },
{ "modemrevquerycmd",		&ModemConfig::revQueryCmd },
{ "modemsendbegincmd",		&ModemConfig::sendBeginCmd },
{ "modemrecvsuccesscmd",	&ModemConfig::recvSuccessCmd },
{ "modemclassquerycmd",		&ModemConfig::classQueryCmd,	"AT+FCLASS=?" },
{ "class0cmd",			&ModemConfig::class0Cmd,	"AT+FCLASS=0" },
{ "class1cmd",			&ModemConfig::class1Cmd },
{ "class1adaptrecvcmd",		&ModemConfig::class1AdaptRecvCmd },
{ "class1enablev34cmd",		&ModemConfig::class1EnableV34Cmd },
{ "class1nflocmd",		&ModemConfig::class1NFLOCmd },
{ "class1sflocmd",		&ModemConfig::class1SFLOCmd },
{ "class1hflocmd",		&ModemConfig::class1HFLOCmd },
{ "class1ppmwaitcmd",		&ModemConfig::class1PPMWaitCmd,	"AT+FTS=7" },
{ "class1responsewaitcmd",	&ModemConfig::class1ResponseWaitCmd,	"" },
{ "class1rmquerycmd",		&ModemConfig::class1RMQueryCmd,	"AT+FRM=?" },
{ "class1tcfwaitcmd",		&ModemConfig::class1TCFWaitCmd,	"AT+FTS=7" },
{ "class1tmquerycmd",		&ModemConfig::class1TMQueryCmd,	"AT+FTM=?" },
{ "class1eopwaitcmd",		&ModemConfig::class1EOPWaitCmd,	"AT+FTS=9" },
{ "class1msgrecvhackcmd",	&ModemConfig::class1MsgRecvHackCmd, "" },
{ "class1tcfrecvhackcmd",	&ModemConfig::class1TCFRecvHackCmd, "" },
{ "class1switchingcmd",		&ModemConfig::class1SwitchingCmd, "AT+FRS=7" },
{ "class2cmd",			&ModemConfig::class2Cmd },
{ "class2borcmd",		&ModemConfig::class2BORCmd },
{ "class2disablev17cmd",	&ModemConfig::class2DisableV17Cmd },
{ "class2relcmd",		&ModemConfig::class2RELCmd },
{ "class2cqcmd",		&ModemConfig::class2CQCmd },
{ "class2abortcmd",		&ModemConfig::class2AbortCmd },
{ "class2cqquerycmd",    	&ModemConfig::class2CQQueryCmd,	"AT+FCQ=?" },
{ "class2dccquerycmd",		&ModemConfig::class2DCCQueryCmd },
{ "class2tbccmd",		&ModemConfig::class2TBCCmd },
{ "class2crcmd",		&ModemConfig::class2CRCmd },
{ "class2phctocmd",		&ModemConfig::class2PHCTOCmd },
{ "class2bugcmd",		&ModemConfig::class2BUGCmd },
{ "class2lidcmd",		&ModemConfig::class2LIDCmd },
{ "class2dcccmd",		&ModemConfig::class2DCCCmd },
{ "class2discmd",		&ModemConfig::class2DISCmd },
{ "class2ddiscmd",		&ModemConfig::class2DDISCmd },
{ "class2cigcmd",		&ModemConfig::class2CIGCmd },
{ "class2ptscmd",		&ModemConfig::class2PTSCmd },
{ "class2ptsquerycmd",		&ModemConfig::class2PTSQueryCmd, "AT+FPS?" },
{ "class2splcmd",		&ModemConfig::class2SPLCmd },
{ "class2piecmd",		&ModemConfig::class2PIECmd },
{ "class2nrcmd",		&ModemConfig::class2NRCmd },
{ "class2nflocmd",		&ModemConfig::class2NFLOCmd },
{ "class2sflocmd",		&ModemConfig::class2SFLOCmd },
{ "class2hflocmd",		&ModemConfig::class2HFLOCmd },
{ "class2minspcmd",		&ModemConfig::class2MINSPCmd },
{ "class2apquerycmd",		&ModemConfig::class2APQueryCmd,	"AT+FAP=?" },
{ "class2apcmd",		&ModemConfig::class2APCmd,	"AT+FAP" },
{ "class2sacmd",		&ModemConfig::class2SACmd,	"AT+FSA" },
{ "class2pacmd",		&ModemConfig::class2PACmd,	"AT+FPA" },
{ "class2pwcmd",		&ModemConfig::class2PWCmd,	"AT+FPW" },
};
static struct {
    const char*		 name;
    fxStr ModemConfig::* p;
    const char*		 def;		// NULL is shorthand for ""
} strcmds[] = {
{ "modemtype",			&ModemConfig::type,		"unknown" },
{ "taglinecovernumstring",	&ModemConfig::tagLineCoverNumString },
{ "taglinefont",		&ModemConfig::tagLineFontFile },
{ "taglineformat",		&ModemConfig::tagLineFmt,
  "From %%n|%c|Page %%p of %%t" },
{ "class2recvdatatrigger",	&ModemConfig::class2RecvDataTrigger },
{ "ringdata",			&ModemConfig::ringData },
{ "ringfax",			&ModemConfig::ringFax },
{ "ringvoice",			&ModemConfig::ringVoice },
{ "ringextended",		&ModemConfig::ringExtended },
{ "dringon",			&ModemConfig::dringOn  },
{ "dringoff",			&ModemConfig::dringOff  },
};
static struct {
    const char*		 name;
    u_int ModemConfig::* p;
    u_int		 def;
} fillorders[] = {
{ "modemrecvfillorder",  &ModemConfig::recvFillOrder,  0 }, // will be autodetected
{ "modemsendfillorder",  &ModemConfig::sendFillOrder,  0 }, // will be autodetected
{ "modemframefillorder", &ModemConfig::frameFillOrder, FILLORDER_LSB2MSB },
};
static struct {
    const char*		 name;
    u_int ModemConfig::* p;
    u_int		 def;
} numbers[] = {
{ "ringtimeout",		&ModemConfig::ringTimeout,	     6000 },
{ "percentgoodlines",		&ModemConfig::percentGoodLines,	     95 },
{ "maxconsecutivebadlines",	&ModemConfig::maxConsecutiveBadLines,5 },
{ "minacceptedlinecount",	&ModemConfig::minAcceptedLineCount,  10 },
{ "modemresetdelay",		&ModemConfig::resetDelay,	     2600 },
{ "modemdtrdropdelay",		&ModemConfig::dtrDropDelay,	     75 },
{ "modembaudratedelay",		&ModemConfig::baudRateDelay,	     10 },
{ "modematcmddelay",		&ModemConfig::atCmdDelay,	     0 },
{ "faxt1timer",			&ModemConfig::t1Timer,		     TIMER_T1 },
{ "faxt2timer",			&ModemConfig::t2Timer,		     TIMER_T2 },
{ "faxt4timer",			&ModemConfig::t4Timer,		     TIMER_T4 },
{ "modemdialresponsetimeout",	&ModemConfig::dialResponseTimeout,   3*60*1000},
{ "modemanswerresponsetimeout",	&ModemConfig::answerResponseTimeout, 3*60*1000},
{ "modempagestarttimeout",	&ModemConfig::pageStartTimeout,	     3*60*1000},
{ "modempagedonetimeout",	&ModemConfig::pageDoneTimeout,	     3*60*1000},
{ "modemringsbeforeresponse",	&ModemConfig::ringsBeforeResponse,   0 },
{ "modemsoftresetcmddelay",	&ModemConfig::softResetCmdDelay,     3000 },
{ "modemnoautoanswercmddelay",	&ModemConfig::noAutoAnswerCmdDelay,  0 },
{ "class1tcfrecvtimeout",	&ModemConfig::class1TCFRecvTimeout,  4500 },
{ "class1recvabortok",		&ModemConfig::class1RecvAbortOK,     200 },
{ "class1rmpersistence",	&ModemConfig::class1RMPersistence,   2 },
{ "class1hooksensitivity",	&ModemConfig::class1HookSensitivity, 0 },
{ "class1frameoverhead",	&ModemConfig::class1FrameOverhead,   4 },
{ "class1recvidenttimer",	&ModemConfig::class1RecvIdentTimer,  TIMER_T1 },
{ "class1tcfmaxnonzero",	&ModemConfig::class1TCFMaxNonZero,   10 },
{ "class1tcfminrun",		&ModemConfig::class1TCFMinRun,       (2*TCF_DURATION)/3 },
{ "class1tcfminrunecmmod",	&ModemConfig::class1TCFMinRunECMMod, 2 },
{ "class1tmconnectdelay",	&ModemConfig::class1TMConnectDelay,  0 },
{ "class1ecmframesize",		&ModemConfig::class1ECMFrameSize,    256 },
{ "class1pagelengthsupport",	&ModemConfig::class1PageLengthSupport, LN_ALL },
{ "class1pagewidthsupport",	&ModemConfig::class1PageWidthSupport, WD_ALL },
};
static struct {
    const char*		 name;
    bool ModemConfig::* p;
    bool		 def;
} booleans[] = {
{ "usejobtagline",		&ModemConfig::useJobTagLine,		true	},
{ "countskippedpages",		&ModemConfig::countSkippedPages,	true	},
{ "noanswervoice",		&ModemConfig::noAnswerVoice,		false	},
{ "saverawimage",		&ModemConfig::saverawimage,		false	},
{ "saveunconfirmedpages",	&ModemConfig::saveUnconfirmedPages,	true	},
{ "modemwaitforconnect",	&ModemConfig::waitForConnect,		false	},
{ "modemsoftrtfcc",		&ModemConfig::softRTFCC,		true	},
{ "modemdophasecdebug",		&ModemConfig::doPhaseCDebug,		false	},
{ "modemraiseatcommands",	&ModemConfig::raiseATCmd,		true	},

{ "class1persistentecm",	&ModemConfig::class1PersistentECM,	true	},
{ "class1validatev21frames",	&ModemConfig::class1ValidateV21Frames,	false	},
{ "class1modemhasdlebug",	&ModemConfig::class1ModemHasDLEBug,	false	},
{ "class1hasrhconnectbug",	&ModemConfig::class1HasRHConnectBug,	false	},
{ "class1ecmsupport",		&ModemConfig::class1ECMSupport,		true	},
{ "class1mrsupport",		&ModemConfig::class1MRSupport,		true	},
{ "class1mmrsupport",		&ModemConfig::class1MMRSupport,		true	},
#ifdef PHOTOMETRIC_ITULAB
{ "class1greyjpegsupport",	&ModemConfig::class1GreyJPEGSupport,	false	},
{ "class1colorjpegsupport",	&ModemConfig::class1ColorJPEGSupport,	false	},
#endif

{ "class2xmitwaitforxon",	&ModemConfig::class2XmitWaitForXON,	true	},
{ "class2sendrtc",		&ModemConfig::class2SendRTC,		false	},
{ "class2usehex",		&ModemConfig::class2UseHex,		false	},
{ "class2hexnsf",		&ModemConfig::class2HexNSF,		true	},
{ "class2uselinecount",		&ModemConfig::class2UseLineCount,	false	},
{ "class2rtfcc",		&ModemConfig::class2RTFCC,		false	},
#ifdef PHOTOMETRIC_ITULAB
{ "class2jpegsupport",		&ModemConfig::class2JPEGSupport,	false	},
#endif
};

void
ModemConfig::setupConfig()
{
    int i;

    for (i = N(atcmds)-1; i >= 0; i--)
	(*this).*atcmds[i].p = (atcmds[i].def ? atcmds[i].def : "");
    for (i = N(strcmds)-1; i >= 0; i--)
	(*this).*strcmds[i].p = (strcmds[i].def ? strcmds[i].def : "");
    for (i = N(fillorders)-1; i >= 0; i--)
	(*this).*fillorders[i].p = fillorders[i].def;
    for (i = N(numbers)-1; i >= 0; i--)
	(*this).*numbers[i].p = numbers[i].def;
    for (i = N(booleans)-1; i >= 0; i--)
	(*this).*booleans[i].p = booleans[i].def;

    for (i=0; i < 5; ++i) {
    	distinctiveRings[i].type = ClassModem::CALLTYPE_UNKNOWN;
	for (u_int j=0; j < 5; ++j)
	    distinctiveRings[i].cadence[j] = 0;
	distinctiveRings[i].magsqrd = 0;
    }

    flowControl         = ClassModem::FLOW_XONXOFF;// software flow control
    maxRate		= ClassModem::BR19200;	// reasonable for most modems
    minSpeed		= BR_2400;		// minimum transmit speed
    class2ECMType	= ClassModem::ECMTYPE_UNSET;// follow the service type default
#ifdef HAVE_JBIG
#ifdef HAVE_JBIGTIFF
    class1JBIGSupport	= FaxModem::JBIG_FULL;	// full support for monochrome JBIG
#else
    class1JBIGSupport	= FaxModem::JBIG_SEND;	// send support for monochrome JBIG
#endif
#else
#ifdef HAVE_JBIGTIFF
    class1JBIGSupport	= FaxModem::JBIG_RECV;	// receive support for monochrome JBIG
#else
    class1JBIGSupport	= FaxModem::JBIG_NONE;	// no support for monochrome JBIG
#endif
#endif

    class1Resolutions	= VR_ALL;		// resolutions support
    setVolumeCmds("ATM0 ATL0M1 ATL1M1 ATL2M1 ATL3M1");
    recvDataFormat	= DF_ALL;		// default to no transcoding
    rtnHandling         = FaxModem::RTN_RETRANSMITIGNORE; // retransmit until MCF/MPS
    badPageHandling	= FaxModem::BADPAGE_RTNSAVE; // send RTN but save the page

    idConfig.resize(0);
    callidIndex		= (u_int) -1;

    playList.resize(0);
}

void
ModemConfig::resetConfig()
{
    FaxConfig::resetConfig();
    setupConfig();
}

#define	valeq(a,b)	(strcasecmp(a,b)==0)

bool
ModemConfig::findRate(const char* cp, BaudRate& br)
{
    static const struct {
	const char* name;
	BaudRate    b;
    } rates[] = {
	{    "300", ClassModem::BR300 },
	{   "1200", ClassModem::BR1200 },
	{   "2400", ClassModem::BR2400 },
	{   "4800", ClassModem::BR4800 },
	{   "9600", ClassModem::BR9600 },
	{  "19200", ClassModem::BR19200 },
	{  "38400", ClassModem::BR38400 },
	{  "57600", ClassModem::BR57600 },
	{  "76800", ClassModem::BR76800 },
	{ "115200", ClassModem::BR115200 },
    };
    for (int i = N(rates)-1; i >= 0; i--)
	if (streq(cp, rates[i].name)) {
	    br = rates[i].b;
	    return (true);
	}
    return (false);
}

BaudRate
ModemConfig::getRate(const char* cp)
{
    BaudRate br;
    if (!findRate(cp, br)) {
	configError("Unknown baud rate \"%s\", using 19200", cp);
	br = ClassModem::BR19200;		// default
    }
    return (br);
}

ECMType
ModemConfig::getECMType(const char* cp)
{
    if (valeq(cp, "2"))
	return (ClassModem::ECMTYPE_CLASS2);
    if (valeq(cp, "2.0"))
	return (ClassModem::ECMTYPE_CLASS20);
    configError("Unknown ECM type specification \"%s\", using default", cp);
    return (ClassModem::ECMTYPE_UNSET);
}

bool
ModemConfig::findATResponse(const char* cp, ATResponse& resp)
{
    static const struct {
	const char* name;
	ATResponse  r;
    } responses[] = {
	{    "NOTHING", ClassModem::AT_NOTHING },
	{         "OK",	ClassModem::AT_OK },
	{    "CONNECT", ClassModem::AT_CONNECT },
	{   "NOANSWER", ClassModem::AT_NOANSWER },
	{  "NOCARRIER", ClassModem::AT_NOCARRIER },
	{ "NODIALTONE", ClassModem::AT_NODIALTONE },
	{       "BUSY", ClassModem::AT_BUSY },
	{    "OFFHOOK", ClassModem::AT_OFFHOOK },
	{       "RING", ClassModem::AT_RING },
	{      "ERROR", ClassModem::AT_ERROR },
	{       "VCON", ClassModem::AT_VCON },
	{      "OTHER", ClassModem::AT_OTHER },
    };
    for (u_int i = 0; i < N(responses); i++)
	if (valeq(cp, responses[i].name)) {
	    resp = responses[i].r;
	    return (true);
	}
    return (false);
}

u_int
ModemConfig::getFill(const char* cp)
{
    if (valeq(cp, "LSB2MSB"))
	return (FILLORDER_LSB2MSB);
    else if (valeq(cp, "MSB2LSB"))
	return (FILLORDER_MSB2LSB);
    else {
	configError("Unknown fill order \"%s\"", cp);
        return ((u_int) -1);
    }
}

bool
ModemConfig::findFlow(const char* cp, FlowControl& fc)
{
    static const struct {
	const char* name;
	FlowControl f;
    } fcnames[] = {
	{ "XONXOFF", ClassModem::FLOW_XONXOFF },
	{  "RTSCTS", ClassModem::FLOW_RTSCTS },
	{    "NONE", ClassModem::FLOW_NONE },
	{     "XON", ClassModem::FLOW_XONXOFF },
	{     "RTS", ClassModem::FLOW_RTSCTS },
    };
    for (u_int i = 0; i < N(fcnames); i++)
	if (valeq(cp, fcnames[i].name)) {
	    fc = fcnames[i].f;
	    return (true);
	}
    return (false);
}

FlowControl
ModemConfig::getFlow(const char* cp)
{
    FlowControl fc;
    if (!findFlow(cp, fc)) {
	configError("Unknown flow control \"%s\", using xonxoff", cp);
	fc = ClassModem::FLOW_XONXOFF;		// default
    }
    return (fc);
}

void
ModemConfig::setVolumeCmds(const fxStr& tag)
{
    u_int l = 0;
    for (int i = ClassModem::OFF; i <= ClassModem::HIGH; i++)
	setVolumeCmd[i] = parseATCmd(tag.token(l, " \t"));
}

/*
 * Scan AT command strings and convert <...> escape
 * commands into single-byte escape codes that are
 * interpreted by ClassModem::atCmd.  Note that the
 * baud rate setting commands are carefully ordered
 * so that the desired baud rate can be extracted
 * from the low nibble.
 */
fxStr
ModemConfig::parseATCmd(const char* cp)
{
    fxStr cmd(cp);
    u_int pos = 0;
    while ((pos = cmd.next(pos, '<')) != cmd.length()) {
	u_int epos = pos+1;
	fxStr esc = cmd.token(epos, '>');
	esc.lowercase();

	FlowControl fc;
	BaudRate br;
	u_char ecode[2];
	if (findFlow(esc, fc)) {
	    ecode[0] = ESC_SETFLOW;
	    ecode[1] = (u_char) fc;
	} else if (findRate(esc, br)) {
	    ecode[0] = ESC_SETBR;
	    ecode[1] = (u_char) br;
	} else if (esc == "flush") {
	    cmd.remove(pos, epos-pos);
	    cmd.insert(ESC_FLUSH, pos);
	    continue;
	} else if (esc == "") {		// NB: "<>" => <
	    cmd.remove(pos, epos-pos);
	    cmd.insert('<', pos);
	    continue;
	} else if (esc.length() > 6 && strneq(esc, "delay:", 6)) {
	    u_int delay = (u_int) atoi(&esc[6]);
	    if (delay > 255) {
		configError("Bad AT delay value \"%s\", must be <256", &esc[6]);
		pos = epos;
		continue;
	    }
	    ecode[0] = ESC_DELAY;
	    ecode[1] = (u_char) delay;
	} else if (esc.length() > 8 && strneq(esc, "waitfor:", 8)) {
	    ATResponse resp;
	    if (!findATResponse(&esc[8], resp)) {
		configError("Unknown AT response code \"%s\"", &esc[8]);
		pos = epos;
		continue;
	    }
	    ecode[0] = ESC_WAITFOR;
	    ecode[1] = (u_char) resp;
	} else if (esc.length() > 5 && strneq(esc, "play:", 5)) {
	    if (playList.length() > 255) {
		configError("Can only have 255 PLAY escapes configured");
		pos = epos;
		continue;
	    }
	    ecode[0] = ESC_PLAY;
	    ecode[1] = (u_char) playList.length();
configTrace("Storring \"%s\" to playList[%d]", (const
    char*)esc.tail(esc.length()-5), playList.length());
	    playList.append(esc.tail(esc.length()-5));
	} else {
	    configError("Unknown AT escape code \"%s\"", (const char*) esc);
	    pos = epos;
	    continue;
	}
	cmd.remove(pos, epos-pos);
	cmd.insert((char*) ecode, pos, 2);
	pos += 2;	// don't try parsing our newly inserted ecode
    }
    return (cmd);
}

u_int
ModemConfig::getSpeed(const char* value)
{
    switch (atoi(value)) {
    case 2400:	return (BR_2400);
    case 4800:	return (BR_4800);
    case 7200:	return (BR_7200);
    case 9600:	return (BR_9600);
    case 12000:	return (BR_12000);
    case 14400:	return (BR_14400);
    case 16800:	return (BR_16800);
    case 19200:	return (BR_19200);
    case 21600:	return (BR_21600);
    case 24000:	return (BR_24000);
    case 26400:	return (BR_26400);
    case 28800:	return (BR_28800);
    case 31200:	return (BR_31200);
    case 33600:	return (BR_33600);
    }
    configError("Invalid minimum transmit speed \"%s\"", value);
    return (BR_2400);
}

bool
ModemConfig::findDataFormat(const char* cp, u_int& df)
{
    static const struct {
	const char* name;
	u_int d;
    } dfnames[] = {
	{       "1DMH", DF_1DMH },
	{       "2DMR", DF_2DMR },
	{ "2DMRUNCOMP", DF_2DMRUNCOMP },
	{      "2DMMR", DF_2DMMR },
	{   "adaptive", DF_ALL },
    };
    char v[30];
    u_int i = 0;
    for (; *cp; cp++) {
	if (*cp == '-' || isspace(*cp))		// strip ``-'' and white space
	    continue;
	if (i >= sizeof (v))
	    break;
	v[i++] = *cp;
    }
    v[i] = '\0';
    for (i = 0; i < N(dfnames); i++)
	if (valeq(v, dfnames[i].name)) {
	    df = dfnames[i].d;
	    return (true);
	}
    return (false);
}

u_int
ModemConfig::getDataFormat(const char* cp)
{
    u_int df;
    if (!findDataFormat(cp, df)) {
	configError("Unknown data format \"%s\", disabling transcoding", cp);
	df = DF_ALL;				// default
    }
    return (df);
}

bool
ModemConfig::findRTNHandling(const char* cp, RTNHandling& rh)
{
    static const struct {
        const char* name;
        RTNHandling rh;
    } rhnames[] = {
        { "RETRANSMIT", FaxModem::RTN_RETRANSMIT },
        {     "GIVEUP", FaxModem::RTN_GIVEUP },
        {     "IGNORE", FaxModem::RTN_IGNORE },
        { "RETRANSMIT-IGNORE", FaxModem::RTN_RETRANSMITIGNORE }
    };
    for (u_int i = 0; i < N(rhnames); i++)
        if (valeq(cp, rhnames[i].name)) {
            rh = rhnames[i].rh;
            return (true);
        }
    return (false);
}

u_int
ModemConfig::getRTNHandling(const char* cp)
{
    RTNHandling rh;
    if (!findRTNHandling(cp, rh)) {
        configError("Unknown RTN handling method \"%s\", using RETRANSMIT-IGNORE", cp);
        rh = FaxModem::RTN_RETRANSMITIGNORE;   // default
    }
    return (rh);
}

u_int
ModemConfig::getBadPageHandling(const char* cp)
{
    BadPageHandling bph;
    if (valeq(cp, "RTN")) {
	bph = FaxModem::BADPAGE_RTN;
    } else if (valeq(cp, "DCN")) {
	bph = FaxModem::BADPAGE_DCN;
    } else {
	bph = FaxModem::BADPAGE_RTNSAVE;
    }
    return (bph);
}

u_int
ModemConfig::getJBIGSupport(const char* cp)
{
    JBIGSupport js;
    if (valeq(cp, "FULL")) {
	js = FaxModem::JBIG_FULL;
    } else if (valeq(cp, "YES")) {
	js = FaxModem::JBIG_FULL;
    } else if (valeq(cp, "TRUE")) {
	js = FaxModem::JBIG_FULL;
    } else if (valeq(cp, "NONE")) {
	js = FaxModem::JBIG_NONE;
    } else if (valeq(cp, "NO")) {
	js = FaxModem::JBIG_NONE;
    } else if (valeq(cp, "FALSE")) {
	js = FaxModem::JBIG_NONE;
    } else if (valeq(cp, "SEND")) {
	js = FaxModem::JBIG_SEND;
    } else if (valeq(cp, "RECEIVE")) {
	js = FaxModem::JBIG_RECV;
    } else if (valeq(cp, "RECV")) {
	js = FaxModem::JBIG_RECV;
    } else {
#ifdef HAVE_JBIG
#ifdef HAVE_JBIGTIFF
	js = FaxModem::JBIG_FULL;
#else
	js = FaxModem::JBIG_SEND;
#endif
#else
#ifdef HAVE_JBIGTIFF
	js = FaxModem::JBIG_RECV;
#else
	js = FaxModem::JBIG_NONE;
#endif
#endif
    }
    return (js);
}

void
ModemConfig::parseCallID(const char* rbuf, CallID& callid) const
{
    for (size_t i = 0; i < idConfig.length(); i++) {
	fxAssert(i < callid.size(), "Miss matched Call ID Size with parsing");
	const fxStr& pat = idConfig[i].pattern;
	if (pat.length() && strneq(rbuf, pat, pat.length()))
	    callid[i].append(rbuf+pat.length());
    }
}

void
ModemConfig::parseDR(const char* cin)
{
    if (strlen(cin) < 3) return;
    char buf[2048];
    strncpy(buf, cin, sizeof (buf));
    char* cp = buf;
    u_int i = 0;
    char* cp1 = cp;
    while (*cp++) {
    	if (*cp == ',') {
		*cp = '\0';                     // Nuke the ','
		processDRString(cp1, i++);
		cp1 = ++cp;
	}
    }
    processDRString(cp1, i);
    NoDRings = i + 1;
}

void
ModemConfig::processDRString(char* cp, const u_int i)
{
    if (*cp == 'V')
    	distinctiveRings[i].type = ClassModem::CALLTYPE_VOICE;
    else if (*cp == 'F')
    	distinctiveRings[i].type = ClassModem::CALLTYPE_FAX;
    else if (*cp == 'D')
    	distinctiveRings[i].type = ClassModem::CALLTYPE_DATA;

    while (*cp != '-'){ cp++;}
    *cp = '\0';
    u_int j = 0;
    int sign = 1;
    char *cp1 = cp += 1;
    while (*cp++) {
    	if (*cp == '-') {
	    *cp = '\0';                             // Nuke the '-'
    	    distinctiveRings[i].cadence[j++] = sign*atoi(cp1);
	    sign = -sign;
	    cp1 = ++cp;
	}
    }
    distinctiveRings[i].cadence[j] =  sign*atoi(cp1);

    double sum = 0;
    for ( u_int k=0; k < 5; ++k ) 
        sum += distinctiveRings[i].cadence[k]*distinctiveRings[i].cadence[k];
    distinctiveRings[i].magsqrd = sum;
}

bool
ModemConfig::setConfigItem(const char* tag, const char* value)
{
    u_int ix;
    if (findTag(tag, (const tags*)atcmds, N(atcmds), ix))
	(*this).*atcmds[ix].p = parseATCmd(value);
    else if (findTag(tag, (const tags*)strcmds, N(strcmds), ix))
	(*this).*strcmds[ix].p = value;
    else if (findTag(tag, (const tags*)fillorders, N(fillorders), ix))
	(*this).*fillorders[ix].p = getFill(value);
    else if (findTag(tag, (const tags*)numbers, N(numbers), ix))
	(*this).*numbers[ix].p = atoi(value);
    else if (findTag(tag, (const tags*)booleans, N(booleans), ix))
	(*this).*booleans[ix].p = getBoolean(value);

    else if (streq(tag, "modemsetvolumecmd"))
	setVolumeCmds(value);
    else if (streq(tag, "modemflowcontrol"))
	flowControl = getFlow(value);
    else if (streq(tag, "modemrate"))
	maxRate = getRate(value);
    else if (streq(tag, "class1jbigsupport"))
        class1JBIGSupport = getJBIGSupport(value);
    else if (streq(tag, "class1extendedres"))
	class1Resolutions = getBoolean(value) ? VR_ALL : (VR_NORMAL | VR_FINE);
    else if (streq(tag, "class1resolutions"))
	class1Resolutions = getNumber(value);
    else if (streq(tag, "class1tcfrecvhack") && getBoolean(value))
	class1TCFRecvHackCmd = "AT+FRS=1";	// backwards compatibility
    else if (streq(tag, "modemminspeed"))
	minSpeed = getSpeed(value);
    else if (streq(tag, "recvdataformat"))
	recvDataFormat = getDataFormat(value);
    else if (streq(tag, "rtnhandlingmethod"))
        rtnHandling = getRTNHandling(value);
    else if (streq(tag, "badpagehandlingmethod"))
        badPageHandling = getBadPageHandling(value);
    else if (streq(tag, "class2ecmtype"))
	class2ECMType = getECMType(value);
    else if (streq(tag, "distinctiverings"))
    	parseDR(value);
    else if (streq(tag, "callidpattern") || streq(tag, "callidanswerlength") ) {
	if (tag[6] == 'p') callidIndex++;	// only increment on instances of "Pattern"
	if (idConfig.length() < callidIndex+1 && callidIndex != (u_int) -1)
	    idConfig.resize(callidIndex+1);
	if (tag[6] == 'p') {
	    idConfig[callidIndex].answerlength = 0;	// we must initialize this
	    idConfig[callidIndex].pattern = value;
	    configTrace("CallID[%d].pattern = \"%s\"", callidIndex,
		    (const char*)idConfig[callidIndex].pattern);
	} else {
	    if (callidIndex != (u_int) -1) {
		idConfig[callidIndex].answerlength = atoi(value);
		configTrace("CallID[%d].answerlength = %d", callidIndex,
		    idConfig[callidIndex].answerlength);
	    } else
		configError("No index for Call ID Answer Length");
	}
    } else if (streq(tag, "cidnumber")) {
	if (idConfig.length() < CallID::NUMBER+1)
	    idConfig.resize(CallID::NUMBER+1);
	idConfig[CallID::NUMBER].pattern = value;
	idConfig[CallID::NUMBER].answerlength = 0;
    } else if (streq(tag, "cidname")) {
	if (idConfig.length() < CallID::NAME+1)
	    idConfig.resize(CallID::NAME+1);
	idConfig[CallID::NAME].pattern = value;
	idConfig[CallID::NAME].answerlength = 0;
    } else if (streq(tag, "cidnumberanswerlength")) {
	if (idConfig.length() < CallID::NUMBER+1)
	    idConfig.resize(CallID::NUMBER+1);
	idConfig[CallID::NUMBER].answerlength = getNumber(value);
    } else if (streq(tag, "cidnameanswerlength")) {
	if (idConfig.length() < CallID::NAME+1)
	    idConfig.resize(CallID::NAME+1);
	idConfig[CallID::NAME].answerlength = getNumber(value);
    } else
	return (false);
    return (true);
}
#undef N

fxIMPLEMENT_ObjArray(IDConfArray, id_config);
