/* API to use string hashes for keywords in place of strcmp()
 *
 *  cw_hash.c
 *  CallWeaver Keywords
 *
 * Hash functions
 *
 * Author: Benjamin Kowarsch <benjamin at sunrise dash tel dot com>
 *
 * (C) 2006 Sunrise Telephone Systems Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. Where software
 * packages making use of this software have a command line interface, the
 * above copyright notice and a reference to the license terms must be
 * displayed whenever the command line interface is invoked. Where software
 * packages making use of this software have a graphical user interface, the
 * above copyright notice and a reference to the license terms must be
 * displayed in the application's "about this software" window or equivalent.
 * Installers which install this software must display the above copyright
 * notice and these license terms in full.
 *
 * Under no circumstances is it permitted for any licensee to take credit for
 * the creation of this software, to claim authorship or ownership in this
 * software or in any other way give the impression that they have created
 * this software. Credit to the true authors and rights holders of this
 * software is absolutely mandatory and must be given at all times.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * In countries and territories where the above no-warranty disclaimer is
 * not permissible by applicable law, the following terms apply:
 *
 * NO PERMISSION TO USE THE SOFTWARE IS GRANTED AND THE SOFTWARE MUST NOT BE
 * USED AT ALL IN SUCH COUNTRIES AND TERRITORIES WHERE THE ABOVE NO-WARRANTY
 * DISCLAIMER IS NOT PERMISSIBLE AND INVALIDATED BY APPLICABLE LAW. HOWEVER,
 * THE COPYRIGHT HOLDERS HEREBY WAIVE THEIR RIGHT TO PURSUE OFFENDERS AS LONG
 * AS THEY OTHERWISE ABIDE BY THE TERMS OF THE LICENSE AS APPLICABLE FOR USE
 * OF THE SOFTWARE IN COUNTRIES AND TERRITORIES WHERE THE ABOVE NO-WARRANTY
 * DISCLAIMER IS PERMITTED BY APPLICABLE LAW. THIS WAIVER DOES NOT CONSTITUTE
 * A LICENSE TO USE THE SOFTWARE IN COUNTRIES AND TERRITORIES WHERE THE ABOVE
 * NO-WARRANTY DISCLAIMER IS NOT PERMISSIBLE AND INVALIDATED BY APPLICABLE
 * LAW. ANY LIABILITY OF ANY KIND IS CATEGORICALLY RULED OUT AT ALL TIMES.
 */


#include <string.h>
#include "callweaver/callweaver_hash.h"


// --------------------------------------------------------------------------
// function:  cw_hash_string(string)
// --------------------------------------------------------------------------
//
// Returns the hash value of the null terminated C string 'string' using the
// SDBM hash algorithm. The number of significant characters for which the
// hash value will be calculated is limited to CWHASH_SIGNIFICANT_CHARS.
// Returns 0 if 'string' is a zero-length string or NULL.

unsigned int cw_hash_string(const char *string) {
	register unsigned int len, index, hash = 0;
	register char ch;
	
	if (string == NULL)
		return 0;

	len = strlen(string);
	if (len > CWHASH_SIGNIFICANT_CHARS) {
		len = CWHASH_SIGNIFICANT_CHARS;
	} // end if
	
	// PUBLIC DOMAIN ALGORITHM FOLLOWS
	for (index = 0; index < len; index++) {
		ch = string[index];
		hash = ch + (hash << 6) + (hash << 16) - hash;
	} // end for
	
	return (hash & 0x7FFFFFFF);
} // end cw_hash_string


// --------------------------------------------------------------------------
// function:  cw_hash_string_tolower(string)
// --------------------------------------------------------------------------
//
// Returns the hash value of the null terminated C string 'string' using the
// SDBM hash algorithm after converting 'string' to its lowercase equivalent.
// Case conversion is limited to characters in the ASCII range 'A' to 'Z'.
// The number of significant characters for which the hash value will be
// calculated is limited to CWHASH_SIGNIFICANT_CHARS.
// Returns 0 if 'string' is a zero-length string or NULL.

unsigned int cw_hash_string_tolower(const char *string) {
	register unsigned int len, index, hash = 0;
	register char ch;
	
	if (string == NULL)
		return 0;

	len = strlen(string);
	if (len > CWHASH_SIGNIFICANT_CHARS) {
		len = CWHASH_SIGNIFICANT_CHARS;
	} // end if
	
	for (index = 0; index < len; index++) {
		ch = string[index];
		// checking for ch <= 'Z' first is faster
		if /* UPPERCASE CHARACTER */ ((ch <= 'Z') && (ch >= 'A')) {
			ch = ch + 32;
		} // end if
		hash = ch + (hash << 6) + (hash << 16) - hash;
	} // end for
	
	return (hash & 0x7FFFFFFF);
} // end cw_hash_string_tolower


// --------------------------------------------------------------------------
// function:  cw_hash_string_toupper(string)
// --------------------------------------------------------------------------
//
// Returns the hash value of the null terminated C string 'string' using the
// SDBM hash algorithm after converting 'string' to its uppercase equivalent.
// Case conversion is limited to characters in the ASCII range 'a' to 'z'.
// The number of significant characters for which the hash value will be
// calculated is limited to CWHASH_SIGNIFICANT_CHARS.
// Returns 0 if 'string' is a zero-length string or NULL.

unsigned int cw_hash_string_toupper(const char *string) {
	register unsigned int len, index, hash = 0;
	register char ch;
	
	if (string == NULL)
		return 0;

	len = strlen(string);
	if (len > CWHASH_SIGNIFICANT_CHARS) {
		len = CWHASH_SIGNIFICANT_CHARS;
	} // end if
	
	for (index = 0; index < len; index++) {
		ch = string[index];
		if /* LOWERCASE CHARACTER */ ((ch >= 'a') && (ch <= 'z')) {
			ch = ch - 32;
		} // end if
		hash = ch + (hash << 6) + (hash << 16) - hash;
	} // end for
	
	return (hash & 0x7FFFFFFF);
} // end cw_hash_string_toupper


// --------------------------------------------------------------------------
// function:  cw_hash_string_with_limit(string, limit)
// --------------------------------------------------------------------------
//
// Returns the hash value of the null terminated C string 'string' using the
// SDBM hash algorithm. The number of significant characters for which the
// hash value will be calculated is limited to 'limit'.
// Returns 0 if 'string' is a zero-length string or NULL.

unsigned int cw_hash_string_with_limit(const char *string, unsigned int limit) {
	register unsigned int len, index, hash = 0;
	register char ch;

	if (string == NULL)
		return 0;
	
	len = strlen(string);
	if (len > limit) {
		len = limit;
	} // end if
	
	// PUBLIC DOMAIN ALGORITHM FOLLOWS
	for (index = 0; index < len; index++) {
		ch = string[index];
		hash = ch + (hash << 6) + (hash << 16) - hash;
	} // end for
	
	return (hash & 0x7FFFFFFF);
} // end cw_hash_string_with_limit


// END OF FILE
