#ifndef __CHAN_SCCP_H
#define __CHAN_SCCP_H

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <asterisk/frame.h>
#include <asterisk/module.h>
#include <asterisk/channel.h>
#include <asterisk/lock.h>
#include <asterisk/logger.h>
#include <asterisk/sched.h>
#include <asterisk/io.h>
#include <asterisk/rtp.h>
#include <asterisk/config.h>
#include <asterisk/options.h>
#include <asterisk/channel_pvt.h>

#define DEFAULT_SCCP_PORT             2000 /* SCCP uses port 2000. */
#define DEFAULT_SCCP_BACKLOG             2 /* the listen baklog. */
#define SCCP_MAX_AUTOLOGIN             100 /* Maximum allowed of autologins per device */
#define SCCP_KEEPALIVE                   5 /* Default keepalive time if not specified in sccp.conf. */

#define StationMaxDeviceNameSize        16
#define StationMaxButtonTemplateSize    42
#define StationDateTemplateSize          6
#define StationMaxDisplayTextSize       33
#define StationMaxDirnumSize            24
#define StationMaxNameSize              40
#define StationMaxSoftKeyDefinition     32
#define StationMaxSoftKeySetDefinition  16
#define StationMaxSoftKeyIndex          16
#define StationMaxSoftKeyLabelSize      16
#define StationMaxVersionSize           16

typedef enum {

  /* Client -> Server */

  KeepAliveMessage                = 0x0000,
  RegisterMessage                 = 0x0001,
  IpPortMessage                   = 0x0002,
  KeypadButtonMessage             = 0x0003,
  EnblocCallMessage               = 0x0004,
  StimulusMessage                 = 0x0005,
  OffHookMessage                  = 0x0006,
  OnHookMessage                   = 0x0007,
  HookFlashMessage                = 0x0008,
  ForwardStatReqMessage           = 0x0009,
  SpeedDialStatReqMessage         = 0x000A,
  LineStatReqMessage              = 0x000B,
  ConfigStatReqMessage            = 0x000C,
  TimeDateReqMessage              = 0x000D,
  ButtonTemplateReqMessage        = 0x000E,
  VersionReqMessage               = 0x000F,
  CapabilitiesResMessage          = 0x0010,
  MediaPortListMessage            = 0x0011,
  ServerReqMessage                = 0x0012,
  AlarmMessage                    = 0x0020,
  MulticastMediaReceptionAck      = 0x0021,
  OpenReceiveChannelAck           = 0x0022,
  ConnectionStatisticsRes         = 0x0023,
  OffHookWithCgpnMessage          = 0x0024,
  SoftKeySetReqMessage            = 0x0025,
  SoftKeyEventMessage             = 0x0026,
  UnregisterMessage               = 0x0027,
  SoftKeyTemplateReqMessage       = 0x0028,
  RegisterTokenReq                = 0x0029,
  HeadsetStatusMessage            = 0x002B,
  unknownClientMessage2           = 0x002D,

  /* Server -> Client */

  RegisterAckMessage              = 0x0081,
  StartToneMessage                = 0x0082,
  StopToneMessage                 = 0x0083,
  // ??
  SetRingerMessage                = 0x0085,
  SetLampMessage                  = 0x0086,
  SetHkFDetectMessage             = 0x0087,
  SetSpeakerModeMessage           = 0x0088,
  SetMicroModeMessage             = 0x0089,
  StartMediaTransmission          = 0x008A,
  StopMediaTransmission           = 0x008B,
  StartMediaReception             = 0x008C,
  StopMediaReception              = 0x008D,
  // ?
  CallInfoMessage                 = 0x008F,

  ForwardStatMessage              = 0x0090,
  SpeedDialStatMessage            = 0x0091,
  LineStatMessage                 = 0x0092,
  ConfigStatMessage               = 0x0093,
  DefineTimeDate                  = 0x0094,
  StartSessionTransmission        = 0x0095,
  StopSessionTransmission         = 0x0096,
  ButtonTemplateMessage           = 0x0097,
  VersionMessage                  = 0x0098,
  DisplayTextMessage              = 0x0099,
  ClearDisplay                    = 0x009A,
  CapabilitiesReqMessage          = 0x009B,
  EnunciatorCommandMessage        = 0x009C,
  RegisterRejectMessage           = 0x009D,
  ServerResMessage                = 0x009E,
  Reset                           = 0x009F,

  KeepAliveAckMessage             = 0x0100,
  StartMulticastMediaReception    = 0x0101,
  StartMulticastMediaTransmission = 0x0102,
  StopMulticastMediaReception     = 0x0103,
  StopMulticastMediaTransmission  = 0x0104,
  OpenReceiveChannel              = 0x0105,
  CloseReceiveChannel             = 0x0106,
  ConnectionStatisticsReq         = 0x0107,
  SoftKeyTemplateResMessage       = 0x0108,
  SoftKeySetResMessage            = 0x0109,

  SelectSoftKeysMessage           = 0x0110, 
  CallStateMessage                = 0x0111,
  DisplayPromptStatusMessage      = 0x0112, 
  ClearPromptStatusMessage        = 0x0113, // SkinnyCompleteRegistration??
  DisplayNotifyMessage            = 0x0114,
  ClearNotifyMessage              = 0x0115,
  ActivateCallPlaneMessage        = 0x0116,
  DeactivateCallPlaneMessage      = 0x0117,
  UnregisterAckMessage            = 0x0118,
  BackSpaceReqMessage             = 0x0119,
  RegisterTokenAck                = 0x011A,
  RegisterTokenReject             = 0x011B,

  unknownForwardMessage1          = 0x011D,

} sccp_message_t;


typedef uint32_t StationUserId; 
typedef uint32_t StationInstance; 

typedef enum { 
  Media_SilenceSuppression_Off = 0 , 
  Media_SilenceSuppression_On = 1 
} Media_SilenceSuppression; 

typedef enum { 
  Media_EchoCancellation_Off = 0 , 
  Media_EchoCancellation_On = 1 
} Media_EchoCancellation; 

typedef enum { 
  Media_G723BRate_5_3 = 1 , 
  Media_G723BRate_6_4 = 2 
} Media_G723BitRate; 

typedef struct {
  uint32_t precedenceValue; 
  Media_SilenceSuppression ssValue; 
  uint32_t maxFramesPerPacket; 
  Media_G723BitRate g723BitRate; /* only used with G.723 payload */
} Media_QualifierOutgoing; 

typedef struct {
  Media_EchoCancellation vadValue; 
  Media_G723BitRate g723BitRate; /* only used with G.723 payload */
} Media_QualifierIncoming; 

typedef struct { 
  char            deviceName[StationMaxDeviceNameSize];
  StationUserId   reserved;
  StationInstance instance;
} StationIdentifier;

typedef enum {
  DEVICE_RESET = 1 ,
  DEVICE_RESTART = 2
} DeviceResetType;

typedef enum {
  StationRingOff = 0x1,
  StationInsideRing = 0x2,
  StationOutsideRing = 0x3,
  StationFeatureRing = 0x4
} StationRingMode;

typedef enum {
  StationSpeakerOn = 1 ,
  StationSpeakerOff = 2
} StationSpeakerMode;

typedef enum {
  StationMicOn = 1 ,
  StationMicOff = 2
} StationMicrophoneMode;

typedef enum {
  StationHeadsetOn = 1 ,
  StationHeadsetOff = 2
} StationHeadsetMode;

typedef enum { 
  TsOffHook = 1 , 
  TsOnHook = 2 , 
  TsRingOut = 3 , 
  TsRingIn = 4 , 
  TsConnected = 5 , 
  TsBusy = 6 , 
  TsCongestion = 7 , 
  TsHold = 8 , 
  TsCallWaiting = 9 , 
  TsCallTransfer = 10, 
  TsCallPark = 11, 
  TsProceed = 12, 
  TsCallRemoteMultiline = 13, // Up!
  TsInvalidNumber = 14 
} StationDCallState; 

typedef enum {
  BtLastNumberRedial = 0x1,
  BtSpeedDial = 0x2,
  BtHold = 0x3,
  BtTransfer = 0x4,
  BtForwardAll = 0x5,
  BtForwardBusy = 0x6,
  BtForwardNoAnswer = 0x7,
  BtDisplay = 0x8,
  BtLine = 0x9,
  BtT120Chat = 0xA,
  BtT120Whiteboard = 0xB,
  BtT120ApplicationSharing = 0xC,
  BtT120FileTransfer = 0xD,
  BtVideo = 0xE,
  BtVoiceMail = 0xF,
  BtAutoAnswer = 0x11,
  BtGenericAppB1 = 0x21,
  BtGenericAppB2 = 0x22,
  BtGenericAppB3 = 0x23,
  BtGenericAppB4 = 0x24,
  BtGenericAppB5 = 0x25,
  BtMeetMeConference = 0x7B,
  BtConference = 0x7D,
  BtCallPark = 0x7E,
  BtCallPickup = 0x7F,
  BtGroupCallPickup = 0x80,
  BtNone = 0xFF,
  BtKeypad = 0xF0,
} StationButtonType;

typedef struct {
  int    key;
  char * value;
} value_string;

typedef struct {
  int key;
  int value;
} value_value;

typedef struct {
  int type;
} btnlist;

typedef struct {
  const char * type;
  int buttonCount;
  const btnlist * buttons;
} button_modes;

typedef struct { 
  uint8_t instanceNumber;   /* set to instance number or StationKeyPadButton value */
  uint8_t buttonDefinition; /* set to one of the preceding Bt values */
} StationButtonDefinition; 

typedef struct {
  uint32_t payloadCapability;
  uint32_t maxFramesPerPacket;
  union { 
    uint8_t futureUse[8]; 
    Media_G723BitRate g723BitRate; 
  } PAYLOADS; 
} MediaCapabilityStructure; 

typedef struct { 
  char     softKeyLabel[StationMaxSoftKeyLabelSize];
  uint32_t softKeyEvent;
} StationSoftKeyDefinition; 

typedef struct { 
  uint8_t  softKeyTemplateIndex[StationMaxSoftKeyIndex];
  uint16_t softKeyInfoIndex[StationMaxSoftKeyIndex];
} StationSoftKeySetDefinition;


typedef union {

  // No struct
  struct { } StationKeepAliveMessage;

  struct {
    StationIdentifier sId;
    uint32_t          stationIpAddr;
    uint32_t          deviceType;
    uint32_t          maxStreams;
    uint32_t          _unknown1;
    uint8_t          protocolVer;
    uint32_t          _unknown2;
    uint32_t          _unknown3;
    uint32_t          monitorObject;
    uint32_t          _unknown4;

    // 7910:
    // 02 00 00 00 // protocolVer (1st bit)
    // 08 00 00 00 == 8
    // 00 00 00 00
    // 02 00 00 00 == 2
    // ce f1 00 00 // == (61092 / 206 / 241) 1668 dn-size 420
  } RegisterMessage;

  struct {
    uint16_t rtpMediaPort;
  } IpPortMessage;

  struct {
    uint32_t kpButton; 
  } KeypadButtonMessage;

  struct {} EnblocCallMessage;

  struct {
    uint32_t stimulus; 
    uint32_t stimulusInstance; /* normally set to 1 (except speed dial and line) */
  } StimulusMessage;


  struct {} OffHookMessage;
  struct {} OnHookMessage;
  struct {} HookFlashMessage;
  struct {} ForwardStatReqMessage;

  struct {
    uint32_t speedDialNumber;
  } SpeedDialStatReqMessage;

  struct {
    uint32_t lineNumber;
  } LineStatReqMessage;

  struct {} ConfigStatReqMessage;
  struct {} TimeDateReqMessage;
  struct {} ButtonTemplateReqMessage;
  struct {} VersionReqMessage;

  struct {
    uint32_t                 count;
    MediaCapabilityStructure caps[18];
  } CapabilitiesResMessage;

  struct {} MediaPortListMessage;
  struct {} ServerReqMessage;

  struct {
    uint32_t alarmSeverity;
    char     text[80];
    uint32_t parm1;
    uint32_t parm2;
  } AlarmMessage;

  struct {} MulticastMediaReceptionAck;

  struct {
    uint32_t orcStatus; /* OpenReceiveChanStatus */
    uint32_t ipAddr;
    uint32_t portNumber;
    uint32_t passThruPartyId;
  } OpenReceiveChannelAck;

  struct {} ConnectionStatisticsRes;
  struct {} OffHookWithCgpnMessage;
  struct {} SoftKeySetReqMessage;

  struct {
    uint32_t softKeyEvent;
    uint32_t lineInstance;
    uint32_t callReference;
  } SoftKeyEventMessage;

  struct {} UnregisterMessage;
  struct {} SoftKeyTemplateReqMessage;
  struct {} RegisterTokenReq;

  struct {
    StationHeadsetMode hsMode;
  } HeadsetStatusMessage;

  struct {} unknownClientMessage2;

  struct {
    uint32_t keepAliveInterval;
    char     dateTemplate[StationDateTemplateSize];
    uint16_t _filler1;
    uint32_t secondaryKeepAliveInterval;
    uint32_t protocolVer;
  } RegisterAckMessage;

  struct {
    uint32_t tone;
  } StartToneMessage;

  struct {
  } StopToneMessage;

  struct {
    uint32_t ringMode;
  } SetRingerMessage;

  struct {
    uint32_t stimulus; 
    uint32_t stimulusInstance;
    uint32_t lampMode; 
  } SetLampMessage;

  struct {} SetHkFDetectMessage;

  struct {
    StationSpeakerMode speakerMode;
  } SetSpeakerModeMessage;

  struct {
    StationMicrophoneMode micMode;
  } SetMicroModeMessage;

  struct {
    uint32_t conferenceId;
    uint32_t passThruPartyId;
    uint32_t remoteIpAddr;
    uint32_t remotePortNumber;
    uint32_t millisecondPacketSize;
    uint32_t payloadType; /* Media_PayloadType */
    Media_QualifierOutgoing qualifierOut;
  } StartMediaTransmission;

  struct {
    int conferenceId;
    int passThruPartyId;
  } StopMediaTransmission;

  struct {
  } StartMediaReception;

  struct {
    int conferenceId;
    int passThruPartyId;
  } StopMediaReception;

  struct {
    char callingPartyName[40];
    char callingParty[24];
    char calledPartyName[40];
    char calledParty[24];
    int  lineId;
    int  callRef;
    int  callType; /* INBOUND=1, OUTBOUND=2, FORWARD=3 */
    char originalCalledPartyName[40];
    char originalCalledParty[24];
  } CallInfoMessage;

  struct {} ForwardStatMessage;

  struct {
    uint32_t speedDialNumber;
    char     speedDialDirNumber[StationMaxDirnumSize]; 
    char     speedDialDisplayName[StationMaxNameSize]; 
  } SpeedDialStatMessage;

  struct {
    uint32_t lineNumber;
    char     lineDirNumber[StationMaxDirnumSize]; 
    char     lineFullyQualifiedDisplayName[StationMaxNameSize]; 
  } LineStatMessage;

  struct {} ConfigStatMessage;

  struct {
    uint32_t year; 
    uint32_t month; 
    uint32_t dayOfWeek; 
    uint32_t day; 
    uint32_t hour; 
    uint32_t minute; 
    uint32_t seconds;
    uint32_t milliseconds; 
    uint32_t systemTime; 
  } DefineTimeDate;

  struct {} StartSessionTransmission;
  struct {} StopSessionTransmission;

  struct {
    uint32_t                buttonOffset;
    uint32_t                buttonCount;
    uint32_t                totalButtonCount;
    StationButtonDefinition definition[StationMaxButtonTemplateSize]; 
  } ButtonTemplateMessage;

  struct {
    char requiredVersion[StationMaxVersionSize];
  } VersionMessage;

  struct {
    char     displayMessage[32];
    uint32_t displayTimeout;
  } DisplayTextMessage;

  struct {} ClearDisplay;

  struct {} CapabilitiesReqMessage;
  struct {} EnunciatorCommandMessage;

  struct {
    char text[StationMaxDisplayTextSize];
  } RegisterRejectMessage;

  struct {} ServerResMessage;

  struct {
    int resetType;
  } Reset;

  struct {} KeepAliveAckMessage;
  struct {} StartMulticastMediaReception;
  struct {} StartMulticastMediaTransmission;
  struct {} StopMulticastMediaReception;
  struct {} StopMulticastMediaTransmission;

  struct {
    uint32_t conferenceId;
    uint32_t passThruPartyId;
    uint32_t millisecondPacketSize; 
    uint32_t payloadType; /* Media_PayloadType */
    Media_QualifierIncoming qualifierIn;
  } OpenReceiveChannel;

  struct {
    int conferenceId;
    int passThruPartyId;
  } CloseReceiveChannel;

  struct {} ConnectionStatisticsReq;

  struct {
    uint32_t                   softKeyOffset;
    uint32_t                   softKeyCount;
    uint32_t                   totalSoftKeyCount;
    StationSoftKeyDefinition   definition[StationMaxSoftKeyDefinition]; 
  } SoftKeyTemplateResMessage;

  struct {
    uint32_t                    softKeySetOffset;
    uint32_t                    softKeySetCount;
    uint32_t                    totalSoftKeySetCount;
    StationSoftKeySetDefinition definition[StationMaxSoftKeySetDefinition]; 
  } SoftKeySetResMessage;

  struct {
    uint32_t lineInstance;
    uint32_t callReference;
    uint32_t softKeySetIndex;
    uint16_t validKeyMask1;
    uint16_t validKeyMask2;
  } SelectSoftKeysMessage;

  struct {
    StationDCallState callState;
    uint32_t          lineInstance;
    uint32_t          callReference;
  } CallStateMessage;

  struct {
    uint32_t messageTimeout;
    char     promptMessage[32];
    uint32_t lineInstance;
    uint32_t callReference;
  } DisplayPromptStatusMessage;

  struct {
    uint32_t callReference;
    uint32_t lineInstance;
  } ClearPromptStatusMessage;

  struct {
    uint32_t displayTimeout;
    char     displayMessage[100];
    uint32_t _unkn2;
    uint32_t _unkn3;
    uint32_t _unkn4;
    uint32_t _unkn5;
  } DisplayNotifyMessage;

  // No Struct.
  struct {} ClearNotifyMessage;

  struct {
    uint32_t lineInstance;
  } ActivateCallPlaneMessage;

  // No Struct.
  struct {} DeactivateCallPlaneMessage;

  struct {} UnregisterAckMessage;
  struct {} BackSpaceReqMessage;
  struct {} RegisterTokenAck;
  struct {} RegisterTokenReject;
  struct {} unknownForwardMessage1;

} sccp_data_t;

/* I'm not quiet sure why this is called sccp_moo_t -
 * the only reason I can think of is I was very tired 
 * when i came up with it...  */

typedef struct {
  int         length;
  int         reserved;
  int         messageId;
  sccp_data_t msg;
} sccp_moo_t;

/* So in theory, a message should never be bigger than this.  
 * If it is, we abort the connection */
#define SCCP_MAX_PACKET sizeof(sccp_moo_t)

typedef struct sccp_channel  sccp_channel_t;
typedef struct sccp_session  sccp_session_t;
typedef struct sccp_line     sccp_line_t;
typedef struct sccp_speed    sccp_speed_t;
typedef struct sccp_device   sccp_device_t;
typedef struct sccp_intercom sccp_intercom_t;
typedef struct sccp_pvt      sccp_pvt_t;

/* An asterisk channel can now point to a channel (single device)
 * or an intercom.  So we need somethign to pass around :) */
struct sccp_pvt {
  sccp_channel_t  * chan;
  sccp_intercom_t * icom;
};

/* A line is a the equiv of a 'phone line' going to the phone. */
struct sccp_line {

  /* lockmeupandtiemedown */
  ast_mutex_t lock;

  /* This line's ID, used for logging into (for mobility) */
  char id[4];

  /* PIN number for mobility/roaming. */
  char pin[8];

  /* The lines position/instanceId on the current device*/
  uint8_t instance;

  /* the name of the line, so use in asterisk (i.e SCCP/<name>) */
  char name[80];

  /* A description for the line, displayed on in header (on7960/40) 
   * or on main  screen on 7910 */
  char description[80];

  /* A name for the line, displayed next to the button (7960/40). */
  char label[42];

  /* mainbox numbers (seperated by commas) to check for messages */
  char mailbox[AST_MAX_EXTENSION];

  /* The context we use for outgoing calls. */
  char context[AST_MAX_EXTENSION];

  /* CallerId to use on outgoing calls*/
  char callerid[AST_MAX_EXTENSION];

  /* The currently active channel. */
  sccp_channel_t * activeChannel;

  /* Linked list of current channels for this line */
  sccp_channel_t * channels;

  /* Number of currently active channels */
  unsigned int channelCount;

  /* Next line in the global list of devices */
  sccp_line_t * next;

  /* Next line on the current device. */
  sccp_line_t * lnext;

  /* The device this line is currently registered to. */
  sccp_device_t * device;

  /* current state of the hook on this line */
  // enum { DsDown, DsIdle, DsSeize, DsAlerting } dnState; 

  /* If we want VAD on this line */
  // XXX:T: Asterisk RTP implementation doesn't seem to handle this
  // XXX:T: correctly atm, althoguh not *really* looked/checked.

  unsigned int         vad:1;

  unsigned int         hasMessages:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
  unsigned int         spareBit6:1;
  unsigned int         spareBit7:1;

  StationDCallState dnState;

};

/* This defines a speed dial button */
struct sccp_speed {

  /* The name of the speed dial button */
  char name[40];

  /* The number to dial when it's hit */
  char ext[24];

  /* The index (instance) n the current device */
  int index;

  /* The parent device we're currently on */
  sccp_device_t * device;

  /* Pointer to next speed dial */
  sccp_speed_t * next;

};


struct sccp_device {

  /* SEP<macAddress> of the device. */
  char id[StationMaxDeviceNameSize];

  char description[StationMaxDeviceNameSize];

  /* lines to auto login this device to */
  char autologin[SCCP_MAX_AUTOLOGIN];

  /* model of this phone used for setting up features/softkeys/buttons etc. */
  int type;

  /* Current softkey set we're using */
  int currentKeySet;
  int currentKeySetLine;

  /* If the device has been rully registered yet */
  enum { RsNone, RsProgress, RsFailed, RsOK, RsTimeout } registrationState;

  /* time() the last call ended. */
  int lastCallEndTime;


  int keyset;
  int ringermode;
  int speaker;
  int mic;
  int linesWithMail;
  int currentTone;
  int registered;
  int capability;

  unsigned int         hasMessages:1;
  unsigned int         dnd:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
  unsigned int         spareBit6:1;
  unsigned int         spareBit7:1;

  sccp_channel_t * active_channel;
  sccp_speed_t * speed_dials;
  sccp_line_t * lines;
  sccp_line_t * currentLine;
  sccp_session_t * session;
  sccp_device_t * next;

  const button_modes * buttonSet;
  char lastNumber[AST_MAX_EXTENSION];
  int  lastNumberLine;

  ast_mutex_t lock;

  struct ast_ha * ha;
  struct sockaddr_in addr;
  struct in_addr ourip;
};

struct sccp_intercom {
  ast_mutex_t       lock;
  char              id[24];
  char              description[24];
  sccp_device_t  ** devices;
  sccp_intercom_t * next;
};

struct sccp_session {
  ast_mutex_t          lock;
  pthread_t            t;
  char               * in_addr;
  void * buffer;
  size_t buffer_size;



  struct sockaddr_in   sin;
  int                  lastKeepAlive;
  int                  fd;
  int                  rtpPort;
  char                 inbuf[SCCP_MAX_PACKET];
  sccp_device_t      * device;
  sccp_session_t     * next;
};

struct sccp_channel {
  ast_mutex_t          lock;
  char                 calledPartyName[40];
  char                 calledPartyNumber[24];
  uint32_t             callid;
  struct ast_channel * owner;
  sccp_line_t        * line;
  struct ast_rtp     * rtp;
  sccp_channel_t     * next,
                     * lnext;
  unsigned int         isOutgoing:1;
  unsigned int         isRinging:1;
  unsigned int         sentCall:1;
  unsigned int         spareBit1:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
};


static const value_string message_list [] = {

  /* Station -> Callmanager */
  {0x0000, "KeepAliveMessage"},
  {0x0001, "RegisterMessage"},
  {0x0002, "IpPortMessage"},
  {0x0003, "KeypadButtonMessage"},
  {0x0004, "EnblocCallMessage"},
  {0x0005, "StimulusMessage"},
  {0x0006, "OffHookMessage"},
  {0x0007, "OnHookMessage"},
  {0x0008, "HookFlashMessage"},
  {0x0009, "ForwardStatReqMessage"},
  {0x000A, "SpeedDialStatReqMessage"},
  {0x000B, "LineStatReqMessage"},
  {0x000C, "ConfigStatReqMessage"},
  {0x000D, "TimeDateReqMessage"},
  {0x000E, "ButtonTemplateReqMessage"},
  {0x000F, "VersionReqMessage"},
  {0x0010, "CapabilitiesResMessage"},
  {0x0011, "MediaPortListMessage"},
  {0x0012, "ServerReqMessage"},
  {0x0020, "AlarmMessage"},
  {0x0021, "MulticastMediaReceptionAck"},
  {0x0022, "OpenReceiveChannelAck"},
  {0x0023, "ConnectionStatisticsRes"},
  {0x0024, "OffHookWithCgpnMessage"},
  {0x0025, "SoftKeySetReqMessage"},
  {0x0026, "SoftKeyEventMessage"},
  {0x0027, "UnregisterMessage"},
  {0x0028, "SoftKeyTemplateReqMessage"},
  {0x0029, "RegisterTokenReq"},
  {0x002B, "HeadsetStatusMessage"},
  {0x002D, "unknownClientMessage2"},

  /* Callmanager -> Station */
  /* 0x0000, 0x0003? */
  {0x0081, "RegisterAckMessage"},
  {0x0082, "StartToneMessage"},
  {0x0083, "StopToneMessage"},
  {0x0085, "SetRingerMessage"},
  {0x0086, "SetLampMessage"},
  {0x0087, "SetHkFDetectMessage"},
  {0x0088, "SetSpeakerModeMessage"},
  {0x0089, "SetMicroModeMessage"},
  {0x008A, "StartMediaTransmission"},
  {0x008B, "StopMediaTransmission"},
  {0x008C, "StartMediaReception"},
  {0x008D, "StopMediaReception"},
  {0x008F, "CallInfoMessage"},
  {0x0090, "ForwardStatMessage"},
  {0x0091, "SpeedDialStatMessage"},
  {0x0092, "LineStatMessage"},
  {0x0093, "ConfigStatMessage"},
  {0x0094, "DefineTimeDate"},
  {0x0095, "StartSessionTransmission"},
  {0x0096, "StopSessionTransmission"},
  {0x0097, "ButtonTemplateMessage"},
  {0x0098, "VersionMessage"},
  {0x0099, "DisplayTextMessage"},
  {0x009A, "ClearDisplay"},
  {0x009B, "CapabilitiesReqMessage"},
  {0x009C, "EnunciatorCommandMessage"},
  {0x009D, "RegisterRejectMessage"},
  {0x009E, "ServerResMessage"},
  {0x009F, "Reset"},
  {0x0100, "KeepAliveAckMessage"},
  {0x0101, "StartMulticastMediaReception"},
  {0x0102, "StartMulticastMediaTransmission"},
  {0x0103, "StopMulticastMediaReception"},
  {0x0104, "StopMulticastMediaTransmission"},
  {0x0105, "OpenReceiveChannel"},
  {0x0106, "CloseReceiveChannel"},
  {0x0107, "ConnectionStatisticsReq"},
  {0x0108, "SoftKeyTemplateResMessage"},
  {0x0109, "SoftKeySetResMessage"},
  {0x0110, "SelectSoftKeysMessage"},
  {0x0111, "CallStateMessage"},
  {0x0112, "DisplayPromptStatusMessage"},
  {0x0113, "ClearPromptStatusMessage"},
  {0x0114, "DisplayNotifyMessage"},
  {0x0115, "ClearNotifyMessage"},
  {0x0116, "ActivateCallPlaneMessage"},
  {0x0117, "DeactivateCallPlaneMessage"},
  {0x0118, "UnregisterAckMessage"},
  {0x0119, "BackSpaceReqMessage"},
  {0x011A, "RegisterTokenAck"},
  {0x011B, "RegisterTokenReject"},
  {0x011D, "unknownForwardMessage1"},
  {0      , NULL}	/* terminator */
};



static const value_string codec_list [] = {
  {1   , "Non-standard codec"},
  {2   , "G.711 A-law 64k"},
  {3   , "G.711 A-law 56k"},
  {4   , "G.711 u-law 64k"},
  {5   , "G.711 u-law 56k"},
  {6   , "G.722 64k"},
  {7   , "G.722 56k"},
  {8   , "G.722 48k"},
  {9   , "G.723.1"},
  {10  , "G.728"},
  {11  , "G.729"},
  {12  , "G.729 Annex A"},
  {13  , "IS11172 AudioCap"},	/* IS11172 is an ISO MPEG standard */
  {14  , "IS13818 AudioCap"},	/* IS13818 is an ISO MPEG standard */
  {15  , "G.729 Annex B"},
  {16  , "G.729 Annex A+Annex B"},
  {18  , "GSM Full Rate"},
  {19  , "GSM Half Rate"},
  {20  , "GSM Enhanced Full Rate"},
  {25  , "Wideband 256k"},
  {32  , "Data 64k"},
  {33  , "Data 56k"},
  {80  , "GSM"},
  {81  , "ActiveVoice"},
  {82  , "G.726 32K"},
  {83  , "G.726 24K"},
  {84  , "G.726 16K"},
  {85  , "G.729B"},
  {86  , "G.729B Low Complexity"},
  {0  , NULL}
};

static const value_string tone_list [] = {
  {0    , "Silence"},
  {1    , "Dtmf1"},
  {2    , "Dtmf2"},
  {3    , "Dtmf3"},
  {4    , "Dtmf4"},
  {5    , "Dtmf5"},
  {6    , "Dtmf6"},
  {7    , "Dtmf7"},
  {8    , "Dtmf8"},
  {9    , "Dtmf9"},
  {0xa  , "Dtmf0"},
  {0xe  , "DtmfStar"},
  {0xf  , "DtmfPound"},
  {0x10 , "DtmfA"},
  {0x11 , "DtmfB"},
  {0x12 , "DtmfC"},
  {0x13 , "DtmfD"},
  {0x21 , "InsideDialTone"}, // OK
  {0x22 , "OutsideDialTone"}, // OK
  {0x23 , "LineBusyTone"}, // OK
  {0x24 , "AlertingTone"}, // OK
  {0x25 , "ReorderTone"}, // OK
  {0x26 , "RecorderWarningTone"},
  {0x27 , "RecorderDetectedTone"},
  {0x28 , "RevertingTone"},
  {0x29 , "ReceiverOffHookTone"},
  {0x2a , "PartialDialTone"},
  {0x2b , "NoSuchNumberTone"},
  {0x2c , "BusyVerificationTone"},
  {0x2d , "CallWaitingTone"}, // OK
  {0x2e , "ConfirmationTone"},
  {0x2f , "CampOnIndicationTone"},
  {0x30 , "RecallDialTone"},
  {0x31 , "ZipZip"}, // OK
  {0x32 , "Zip"}, // OK
  {0x33 , "BeepBonk"}, // OK (Beeeeeeeeeeeeeeeeeeeeeeeep)
  {0x34 , "MusicTone"},
  {0x35 , "HoldTone"}, // OK
  {0x36 , "TestTone"},
  {0x40 , "AddCallWaiting"},
  {0x41 , "PriorityCallWait"},
  {0x42 , "RecallDial"},
  {0x43 , "BargIn"},
  {0x44 , "DistinctAlert"},
  {0x45 , "PriorityAlert"},
  {0x46 , "ReminderRing"},
  {0x50 , "MF1"},
  {0x51 , "MF2"},
  {0x52 , "MF3"},
  {0x53 , "MF4"},
  {0x54 , "MF5"},
  {0x55 , "MF6"},
  {0x56 , "MF7"},
  {0x57 , "MF8"},
  {0x58 , "MF9"},
  {0x59 , "MF0"},
  {0x5a , "MFKP1"},
  {0x5b , "MFST"},
  {0x5c , "MFKP2"},
  {0x5d , "MFSTP"},
  {0x5e , "MFST3P"},
  {0x5f , "MILLIWATT"}, // OK
  {0x60 , "MILLIWATTTEST"},
  {0x61 , "HIGHTONE"},
  {0x62 , "FLASHOVERRIDE"},
  {0x63 , "FLASH"},
  {0x64 , "PRIORITY"},
  {0x65 , "IMMEDIATE"},
  {0x66 , "PREAMPWARN"},
  {0x67 , "2105HZ"},
  {0x68 , "2600HZ"},
  {0x69 , "440HZ"},
  {0x6a , "300HZ"},
  {0x7f , "NoTone"},
  {0   , NULL}
};

static const value_string deviceStimuli[] = {
  {1    , "LastNumberRedial"},
  {2    , "SpeedDial"},
  {3    , "Hold"},
  {4    , "Transfer"},
  {5    , "ForwardAll"},
  {6    , "ForwardBusy"},
  {7    , "ForwardNoAnswer"},
  {8    , "Display"},
  {9    , "Line"},
  {0xA  , "T120Chat"},
  {0xB  , "T120Whiteboard"},
  {0xC  , "T120ApplicationSharing"},
  {0xD  , "T120FileTransfer"},
  {0xE  , "Video"},
  {0xF  , "VoiceMail"},
  {0x11 , "AutoAnswer"},
  {0x21 , "GenericAppB1"},
  {0x22 , "GenericAppB2"},
  {0x23 , "GenericAppB3"},
  {0x24 , "GenericAppB4"},
  {0x25 , "GenericAppB5"},
  {0x7b , "MeetMeConference"},
  {0x7d , "Conference=0x7d"},
  {0x7e , "CallPark=0x7e"},
  {0x7f , "CallPickup"},
  {0x80 , "GroupCallPickup=80"},
  {0,NULL}
};

static const value_string buttonDefinitions[] = {
  {1    , "LastNumberRedial"},
  {2    , "SpeedDial"},
  {3    , "Hold"},
  {4    , "Transfer"},
  {5    , "ForwardAll"},
  {6    , "ForwardBusy"},
  {7    , "ForwardNoAnswer"},
  {8    , "Display"},
  {9    , "Line"},
  {0xa  , "T120Chat"},
  {0xb  , "T120Whiteboard"},
  {0xc  , "T120ApplicationSharing"},
  {0xd  , "T120FileTransfer"},
  {0xe  , "Video"},
  {0x10 , "AnswerRelease"},
  {0xf0 , "Keypad"},
  {0xfd , "AEC"},
  {0xff , "Undefined"},
  {0   , NULL}
};

static const value_string alarmSeverities[] = {
  {0   , "Critical"},
  {1   , "Warning"},
  {2   , "Informational"},
  {4   , "Unknown"},
  {7   , "Major"},
  {8   , "Minor"},
  {10  , "Marginal"},
  {20  , "TraceInfo"},
  {0   , NULL}
};

static const value_string device_types[] = {
  {1  , "30SPplus"},
  {2  , "12SPplus"},
  {3  , "12SP"},
  {4  , "12"},
  {5  , "30VIP"},
  {6  , "Telecaster"},
  {7  , "TelecasterMgr"},
  {8  , "TelecasterBus"},
  {20 , "Virtual30SPplus"},
  {21 , "PhoneApplication"},
  {30 , "AnalogAccess"},
  {40 , "DigitalAccessPRI"},
  {41 , "DigitalAccessT1"},
  {42 , "DigitalAccessTitan2"},
  {47 , "AnalogAccessElvis"},
  {49 , "DigitalAccessLennon"},
  {50 , "ConferenceBridge"},
  {51 , "ConferenceBridgeYoko"},
  {60 , "H225"},
  {61 , "H323Phone"},
  {62 , "H323Trunk"},
  {70 , "MusicOnHold"},
  {71 , "Pilot"},
  {72 , "TapiPort"},
  {73 , "TapiRoutePoint"},
  {80 , "VoiceInBox"},
  {81 , "VoiceInboxAdmin"},
  {82 , "LineAnnunciator"},
  {90 , "RouteList"},
  {100, "LoadSimulator"},
  {110, "MediaTerminationPoint"},
  {111, "MediaTerminationPointYoko"},
  {120, "MGCPStation"},
  {121, "MGCPTrunk"},
  {122, "RASProxy"},
  {255, "NotDefined"},
  { 0    , NULL}
};


#define MAX_TsCallStatusText 14
extern char * TsCallStatusText[MAX_TsCallStatusText];
int handle_message(sccp_moo_t * r, sccp_session_t * s);

typedef void sk_func (sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);

typedef struct {
  int       id;
  char    * txt;
  sk_func * func;
} softkeytypes;

#include "sccp_softkeys.h"

static const softkeytypes button_labels [] = {
  {  1, "Redial",   sccp_sk_redial },
  {  2, "NewCall",  sccp_sk_newcall },
  {  3, "Hold",     sccp_sk_hold },
  {  4, "Reject",   sccp_sk_reject },
  {  5, "Answer",   sccp_sk_answer },
  {  6, "EndCall",  sccp_sk_endcall },
  {  7, "Confrn",   sccp_sk_conference },
  {  8, "Trnsfer",  sccp_sk_transfer },
  {  9, "BlindXfr", sccp_sk_blindxfr },
  { 10, "DND",      sccp_sk_dnd },
  { 11, "AddLine",  sccp_sk_addline },
  { 12, "<<",       sccp_sk_back },
  { 13, "Dial",     sccp_sk_dial },
  { 14, "Clear",    sccp_sk_clear },
  {  0,  NULL,      NULL },
};

typedef struct {
  int labelId;
  void * callBack;
} btndef;

typedef struct {
  int   setId;
  const btndef * ptr;
} softkey_modes;

#define KEYMODE_UNREGISTERED 0
static const btndef skSet0 [] = {
  {11, sccp_sk_redial  },
  { 0, NULL },
};

#define KEYMODE_ONHOOK 1
static const btndef skSet1 [] = {
  { 1, sccp_sk_redial  },
  { 2, sccp_sk_newcall },
  {10, sccp_sk_newcall },
  { 0, NULL },
};

#define KEYMODE_OFFHOOK 2
static const btndef skSet2 [] = {
  { 1, sccp_sk_redial  },
  { 6, sccp_sk_newcall },
  {12, sccp_sk_newcall },
  { 0, NULL },
};

#define KEYMODE_DIALING 3
static const btndef skSet3 [] = {
  {12, sccp_sk_newcall },
  {13, sccp_sk_redial  },
  {14, sccp_sk_redial  },
  { 6, sccp_sk_newcall },
  { 0, NULL },
};

#define KEYMODE_PROGRESS 4
static const btndef skSet4 [] = {
  {-1, sccp_sk_newcall },
  { 6, sccp_sk_newcall },
  { 0, NULL },
};

#define KEYMODE_CONNECTED 5
static const btndef skSet5 [] = {
  { 3, NULL },
  { 6, NULL },
  { 7, NULL },
  { 8, NULL },
  {-1, NULL },
  { 9, NULL },
  { 0, NULL }
};

#define KEYMODE_INCOMING 6
static const btndef skSet6 [] = {
  { 3, NULL },
  { 4, NULL },
};

static const softkey_modes SoftKeyModes [] = {
  { KEYMODE_UNREGISTERED,  skSet0 },
  { KEYMODE_ONHOOK,        skSet1 },
  { KEYMODE_OFFHOOK,       skSet2 },
  { KEYMODE_DIALING,       skSet3 },
  { KEYMODE_PROGRESS,      skSet4 },
  { KEYMODE_CONNECTED,     skSet5 },
  { KEYMODE_INCOMING,      skSet6 },
  { 0, NULL }
};

static const btnlist layout_7910 [] = {
  {BtLine},
  {BtHold},
  {BtTransfer},
  {BtDisplay},
  {BtVoiceMail},
  {BtConference},
  {BtForwardAll},
  {BtSpeedDial},
  {BtSpeedDial},
  {BtLastNumberRedial},
};

static const btnlist layout_7920 [] = {
  { BtLine },
  { BtLine },
};

static const btnlist layout_7940 [] = {
  { BtLine },
  { BtLine },
};

static const btnlist layout_7960 [] = {
  { BtLine },
  { BtLine },
  { BtLine },
  { BtLine },
  { BtLine },
  { BtLine },
};

static const button_modes default_layouts [] = {
  { "7910", 10, layout_7910 },
  { "7920", 2,  layout_7920 },
  { "7940", 2,  layout_7940 },
  { "7960", 6,  layout_7960 },
  {   NULL, 0 },
};

extern ast_mutex_t      intercomlock;
extern ast_mutex_t      devicelock;
extern ast_mutex_t      chanlock;
extern ast_mutex_t      sessionlock;
extern ast_mutex_t      linelock;
extern ast_mutex_t      usecnt_lock;
extern sccp_session_t * sessions;
extern sccp_device_t  * devices;
extern sccp_intercom_t* intercoms;
extern sccp_line_t    * lines;
extern sccp_channel_t * chans;
extern int              capability;
extern int              keepalive;
extern int              usecnt;
extern int              sccp_debug;
extern struct in_addr   __ourip;
extern char             date_format[6];
extern struct sched_context * sccp_sched;



#endif /* __CHAN_SCCP_H */

