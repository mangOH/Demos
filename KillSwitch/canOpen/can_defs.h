/*	-- $Header$ --
 *
 *	projekt   :  CAN - Controller Area Network
 *
 *	purpose   :  Definitions and Options (socketCAN)
 *
 *	copyright :  (c) 2007, UV Software, Friedrichshafen
 *
 *	compiler  :  GCC - GNU C Compiler (Linux Kernel 2.6)
 *
 *	export    :  (see below)
 *
 *	includes  :  (none)
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	CAN Definitions and Options.
 *
 *	For berliOS socketCAN (v2.17) over PEAK PCAN-USB Dongle. For information
 *	about socketCAN see "http://socketcan.berlios.de/".
 * 
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log$
 */

#ifndef __CAN_DEFS_H
#define __CAN_DEFS_H


/*  -----------  options  --------------------------------------------------
 */

#define _CAN_EVENT_QUEUE				// Receiver Queue for Event-handling


/*  -----------  defines  --------------------------------------------------
 */

#ifndef _CAN_DEFS						// socketCAN Interfaces

 #define CAN_NETDEV				(-1L)	//   It´s a network device
 
 struct _can_param						//   Installation parameters:
 {
 	char* ifname;						//     Interface name
 	int   family;						//     Protocol family
 	int   type;							//     Communication semantics
 	int   protocol;						//     Protocol to be used with the socket
 };
 #define CANERR_SOCKET			(-10000)//   socketCAN error (variable 'errno' is set)

 #define CAN_TRM_QUEUE_SIZE	  	  65536	//   Größe der Transmit-Queue
 #define CAN_RCV_QUEUE_SIZE	  	  65536	//   Größe der Receive-Queue
 #define CAN_RCV_QUEUE_READ			100	//   Einträge aus der Receive-Queue lesen
 #define CAN_EVENT_QUEUE_SIZE	  16384 //   Größe der Event-Queue (message object 14)
#endif

/*  -----------  useful stuff  ---------------------------------------------
 */

/* *** **
typedef int					BOOL;
#define FALSE				0
#define TRUE				1
#define OK					0

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;

typedef char				CHAR;
typedef short				SHORT;
typedef long				LONG;

typedef char*           	PSTR;
typedef char*           	NPSTR;

typedef char*           	LPSTR;
typedef const char*     	LPCSTR;
typedef BYTE*           	PBYTE;
typedef BYTE*           	LPBYTE;
typedef int*            	PINT;
typedef int*            	LPINT;
typedef WORD*           	PWORD;
typedef WORD*           	LPWORD;
typedef long*           	PLONG;
typedef long*           	LPLONG;
typedef DWORD*          	PDWORD;
typedef DWORD*          	LPDWORD;
typedef void*           	LPVOID;

#define BYTES2WORD(low,high)	(((unsigned int)((unsigned char)(low)))  << 0) | \
				(((unsigned int)((unsigned char)(high))) << 8)
#define WORDS2DWORD(low,high)	(((unsigned long)((unsigned int)(low)))  << 0) | \
				(((unsigned long)((unsigned int)(high))) << 16)
#define BYTES2DWORD(ll,lh,hl,hh)(((unsigned long)((unsigned char)(ll)))  << 0) | \
				(((unsigned long)((unsigned char)(lh))) << 8) | \
				(((unsigned long)((unsigned char)(hl))) << 16) | \
				(((unsigned long)((unsigned char)(hh))) << 24)

#define BYTEV2WORD(array)	(((unsigned int)((unsigned char)(array[0]))) << 0) | \
				(((unsigned int)((unsigned char)(array[1]))) << 8)
#define BYTEV2DWORD(array)	(((unsigned long)((unsigned char)(array[0]))) << 0) | \
				(((unsigned long)((unsigned char)(array[1]))) << 8) | \
				(((unsigned long)((unsigned char)(array[2]))) << 16) | \
				(((unsigned long)((unsigned char)(array[3]))) << 24)
** *** */

/*  -----------  types  ----------------------------------------------------
 */


/*  -----------  variables  ------------------------------------------------
 */


/*  -----------  prototypes  -----------------------------------------------
 */


#endif      // __CAN_DEFS_H

/*  -------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047470, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
