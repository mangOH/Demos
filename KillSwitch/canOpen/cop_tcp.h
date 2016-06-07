/*	-- $Header$ --
 *
 *	projekt   :  CAN - Controller Area Network.
 *
 *	purpose   :  Interfacing CANopen with TCP/IP.
 *
 *	copyright :  (C) 2009, UV Software, Friedrichshafen.
 *
 *	compiler  :  GCC - GNU C Compiler (Linux Kernel 2.6)
 *
 *	export    :  <export>
 *
 *	includes  :  default.h
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	CANopen Master - Interfacing CANopen with TCP/IP (ASCII Mapping).
 *
 *		Implements the ASCII-based communication syntax for CANopen gateway
 *		devices to interface CANopen networks to a TCP/IP-based network
 *		according to CiA DS-309 (Part 1 and 3, Versioon 1.1 of 2006).
 *
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log$
 */

#ifndef __COP_TCP_H
#define __COP_TCP_H


/*  -----------  includes  -------------------------------------------------
 */

#ifndef WIN32
 #include "default.h"					// Syntax extensions for C/C++
#else
 #include <windows.h>					// Master include file for Windows
#endif
#include <stdio.h>						// Standard I/O routines


/*  -----------  defines  --------------------------------------------------
 */

typedef struct _cop_tcp_settings		/* settings for the gateway: */
{
	BYTE net;							/*   default network number */
	BYTE node;							/*   default node-id */
	BYTE master;						/*   node-id of the master */
}	COP_TCP_SETTINGS;

/*  -----------  types  ----------------------------------------------------
 */


/*  -----------  variables  ------------------------------------------------
 */


/*  -----------  prototypes  -----------------------------------------------
 */

int cop_tcp_parse(char *request, COP_TCP_SETTINGS *settings, char *response, int nbyte);
/*
 *	function  :  ...
 *
 *	parameter :  request  - ...
 *               settings - ... 
 *               response - ... 
 *               nbyte    - ... 
 *
 *	result    :  0 if successful, or a negative value on error. 
 */

int cop_tcp_sequence(char *string, unsigned long *sequence);
/*
 *	function  :  ...
 *
 *	parameter :  string   - ...
 *               sequence - ... 
 *
 *	result    :  0 if successful, or a negative value on error. 
 */

void cop_tcp_syntax(FILE *stream);
/*
 *	function  :  ...
 *
 *	parameter :  stream - ...
 *
 *	result    :  (none) 
 */

char* cop_tcp_version();
/*
 *	function  :  retrieve RCS info of this module as a string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to RCS info (zero-terminated string)
 */


#endif	// __COP_TCP_H

/*  -------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
