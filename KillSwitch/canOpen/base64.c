/*	-- $Header: P:/source/c/RCS/base64.c 1.3 2009/02/18 10:06:29 saturn Sav $ --
 *
 *	projekt   :  UV Software.
 *
 *	purpose   :  Base64 - Data Encoding.
 *
 *	copyright :  (c) 2009, UV Software, Friedrichshafen.
 *
 *	compiler  :  Any ANSI-C Compiler.
 *
 *	export    :  (see header file)
 *
 *	includes  :  base64.h
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	Base64 data encoding used in MIME (Multipurpose Internet Mail Extensions). 
 *
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log: base64.c $
 *	Revision 1.3  2009/02/18 10:06:29  saturn
 *	The functions now return the number of bytes/chars in the output buffer.
 *
 *	Revision 1.2  2009/02/05 15:02:06  vogt
 *	An annoying waring eliminated.
 *
 *	Revision 1.1  2009/01/30 20:10:20  vogt
 *	Initial revision
 *
 */

static char _id[] = "$Id: base64.c 1.3 2009/02/18 10:06:29 saturn Sav $";


/*  -----------  includes  -------------------------------------------------
 */

#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*  -----------  defines  --------------------------------------------------
 */

#define BYTE64(x) ((x) >= 'a'? (x)+26-'a' : \
                  ((x) >= 'A'? (x)-'A' : \
                  ((x) >= '0'? (x)+52-'0' : \
                  ((x) == '+'? 62 : \
                  ((x) == '/'? 63 : 0)))))
#define BASE64(x) ('0' <= (x) && (x) <= '9') || \
                  ('a' <= (x) && (x) <= 'z') || \
                  ('A' <= (x) && (x) <= 'Z') || \
                  ((x) == '+') || \
                  ((x) == '/')

/*  -----------  types  ----------------------------------------------------
 */


/*  -----------  prototypes  -----------------------------------------------
 */


/*  -----------  variables  ------------------------------------------------
 */

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/*  -----------  functions  ------------------------------------------------
 */

int base64_encode(unsigned char *input, int length, unsigned char *output, int nbyte)
{
	int n = nbyte;
	
	if(nbyte > 3) {
		output[3] = (unsigned char)(length > 2 ? base64[input[2] & 0x3F] : '=');
		n = length > 2 ? n : 3;
	}
	if(nbyte > 2) {
		output[2] = (unsigned char)(length > 1 ? base64[((input[1] & 0x0F) << 2) | ((input[2] & 0xC0) >> 6)] : '=');
		n = length > 1 ? n : 2;
	}
	if(nbyte > 1) {
		output[1] = (unsigned char)(length > 0 ? base64[((input[0] & 0x03) << 4) | (length > 1 ? ((input[1] & 0xF0) >> 4) : 0)] : '=');
		n = length > 0 ? n : 1;
	}
	if(nbyte > 0) {
		output[0] = (unsigned char)(length > 0 ? base64[input[0] >> 2] : '=');
		n = length > 0 ? n : 0;
	}
	return n;
}

int base64_decode(unsigned char *input, int length, unsigned char *output, int nbyte)
{
	int n = 0;
	
	if(nbyte > 0) {
		n += ((length > 0) && BASE64(input[0])) ? 1 : 0;
		output[0] = (unsigned char)((length > 0 ? BYTE64(input[0]) << 2 : 0) | (length > 1 ? BYTE64(input[1]) >> 4 : 0));
	}
	if(nbyte > 1) {
		n += ((length > 1) && BASE64(input[1])) ? ((length == 2) && ((BYTE64(input[1]) & 0x0F) == 0x00) ? 0 : 1) : 0;
		output[1] = (unsigned char)((length > 1 ? BYTE64(input[1]) << 4 : 0) | (length > 2 ? BYTE64(input[2]) >> 2 : 0));
	}
	if(nbyte > 2) {
		n += ((length > 2) && BASE64(input[2])) ? ((length == 3) && ((BYTE64(input[2]) & 0x03) == 0x00) ? 0 : 1) : 0;
		output[2] = (unsigned char)((length > 2 ? BYTE64(input[2]) << 6 : 0) | (length > 3 ? BYTE64(input[3]) : 0));
	}
	return n;
}

/*  -----------  revision control  -----------------------------------------
 */

char* base64_version()
{
	return (char*)_id;
}

/*	-------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
