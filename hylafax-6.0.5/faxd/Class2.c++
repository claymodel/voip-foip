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
#include "Class2.h"
#include "ModemConfig.h"

#include <ctype.h>

Class2Modem::Class2Modem(FaxServer& s, const ModemConfig& c) : FaxModem(s,c)
{
    hangupCode[0] = '\0';
    serviceType = 0;			// must be set in derived class
    useExtendedDF = false;		// T.32 Amendment 1 extension for data format is detectable
    useJP = false;			// JP +FCC option support is detectable
    imagefd = 0;
}

Class2Modem::~Class2Modem()
{
}

/*
 * Check if the modem is a Class 2 modem and, if so,
 * configure it for use.  We try to confine a lot of
 * the manufacturer-specific bogosity here and leave
 * the remainder of the Class 2 support fairly generic.
 */
bool
Class2Modem::setupModem(bool isSend)
{
    if (!selectBaudRate(conf.maxRate, conf.flowControl, conf.flowControl))
	return (false);
    // Query service support information
    fxStr s;
    if (doQuery(conf.classQueryCmd, s, 5000) && FaxModem::parseRange(s, modemServices))
	traceBits(modemServices & SERVICE_ALL, serviceNames);
    if ((modemServices & serviceType) == 0)
	return (false);
    atCmd(classCmd);

    /*
     * Query manufacturer, model, and firmware revision.
     * We use the manufacturer especially as a key to
     * working around firmware bugs (yech!).
     */
    if (setupManufacturer(modemMfr)) {
	modemCapability("Mfr " | modemMfr);
	modemMfr.raisecase();
    }
    (void) setupModel(modemModel);
    (void) setupRevision(modemRevision);
    if (modemModel != "")
	modemCapability("Model " | modemModel);
    if (modemRevision != "")
	modemCapability("Revision " | modemRevision);

    /*
     * Get modem capabilities and calculate best signalling
     * rate, data formatting capabilities, etc. for use in
     * T.30 negotiations.
     */
    fxStr t30parms;
    if (!doQuery(dccQueryCmd, t30parms, 500)) {
	serverTrace("Error getting modem capabilities");
	return (false);
    }
    /*
     * Syntax: (vr),(br),(wd),(ln),(df),(ec),(bf),(st)[,(jp)]
     * where,
     *	vr	vertical resolution
     *	br	bit rate
     *	wd	page width
     *	ln	page length
     *	df	data compression
     *	ec	error correction
     *	bf	binary file transfer
     *	st	scan time/line
     *	jp	JPEG support (optional)
     */
    if (!parseRange(t30parms, modemParams)) {
	serverTrace("Error parsing " | dccQueryCmd | " response: "
	    "\"" | t30parms | "\"");
	return (false);
    }
    /*
     * The EC parameter varies between the Class 2 and Class 2.0 specs.
     * And because modems don't all adhere to the respective spec, we must adjust
     * modemParams based both on serviceType and on configuration to compensate.
     */
    if (conf.class2ECMType == ClassModem::ECMTYPE_CLASS20 ||
       (conf.class2ECMType == ClassModem::ECMTYPE_UNSET && serviceType != SERVICE_CLASS2)) {
	    // The Class 2.0 spec wisely lumps both 64-byte and 256-byte ECM together
	    // because an ECM receiver must support both anyway per T.30-A.
	    modemParams.ec ^= BIT(EC_DISABLE);	// don't adjust EC_DISABLE
	    modemParams.ec <<= 1;		// simple adjustment
	    modemParams.ec |= BIT(EC_DISABLE);	// reset EC_DISABLE
	    if (modemParams.ec & BIT(EC_ENABLE256))
		modemParams.ec |= BIT(EC_ENABLE64);
    }
    traceModemParams();
    /*
     * Check to see if the modem supports copy quality checking.
     * If the modem is capable, then enable it using the configured
     * commands.  If the modem is incapable of doing copy quality
     * checking, then the host will do the work.
     *
     * The recommendation for AT+FCQ varies significantly between
     * the Class 2 and Class 2.0 specification.
     *
     * An assumption is made that Class2CQCmd, if configured, enables
     * all available copy quality services (or disables them if none
     * are available).
     */
    cqCmds = "";
    sendCQ = 0;
    if (serviceType == SERVICE_CLASS2) {
	/*
	 * The AT+FCQ=? response indicates which compression
         * formats are copy-quality checked.
	 */
	if (doQuery(conf.class2CQQueryCmd, s) && FaxModem::parseRange(s, modemCQ)) {
	    if (modemCQ >>= 1)
		cqCmds = conf.class2CQCmd;
	} else
	    modemCQ = 0;
	static const char* whatCQ[4] = { "no", "1D", "2D", "1D+2D" };
	modemSupports("%s copy quality checking%s", whatCQ[modemCQ&3],
	    (modemCQ && cqCmds == "" ? " (but not enabled)" : ""));
    } else {		// SERVICE_CLASS20, SERVICE_CLASS21
	/*
	 * The AT+FCQ=? response indicates whether or not copy-quality
         * checking and/or correction are supported.  Host CQ cannot
	 * be used if the modem performs copy-quality correction.
	 */
	cqCmds = conf.class2CQCmd;
	if (doQuery(conf.class2CQQueryCmd, s) && FaxModem::vparseRange(s, 0, 2, &modemCQ, &sendCQ)) {
	    modemCQ >>= 1;
	    sendCQ >>= 1;
	} else {
	    modemCQ = 0;
	    sendCQ = 0;
	}
	static const char* whatCQ[4] = { "no", "checking", "correction", "checking and correction" };
	if (modemCQ)
	    modemSupports("receiving copy quality %s", whatCQ[modemCQ&3]);
	else modemSupports("no receiving copy quality services");
	if (sendCQ)
	    modemSupports("sending copy quality %s%s", whatCQ[sendCQ&3],
		(sendCQ && cqCmds == "" ? " (but not enabled)" : ""));
	else modemSupports("no sending copy quality services");
    }
    /*
     * In Class 2 we follow spec defaults and assume that if cqCmds is null 
     * that CQ is not enabled.  In Class 2.0/2.1 we follow spec default and
     * assume the opposite.  In order to know otherwise we'd need to make
     * sense of AT+FCQ? and incorporate that in the messages above.
     */
    if (serviceType == SERVICE_CLASS2)
	if (cqCmds == "") modemCQ = 0;
    else {	// SERVICE_CLASS20, SERVICE_CLASS21
	if (cqCmds == "" && modemCQ) modemCQ = 1;
    }
    /*
     * Deduce if modem supports T.class2-defined suport for
     * subaddress, selective polling address, and passwords.
     */
    int sub = 0;
    int sep = 0;
    int pwd = 0;
    // some modems don't support an AP-query command
    if (strcasecmp(conf.class2APQueryCmd, "none") != 0) {
	if (doQuery(conf.class2APQueryCmd, s))
	    (void) vparseRange(s, 0, 3, &sub, &sep, &pwd);
    }
    if (sub & BIT(1)) {
	saCmd = conf.class2SACmd;
	modemSupports("subaddressing");
    } else
	saCmd = "";
    if (sep & BIT(1)) {
	paCmd = conf.class2PACmd;
	modemSupports("selective polling");
    } else
	paCmd = "";
    if (pwd & BIT(1)) {
	pwCmd = conf.class2PWCmd;
	modemSupports("passwords");
    } else
	pwCmd = "";
    if ((sub|sep|pwd) & BIT(1))
	apCmd = conf.class2APCmd;
    /*
     * Check if the modem supports polled reception of documents.
     */
    u_int t;
    // some modems don't support an SPL command
    if (strcasecmp(splCmd, "none") != 0) {
	if (doQuery(splCmd | "=?", s) && FaxModem::parseRange(s, t))
	    hasPolling = (t & BIT(1)) != 0;
    }
    /*
     * Define the code to send to the modem to trigger the
     * transfer of received Phase C data to the host.  Most
     * modems based on SP-2388-A (Class 2) use DC1 while those
     * based on SP-2388-B (Class 2.0) use DC2.  There are some
     * exceptions (ZyXEL and modems based on Rockwell RC32ACL
     * parts), but they are expected to set the appropriate
     * value in the config file.
     */
    if (conf.class2RecvDataTrigger == "")
	recvDataTrigger = (serviceType == SERVICE_CLASS2 ? DC1 : DC2);
    else
	recvDataTrigger = conf.class2RecvDataTrigger[0];
    /*
     * SP-2388-A (Class 2) specifies that the modem should send
     * XON to the host when it is ready to received page data
     * during a transmission.  This went away in SP-2388-B and in
     * the final Class 2.0 spec.  Consequently we configure the
     * modem either to ignore it (Class 2.0/2.1) or to use whatever
     * is configured (it defaults to true).
     */
    if ((serviceType == SERVICE_CLASS20) || (serviceType == SERVICE_CLASS21))
	xmitWaitForXON = false;
    else
	xmitWaitForXON = conf.class2XmitWaitForXON;

    /*
     * Most Class2 modems require recvFillOrder == MSB2LSB
     * (Rockwell bug, which has become "standard" now).
     *
     * The only known exception is early Multitech modems,
     * that follow original Class2 specs (LSB2MSB). They  report the
     * manufacturer as "Multi-Tech Sys" or "Multi-Tech Systems".
     *
     * Other Class2 modems that requires recvFillOrder == LSB2MSB
     * (if any) are expected to set the appropriate value in the
     * config file.
     */
    if( conf.recvFillOrder == 0 &&  serviceType == SERVICE_CLASS2 ){
        if ( modemMfr.find(0, "MULTI-TECH") >= modemMfr.length()  ){
            // Not a Multitech
            recvFillOrder = FILLORDER_MSB2LSB;
        }
    }

    setupClass2Parameters();			// send parameters to the modem
    return (true);
}

void
Class2Modem::pokeConfig(bool isSend)
{
}

/*
 * Switch the modem to Class 2/2.0 and  insure parameters
 * are set as we want them since some modems reset state
 * on switching into Class 2/2.0.  Note that receive-related
 * parameters are only set on modem reset because some
 * commands, like setupAACmd, can reset the modem to Class
 * 0.  This interface is used only for outbound use or when
 * followed by setup of receive-specific parameters.
 */
bool
Class2Modem::setupClass2Parameters(bool enableV34, bool enableV17)
{
    if (modemServices & serviceType) {		// when resetting at startup
	setupFlowControl(flowControl);		// flow control
	// some modems don't support a TBC command
	if (strcasecmp(tbcCmd, "none") != 0) {
	    atCmd(tbcCmd);			// stream mode
	}
	atCmd(borCmd);				// Phase B+C bit order
	/*
	 * Set Phase C data transfer timeout parameter.
	 * Note that we also have our own timeout parameter
	 * that should be at least as large (though it
	 * probably doesn't matter).  Some modem manufacturers
	 * do not support this command (or perhaps they
	 * hide it under a different name).
	 */
	if (strcasecmp(phctoCmd, "none") != 0) {
	    atCmd(phctoCmd);
	}

	(void) atCmd(cqCmds);			// copy quality checking
	(void) atCmd(nrCmd);			// negotiation reporting
	(void) atCmd(apCmd);			// address&polling reporting
	(void) atCmd(pieCmd);			// program interrupt enable
						// HDLC frame tracing
	// some modems don't support a BUG command
	if (getHDLCTracing() && strcasecmp(bugCmd, "none") != 0)
	    atCmd(bugCmd);
	/*
	 * Force the DCC so that we can override
	 * whatever the modem defaults are.
	 */
	setupDCC(enableV34, enableV17);
    }
    return (true);
}

/*
 * Setup receive-specific parameters.
 */
bool
Class2Modem::setupReceive()
{
    /*
     * Try to setup byte-alignment of received EOL's.
     * As always, this is problematic.  If the modem
     * does not support this, but accepts the command
     * (as many do!), then received facsimile will be
     * incorrectly tagged as having byte-aligned EOL
     * codes in them--not usually much of a problem.
     */
    if (conf.class2RELCmd != "" && atCmd(conf.class2RELCmd))
	group3opts |= GROUP3OPT_FILLBITS;
    else
	group3opts &= ~GROUP3OPT_FILLBITS;
    atCmd(crCmd);				// enable receiving
    /*
     * Enable adaptive-answer support.  If we're configured,
     * we'll act like getty and initiate a login session if
     * we get a data connection.  Note that we do this last
     * so that the modem can be left in a state other than
     * +FCLASS=2 (e.g. Rockwell-based modems often need to be
     * in Class 0).
     */
    return atCmd(conf.setupAACmd);
}

/*
 * Send the modem any commands needed to force use of
 * the specified flow control scheme.
 */
bool
Class2Modem::setupFlowControl(FlowControl fc)
{
    switch (fc) {
    case FLOW_NONE:	return atCmd(noFlowCmd);
    case FLOW_XONXOFF:	return atCmd(softFlowCmd);
    case FLOW_RTSCTS:	return atCmd(hardFlowCmd);
    }
    return (true);
}

/*
 * Setup DCC to reflect best capabilities of the server.
 */
bool
Class2Modem::setupDCC(bool enableV34, bool enableV17)
{
    params.vr = getVRes();
    params.br = enableV34 ? getBestSignallingRate() : fxmin((u_int) BR_14400, getBestSignallingRate());
    params.br = enableV17 ? params.br : fxmin((u_int) BR_9600, getBestSignallingRate());
    params.wd = getBestPageWidth();
    params.ln = getBestPageLength();
    params.df = useExtendedDF ? modemParams.df : getBestDataFormat();
    params.ec = getBestECM();
    params.bf = BF_DISABLE;
    params.st = getBestScanlineTime();
    params.jp = modemParams.jp;
    return class2Cmd(dccCmd, params, true);
}

/*
 * Parse a ``capabilities'' string from the modem and
 * return the values through the params parameter.
 */
bool
Class2Modem::parseClass2Capabilities(const char* cap, Class2Params& params, bool isDIS)
{
    /*
     * Some modems report capabilities in hex values, others decimal.
     */
    fxStr notation;
    if (conf.class2UseHex) {
	if (useJP) notation = "%X,%X,%X,%X,%X,%X,%X,%X,%X";
	else notation = "%X,%X,%X,%X,%X,%X,%X,%X";
    } else {
	if (useJP) notation = "%d,%d,%d,%d,%d,%d,%d,%d,%d";
	else notation = "%d,%d,%d,%d,%d,%d,%d,%d";
    }
    int n = 0;
    if (useJP) {
	n = sscanf(cap, notation,
	    &params.vr, &params.br, &params.wd, &params.ln,
	    &params.df, &params.ec, &params.bf, &params.st, &params.jp);
    } else {
	n = sscanf(cap, notation,
	    &params.vr, &params.br, &params.wd, &params.ln,
	    &params.df, &params.ec, &params.bf, &params.st);
	params.jp = 0;
    }
    if ((useJP && n == 9) || (n == 8)) {
	if (useJP && (n == 8)) {
	    /*
	     * The modem was previously determined to support JP; however,
	     * this response does not include it.  So we just set it to zero.
	     */
	    params.jp = 0;
	}
	if (params.ec != EC_DISABLE && (conf.class2ECMType == ClassModem::ECMTYPE_CLASS20 ||
	   (conf.class2ECMType == ClassModem::ECMTYPE_UNSET && serviceType != SERVICE_CLASS2)))
	    params.ec += 1;		// simple adjustment, drops EC_ENABLE64
	/*
	 * Clamp values to insure modem doesn't feed us
	 * nonsense; should log bogus stuff also.
	 */
	params.vr = params.vr & VR_ALL;
	params.br = fxmin(params.br, (u_int) BR_33600);
	params.wd = fxmin(params.wd, (u_int) WD_A3);
	params.ln = fxmin(params.ln, (u_int) LN_INF);
	if (useExtendedDF) {
	    /*
	     * The T.32-A1 DF extension presents us with a bitmap-like presentation
	     * similar to VR here... but leaves 2D-MMR = 3 for backwards-compatibility.
	     *
	     * 0 = 1D-MH, 1 = 2D-MR, 3 = 2D-MMR, 4 = JBIG-L0, 8 = JBIG
	     */
	    u_int dfscan = params.df;
	    if (isDIS) {
		params.df = BIT(DF_1DMH);
		if (dfscan & 0x1) params.df |= BIT(DF_2DMR);
		if (dfscan & 0x2) params.df |= BIT(DF_2DMMR);	// don't require MR for MMR
		if (dfscan & 0x4) params.df |= BIT(DF_JBIG);	// JBIG L0 is JBIG to us
		if (dfscan & 0x8) params.df |= BIT(DF_JBIG);
	    } else {
		// Dex 855 is known to indicate MMR in addition to JBIG when sending JBIG (0x7).
		// Thus only interpret DF=MMR when 0x3.
		params.df = DF_1DMH;
		if (dfscan == 0x3) params.df = DF_2DMMR;
		else if (dfscan & 0x1) params.df = DF_2DMR;
		else if (dfscan & 0x4) params.df = DF_JBIG;	// JBIG L0 is JBIG to us
		else if (dfscan & 0x8) params.df = DF_JBIG;
	    }
	} else {
	    params.df = fxmin(params.df, (u_int) DF_2DMMR);
	    /*
	     * Table 21 T.32 does not match Table 2 T.30 very well in some aspects.
	     * Data format is one of those things.  When dealing with DIS we use DF 
	     * as a bitmap to suit T.30, but a T.32-following modem will only report 
	     * one supported receiver format (and not all of them).  Thus when 
	     * parsing T.32 DIS we must convert the modem response to a bitmap.  
	     * However, due to the incongruency between T.30 and T.32 the bitmap will
	     * only contain the the reported format and the required format.
	     */
	    if (isDIS) {
		params.df = BIT(params.df) | BIT(DF_1DMH);
	    }
	}
	if (params.ec > EC_ECLFULL)		// unknown, disable use
	    params.ec = EC_DISABLE;
	if (params.bf > BF_ENABLE)
	    params.bf = BF_DISABLE;
	params.st = fxmin(params.st, (u_int) ST_40MS);
	int jpscan = params.jp;
	params.jp = 0;
	if (isDIS) {
	    if (jpscan & 0x1) params.jp |= BIT(JP_GREY);
	    if (jpscan & 0x2) params.jp |= BIT(JP_COLOR);
	} else {
	    if (jpscan == 0x1) params.jp = JP_GREY;
	    else if (jpscan & 0x2) params.jp = JP_COLOR;
	}
	return (true);
    } else {
	protoTrace("MODEM protocol botch, can not parse \"%s\"", cap);
	return (false);
    }
}

/*
 * Place the modem into the appropriate state
 * for sending facsimile.
 */
bool
Class2Modem::faxService(bool enableV34, bool enableV17)
{
    if (!enableV17 && conf.class2DisableV17Cmd != "" && !atCmd(conf.class2DisableV17Cmd)) return (false);
    return setupClass2Parameters(enableV34, enableV17);
}


bool
Class2Modem::setupRevision(fxStr& revision)
{
    if (FaxModem::setupRevision(revision)) {
	/*
	 * Cleanup ZyXEL response (argh), modem gives:
	 * +FREV?	"U1496E   V 5.02 M    "
	 */
	if (modemMfr == "ZYXEL") {
	    u_int pos = modemRevision.next(0, ' ');
	    if (pos != modemRevision.length()) {	// rev. has model 1st
		pos = modemRevision.skip(pos, ' ');
		modemRevision.remove(0, pos);
	    }
	}
	return (true);
    } else
	return (false);
}

bool
Class2Modem::setupModel(fxStr& model)
{
    if (FaxModem::setupModel(model)) {
	/*
	 * Cleanup ZyXEL response (argh), modem gives:
	 * +FMDL?	"U1496E   V 5.02 M    "
	 */
	if (modemMfr == "ZYXEL")
	    modemModel.resize(modemModel.next(0, ' '));	// model is first word
	return (true);
    } else
	return (false);
}

bool
Class2Modem::supportsPolling() const
{
    return (hasPolling);
}

/*
 * Strip any quote marks from a string.  This
 * is used for received TSI+CSI strings passed
 * to the server.
 */
fxStr
Class2Modem::stripQuotes(const char* cp)
{
    fxStr s(cp);
    u_int pos = s.next(0, '"');
    while (pos != s.length()) {
	s.remove(pos,1);
	pos = s.next(0,'"');
    }
    return (s);
}

/*
 * Construct the Calling Station Identifier (CSI) string
 * for the modem.  We permit any ASCII printable characters
 * in the string though the spec says otherwise.  The max
 * length is 20 characters (per the spec), and we pad the
 * string to that length to avoid buggy modems from putting
 * garbage in the void.
 */
void
Class2Modem::setLID(const fxStr& number)
{
    lid.resize(0);
    for (u_int i = 0, n = number.length(); i < 20; i++) {
	if (i < n) {
	    char c = number[i];
	    if (isprint(c) || c == ' ')
		lid.append(c);
	} else
	    lid.append(' ');
    }
    class2Cmd(lidCmd, lid);	// for DynamicConfig
}

/* 
 * Modem manipulation support.
 */

/*
 * Reset a Class 2 modem.
 */
bool
Class2Modem::reset(long ms)
{
    return (FaxModem::reset(ms) && setupClass2Parameters());
}

/*
 * Wait (carefully) for some response from the modem.
 * In particular, beware of unsolicited hangup messages
 * from the modem.  Some modems seem to not use the
 * Class 2 required +FHNG response--and instead give
 * us an unsolicited NO CARRIER message.  Isn't life
 * wondeful?
 */
bool
Class2Modem::waitFor(ATResponse wanted, long ms)
{
    for (;;) {
	ATResponse response = atResponse(rbuf, ms);
	if (response == wanted)
	    return (true);
	switch (response) {
	case AT_TIMEOUT:
	case AT_EMPTYLINE:
	case AT_ERROR:
	case AT_NOCARRIER:
	case AT_NODIALTONE:
	case AT_NOANSWER:
	case AT_RING:
	    modemTrace("MODEM %s", ATresponses[response]);
	case AT_OK:
	    return (false);
	case AT_FHNG:
	    // return hangup status, but try to wait for requested response
	    { char buf[1024]; (void) atResponse(buf, 2*1000); }
	    return (isNormalHangup());
	}
    }
}

/*
 * Interfaces for sending a Class 2 commands.
 */

/*
 * Send <cmd>=<a0> and wait for response.
 */
bool
Class2Modem::class2Cmd(const fxStr& cmd, int a0, ATResponse r, long ms)
{
    return atCmd(cmd | fxStr(a0, "=%u"), r, ms);
}

/*
 * Send <cmd>=<t.30 parameters> and wait response.
 */
bool
Class2Modem::class2Cmd(const fxStr& cmd, const Class2Params& p, bool isDCC, ATResponse r, long ms)
{
    bool ecm20 = false;
    if (conf.class2ECMType == ClassModem::ECMTYPE_CLASS20 ||
       (conf.class2ECMType == ClassModem::ECMTYPE_UNSET && serviceType != SERVICE_CLASS2))
	ecm20 = true;
    return atCmd(cmd | "=" | p.cmd(conf.class2UseHex, ecm20, (isDCC && useExtendedDF), useJP), r, ms);
}

/*
 * Send <cmd>="<s>" and wait for response.
 */
bool
Class2Modem::class2Cmd(const fxStr& cmd, const fxStr& s, ATResponse r, long ms)
{
    return atCmd(cmd | "=\"" | s | "\"", r, ms);	// XXX handle escapes
}

/*
 * Parse a Class 2 parameter specification
 * and return the resulting bit masks.
 */
bool
Class2Modem::parseRange(const char* cp, Class2Params& p)
{
    /*
     * VR, BF, and JP are already reported as bitmap
     * values accoring to T.32 Table 21.
     * In vparseRange(), VR:nargs=7, BF:nargs=1.
     */
    int masked = (1 << 7) + (1 << 1);	// reversed, count-down style
    if (!vparseRange(cp, masked, 8, &p.vr,&p.br,&p.wd,&p.ln,&p.df,&p.ec,&p.bf,&p.st))
	return (false);
    p.vr &= VR_ALL;
    p.br &= BR_ALL;
    p.wd &= WD_ALL;
    p.ln &= LN_ALL;
    if ((p.df & 0x10) && (p.df & 0x100)) {	// supports JBIG via T.32-A1 extension
	/*
	 * Old T.32 Table 21 does not provide for JBIG data formats.
	 * In amendment 1 the ITU has extended the DF parameter to include JBIG by 
	 * assigning "4" to JBIG L0 and "8" to JBIG.  The +FCC response for DF may  
	 * look like (00-0F) or possibly even (00-01,03-05,07-09,0B-0D,0F).  However, 
	 * in so doing the ITU has also changed the DF parameter (in +FIS and +FCS 
	 * responses) into a bitmap.  Yet, an +FCC response for DF of "(5DDD)" or "(0F)" 
	 * is not backwards-compatible with older T.32.  So the +FCC response is in 
	 * this backwards-compatible presentation (00-0F), and +FCS and +FIS responses 
	 * are not.
	 */
	useExtendedDF = true;
	p.df &= DF_ALL;
	p.df |= BIT(DF_JBIG);
    } else
	p.df &= DF_ALL;
    p.ec &= EC_ALL;
    p.bf &= BF_ALL;
    p.st &= ST_ALL;

    /*
     * As JP is optional we do a second, non-fatal parse for it.
     */
    int n;
    masked = 1;
    if (vparseRange(cp, masked, 9, &n,&n,&n,&n,&n,&n,&n,&n,&p.jp)) {
	useJP = true;
	if (conf.class2JPEGSupport) p.jp &= JP_ALL;
	else p.jp = 0;
    } else
	p.jp = 0;

    return true;
}

const char*
Class2Modem::skipStatus(const char* s)
{
    const char* cp;
    for (cp = s; *cp != '\0' && *cp != ':'; cp++)
	;
    return (*cp == ':' ? cp+1 : s);
}

/*
 * Hangup codes are broken up according to:
 *   2388/89
 *   2388/90 and 2388-A
 *   2388-B (Class 2.0)
 * The table to search is based on the modem type deduced
 * at modem configuration time.
 *
 * Note that we keep the codes as strings to avoid having
 * to distinguish the decimal numbers of 2388-A from the
 * hexadecimal renumbering done in 2388-B!
 */
static struct HangupCode {
    const char*	code[3];	// from 2388/89, 2388/90, 2388-A, 2388-B
    int status;		// HylaFAX code
    const char*	message;	// what code means
} hangupCodes[] = {
// Call placement and termination
    {{  "0",  "0",  "0"}, 211, "Normal and proper end of connection" },
    {{  "1",  "1",  "1"}, 212, "Ring detect without successful handshake" },
    {{  "2",  "2",  "2"}, 213, "Call aborted,  from +FK or <CAN>" },
    {{ NULL,  "3",  "3"}, 214, "No loop current" },
    {{ NULL, NULL,  "4"}, 215, "Ringback detected, no answer (timeout)" },
    {{ NULL, NULL,  "5"}, 216, "Ringback detected, no answer without CED" },
// Transmit Phase A & miscellaneous errors
    {{ "10", "10", "10"}, 217, "Unspecified Phase A error" },
    {{ "11", "11", "11"}, 218, "No answer (T.30 T1 timeout)" },
// Transmit Phase B
    {{ "20", "20", "20"}, 219, "Unspecified Transmit Phase B error" },
    {{ "21", "21", "21"}, 220, "Remote cannot be polled" },
    {{ "22", "22", "22"}, 221, "COMREC error in transmit Phase B/got DCN" },
    {{ "23", "23", "23"}, 222, "COMREC invalid command received/no DIS or DTC" },
    {{ "24", "24", "24"}, 223, "RSPREC error/got DCN" },
    {{ "25", "25", "25"}, 224, "DCS sent 3 times without response" },
    {{ "26", "26", "26"}, 225, "DIS/DTC received 3 times; DCS not recognized" },
    {{ "27", "27", "27"}, 226, "Failure to train at 2400 bps or +FMINSP value" },
    {{ "28", "28", "28"}, 227, "RSPREC invalid response received" },
// Transmit Phase C
    {{ "30", "40", "40"}, 228, "Unspecified Transmit Phase C error" },
    {{ NULL, NULL, "41"}, 229, "Unspecified Image format error" },
    {{ NULL, NULL, "42"}, 230, "Image conversion error" },
    {{ "33", "43", "43"}, 231, "DTE to DCE data underflow" },
    {{ NULL, NULL, "44"}, 232, "Unrecognized Transparent data command" },
    {{ NULL, NULL, "45"}, 233, "Image error, line length wrong" },
    {{ NULL, NULL, "46"}, 234, "Image error, page length wrong" },
    {{ NULL, NULL, "47"}, 235, "Image error, wrong compression code" },
// Transmit Phase D
    {{ "40", "50", "50"}, 236, "Unspecified Transmit Phase D error, including"
				   " +FPHCTO timeout between data and +FET command" },
    {{ "41", "51", "51"}, 237, "RSPREC error/got DCN" },
    {{ "42", "52", "52"}, 238, "No response to MPS repeated 3 times" },
    {{ "43", "53", "53"}, 239, "Invalid response to MPS" },
    {{ "44", "54", "54"}, 240, "No response to EOP repeated 3 times" },
    {{ "45", "55", "55"}, 241, "Invalid response to EOP" },
    {{ "46", "56", "56"}, 242, "No response to EOM repeated 3 times" },
    {{ "47", "57", "57"}, 243, "Invalid response to EOM" },
    {{ "48", "58", "58"}, 244, "Unable to continue after PIN or PIP" },
// Received Phase B
    {{ "50", "70", "70"}, 245, "Unspecified Receive Phase B error" },
    {{ "51", "71", "71"}, 246, "RSPREC error/got DCN" },
    {{ "52", "72", "72"}, 247, "COMREC error" },
    {{ "53", "73", "73"}, 248, "T.30 T2 timeout, expected page not received" },
    {{ "54", "74", "74"}, 249, "T.30 T1 timeout after EOM received" },
// Receive Phase C
    {{ "60", "90", "90"}, 250, "Unspecified Phase C error, including too much delay"
				   " between TCF and +FDR command" },
    {{ "61", "91", "91"}, 251, "Missing EOL after 5 seconds (section 3.2/T.4)" },
    {{ "63", "93", "93"}, 252, "DCE to DTE buffer overflow" },
    {{ "64", "94", "92"}, 253, "Bad CRC or frame (ECM or BFT modes)" },
// Receive Phase D
    {{ "70","100", "A0"}, 254, "Unspecified Phase D error" },
    {{ "71","101", "A1"}, 255, "RSPREC invalid response received" },
    {{ "72","102", "A2"}, 256, "COMREC invalid response received" },
    {{ "73","103", "A3"}, 257, "Unable to continue after PIN or PIP, no PRI-Q" },
// Agere proprietary error codes
    {{ NULL, NULL, "E0"}, 258, "Command or signal 10 sec. timeout" },
// Everex proprietary error codes (9/28/90)
    {{ NULL,"128", NULL}, 259, "Cannot send: +FMINSP > remote's +FDIS(BR) code" },
    {{ NULL,"129", NULL}, 260, "Cannot send: remote is V.29 only,"
				   " local DCE constrained to 2400 or 4800 bps" },
    {{ NULL,"130", NULL}, 261, "Remote station cannot receive (DIS bit 10)" },
    {{ NULL,"131", NULL}, 262, "+FK aborted or <CAN> aborted" },
    {{ NULL,"132", NULL}, 263, "+Format conversion error in +FDT=DF,VR, WD,LN"
				   " Incompatible and inconvertable data format" },
    {{ NULL,"133", NULL}, 264, "Remote cannot receive" },
    {{ NULL,"134", NULL}, 265, "After +FDR, DCE waited more than 30 seconds for"
				   " XON from DTE after XOFF from DTE" },
    {{ NULL,"135", NULL}, 266, "In Polling Phase B, remote cannot be polled" },
};
#define	NCODES	(sizeof (hangupCodes) / sizeof (hangupCodes[0]))

/*
 * Given a hangup code from a modem return a
 * descriptive string.  We use strings here to
 * avoid having to know what type of modem we're
 * talking to (Rev A or Rev B or SP-2388).  This
 * works right now becase the codes don't overlap
 * (as strings).  Hopefully this'll continue to
 * be true.
 */
const char*
Class2Modem::hangupCause(const char* code)
{
    for (u_int i = 0; i < NCODES; i++) {
	const HangupCode& c = hangupCodes[i];
	if ((c.code[1] != NULL && strcasecmp(code, c.code[1]) == 0) ||
	    (c.code[2] != NULL && strcasecmp(code, c.code[2]) == 0))
	    return (c.message);
    }
    return ("Unknown hangup code");
}

const Status& Class2Modem::hangupStatus (const char* code)
{
    static Status s;
    s.clear();
    for (u_int i = 0; i < NCODES; i++) {
	const HangupCode& c = hangupCodes[i];
	if ((c.code[1] != NULL && strcasecmp(code, c.code[1]) == 0) ||
	    (c.code[2] != NULL && strcasecmp(code, c.code[2]) == 0)) {
	    s = Status(c.status, c.message);
	    return s;
	}
    }
    s = Status(210, "Unknown hangup code");
    return s;
}

/*
 * Process a hangup code string.
 */
void
Class2Modem::processHangup(const char* cp)
{
    while (isspace(*cp))			// strip leading white space
	cp++;
    while (*cp == '0' && cp[1] != '\0')		// strip leading 0's
	cp++;
    strncpy(hangupCode, cp, sizeof (hangupCode));
    protoTrace("REMOTE HANGUP: %s (code %s)",
	hangupCause(hangupCode), hangupCode);
}

/*
 * Does the current hangup code string indicate
 * that the remote side hung up the phone in a
 * "normal and proper way".
 */
bool
Class2Modem::isNormalHangup()
{
    // normal hangup is "", "0", or "00"
    return (hangupCode[0] == '\0' ||
	(hangupCode[0] == '0' &&
	 (hangupCode[1] == '0' || hangupCode[1] == '\0')));
}

#include "t.30.h"

void
Class2Modem::tracePPM(const char* dir, u_int ppm)
{
    static u_int ppm2fcf[8] = {
	FCF_MPS,
	FCF_EOM,
	FCF_EOP,
	0,
	FCF_PRI_MPS,
	FCF_PRI_EOM,
	FCF_PRI_EOP,
	0
    };
    if (ppm == PPH_SKIP)
        protoTrace("%s %s", dir, "NOTHING (skipped page, simulating MCF)");
    else
	FaxModem::traceFCF(dir, ppm2fcf[ppm&7]);
}

void
Class2Modem::tracePPR(const char* dir, u_int ppr)
{
    static u_int ppr2fcf[8] = {
	0,
	FCF_MCF,
	FCF_RTN,
	FCF_RTP,
	FCF_PIN,
	FCF_PIP,
	0,
	0
    };
    FaxModem::traceFCF(dir, ppr2fcf[ppr&7]);
}
