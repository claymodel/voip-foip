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
#ifndef _ModemConfig_
#define	_ModemConfig_
/*
 * Modem Configuration.
 */
#include "FaxConfig.h"
#include "FaxModem.h"

#include "Array.h"
#include "StrArray.h"

class id_config
{
    public:
	fxStr	pattern;
	int	answerlength;

	int compare (const id_config* n) const
	{
	    return ::compare(pattern, n->pattern);
	}

};

fxDECLARE_ObjArray(IDConfArray, id_config);

struct ModemConfig : public FaxConfig {
private:
    BaudRate	getRate(const char*);
    u_int	getFill(const char*);
    FlowControl	getFlow(const char*);
    void	setVolumeCmds(const fxStr& value);
    u_int	getSpeed(const char* value);
    u_int	getDataFormat(const char* value);
    u_int       getRTNHandling(const char* cp);
    u_int       getBadPageHandling(const char* cp);
    u_int       getJBIGSupport(const char* cp);
    ECMType	getECMType(const char* cp);

    static bool findRate(const char*, BaudRate&);
    static bool findATResponse(const char*, ATResponse&);
    static bool findFlow(const char*, FlowControl&);
    static bool findDataFormat(const char*, u_int&);
    static bool findRTNHandling(const char*, RTNHandling&);
protected:
    ModemConfig();

    void setupConfig();
    virtual void resetConfig();

    virtual bool setConfigItem(const char* tag, const char* value);
    virtual void configError(const char* fmt, ...) = 0;
    virtual void configTrace(const char* fmt, ...) = 0;
    fxStr parseATCmd(const char*);

    u_int	callidIndex;		// call id index
public:
    fxStr	type;			// modem type
    fxStr	resetCmds;		// extra modem reset commands for start of initialization
    fxStr	readyCmds;		// extra modem reset commands for end of initialization
    fxStr	dialCmd;		// cmd for dialing (%s for number)
    fxStr	answerAnyCmd;		// cmd for answering unknown call type
    fxStr	answerDataCmd;		// cmd for answering data call
    fxStr	answerFaxCmd;		// cmd for answering fax call
    fxStr	answerVoiceCmd;		// cmd for answering voice call
    fxStr	answerDialCmd;		// cmd for answering a dialed call
    fxStr	ringResponse;		// cmd to respond after AT_RING
    fxStr	hardFlowCmd;		// cmd for hardware flow control
    fxStr	softFlowCmd;		// cmd for software flow control
    fxStr	noFlowCmd;		// cmd for disabling flow control
    fxStr	setupDTRCmd;		// cmd for setting up DTR handling
    fxStr	setupDCDCmd;		// cmd for setting up DCD handling
    fxStr	setupAACmd;		// cmd for setting up adaptive answer
    fxStr	noAutoAnswerCmd;	// cmd for disabling auto-answer
    fxStr	setVolumeCmd[5];	// cmd for setting modem speaker volume
    fxStr	echoOffCmd;		// cmd for disabling echo
    fxStr	verboseResultsCmd;	// cmd for enabling verbose result codes
    fxStr	resultCodesCmd;		// cmd for enabling result codes
    fxStr	onHookCmd;		// cmd for placing phone ``on hook''
    fxStr	softResetCmd;		// cmd for doing soft reset
    u_int	softResetCmdDelay;	// time in ms to pause after soft reset
    u_int	noAutoAnswerCmdDelay;	// time in ms to pause after noAutoAnswerCmd
    u_int	ringsBeforeResponse;	// number of rings to wait before ModemRingResponse
    u_int	ringTimeout;		// timeout in ms after RING to reset
    fxStr	waitTimeCmd;		// cmd for setting carrier wait time
    fxStr	pauseTimeCmd;		// cmd for setting "," pause time
    fxStr	mfrQueryCmd;		// cmd for getting modem manufacturer
    fxStr	modelQueryCmd;		// cmd for getting modem model id
    fxStr	revQueryCmd;		// cmd for getting modem firmware rev
    fxStr	answerAnyBeginCmd;	// cmd to start unknown inbound session
    fxStr	answerDataBeginCmd;	// cmd to start inbound data session
    fxStr	answerFaxBeginCmd;	// cmd to start inbound fax session
    fxStr	answerVoiceBeginCmd;	// cmd to start inbound voice session
    fxStr	sendBeginCmd;		// cmd to start outbound session
    fxStr	recvSuccessCmd;		// cmd for after successful reception
    fxStr	class0Cmd;		// cmd for setting Class 0
    fxStr	classQueryCmd;		// cmd for getting modem services
					// distinctive ring
    fxStr	ringData;		// data call ring string
    fxStr	ringFax;		// fax call ring string
    fxStr	ringVoice;		// voice call ring string
    fxStr	ringExtended;		// extended ring
    fxStr	dringOn;		// pattern for distinctive ring silence interval
    fxStr	dringOff;		// pattern for distinctive ring ring interval
    bool	noAnswerVoice;		// leave voice calls unanswered
    bool	saverawimage;		// debugging feature to save raw image data
					// call identification
    IDConfArray	idConfig;		// pattern for ID info
					// protocol timers
    u_int	t1Timer;		// T.30 T1 timer (ms)
    u_int	t2Timer;		// T.30 T2 timer (ms)
    u_int	t4Timer;		// T.30 T4 timer (ms)
    u_int	dialResponseTimeout;	// dialing command timeout (ms)
    u_int	answerResponseTimeout;	// answer command timeout (ms)
    u_int	pageStartTimeout;	// page send/receive timeout (ms)
    u_int	pageDoneTimeout;	// page send/receive timeout (ms)
					// for class 1:
    fxStr	class1Cmd;		// cmd for setting Class 1
    fxStr	class1AdaptRecvCmd;	// cmd to enable adaptive reception control
    fxStr	class1EnableV34Cmd;	// cmd to enable V.34 support in Class 1.0
    fxStr	class1NFLOCmd;		// cmd to setup no flow control
    fxStr	class1SFLOCmd;		// cmd to setup software flow control
    fxStr	class1HFLOCmd;		// cmd to setup hardware flow control
    fxStr	class1PPMWaitCmd;	// cmd to stop and wait prior to PPM
    fxStr	class1ResponseWaitCmd;	// cmd to wait prior to TCF response
    fxStr	class1RMQueryCmd;	// cmd to query modem for receive demodulators
    fxStr	class1TCFWaitCmd;	// cmd to stop and wait prior to TCF
    fxStr	class1TMQueryCmd;	// cmd to query modem for transmit modulators
    fxStr	class1EOPWaitCmd;	// cmd to stop and wait prior to EOP
    fxStr	class1SwitchingCmd;	// after recv HDLC and before sending
    fxStr	class1MsgRecvHackCmd;	// cmd to avoid +FCERROR before image
    fxStr	class1TCFRecvHackCmd;	// cmd to avoid +FCERROR before TCF
    u_int	class1TCFRecvTimeout;	// timeout receiving TCF
    u_int	class1RecvAbortOK;	// if non-zero, OK sent after recv abort
    u_int	class1RMPersistence;	// how many times to persist through +FCERROR
    u_int	class1HookSensitivity;	// how many times to persist through on-hook detections
    u_int	class1Resolutions;	// resolutions support
    u_int	class1FrameOverhead;	// overhead bytes in received frames
    u_int	class1RecvIdentTimer;	// timeout receiving initial identity
    u_int	class1TCFMaxNonZero;	// non-zero threshold for TCF check
    u_int	class1TCFMinRun;	// min length of zero run for TCF check
    u_int	class1TCFMinRunECMMod;	// modify min length in ECM by this factor
    u_int	class1TMConnectDelay;	// delay (ms) after +FTM CONNECT
    u_int	class1ECMFrameSize;	// ECM frame size for transmission
    u_int	class1PageLengthSupport;// page length support
    u_int	class1PageWidthSupport;	// page width support
    bool	class1GreyJPEGSupport;	// Greyscale JPEG support
    bool	class1ColorJPEGSupport;	// Full-color JPEG support
    bool	class1ECMSupport;	// support T.30-A ECM
    bool	class1MRSupport;	// support 2-D MR
    bool	class1MMRSupport;	// support 2-D MMR
    bool	class1PersistentECM;	// continue to correct
    bool	class1ValidateV21Frames;// check received FCS values in V.21
    bool	class1ModemHasDLEBug;	// modem doesn't double-up DLEs in V.21
    bool	class1HasRHConnectBug;	// modem reports CONNECT after +FRH=3 to non-V.21-HDLC data
					// for class 2 and 2.0:
    fxStr	class2Cmd;		// cmd for setting Class 2/2.0
    fxStr	class2DCCQueryCmd;	// cmd to query modem capabilities
    fxStr	class2CQQueryCmd;	// cmd to query copy quality checking
    fxStr	class2BORCmd;		// cmd to set bit order
    fxStr	class2DisableV17Cmd;	// cmd to disable V.17 support
    fxStr	class2RELCmd;		// cmd to enable byte-aligned EOL
    fxStr	class2CQCmd;		// cmd to setup copy quality checking
    fxStr	class2AbortCmd;		// cmd to abort a session
    fxStr	class2TBCCmd;		// cmd to enable stream mode
    fxStr	class2CRCmd;		// cmd to enable receive capability
    fxStr	class2PHCTOCmd;		// cmd to set Phase C timeout parameter
    fxStr	class2BUGCmd;		// cmd to enable HDLC frame tracing
    fxStr	class2LIDCmd;		// cmd to set local identifier string
    fxStr	class2DCCCmd;		// cmd to set modem capabilities
    fxStr	class2DISCmd;		// cmd to set session parameters
    fxStr	class2DDISCmd;		// cmd to set ses. params. before dial
    fxStr	class2CIGCmd;		// cmd to set polling identifier
    fxStr	class2SPLCmd;		// cmd to set polling request
    fxStr	class2PTSCmd;		// cmd to set page status
    fxStr	class2PTSQueryCmd;	// cmd to query page status
    fxStr	class2NFLOCmd;		// cmd to setup no flow control
    fxStr	class2SFLOCmd;		// cmd to setup software flow control
    fxStr	class2HFLOCmd;		// cmd to setup hardware flow control
    fxStr	class2MINSPCmd;		// cmd to setup min transmit speed
    fxStr	class2RecvDataTrigger;	// send to start recv
    bool	class2JPEGSupport;	// use JPEG support as reported by the modem
    bool	class2XmitWaitForXON;	// wait for XON before send
    bool	class2RTFCC;		// real-time fax compression conversion
    bool	class2SendRTC;		// append RTC to page data on transmit
					// for class 2.0:
    fxStr	class2PIECmd;		// cmd to set proc interrupt handling
    fxStr	class2NRCmd;		// cmd to set status reporting
					// for class T.class2:
    fxStr	class2APQueryCmd;	// cmd to query address&polling caps.
    fxStr	class2APCmd;		// cmd to setup address&polling caps.
    ECMType	class2ECMType;		// ECM specification type to use
    fxStr	class2SACmd;		// cmd to set subaddress
    fxStr	class2PACmd;		// cmd to set selective polling address
    fxStr	class2PWCmd;		// cmd to set password for transmit/poll
    bool	class2UseHex;		// parse capabilities strings as hex
    bool	class2HexNSF;		// parse nsf strings as hex
    bool	class2UseLineCount;	// use the firmware decoder's line count

    FlowControl	flowControl;		// DTE-DCE flow control method
    BaudRate	maxRate;		// max DTE-DCE rate to try
    u_int	recvFillOrder;		// bit order of recvd data
    u_int	sendFillOrder;		// bit order of sent data
    u_int	frameFillOrder;		// bit order of HDLC frames
    u_int	resetDelay;		// delay (ms) after reseting modem
    u_int	dtrDropDelay;		// delay (ms) after dropping DTR
    u_int	baudRateDelay;		// delay (ms) after setting baud rate
    u_int	atCmdDelay;		// delay (ms) between each AT cmd
    u_int	percentGoodLines;	// required % of good lines in page
    u_int	maxConsecutiveBadLines;	// max consecutive bad lines in page
    u_int	minAcceptedLineCount;	// min accepted number of lines in page
    u_int	minSpeed;		// minimum speed for fax transmits
    bool	softRTFCC;		// real-time fax compression conversion (software)
    bool	waitForConnect;		// modem sends multiple answer responses
    fxStr	tagLineFmt;		// format string for tag lines
    fxStr	tagLineFontFile;	// font file for imaging tag lines
    fxStr	tagLineCoverNumString;	// format string for tagline cover page numbers
    u_int	recvDataFormat;		// received facsimile data format
    bool	useJobTagLine;		// Use Job tagline or use conf taglineformat
    bool	raiseATCmd;		// whether or not to raise-case the AT commands
    bool	doPhaseCDebug;		// Query modem during Phase C for debugging info.

    RTNHandling rtnHandling;            // RTN signal handling method
    BadPageHandling badPageHandling;	// bad page (received) handling method
    JBIGSupport	class1JBIGSupport;	// monochrome JBIG support
    bool	saveUnconfirmedPages;	// don't delete unconfirmed pages
    bool	countSkippedPages;	// count (or nor) and notify skipped pages
    
    fxStrArray	playList;		// List of files for ESC_PLAY

    					// Distinctive ring data as sequences of DRON/DROF intervals
    struct {
    	int cadence[5];                 // the ring cadence as a five-dimensional vector
	double magsqrd;               	// magnitude of the vector squared
	u_int type;                     // call type of voice, fax, or data
    } distinctiveRings[5];              // up to 5 distinctive ring numbers
    u_int NoDRings;                     // number of distinctive rings numbers found

        virtual ~ModemConfig();

    void parseCallID(const char*, CallID&) const;
    const fxStr& getFlowCmd(FlowControl) const;
    void parseDR(const char*);
    void processDRString(char*, const u_int);
};
#endif /* _ModemConfig_ */
