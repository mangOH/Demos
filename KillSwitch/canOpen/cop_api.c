/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_api.c 1.3 2009/02/05 13:04:22 saturn Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *				 
 *	Zweck     :  CANopen Master API - Data Link Layer.
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
 *	CANopen Master API - Data Link Layer.
 *
 *		Implements the CANopen Data Link Layer for accessing the CAN Controller
 *		and common communication functions for the CANopen Master interface.
 *
 *
 *	-----------  Änderungshistorie  ------------------------------------------
 *
 *	$Log: cop_api.c $
 *	Revision 1.3  2009/02/05 13:04:22  saturn
 *	Ported to the linux gcc environment.
 *
 *	Revision 1.2  2005/10/24 13:12:13  vogt
 *	Function cop_status returns the error code of the last operation,
 *	except it fails while reading the CAN Controller status register.
 *
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */

#ifdef _DEBUG
 static char _id[] = "$Id: cop_api.c 18 2009-02-07 14:53:44Z mars $ _DEBUG";
#else
 static char _id[] = "$Id: cop_api.c 18 2009-02-07 14:53:44Z mars $";
#endif

/*	-----------  Include-Dateien  --------------------------------------------
 */

#include "cop_api.h"					// Interface prototypes
#include "can_ctrl.h"					// CAN Controller interface

#include <stdio.h>						// Standard I/O routines
#include <errno.h>						// System wide error numbers
#include <string.h>						// String manipulation functions
#include <stdlib.h>						// Commonly used library functions
#ifdef __linux__
#include <unistd.h>						// POSIX standard: symbolic constants
#endif

/*	-----------  Definitionen  -----------------------------------------------
 */


/*	-----------  Typen  ------------------------------------------------------
 */


/*	-----------  Prototypen  -------------------------------------------------
 */


/*	-----------  Variablen  --------------------------------------------------
 */

BYTE cop_baudrate = CANBDR_20;			// actual baudrate
LONG cop_error = CANERR_NOERROR;		// last error code
BYTE cop_buffer[8] = {0,0,0,0,0,0,0};	// data buffer (8)


/*	-----------  Funktionen  -------------------------------------------------
 */

LONG cop_init(LONG board, LPVOID param, BYTE baudrate)
{
	// 1. Exit CAN if initialized or running
	if((cop_error = can_exit()) != CANERR_NOERROR)
	{
		//return cop_error;
	}
	// 2. Init CAN, operation status = initialized
	if((cop_error = can_init(board, param)) != CANERR_NOERROR)
	{
		can_exit();						// on error: exit CAN
		return cop_error;
	}
	// 3. Start CAN, operation status = running
	if((cop_error = can_start(baudrate)) != CANERR_NOERROR)
	{
		can_reset();					// on error: reset CAN
		can_exit();						//           exit CAN
		return cop_error;
	}
	cop_baudrate = baudrate;			// actual baudrate
	return cop_error;
}

LONG cop_exit()
{
	// Exit CAN, operation status = stopped
	return cop_error = can_exit();
}

LONG cop_reset(BYTE baudrate)
{
	// 1. Reset CAN, operation status = stopped
	if((cop_error = can_reset()) != CANERR_NOERROR)
	{
		return cop_error;
	}
	// 2. Wait 100 milliseconds to continue
#if defined(_WIN32)
	Sleep(100);
#elif defined(__linux__)
	usleep(100000);
#else
	can_start_timer(100); while(!can_is_timeout()) {}
#endif
	// 3. Start CAN, operation status = running
	if((cop_error = can_start(baudrate)) != CANERR_NOERROR)
	{
		can_reset();					// on error: reset CAN
		return cop_error;
	}
	cop_baudrate = baudrate;			// actual baudrate
	return cop_error;
}

LONG cop_transmit(LONG cob_id, SHORT length, BYTE *data)
{
	// 1. Configure transmit message object
	if((cop_error = can_config(CANBUF_TX, cob_id, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Transmit the message
	if((cop_error = can_transmit(CANBUF_TX, length, data)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// Return if the message is transmitted
	can_delete(CANBUF_TX);
	return cop_error;
}

LONG cop_request(LONG cob_id, SHORT *length, BYTE *data)
{
	WORD timeout[9] = {2,2,2,2,2,2,5,10,20};
	BYTE dlc = (BYTE)*length;

	// 1. Configure receive message object with remote request
	if((cop_error = can_config(CANBUF_RX, cob_id, (WORD)CAN_REQUEST(dlc))) != CANERR_NOERROR) {
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Start timer (increased time-out for RTR-frames)
	can_start_timer((WORD)(timeout[cop_baudrate] * CANRTR_FACTOR));

	// 3. Wait until message is received
	do {
		// Return if a message is received, or an error occurred
		if((cop_error = can_receive(CANBUF_RX, length, data)) != COPERR_RX_EMPTY) {
			can_delete(CANBUF_RX);
			return cop_error;
		}
	} while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

LONG cop_status(BYTE *status, BYTE *load)
{
	short rc;							// return value

	// CAN controller status-register and bus-load
	if((rc = can_busload(load, status)) != CANERR_NOERROR)
		return cop_error = rc;
	// Last error code
	return cop_error;
}

LONG cop_queue_read(LONG *cob_id, SHORT *length, BYTE *data)
{
	// Read one message from the event-queue
	return cop_error = can_queue_get_message(cob_id, length, data);
}

LONG cop_queue_clear(void)
{
	// Clear the event-queue
	return cop_error = can_queue_clear();
}

LONG cop_queue_status(BYTE *status, BYTE *load)
{
	// CAN status and bus-load
	cop_error = can_busload(load, status);
	// Status of the event-queue
	return can_queue_status();
}

LPSTR cop_hardware(void)
{
	// Hardware version
	return can_hardware();
}

LPSTR cop_software(void)
{
	// Firmware version
	return can_software();
}

LPSTR cop_version(void)
{
	// Revision number
	return (LPSTR)_id;
}

/*	--------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
