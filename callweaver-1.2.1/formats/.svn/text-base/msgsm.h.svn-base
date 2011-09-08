/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Conversion routines derived from code by guido@sienanet.it
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
 */

#define GSM_MAGIC 0xD

#ifndef GSM_H
typedef unsigned char           gsm_byte;
#endif
typedef unsigned char           wav_byte;

#define readGSM_33(c1) \
{ \
    gsm_byte *c = (c1); \
    LARc[0]  = (*c++ & 0xF) << 2;           /* 1 */ \
    LARc[0] |= (*c >> 6) & 0x3; \
    LARc[1]  = *c++ & 0x3F; \
    LARc[2]  = (*c >> 3) & 0x1F; \
    LARc[3]  = (*c++ & 0x7) << 2; \
    LARc[3] |= (*c >> 6) & 0x3; \
    LARc[4]  = (*c >> 2) & 0xF; \
    LARc[5]  = (*c++ & 0x3) << 2; \
    LARc[5] |= (*c >> 6) & 0x3; \
    LARc[6]  = (*c >> 3) & 0x7; \
    LARc[7]  = *c++ & 0x7; \
    Nc[0]  = (*c >> 1) & 0x7F; \
    bc[0]  = (*c++ & 0x1) << 1; \
    bc[0] |= (*c >> 7) & 0x1; \
    Mc[0]  = (*c >> 5) & 0x3; \
    xmaxc[0]  = (*c++ & 0x1F) << 1; \
    xmaxc[0] |= (*c >> 7) & 0x1; \
    xmc[0][0]  = (*c >> 4) & 0x7; \
    xmc[0][1]  = (*c >> 1) & 0x7; \
    xmc[0][2]  = (*c++ & 0x1) << 2; \
    xmc[0][2] |= (*c >> 6) & 0x3; \
    xmc[0][3]  = (*c >> 3) & 0x7; \
    xmc[0][4]  = *c++ & 0x7; \
    xmc[0][5]  = (*c >> 5) & 0x7; \
    xmc[0][6]  = (*c >> 2) & 0x7; \
    xmc[0][7]  = (*c++ & 0x3) << 1;         /* 10 */ \
    xmc[0][7] |= (*c >> 7) & 0x1; \
    xmc[0][8]  = (*c >> 4) & 0x7; \
    xmc[0][9]  = (*c >> 1) & 0x7; \
    xmc[0][10]  = (*c++ & 0x1) << 2; \
    xmc[0][10] |= (*c >> 6) & 0x3; \
    xmc[0][11]  = (*c >> 3) & 0x7; \
    xmc[0][12]  = *c++ & 0x7; \
    Nc[1]  = (*c >> 1) & 0x7F; \
    bc[1]  = (*c++ & 0x1) << 1; \
    bc[1] |= (*c >> 7) & 0x1; \
    Mc[1]  = (*c >> 5) & 0x3; \
    xmaxc[1]  = (*c++ & 0x1F) << 1; \
    xmaxc[1] |= (*c >> 7) & 0x1; \
    xmc[1][0]  = (*c >> 4) & 0x7; \
    xmc[1][1]  = (*c >> 1) & 0x7; \
    xmc[1][2]  = (*c++ & 0x1) << 2; \
    xmc[1][3] |= (*c >> 6) & 0x3; \
    xmc[1][4]  = (*c >> 3) & 0x7; \
    xmc[1][5]  = *c++ & 0x7; \
    xmc[1][6]  = (*c >> 5) & 0x7; \
    xmc[1][7]  = (*c >> 2) & 0x7; \
    xmc[1][8]  = (*c++ & 0x3) << 1; \
    xmc[1][8] |= (*c >> 7) & 0x1; \
    xmc[1][9]  = (*c >> 4) & 0x7; \
    xmc[1][10]  = (*c >> 1) & 0x7; \
    xmc[1][11]  = (*c++ & 0x1) << 2; \
    xmc[1][11] |= (*c >> 6) & 0x3; \
    xmc[1][12]  = (*c >> 3) & 0x7; \
    xmc[1][13]  = *c++ & 0x7; \
    Nc[2]  = (*c >> 1) & 0x7F; \
    bc[2]  = (*c++ & 0x1) << 1;             /* 20 */ \
    bc[2] |= (*c >> 7) & 0x1; \
    Mc[2]  = (*c >> 5) & 0x3; \
    xmaxc[2]  = (*c++ & 0x1F) << 1; \
    xmaxc[2] |= (*c >> 7) & 0x1; \
    xmc[2][0]  = (*c >> 4) & 0x7; \
    xmc[2][1]  = (*c >> 1) & 0x7; \
    xmc[2][2]  = (*c++ & 0x1) << 2; \
    xmc[2][2] |= (*c >> 6) & 0x3; \
    xmc[2][3]  = (*c >> 3) & 0x7; \
    xmc[2][4]  = *c++ & 0x7; \
    xmc[2][5]  = (*c >> 5) & 0x7; \
    xmc[2][6]  = (*c >> 2) & 0x7; \
    xmc[2][7]  = (*c++ & 0x3) << 1; \
    xmc[2][7] |= (*c >> 7) & 0x1; \
    xmc[2][8]  = (*c >> 4) & 0x7; \
    xmc[2][9]  = (*c >> 1) & 0x7; \
    xmc[2][10]  = (*c++ & 0x1) << 2; \
    xmc[2][10] |= (*c >> 6) & 0x3; \
    xmc[2][11]  = (*c >> 3) & 0x7; \
    xmc[2][12]  = *c++ & 0x7; \
    Nc[3]  = (*c >> 1) & 0x7F; \
    bc[3]  = (*c++ & 0x1) << 1; \
    bc[3] |= (*c >> 7) & 0x1; \
    Mc[3]  = (*c >> 5) & 0x3; \
    xmaxc[3]  = (*c++ & 0x1F) << 1; \
    xmaxc[3] |= (*c >> 7) & 0x1; \
    xmc[3][0]  = (*c >> 4) & 0x7; \
    xmc[3][1]  = (*c >> 1) & 0x7; \
    xmc[3][2]  = (*c++ & 0x1) << 2; \
    xmc[3][2] |= (*c >> 6) & 0x3; \
    xmc[3][3]  = (*c >> 3) & 0x7; \
    xmc[3][4]  = *c++ & 0x7;                /* 30  */ \
    xmc[3][5]  = (*c >> 5) & 0x7; \
    xmc[3][6]  = (*c >> 2) & 0x7; \
    xmc[3][7]  = (*c++ & 0x3) << 1; \
    xmc[3][7] |= (*c >> 7) & 0x1; \
    xmc[3][8]  = (*c >> 4) & 0x7; \
    xmc[3][9]  = (*c >> 1) & 0x7; \
    xmc[3][10]  = (*c++ & 0x1) << 2; \
    xmc[3][10] |= (*c >> 6) & 0x3; \
    xmc[3][11]  = (*c >> 3) & 0x7; \
    xmc[3][12]  = *c & 0x7;                 /* 33 */ \
}

static inline void conv66(gsm_byte *d, wav_byte *c)
{
	gsm_byte frame_chain;
    unsigned int sr;
	unsigned int LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[4][13];
	
	readGSM_33(d);
	sr = 0;
	sr = (sr >> 6) | (LARc[0] << 10);
	sr = (sr >> 6) | (LARc[1] << 10);
	*c++ = sr >> 4;
	sr = (sr >> 5) | (LARc[2] << 11);
	*c++ = sr >> 7;
	sr = (sr >> 5) | (LARc[3] << 11);
	sr = (sr >> 4) | (LARc[4] << 12);
	*c++ = sr >> 6;
	sr = (sr >> 4) | (LARc[5] << 12);
	sr = (sr >> 3) | (LARc[6] << 13);
	*c++ = sr >> 7;
	sr = (sr >> 3) | (LARc[7] << 13);
	sr = (sr >> 7) | (Nc[0] << 9);
	*c++ = sr >> 5;
	sr = (sr >> 2) | (bc[0] << 14);
	sr = (sr >> 2) | (Mc[0] << 14);
	sr = (sr >> 6) | (xmaxc[0] << 10);
	*c++ = sr >> 3;
	sr = (sr >> 3 )|( xmc[0][0] << 13);
	*c++ = sr >> 8;
	sr = (sr >> 3 )|( xmc[0][1] << 13);
	sr = (sr >> 3 )|( xmc[0][2] << 13);
    sr = (sr >> 3 )|( xmc[0][3] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[0][4] << 13);
    sr = (sr >> 3 )|( xmc[0][5] << 13);
    sr = (sr >> 3 )|( xmc[0][6] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[0][7] << 13);
    sr = (sr >> 3 )|( xmc[0][8] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[0][9] << 13);
    sr = (sr >> 3 )|( xmc[0][10] << 13);
    sr = (sr >> 3 )|( xmc[0][11] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[0][12] << 13);
    sr = (sr >> 7 )|( Nc[1] << 9);
    *c++ = sr >> 5;
    sr = (sr >> 2 )|( bc[1] << 14);
    sr = (sr >> 2 )|( Mc[1] << 14);
    sr = (sr >> 6 )|( xmaxc[1] << 10);
    *c++ = sr >> 3;
    sr = (sr >> 3 )|( xmc[1][0] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[1][1] << 13);
    sr = (sr >> 3 )|( xmc[1][2] << 13);
    sr = (sr >> 3 )|( xmc[1][3] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[1][4] << 13);
    sr = (sr >> 3 )|( xmc[1][5] << 13);
    sr = (sr >> 3 )|( xmc[1][6] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[1][7] << 13);
    sr = (sr >> 3 )|( xmc[1][8] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[1][9] << 13);
    sr = (sr >> 3 )|( xmc[1][10] << 13);
    sr = (sr >> 3 )|( xmc[1][11] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[1][12] << 13);
    sr = (sr >> 7 )|( Nc[2] << 9);
    *c++ = sr >> 5;
    sr = (sr >> 2 )|( bc[2] << 14);
    sr = (sr >> 2 )|( Mc[2] << 14);
    sr = (sr >> 6 )|( xmaxc[2] << 10);
    *c++ = sr >> 3;
    sr = (sr >> 3 )|( xmc[2][0] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[2][1] << 13);
    sr = (sr >> 3 )|( xmc[2][2] << 13);
    sr = (sr >> 3 )|( xmc[2][3] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[2][4] << 13);
    sr = (sr >> 3 )|( xmc[2][5] << 13);
    sr = (sr >> 3 )|( xmc[2][6] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[2][7] << 13);
    sr = (sr >> 3 )|( xmc[2][8] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[2][9] << 13);
    sr = (sr >> 3 )|( xmc[2][10] << 13);
    sr = (sr >> 3 )|( xmc[2][11] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[2][12] << 13);
    sr = (sr >> 7 )|( Nc[3] << 9);
    *c++ = sr >> 5;
    sr = (sr >> 2 )|( bc[3] << 14);
    sr = (sr >> 2 )|( Mc[3] << 14);
    sr = (sr >> 6 )|( xmaxc[3] << 10);
    *c++ = sr >> 3;
    sr = (sr >> 3 )|( xmc[3][0] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[3][1] << 13);
    sr = (sr >> 3 )|( xmc[3][2] << 13);
    sr = (sr >> 3 )|( xmc[3][3] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[3][4] << 13);
    sr = (sr >> 3 )|( xmc[3][5] << 13);
    sr = (sr >> 3 )|( xmc[3][6] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[3][7] << 13);
    sr = (sr >> 3 )|( xmc[3][8] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[3][9] << 13);
    sr = (sr >> 3 )|( xmc[3][10] << 13);
    sr = (sr >> 3 )|( xmc[3][11] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[3][12] << 13);
    sr = sr >> 4;
    *c = sr >> 8;
    frame_chain = *c;
    readGSM_33(d+33); /* puts all the parameters into LARc etc. */


    sr = 0;
/*                       sr = (sr >> 4 )|( s->frame_chain << 12); */
    sr = (sr >> 4 )|( frame_chain << 12);

    sr = (sr >> 6 )|( LARc[0] << 10);
    *c++ = sr >> 6;
    sr = (sr >> 6 )|( LARc[1] << 10);
    *c++ = sr >> 8;
    sr = (sr >> 5 )|( LARc[2] << 11);
    sr = (sr >> 5 )|( LARc[3] << 11);
    *c++ = sr >> 6;
    sr = (sr >> 4 )|( LARc[4] << 12);
    sr = (sr >> 4 )|( LARc[5] << 12);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( LARc[6] << 13);
    sr = (sr >> 3 )|( LARc[7] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 7 )|( Nc[0] << 9);
    sr = (sr >> 2 )|( bc[0] << 14);
    *c++ = sr >> 7;
    sr = (sr >> 2 )|( Mc[0] << 14);
    sr = (sr >> 6 )|( xmaxc[0] << 10);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[0][0] << 13);
    sr = (sr >> 3 )|( xmc[0][1] << 13);
    sr = (sr >> 3 )|( xmc[0][2] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[0][3] << 13);
    sr = (sr >> 3 )|( xmc[0][4] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[0][5] << 13);
    sr = (sr >> 3 )|( xmc[0][6] << 13);
    sr = (sr >> 3 )|( xmc[0][7] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[0][8] << 13);
    sr = (sr >> 3 )|( xmc[0][9] << 13);
    sr = (sr >> 3 )|( xmc[0][10] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[0][11] << 13);
    sr = (sr >> 3 )|( xmc[0][12] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 7 )|( Nc[1] << 9);
    sr = (sr >> 2 )|( bc[1] << 14);
    *c++ = sr >> 7;
    sr = (sr >> 2 )|( Mc[1] << 14);
    sr = (sr >> 6 )|( xmaxc[1] << 10);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[1][0] << 13);
    sr = (sr >> 3 )|( xmc[1][1] << 13);
    sr = (sr >> 3 )|( xmc[1][2] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[1][3] << 13);
    sr = (sr >> 3 )|( xmc[1][4] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[1][5] << 13);
    sr = (sr >> 3 )|( xmc[1][6] << 13);
    sr = (sr >> 3 )|( xmc[1][7] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[1][8] << 13);
    sr = (sr >> 3 )|( xmc[1][9] << 13);
    sr = (sr >> 3 )|( xmc[1][10] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[1][11] << 13);
    sr = (sr >> 3 )|( xmc[1][12] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 7 )|( Nc[2] << 9);
    sr = (sr >> 2 )|( bc[2] << 14);
    *c++ = sr >> 7;
    sr = (sr >> 2 )|( Mc[2] << 14);
    sr = (sr >> 6 )|( xmaxc[2] << 10);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[2][0] << 13);
    sr = (sr >> 3 )|( xmc[2][1] << 13);
    sr = (sr >> 3 )|( xmc[2][2] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[2][3] << 13);
    sr = (sr >> 3 )|( xmc[2][4] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[2][5] << 13);
    sr = (sr >> 3 )|( xmc[2][6] << 13);
    sr = (sr >> 3 )|( xmc[2][7] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[2][8] << 13);
    sr = (sr >> 3 )|( xmc[2][9] << 13);
    sr = (sr >> 3 )|( xmc[2][10] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[2][11] << 13);
    sr = (sr >> 3 )|( xmc[2][12] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 7 )|( Nc[3] << 9);
    sr = (sr >> 2 )|( bc[3] << 14);
    *c++ = sr >> 7;
    sr = (sr >> 2 )|( Mc[3] << 14);
    sr = (sr >> 6 )|( xmaxc[3] << 10);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[3][0] << 13);
    sr = (sr >> 3 )|( xmc[3][1] << 13);
    sr = (sr >> 3 )|( xmc[3][2] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[3][3] << 13);
    sr = (sr >> 3 )|( xmc[3][4] << 13);
    *c++ = sr >> 8;
    sr = (sr >> 3 )|( xmc[3][5] << 13);
    sr = (sr >> 3 )|( xmc[3][6] << 13);
    sr = (sr >> 3 )|( xmc[3][7] << 13);
    *c++ = sr >> 7;
    sr = (sr >> 3 )|( xmc[3][8] << 13);
    sr = (sr >> 3 )|( xmc[3][9] << 13);
    sr = (sr >> 3 )|( xmc[3][10] << 13);
    *c++ = sr >> 6;
    sr = (sr >> 3 )|( xmc[3][11] << 13);
    sr = (sr >> 3 )|( xmc[3][12] << 13);
    *c++ = sr >> 8;
}

#define writeGSM_33(c1) \
{ \
    gsm_byte *c = (c1); \
    *c++ =   ((GSM_MAGIC & 0xF) << 4)               /* 1 */ \
           | ((LARc[0] >> 2) & 0xF); \
    *c++ =   ((LARc[0] & 0x3) << 6) \
           | (LARc[1] & 0x3F); \
    *c++ =   ((LARc[2] & 0x1F) << 3) \
           | ((LARc[3] >> 2) & 0x7); \
    *c++ =   ((LARc[3] & 0x3) << 6) \
           | ((LARc[4] & 0xF) << 2) \
           | ((LARc[5] >> 2) & 0x3); \
    *c++ =   ((LARc[5] & 0x3) << 6) \
           | ((LARc[6] & 0x7) << 3) \
           | (LARc[7] & 0x7);   \
    *c++ =   ((Nc[0] & 0x7F) << 1) \
           | ((bc[0] >> 1) & 0x1); \
    *c++ =   ((bc[0] & 0x1) << 7) \
           | ((Mc[0] & 0x3) << 5) \
           | ((xmaxc[0] >> 1) & 0x1F); \
    *c++ =   ((xmaxc[0] & 0x1) << 7) \
           | ((xmc[0][0] & 0x7) << 4) \
           | ((xmc[0][1] & 0x7) << 1) \
           | ((xmc[0][2] >> 2) & 0x1); \
    *c++ =   ((xmc[0][2] & 0x3) << 6) \
           | ((xmc[0][3] & 0x7) << 3) \
           |  (xmc[0][4] & 0x7); \
    *c++ =   ((xmc[0][5] & 0x7) << 5)               /* 10 */ \
           | ((xmc[0][6] & 0x7) << 2) \
           | ((xmc[0][7] >> 1) & 0x3); \
    *c++ =   ((xmc[0][7] & 0x1) << 7) \
           | ((xmc[0][8] & 0x7) << 4) \
           | ((xmc[0][9] & 0x7) << 1) \
           | ((xmc[0][10] >> 2) & 0x1); \
    *c++ =   ((xmc[0][10] & 0x3) << 6) \
           | ((xmc[0][11] & 0x7) << 3) \
           |  (xmc[0][12] & 0x7); \
    *c++ =   ((Nc[1] & 0x7F) << 1) \
           | ((bc[1] >> 1) & 0x1); \
    *c++ =   ((bc[1] & 0x1) << 7) \
           | ((Mc[1] & 0x3) << 5) \
           | ((xmaxc[1] >> 1) & 0x1F);  \
    *c++ =   ((xmaxc[1] & 0x1) << 7) \
           | ((xmc[1][0] & 0x7) << 4) \
           | ((xmc[1][1] & 0x7) << 1) \
           | ((xmc[1][2] >> 2) & 0x1); \
    *c++ =   ((xmc[1][2] & 0x3) << 6) \
           | ((xmc[1][3] & 0x7) << 3) \
           |  (xmc[1][4] & 0x7); \
    *c++ =   ((xmc[1][5] & 0x7) << 5) \
           | ((xmc[1][6] & 0x7) << 2) \
           | ((xmc[1][7] >> 1) & 0x3); \
    *c++ =   ((xmc[1][7] & 0x1) << 7) \
           | ((xmc[1][8] & 0x7) << 4) \
           | ((xmc[1][9] & 0x7) << 1) \
           | ((xmc[1][10] >> 2) & 0x1); \
    *c++ =   ((xmc[1][10] & 0x3) << 6) \
           | ((xmc[1][11] & 0x7) << 3) \
           |  (xmc[1][12] & 0x7); \
    *c++ =   ((Nc[2] & 0x7F) << 1)                  /* 20 */ \
           | ((bc[2] >> 1) & 0x1); \
    *c++ =   ((bc[2] & 0x1) << 7) \
           | ((Mc[2] & 0x3) << 5) \
           | ((xmaxc[2] >> 1) & 0x1F); \
    *c++ =   ((xmaxc[2] & 0x1) << 7)   \
           | ((xmc[2][0] & 0x7) << 4) \
           | ((xmc[2][1] & 0x7) << 1) \
           | ((xmc[2][2] >> 2) & 0x1); \
    *c++ =   ((xmc[2][2] & 0x3) << 6) \
           | ((xmc[2][3] & 0x7) << 3) \
           |  (xmc[2][4] & 0x7); \
    *c++ =   ((xmc[2][5] & 0x7) << 5) \
           | ((xmc[2][6] & 0x7) << 2) \
           | ((xmc[2][7] >> 1) & 0x3); \
    *c++ =   ((xmc[2][7] & 0x1) << 7) \
           | ((xmc[2][8] & 0x7) << 4) \
           | ((xmc[2][9] & 0x7) << 1) \
           | ((xmc[2][10] >> 2) & 0x1); \
    *c++ =   ((xmc[2][10] & 0x3) << 6) \
           | ((xmc[2][11] & 0x7) << 3) \
           |  (xmc[2][12] & 0x7); \
    *c++ =   ((Nc[3] & 0x7F) << 1) \
           | ((bc[3] >> 1) & 0x1); \
    *c++ =   ((bc[3] & 0x1) << 7)  \
           | ((Mc[3] & 0x3) << 5) \
           | ((xmaxc[3] >> 1) & 0x1F); \
    *c++ =   ((xmaxc[3] & 0x1) << 7) \
           | ((xmc[3][0] & 0x7) << 4) \
           | ((xmc[3][1] & 0x7) << 1) \
           | ((xmc[3][2] >> 2) & 0x1); \
    *c++ =   ((xmc[3][2] & 0x3) << 6)               /* 30 */ \
           | ((xmc[3][3] & 0x7) << 3) \
           |  (xmc[3][4] & 0x7); \
    *c++ =   ((xmc[3][5] & 0x7) << 5) \
           | ((xmc[3][6] & 0x7) << 2) \
           | ((xmc[3][7] >> 1) & 0x3); \
    *c++ =   ((xmc[3][7] & 0x1) << 7) \
           | ((xmc[3][8] & 0x7) << 4) \
           | ((xmc[3][9] & 0x7) << 1) \
           | ((xmc[3][10] >> 2) & 0x1); \
    *c++ =   ((xmc[3][10] & 0x3) << 6) \
           | ((xmc[3][11] & 0x7) << 3) \
           |  (xmc[3][12] & 0x7); \
}

static inline void conv65( wav_byte * c, gsm_byte * d)
{
    unsigned int sr = 0;
    unsigned int frame_chain;
	unsigned int LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[4][13];
 
    sr = *c++;
    LARc[0] = sr & 0x3f;  sr >>= 6;
    sr |= (unsigned int) *c++ << 2;
    LARc[1] = sr & 0x3f;  sr >>= 6;
    sr |= (unsigned int) *c++ << 4;
    LARc[2] = sr & 0x1f;  sr >>= 5;
    LARc[3] = sr & 0x1f;  sr >>= 5;
    sr |= (unsigned int) *c++ << 2;
    LARc[4] = sr & 0xf;  sr >>= 4;
    LARc[5] = sr & 0xf;  sr >>= 4;
    sr |= (unsigned int) *c++ << 2;                 /* 5 */
    LARc[6] = sr & 0x7;  sr >>= 3;
    LARc[7] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 4;
    Nc[0] = sr & 0x7f;  sr >>= 7;
    bc[0] = sr & 0x3;  sr >>= 2;
    Mc[0] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 1;
    xmaxc[0] = sr & 0x3f;  sr >>= 6;
    xmc[0][0] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[0][1] = sr & 0x7;  sr >>= 3;
    xmc[0][2] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[0][3] = sr & 0x7;  sr >>= 3;
    xmc[0][4] = sr & 0x7;  sr >>= 3;
    xmc[0][5] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;                 /* 10 */
    xmc[0][6] = sr & 0x7;  sr >>= 3;
    xmc[0][7] = sr & 0x7;  sr >>= 3;
    xmc[0][8] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[0][9] = sr & 0x7;  sr >>= 3;
    xmc[0][10] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[0][11] = sr & 0x7;  sr >>= 3;
    xmc[0][12] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 4;
    Nc[1] = sr & 0x7f;  sr >>= 7;
    bc[1] = sr & 0x3;  sr >>= 2;
    Mc[1] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 1;
    xmaxc[1] = sr & 0x3f;  sr >>= 6;
    xmc[1][0] = sr & 0x7;  sr >>= 3;
    sr = *c++;                              /* 15 */
    xmc[1][1] = sr & 0x7;  sr >>= 3;
    xmc[1][2] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[1][3] = sr & 0x7;  sr >>= 3;
    xmc[1][4] = sr & 0x7;  sr >>= 3;
    xmc[1][5] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[1][6] = sr & 0x7;  sr >>= 3;
    xmc[1][7] = sr & 0x7;  sr >>= 3;
    xmc[1][8] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[1][9] = sr & 0x7;  sr >>= 3;
    xmc[1][10] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[1][11] = sr & 0x7;  sr >>= 3;
    xmc[1][12] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 4;                 /* 20 */
    Nc[2] = sr & 0x7f;  sr >>= 7;
    bc[2] = sr & 0x3;  sr >>= 2;
    Mc[2] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 1;
    xmaxc[2] = sr & 0x3f;  sr >>= 6;
    xmc[2][0] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[2][1] = sr & 0x7;  sr >>= 3;
    xmc[2][2] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[2][3] = sr & 0x7;  sr >>= 3;
    xmc[2][4] = sr & 0x7;  sr >>= 3;
    xmc[2][5] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[2][6] = sr & 0x7;  sr >>= 3;
    xmc[2][7] = sr & 0x7;  sr >>= 3;
    xmc[2][8] = sr & 0x7;  sr >>= 3;
    sr = *c++;                              /* 25 */
    xmc[2][9] = sr & 0x7;  sr >>= 3;
    xmc[2][10] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[2][11] = sr & 0x7;  sr >>= 3;
    xmc[2][12] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 4;
    Nc[3] = sr & 0x7f;  sr >>= 7;
    bc[3] = sr & 0x3;  sr >>= 2;
    Mc[3] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 1;
    xmaxc[3] = sr & 0x3f;  sr >>= 6;
    xmc[3][0] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[3][1] = sr & 0x7;  sr >>= 3;
    xmc[3][2] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;                 /* 30 */
    xmc[3][3] = sr & 0x7;  sr >>= 3;
    xmc[3][4] = sr & 0x7;  sr >>= 3;
    xmc[3][5] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[3][6] = sr & 0x7;  sr >>= 3;
    xmc[3][7] = sr & 0x7;  sr >>= 3;
    xmc[3][8] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[3][9] = sr & 0x7;  sr >>= 3;
    xmc[3][10] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[3][11] = sr & 0x7;  sr >>= 3;
    xmc[3][12] = sr & 0x7;  sr >>= 3;

    frame_chain = sr & 0xf;

    writeGSM_33(d);/* LARc etc. -> array of 33 GSM bytes */

    sr = frame_chain;
    sr |= (unsigned int) *c++ << 4;                 /* 1 */
    LARc[0] = sr & 0x3f;  sr >>= 6;
    LARc[1] = sr & 0x3f;  sr >>= 6;
    sr = *c++;
    LARc[2] = sr & 0x1f;  sr >>= 5;
    sr |= (unsigned int) *c++ << 3;
    LARc[3] = sr & 0x1f;  sr >>= 5;
    LARc[4] = sr & 0xf;  sr >>= 4;
    sr |= (unsigned int) *c++ << 2;
    LARc[5] = sr & 0xf;  sr >>= 4;
    LARc[6] = sr & 0x7;  sr >>= 3;
    LARc[7] = sr & 0x7;  sr >>= 3;
    sr = *c++;                                      /* 5 */
    Nc[0] = sr & 0x7f;  sr >>= 7;
    sr |= (unsigned int) *c++ << 1;
    bc[0] = sr & 0x3;  sr >>= 2;
    Mc[0] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 5;
    xmaxc[0] = sr & 0x3f;  sr >>= 6;
    xmc[0][0] = sr & 0x7;  sr >>= 3;
    xmc[0][1] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[0][2] = sr & 0x7;  sr >>= 3;
    xmc[0][3] = sr & 0x7;  sr >>= 3;
    xmc[0][4] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[0][5] = sr & 0x7;  sr >>= 3;
    xmc[0][6] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;                 /* 10 */
    xmc[0][7] = sr & 0x7;  sr >>= 3;
    xmc[0][8] = sr & 0x7;  sr >>= 3;
    xmc[0][9] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[0][10] = sr & 0x7;  sr >>= 3;
    xmc[0][11] = sr & 0x7;  sr >>= 3;
    xmc[0][12] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    Nc[1] = sr & 0x7f;  sr >>= 7;
    sr |= (unsigned int) *c++ << 1;
    bc[1] = sr & 0x3;  sr >>= 2;
    Mc[1] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 5;
    xmaxc[1] = sr & 0x3f;  sr >>= 6;
    xmc[1][0] = sr & 0x7;  sr >>= 3;
    xmc[1][1] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;                 /* 15 */
    xmc[1][2] = sr & 0x7;  sr >>= 3;
    xmc[1][3] = sr & 0x7;  sr >>= 3;
    xmc[1][4] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[1][5] = sr & 0x7;  sr >>= 3;
    xmc[1][6] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[1][7] = sr & 0x7;  sr >>= 3;
    xmc[1][8] = sr & 0x7;  sr >>= 3;
    xmc[1][9] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[1][10] = sr & 0x7;  sr >>= 3;
    xmc[1][11] = sr & 0x7;  sr >>= 3;
    xmc[1][12] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    Nc[2] = sr & 0x7f;  sr >>= 7;
    sr |= (unsigned int) *c++ << 1;                 /* 20 */
    bc[2] = sr & 0x3;  sr >>= 2;
    Mc[2] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 5;
    xmaxc[2] = sr & 0x3f;  sr >>= 6;
    xmc[2][0] = sr & 0x7;  sr >>= 3;
    xmc[2][1] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[2][2] = sr & 0x7;  sr >>= 3;
    xmc[2][3] = sr & 0x7;  sr >>= 3;
    xmc[2][4] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    xmc[2][5] = sr & 0x7;  sr >>= 3;
    xmc[2][6] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[2][7] = sr & 0x7;  sr >>= 3;
    xmc[2][8] = sr & 0x7;  sr >>= 3;
    xmc[2][9] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;                 /* 25 */
    xmc[2][10] = sr & 0x7;  sr >>= 3;
    xmc[2][11] = sr & 0x7;  sr >>= 3;
    xmc[2][12] = sr & 0x7;  sr >>= 3;
    sr = *c++;
    Nc[3] = sr & 0x7f;  sr >>= 7;
    sr |= (unsigned int) *c++ << 1;
    bc[3] = sr & 0x3;  sr >>= 2;
    Mc[3] = sr & 0x3;  sr >>= 2;
    sr |= (unsigned int) *c++ << 5;
    xmaxc[3] = sr & 0x3f;  sr >>= 6;
    xmc[3][0] = sr & 0x7;  sr >>= 3;
    xmc[3][1] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[3][2] = sr & 0x7;  sr >>= 3;
    xmc[3][3] = sr & 0x7;  sr >>= 3;
    xmc[3][4] = sr & 0x7;  sr >>= 3;
    sr = *c++;                                      /* 30 */
    xmc[3][5] = sr & 0x7;  sr >>= 3;
    xmc[3][6] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 2;
    xmc[3][7] = sr & 0x7;  sr >>= 3;
    xmc[3][8] = sr & 0x7;  sr >>= 3;
    xmc[3][9] = sr & 0x7;  sr >>= 3;
    sr |= (unsigned int) *c++ << 1;
    xmc[3][10] = sr & 0x7;  sr >>= 3;
    xmc[3][11] = sr & 0x7;  sr >>= 3;
    xmc[3][12] = sr & 0x7;  sr >>= 3;
    writeGSM_33(d+33);
}
