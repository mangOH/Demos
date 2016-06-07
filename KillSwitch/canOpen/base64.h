/*	-- $Header: P:/source/c/RCS/base64.h 1.3 2009/02/18 10:06:29 saturn Sav $ --
 *
 *	projekt   :  UV Software.
 *
 *	purpose   :  Base64 - Data Encoding.
 *
 *	copyright :  (c) 2009, UV Software, Friedrichshafen.
 *
 *	compiler  :  Any ANSI-C Compiler.
 *
 *	export    :  void base64_encode(unsigned char *input, int length, unsigned char *output, int nbyte);
 *	             void base64_decode(unsigned char *input, int length, unsigned char *output, int nbyte);
 *
 *	includes  :  (none)
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen.
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	Base64 data encoding used in MIME (Multipurpose Internet Mail Extensions). 
 *
 *
 *	|     byte1     |     byte2     |     byte3     |
 *	+---------------+---------------+---------------+
 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *	+---------------+---------------+---------------+
 *	'           '           '           '           '
 *	'           '           '           '           '
 *	'           '           '           '           '
 *	+-----------+-----------+-----------+-----------+
 *	|5|4|3|2|1|0|5|4|3|2|1|0|5|4|3|2|1|0|5|4|3|2|1|0|
 *	+-----------+-----------+-----------+-----------+
 *	|   char1   |   char2   |   char3   |   char4   |
 *
 *
 *	|  Base64 encoding table                        |
 *	+-----------+-----------+-----------+-----------+
 *	|  0  = 'A' |  16 = 'Q' |  32 = 'g' |  48 = 'w' |
 *	|  1  = 'B' |  17 = 'R' |  33 = 'h' |  49 = 'x' |
 *	|  2  = 'C' |  18 = 'S' |  34 = 'i' |  50 = 'y' |
 *	|  3  = 'D' |  19 = 'T' |  35 = 'j' |  51 = 'z' |
 *	|  4  = 'E' |  20 = 'U' |  36 = 'k' |  52 = '0' |
 *	|  5  = 'F' |  21 = 'V' |  37 = 'l' |  53 = '1' |
 *	|  6  = 'G' |  22 = 'W' |  38 = 'm' |  54 = '2' |
 *	|  7  = 'H' |  23 = 'X' |  39 = 'n' |  55 = '3' |
 *	|  8  = 'I' |  24 = 'Y' |  30 = 'o' |  56 = '4' |
 *	|  9  = 'J' |  25 = 'Z' |  41 = 'p' |  57 = '5' |
 *	|  10 = 'K' |  26 = 'a' |  42 = 'q' |  58 = '6' |
 *	|  11 = 'L' |  27 = 'b' |  43 = 'r' |  59 = '7' |
 *	|  12 = 'M' |  28 = 'c' |  44 = 's' |  60 = '8' |
 *	|  13 = 'N' |  29 = 'd' |  45 = 't' |  61 = '9' |
 *	|  14 = 'O' |  30 = 'e' |  46 = 'u' |  62 = '+' |
 *	|  15 = 'P' |  31 = 'f' |  47 = 'v' |  63 = '/' |
 *	+-----------+-----------+-----------+-----------+
 *
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log: base64.h $
 *	Revision 1.3  2009/02/18 10:06:29  saturn
 *	The functions now return the number of bytes/chars in the output buffer.
 *
 *	Revision 1.2  2009/02/05 15:07:14  vogt
 *	An annoying waring eliminated.
 *
 *	Revision 1.1  2009/01/30 20:08:24  vogt
 *	Initial revision
 *
 */

#ifndef __BASE64_H
#define __BASE64_H


/*  -----------  prototypes  -----------------------------------------------
 */

int base64_encode(unsigned char *input, int length, unsigned char *output, int nbyte);
int base64_decode(unsigned char *input, int length, unsigned char *output, int nbyte);

char* base64_version();
/*
 *	function  :  retrieve RCS info of this module as a string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to RCS info (zero-terminated string)
 */

#endif	// __BASE64_H

/*	-------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
