/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2004 - 2006, Steve Underwod <steveu@coppice.org>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 *
 * $Id$
 */

/* Definitions for the land line SMS protocols in ETSI ES 201 912
 * Protocol 1 */

#define DLL_SMS_P1_DATA                         0x11
#define DLL_SMS_P1_ERROR                        0x12
#define DLL_SMS_P1_EST                          0x13
#define DLL_SMS_P1_REL                          0x14
#define DLL_SMS_P1_ACK                          0x15
#define DLL_SMS_P1_NACK                         0x16


#define DLL_SMS_ERROR_WRONG_CHECKSUM            0x01
#define DLL_SMS_ERROR_WRONG_MESSAGE_LEN         0x02
#define DLL_SMS_ERROR_UNKNOWN_MESSAGE_TYPE      0x03
#define DLL_SMS_ERROR_EXTENSION_NOT_SUPPORTED   0x04
#define DLL_SMS_ERROR_UNSPECIFIED_ERROR         0xFF

/* Times in ms */
#define DLL_SMS_T10                             100
#define DLL_SMS_T11                             100
#define DLL_SMS_T12                             400


/* Definitions for the land line SMS protocols in ETSI ES 201 912
 * Protocol 2 */

#define DLL_SMS_P2_INFO_MO                      0x10
#define DLL_SMS_P2_INFO_MT                      0x11
#define DLL_SMS_P2_INFO_STA                     0x12
#define DLL_SMS_P2_NACK                         0x13
#define DLL_SMS_P2_ACK0                         0x14
#define DLL_SMS_P2_ACK1                         0x15
#define DLL_SMS_P2_ENQ                          0x16
#define DLL_SMS_P2_REL                          0x17

/* Times in ms. Tolerance +-10% */
#define DLL_SMS_TM1                              800
#define DLL_SMS_TM2                             7600
#define DLL_SMS_TM3                             7500
#define DLL_SMS_TM4                             3500
#define DLL_SMS_TM5                              800
#define DLL_SMS_TM6                              200
#define DLL_SMS_TM7                             3500
#define DLL_SMS_T1                               200
#define DLL_SMS_T2                               200
#define DLL_SMS_T3                               100

#define DLL_SMS_NRETRY              2
#define DLL_SMS_NWAIT               49

#define DLL_PARM_MEDIA_IDENTIFIER   0x10    /* Media Identifier (length: 1) */
#define DLL_PARM_FIRMWARE           0x11    /* Firmware Version (length: 6) */
#define DLL_PARM_PROVIDER_ID        0x12    /* SMS Provider Identifier (length: 3) */
#define DLL_PARM_DISPLAY_INFO       0x13    /* Display Information (max length: 65 535) */
#define DLL_PARM_DATETIME           0x14    /* Date and Time (length: 8) */
#define DLL_PARM_CLI                0x15    /* Calling Line Identity (max length: 20) */
#define DLL_PARM_CLI_ABSCENCE       0x16    /* Reason for Absence of CLI (length: 1) */
#define DLL_PARM_CTI                0x17    /* Calling Terminal Identity (length: 1) */
#define DLL_PARM_DESTINATION        0x18    /* Called Line Identity (max length: 20) */
#define DLL_PARM_FAX_NAME           0x19    /* Fax Recipient Name (max length: 255) */
#define DLL_PARM_EMAIL_ADDRESS      0x1A    /* E-mail Address (max length: 256) */
#define DLL_PARM_DESTINATION_TERM   0x1B    /* Called Terminal Identity (length: 1) */
#define DLL_PARM_NOTIFY             0x1C    /* Notify (length: 3) */
#define DLL_PARM_PUBLIC_KEY         0x1D    /* Public Key (max length: 5) */
#define DLL_PARM_SMTE_RESOURCES     0x1E    /* SM-TE Resources (max length: 67) */
#define DLL_PARM_RESPONSE_TYPE      0x1F    /* Response Type (length: 1) */
#define DLL_PARM_BEARER_CAP         0x20    /* Bearer Capability (max length: 20) */
#define DLL_PARM_REPLACE_SM_TYPE    0x21    /* Replace Short Message Type (length: 1) */
#define DLL_PARM_VALIDITY_PERIOD    0x22    /* Validity-Period (length: 1) */
#define DLL_PARM_DATA_INFO          0x23    /* Data Information (max length: 65 535) */
