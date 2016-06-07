/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_sdo.c 1.3 2006/10/05 10:26:32 vogt Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *				 
 *	Zweck     :  CANopen Master SDO - Service Data Object.
 *
 *	Copyright :  (c) 2005-2006 by UV Software, Friedrichshafen.
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
 *	CANopen Master SDO - Service Data Object.
 *
 *		Implements the Service Data Object Services and Protocols (SDO)
 *		according to CiA DS-301 (Version 4.02 of February 13, 2002).
 *
 *		SDO-Download Protocol (write obejct value)
 *		- Expedited Transfer for data less or equal 4 byte
 *		- Segmented Transfer for data greater than 4 byte
 *		SDO-Upload Protocol (read object value)
 *		- Expedited Transfer for data less or equal 4 byte
 *		- Segmented Transfer for data greater than 4 byte
 *		Special Functions
 *		- Read/Write an 8-bit value (expedited transfer)
 *		- Read/Write a 16-bit value (expedited transfer)
 *		- Read/Write a 32-bit value (expedited transfer)
 *
 *
 *	-----------  Änderungshistorie  ------------------------------------------
 *
 *	$Log: cop_sdo.c $
 *	Revision 1.3  2006/10/05 10:26:32  vogt
 *	The multiplexor is checked now with the SDO confirmation.
 *	If the multiplexor did not match the frame is skipped.
 *
 *	Revision 1.2  2005/10/22 13:47:16  vogt
 *	A bug with the SDO Segmented Download protocol fixed.
 *
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */

#ifdef _DEBUG
 static char _id[] = "$Id: cop_sdo.c 18 2009-02-07 14:53:44Z mars $ _DEBUG";
#else
 static char _id[] = "$Id: cop_sdo.c 18 2009-02-07 14:53:44Z mars $";
#endif

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

#ifdef _WINDEF_							// F*cking great!!!
 #undef LOBYTE
 #undef HIBYTE
 #undef LOWORD
 #undef HIWORD
#endif
#ifndef  LOBYTE
 #define LOBYTE(value)					*( (unsigned char*) &value)
#endif
#ifndef  HIBYTE
 #define HIBYTE(value)					*(((unsigned char*) &value) + 1)
#endif
#ifndef  LOWORD
 #define LOWORD(value)					*( (unsigned short*) &value)
#endif
#ifndef  HIWORD
 #define HIWORD(value)					*(((unsigned short*) &value) + 1)
#endif
#ifndef  LOLOBYTE
 #define LOLOBYTE(value)				*( (unsigned char*) &value)
#endif
#ifndef  LOHIBYTE
 #define LOHIBYTE(value)				*(((unsigned char*) &value) + 1)
#endif
#ifndef  HILOBYTE
 #define HILOBYTE(value)				*(((unsigned char*) &value) + 2)
#endif
#ifndef  HIHIBYTE
 #define HIHIBYTE(value)				*(((unsigned char*) &value) + 3)
#endif

/*	-----------  Typen  ------------------------------------------------------
 */


/*	-----------  Prototypen  -------------------------------------------------
 */

static LONG sdo_expedited(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data);
static LONG sdo_segmented(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data);
static LONG sdo_receive(BYTE node_id, WORD index, BYTE subindex, SHORT *length, BYTE *data, SHORT max);


/*	-----------  Variablen  --------------------------------------------------
 */

extern BYTE cop_baudrate;				// actual baudrate
extern LONG cop_error;					// last error code
extern BYTE cop_buffer[8];				// data buffer (8)
static WORD cop_timeout = SDO_TIMEOUT;	// time-out value


/*	-----------  Funktionen  -------------------------------------------------
 */

LONG sdo_write(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data)
{
	if(node_id < 1 || 127 < node_id)	// node-id: 1,..,127?
		return cop_error = COPERR_NODE_ID;
	if(data == NULL)					// null pointer assignment?
		return cop_error = COPERR_FATAL;
	if(length > 4)						// segmented SDO protocol
		return sdo_segmented(node_id, index, subindex, length, data);
	else								// expedited SDO protocol
		return sdo_expedited(node_id, index, subindex, length, data);
}

LONG sdo_read(BYTE node_id, WORD index, BYTE subindex, SHORT *length, BYTE *data, SHORT max)
{
	if(node_id < 1 || 127 < node_id)	// node-id: 1,..,127?
		return cop_error = COPERR_NODE_ID;
	if(length == NULL || data == NULL)	// null pointer assignment?
		return cop_error = COPERR_FATAL;
	return sdo_receive(node_id, index, subindex, length, data, max);
}

WORD sdo_timeout(WORD milliseconds)
{
	WORD last_value = cop_timeout;		// copy old time-out value
	cop_timeout = milliseconds;			// set new time-out value
	return last_value;					// return old time-out value
}

LONG sdo_write_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE value)
{
	BYTE buffer[1];
	buffer[0] = (BYTE)(value);
	return sdo_write(node_id, index, subindex, 1, buffer);
}

LONG sdo_read_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE *value)
{
	short length;
	if(sdo_read(node_id, index, subindex, &length, value, 1) == CANERR_NOERROR) {
		if(length != 1)
			cop_error = COPERR_LENGTH;
	}
	return cop_error;
}

LONG sdo_write_16bit(BYTE node_id, WORD index, BYTE subindex, WORD value)
{
	BYTE buffer[2];
	buffer[0] = LOBYTE(value);
	buffer[1] = HIBYTE(value);
	return sdo_write(node_id, index, subindex, 2, buffer);
}

LONG sdo_read_16bit(BYTE node_id, WORD index, BYTE subindex, WORD *value)
{
	short length; BYTE buffer[2];
	if(sdo_read(node_id, index, subindex, &length, buffer, 2) == CANERR_NOERROR) {
		if(length != 2)
			return cop_error = COPERR_LENGTH;
		if(value == NULL)
			return cop_error = COPERR_FATAL;
		LOBYTE(*value) = buffer[0];
		HIBYTE(*value) = buffer[1];
	}
	return cop_error;
}

LONG sdo_write_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD value)
{
	BYTE buffer[4];
	buffer[0] = LOLOBYTE(value);
	buffer[1] = LOHIBYTE(value);
	buffer[2] = HILOBYTE(value);
	buffer[3] = HIHIBYTE(value);
	return sdo_write(node_id, index, subindex, 4, buffer);
}

LONG sdo_read_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD *value)
{
	short length; BYTE buffer[4];
	if(sdo_read(node_id, index, subindex, &length, buffer, 4) == CANERR_NOERROR) {
		if(length != 4)
			return cop_error = COPERR_LENGTH;
		if(value == NULL)
			return cop_error = COPERR_FATAL;
		LOLOBYTE(*value) = buffer[0];
		LOHIBYTE(*value) = buffer[1];
		HILOBYTE(*value) = buffer[2];
		HIHIBYTE(*value) = buffer[3];
	}
	return cop_error;
}

/*	-----------  Lokale Funktionen  ------------------------------------------
 */

static LONG sdo_expedited(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data)
{
	short n;							// data length code
	short rc;							// return value
	
	// ---  Expedited SDO Download  ---
	cop_buffer[0]  = (BYTE)0x23;		// client command specifier
	cop_buffer[0] |= (BYTE)((4 - length) << 2);
	cop_buffer[1]  = LOBYTE(index);		// multiplexor: index (LSB)
	cop_buffer[2]  = HIBYTE(index);		//              index (MSB)
	cop_buffer[3]  = (BYTE)(subindex);	//              subindex
	memset(&cop_buffer[4],0x00,4);		// clear data buffer
	memcpy(&cop_buffer[4],data,length);	// copy data bytes
	n = 8;								// 8 bytes to transmit!

	// 1. Configure transmit message object for client SDO
	if((cop_error = can_config(CANBUF_TX, SDO_CLIENT + node_id, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for server SDO 
	if((cop_error = can_config(CANBUF_RX, SDO_SERVER + node_id, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the client SDO message
	if((cop_error = can_transmit(CANBUF_TX, n, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for SDO time-out
	can_start_timer(cop_timeout);

	// 4. Wait until server message is received
	do	{
		switch((rc = can_receive(CANBUF_RX, &n, cop_buffer)))
		{
		case CANERR_NOERROR:			// confirmation:
			if(n != 8) {								// 8 bytes received?
				cop_error = SDOERR_GENERAL_ERROR;		//   abort: general error
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_LENGTH;
			}
			if((cop_buffer[1] != LOBYTE(index)) ||		// multiplexor? index (LSB)
			   (cop_buffer[2] != HIBYTE(index)) ||		//              index (MSB)
			   (cop_buffer[3] != (BYTE)(subindex))) {	//              subindex
				rc = COPERR_FORMAT;
				break;
			}
			if((cop_buffer[0] & 0xFF) == 0x80) {		// SDO abort received?
				LOLOBYTE(cop_error) = cop_buffer[4];	//   abort code (LSB)
				LOHIBYTE(cop_error) = cop_buffer[5];	//    -"-
				HILOBYTE(cop_error) = cop_buffer[6];	//    -"-
				HIHIBYTE(cop_error) = cop_buffer[7];	//   abort code (MSB)
				// Return value is abort code!
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error;
			}
			if((cop_buffer[0] & 0xFF) != 0x60) {		// unknown command specifier?
				cop_error = SDOERR_UNKNOWN_SPECIFIER;	//   abort: unknown command specifier
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_FORMAT;
			}
			else {										// success: data written!
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_NOERROR;
			}
		case CANERR_RX_EMPTY:			// receiver empty:
			if(can_is_timeout()) {						//   time-out occurred?
				cop_error = SDOERR_PROTOCOL_TIMEOUT;	//   abort: time-out
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_TIMEOUT;
			}
			break;
		default:						// other errors:
			cop_error = SDOERR_GENERAL_ERROR;			//   abort: general error
			cop_buffer[0] = 0x80;						//   command specifier
			cop_buffer[1] = LOBYTE(index);				//   multiplexor: index (LSB)
			cop_buffer[2] = HIBYTE(index);				//                index (MSB)
			cop_buffer[3] = (BYTE)(subindex);			//            subindex
			cop_buffer[4] = LOLOBYTE(cop_error);		// abort code (LSB)
			cop_buffer[5] = LOHIBYTE(cop_error);		//  -"-
			cop_buffer[6] = HILOBYTE(cop_error);		//  -"-
			cop_buffer[7] = HIHIBYTE(cop_error);		// abort code (MSB)
			// Transmit SDO abort and return
			can_transmit(CANBUF_TX, 8, cop_buffer);
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error = rc;
		}
	}	while(1);						// "the torture never stops!"
}

static LONG sdo_segmented(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data)
{
	short n, i;							// data length code
	short rc;							// return value
	short t = 0;						// toggle bit
	
	// ---  Initiate SDO Download  ---
	cop_buffer[0] = (BYTE)0x21;			// client command specifier
	cop_buffer[1] = LOBYTE(index);		// multiplexor: index (LSB)
	cop_buffer[2] = HIBYTE(index);		//              index (MSB)
	cop_buffer[3] = (BYTE)(subindex);	//              subindex
	cop_buffer[4] = LOBYTE(length);		// number of data bytes (LSB)
	cop_buffer[5] = HIBYTE(length);		//  -"-
	cop_buffer[6] = (BYTE)0x00;			//  -"-
	cop_buffer[7] = (BYTE)0x00;			// number of data bytes (MSB)
	n = 8;								// 8 bytes to transmit!

	// 1. Configure transmit message object for client SDO
	if((cop_error = can_config(CANBUF_TX, SDO_CLIENT + node_id, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for server SDO 
	if((cop_error = can_config(CANBUF_RX, SDO_SERVER + node_id, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the client SDO message
	if((cop_error = can_transmit(CANBUF_TX, n, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for SDO time-out
	can_start_timer(cop_timeout);

	// 4. Wait until server message is received
	do	{
		switch((rc = can_receive(CANBUF_RX, &n, cop_buffer)))
		{
		case CANERR_NOERROR:			// confirmation:
			if(n != 8) {								// 8 bytes received?
				cop_error = SDOERR_GENERAL_ERROR;		//   abort: general error
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_LENGTH;
			}
			if((cop_buffer[1] != LOBYTE(index)) ||		// multiplexor? index (LSB)
			   (cop_buffer[2] != HIBYTE(index)) ||		//              index (MSB)
			   (cop_buffer[3] != (BYTE)(subindex))) {	//              subindex
				rc = COPERR_FORMAT;
				break;
			}
			if((cop_buffer[0] & 0xFF) == 0x80) {		// SDO abort received?
				LOLOBYTE(cop_error) = cop_buffer[4];	//   abort code (LSB)
				LOHIBYTE(cop_error) = cop_buffer[5];	//    -"-
				HILOBYTE(cop_error) = cop_buffer[6];	//    -"-
				HIHIBYTE(cop_error) = cop_buffer[7];	//   abort code (MSB)
				// Return value is abort code!
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error;
			}
			if((cop_buffer[0] & 0xFF) != 0x60) {		// unknown command specifier?
				cop_error = SDOERR_UNKNOWN_SPECIFIER;	//   abort: unknown command specifier
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_FORMAT;
			}
		case CANERR_RX_EMPTY:			// receiver empty:
			if(can_is_timeout()) {						//   time-out occurred?
				cop_error = SDOERR_PROTOCOL_TIMEOUT;	//   abort: time-out
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_TIMEOUT;
			}
			break;
		default:						// other errors:
			cop_error = SDOERR_GENERAL_ERROR;			//   abort: general error
			cop_buffer[0] = 0x80;						//   command specifier
			cop_buffer[1] = LOBYTE(index);				//   multiplexor: index (LSB)
			cop_buffer[2] = HIBYTE(index);				//                index (MSB)
			cop_buffer[3] = (BYTE)(subindex);			//            subindex
			cop_buffer[4] = LOLOBYTE(cop_error);		// abort code (LSB)
			cop_buffer[5] = LOHIBYTE(cop_error);		//  -"-
			cop_buffer[6] = HILOBYTE(cop_error);		//  -"-
			cop_buffer[7] = HIHIBYTE(cop_error);		// abort code (MSB)
			// Transmit SDO abort and return
			can_transmit(CANBUF_TX, 8, cop_buffer);
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error = rc;
		}
	}	while(rc != CANERR_NOERROR);

	// ---  Download SDO Segment  ---
	for(i = 0;;)
	{
		if(length <= 7)					// bytes that does not contain data
			n = 7 - length;
		else							// no segment size indicated
			n = 0;
		cop_buffer[0] = (BYTE)(n << 1);	// client command specifier
		memset(&cop_buffer[1], 0x00, 7);// clear data buffer
		memcpy(&cop_buffer[1], &data[i], 7 - n);// copy segment data
		length -= 7 - n;				// remaining number of bytes
		i	   += 7 - n;				// index to remainung bytes
		cop_buffer[0]|= length? 0x00 : 0x01;// last segment to transmit
		cop_buffer[0]|= t;				// toggle bit
		n = 8;							// 8 bytes to transmit!

		// 5. Transmit the client SDO message
		if((cop_error = can_transmit(CANBUF_TX, n, cop_buffer)) != CANERR_NOERROR) {
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
		// 6. Start timer for SDO time-out
		can_start_timer(cop_timeout);

		// 7. Wait until server message is received
		do	{
			switch((rc = can_receive(CANBUF_RX, &n, cop_buffer)))
			{
			case CANERR_NOERROR:			// confirmation:
				if(n != 8) {								// 8 bytes received?
					cop_error = SDOERR_GENERAL_ERROR;		//   abort: general error
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_LENGTH;
				}
				if((cop_buffer[0] & 0xE0) == 0x80) {		// SDO abort received?
					LOLOBYTE(cop_error) = cop_buffer[4];	//   abort code (LSB)
					LOHIBYTE(cop_error) = cop_buffer[5];	//    -"-
					HILOBYTE(cop_error) = cop_buffer[6];	//    -"-
					HIHIBYTE(cop_error) = cop_buffer[7];	//   abort code (MSB)
					// Return value is abort code!
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error;
				}
				if((cop_buffer[0] & 0xE0) != 0x20) {		// unknown command specifier?
					cop_error = SDOERR_UNKNOWN_SPECIFIER;	//   abort: unknown command specifier
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_FORMAT;
				}
				if((cop_buffer[0] & 0x10) != t) {			// toggle bit not altered?
					cop_error = SDOERR_WRONG_TOGGLEBIT;		//   abort: toggle bit not altered
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_FORMAT;
				}
				if(length == 0)	{							// all data tranmitted?
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_NOERROR;
				}
			case CANERR_RX_EMPTY:			// receiver empty:
				if(can_is_timeout()) {						//   time-out occurred?
					cop_error = SDOERR_PROTOCOL_TIMEOUT;	//   abort: time-out
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_TIMEOUT;
				}
				break;
			default:						// other errors:
				cop_error = SDOERR_GENERAL_ERROR;			//   abort: general error
				cop_buffer[0] = 0x80;						//   command specifier
				cop_buffer[1] = LOBYTE(index);				//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);				//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);			//            subindex
				cop_buffer[4] = LOLOBYTE(cop_error);		// abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);		//  -"-
				cop_buffer[6] = HILOBYTE(cop_error);		//  -"-
				cop_buffer[7] = HIHIBYTE(cop_error);		// abort code (MSB)
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = rc;
			}
		}	while(rc != CANERR_NOERROR);

		t = t? 0x00 : 0x10;				// alternate toggle bit!
	}
}

static LONG sdo_receive(BYTE node_id, WORD index, BYTE subindex, SHORT *length, BYTE *data, SHORT max)
{
	short n;							// data length code
	short rc;							// return value
	short t = 0;						// toggle bit
	
	// ---  Initiate SDO Upload  ---
	cop_buffer[0] = 0x40;				// client command specifier
	cop_buffer[1] = LOBYTE(index);		// multiplexor: index (LSB)
	cop_buffer[2] = HIBYTE(index);		//              index (MSB)
	cop_buffer[3] = (BYTE)(subindex);	//              subindex
	cop_buffer[4] = 0x00;				// reserved: set to 00h
	cop_buffer[5] = 0x00;				//   -"-
	cop_buffer[6] = 0x00;				//   -"-
	cop_buffer[7] = 0x00;				//   -"-
	n = 8;								// 8 bytes to transmit!

	// 1. Configure transmit message object for client SDO
	if((cop_error = can_config(CANBUF_TX, SDO_CLIENT + node_id, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for server SDO 
	if((cop_error = can_config(CANBUF_RX, SDO_SERVER + node_id, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the client SDO message
	if((cop_error = can_transmit(CANBUF_TX, n, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for SDO time-out
	can_start_timer(cop_timeout);

	// 4. Wait until server message is received
	do	{
		switch((rc = can_receive(CANBUF_RX, &n, cop_buffer)))
		{
		case CANERR_NOERROR:			// confirmation:
			if(n != 8) {								// 8 bytes received?
				cop_error = SDOERR_GENERAL_ERROR;		//   abort: general error
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
			   *length = 0;								//   no data received!
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_LENGTH;
			}
			if((cop_buffer[1] != LOBYTE(index)) ||		// multiplexor? index (LSB)
			   (cop_buffer[2] != HIBYTE(index)) ||		//              index (MSB)
			   (cop_buffer[3] != (BYTE)(subindex))) {	//              subindex
				rc = COPERR_FORMAT;
				break;
			}
			if((cop_buffer[0] & 0xE0) == 0x80) {// SDO abort received?
				LOLOBYTE(cop_error) = cop_buffer[4];	//   abort code (LSB)
				LOHIBYTE(cop_error) = cop_buffer[5];	//    -"-
				HILOBYTE(cop_error) = cop_buffer[6];	//    -"-
				HIHIBYTE(cop_error) = cop_buffer[7];	//   abort code (MSB)
			   *length = 0;								//   no data received!
				// Return value is abort code!
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error;
			}
			if((cop_buffer[0] & 0xE0) != 0x40) {		// unknown command specifier?
				cop_error = SDOERR_UNKNOWN_SPECIFIER;	//   abort: unknown command specifier
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
			   *length = 0;								//   no data received!
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_FORMAT;
			}
			if((cop_buffer[0] & 0x02) == 0x02) {		// expedited transfer?
				if((cop_buffer[0] & 0x01) == 0x01)
					n = 4 - (short)((cop_buffer[0] & 0x0C) >> 2);
				else
					n = 4;
				memcpy(data, &cop_buffer[4], n < max? n : max);
			   *length = n < max? n : max;				//   data received!!!
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_NOERROR;
			}
			break;
		case CANERR_RX_EMPTY:				// receiver empty:
			if(can_is_timeout()) {						//   time-out occurred?
				cop_error = SDOERR_PROTOCOL_TIMEOUT;	//   abort: time-out
				cop_buffer[0] = 0x80;					//   command specifier
				cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);			//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);		//                subindex
				cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
				cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
				cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
			   *length = 0;								//   no data received!
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = COPERR_TIMEOUT;
			}
			break;
		default:							// other errors:
			cop_error = SDOERR_GENERAL_ERROR;			//   abort: general error
			cop_buffer[0] = 0x80;						//   command specifier
			cop_buffer[1] = LOBYTE(index);				//   multiplexor: index (LSB)
			cop_buffer[2] = HIBYTE(index);				//                index (MSB)
			cop_buffer[3] = (BYTE)(subindex);			//            subindex
			cop_buffer[4] = LOLOBYTE(cop_error);		// abort code (LSB)
			cop_buffer[5] = LOHIBYTE(cop_error);		//  -"-
			cop_buffer[6] = HILOBYTE(cop_error);		//  -"-
			cop_buffer[7] = HIHIBYTE(cop_error);		// abort code (MSB)
		   *length = 0;									//   no data received!
			// Transmit SDO abort and return
			can_transmit(CANBUF_TX, 8, cop_buffer);
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error = rc;
		}
	}	while(rc != CANERR_NOERROR);		// segmented transfer:

	// ---  Upload SDO Segment  ---
	for(*length = 0;;)
	{
		cop_buffer[0] = 0x60 | t;		// client command specifier
		cop_buffer[1] = 0x00;			// reserved: set to 00h
		cop_buffer[2] = 0x00;			//   -"-
		cop_buffer[3] = 0x00;			//   -"-
		cop_buffer[4] = 0x00;			//   -"-
		cop_buffer[5] = 0x00;			//   -"-
		cop_buffer[6] = 0x00;			//   -"-
		cop_buffer[7] = 0x00;			//   -"-
		n = 8;							// 8 bytes to transmit!

		// 5. Transmit the client SDO message
		if((cop_error = can_transmit(CANBUF_TX, n, cop_buffer)) != CANERR_NOERROR) {
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
		// 6. Start timer for SDO time-out
		can_start_timer(cop_timeout);

		// 7. Wait until server message is received
		do	{
			switch((rc = can_receive(CANBUF_RX, &n, cop_buffer)))
			{
			case CANERR_NOERROR:			// confirmation:
				if(n != 8) {								// 8 bytes received?
					cop_error = SDOERR_GENERAL_ERROR;		//   abort: general error
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				   *length = 0;								//   no data received!
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_LENGTH;
				}
				if((cop_buffer[0] & 0xE0) == 0x80) {// SDO abort received?
					LOLOBYTE(cop_error) = cop_buffer[4];	//   abort code (LSB)
					LOHIBYTE(cop_error) = cop_buffer[5];	//    -"-
					HILOBYTE(cop_error) = cop_buffer[6];	//    -"-
					HIHIBYTE(cop_error) = cop_buffer[7];	//   abort code (MSB)
				   *length = 0;								//   no data received!
					// Return value is abort code!
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error;
				}
				if((cop_buffer[0] & 0xE0) != 0x00) {		// unknown command specifier?
					cop_error = SDOERR_UNKNOWN_SPECIFIER;	//   abort: unknown command specifier
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				   *length = 0;								//   no data received!
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_FORMAT;
				}
				if((cop_buffer[0] & 0x10) != t) {			// toggle bit not altered?
					cop_error = SDOERR_WRONG_TOGGLEBIT;		//   abort: toggle bit not altered
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//  abort code (MSB)
				   *length = 0;								//   no data received!
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_FORMAT;
				}
				if((cop_buffer[0] & 0x0E) != 0x00)			// number of segment data bytes
					n = 7 - (int)((cop_buffer[0] & 0x0E) >> 1);
				else
					n = 7;
				if(max - *length > 0)						// copy segment data if space
					memcpy(&data[*length], &cop_buffer[1], *length + n < max? n : max - *length);
			   *length += n;
				if((cop_buffer[0] & 0x01) == 0x01) {		// no more segments?
					if(*length > max)
					   *length = max;						//   truncate to buffer size!
					if(*length < max)
						data[*length] = '\0';				//   for zero-closed strings!
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_NOERROR;
				}
				break;
			case CANERR_RX_EMPTY:				// receiver empty:
				if(can_is_timeout()) {						//   time-out occurred?
					cop_error = SDOERR_PROTOCOL_TIMEOUT;	//   abort: time-out
					cop_buffer[0] = 0x80;					//   command specifier
					cop_buffer[1] = LOBYTE(index);			//   multiplexor: index (LSB)
					cop_buffer[2] = HIBYTE(index);			//                index (MSB)
					cop_buffer[3] = (BYTE)(subindex);		//                subindex
					cop_buffer[4] = LOLOBYTE(cop_error);	//   abort code (LSB)
					cop_buffer[5] = LOHIBYTE(cop_error);	//    -"-
					cop_buffer[6] = HILOBYTE(cop_error);	//    -"-
					cop_buffer[7] = HIHIBYTE(cop_error);	//   abort code (MSB)
				   *length = 0;								//   no data received!
					// Transmit SDO abort and return
					can_transmit(CANBUF_TX, 8, cop_buffer);
					can_delete(CANBUF_TX);
					can_delete(CANBUF_RX);
					return cop_error = COPERR_TIMEOUT;
				}
				break;
			default:							// other errors:
				cop_error = SDOERR_GENERAL_ERROR;			//   abort: general error
				cop_buffer[0] = 0x80;						//   command specifier
				cop_buffer[1] = LOBYTE(index);				//   multiplexor: index (LSB)
				cop_buffer[2] = HIBYTE(index);				//                index (MSB)
				cop_buffer[3] = (BYTE)(subindex);			//            subindex
				cop_buffer[4] = LOLOBYTE(cop_error);		// abort code (LSB)
				cop_buffer[5] = LOHIBYTE(cop_error);		//  -"-
				cop_buffer[6] = HILOBYTE(cop_error);		//  -"-
				cop_buffer[7] = HIHIBYTE(cop_error);		// abort code (MSB)
			   *length = 0;									//   no data received!
				// Transmit SDO abort and return
				can_transmit(CANBUF_TX, 8, cop_buffer);
				can_delete(CANBUF_TX);
				can_delete(CANBUF_RX);
				return cop_error = rc;
			}
		}	while(rc != CANERR_NOERROR);

		t = t? 0x00 : 0x10;				// alternate toggle bit!
	}	
}

LPSTR sdo_version(void)
{
	return (LPSTR)_id;					// Revision number
}

/*	--------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047-470, Fax: +49-69-7912-33292, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
