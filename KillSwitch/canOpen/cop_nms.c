/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_nms.c 1.2 2009/02/05 13:05:46 saturn Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *				 
 *	Zweck     :  CANopen Master NMS - Network Management Services.
 *
 *	Copyright :  (c) 2005-2009 by UV Software, Friedrichshafen.
 *
 *	Compiler  :  Microsoft Visual C/C++ Compiler (Version 6.0)
 *
 *	Export    :  (siehe Header-Datei)
 *
 *	Include   :  cop_api.h (can_defs.h, windows.h), can_ctrl.h
 *
 *	Autor     :  Uwe Vogt, UV Software.
 *
 *	E-Mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  Modulbeschreibung  ------------------------------------------
 *
 *	CANopen Master NMS - Network Management Services.
 *
 *		Implements the Network Management Services and Protocols (NMS)
 *		according to CiA DS-301 (Version 4.02 of February 13, 2002).
 *
 *		Broadcast NMT Commands:
 *		- Start Remote Node
 *		- Stop Remote Node
 *		- Enter Preoperational State
 *		- Reset Node (Application)
 *		- Reset Communication
 *
 *
 *	-----------  Änderungshistorie  ------------------------------------------
 *
 *	$Log: cop_nms.c $
 *	Revision 1.2  2009/02/05 13:05:46  saturn
 *	Some warning eliminated for linux gcc.
 *
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */


/*	-----------  Include-Dateien  --------------------------------------------
 */

#include "cop_api.h"					// Interface prototypes
#include "can_ctrl.h"					// CAN Controller interface

#include <stdio.h>						// Standard I/O routines
#include <errno.h>						// System wide error numbers
#include <string.h>						// String manipulation functions
#include <stdlib.h>						// Commonly used library functions


/*	-----------  Definitionen  -----------------------------------------------
 */


/*	-----------  Typen  ------------------------------------------------------
 */


/*	-----------  Prototypen  -------------------------------------------------
 */


/*	-----------  Variablen  --------------------------------------------------
 */

extern BYTE cop_baudrate;				// actual baudrate
extern LONG cop_error;					// last error code
extern BYTE cop_buffer[8];				// data buffer (8)


/*	-----------  Funktionen  -------------------------------------------------
 */

LONG nmt_start_remote_node(BYTE node_id)
{
	if(/*node_id < 0 ||*/ 127 < node_id)	// node-id: 1,..,127
		return cop_error = COPERR_NODE_ID;	//  0 for all nodes
	
	// ---  NMT Start Remote Node  ---
	cop_buffer[0] = (BYTE)0x01;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id

	return cop_error = cop_transmit(NMT_MASTER, 2, cop_buffer);
}

LONG nmt_stop_remote_node(BYTE node_id)
{
	if(/*node_id < 0 ||*/ 127 < node_id)	// node-id: 1,..,127
		return cop_error = COPERR_NODE_ID;	//  0 for all nodes
	
	// ---  NMT Stop Remote Node  ---
	cop_buffer[0] = (BYTE)0x02;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id

	return cop_error = cop_transmit(NMT_MASTER, 2, cop_buffer);
}

LONG nmt_enter_preoperational(BYTE node_id)
{
	if(/*node_id < 0 ||*/ 127 < node_id)	// node-id: 1,..,127
		return cop_error = COPERR_NODE_ID;	//  0 for all nodes
	
	// ---  NMT Enter Preoperational  ---
	cop_buffer[0] = (BYTE)0x80;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id

	return cop_error = cop_transmit(NMT_MASTER, 2, cop_buffer);
}

LONG nmt_reset_node(BYTE node_id)
{
	if(/*node_id < 0 ||*/ 127 < node_id)	// node-id: 1,..,127
		return cop_error = COPERR_NODE_ID;	//  0 for all nodes
	
	// ---  NMT Reset Node  ---
	cop_buffer[0] = (BYTE)0x81;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id

	return cop_error = cop_transmit(NMT_MASTER, 2, cop_buffer);
}

LONG nmt_reset_communication(BYTE node_id)
{
	if(/*node_id < 0 ||*/ 127 < node_id)	// node-id: 1,..,127
		return cop_error = COPERR_NODE_ID;	//  0 for all nodes
	
	// ---  NMT Reset Communication  ---
	cop_buffer[0] = (BYTE)0x82;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id

	return cop_error = cop_transmit(NMT_MASTER, 2, cop_buffer);
}

/*	--------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
