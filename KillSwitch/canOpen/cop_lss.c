/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_lss.c 1.1 2005/10/09 19:17:00 vogt Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *				 
 *	Zweck     :  CANopen Master - LSS - Layer Setting Services.
 *
 *	Copyright :  (c) 2005 by UV Software, Friedrichshafen.
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
 *	CANopen Master LSS - Layer Setting Services.
 *
 *		Implements the Layer Setting Services and Protocols (LSS) according
 *		to CiA DSP-305 (Version 1.1.1 of November 5, 2002).
 *
 *		SWITCH MODE PROTOCOLS
 *		- Switch Mode Global
 *		- Switch Mode Selective
 *		CONFIGURATION PROTOCOLS
 *		- Configure Modul-Id. Protocol
 *		- Configure Bit-timing Parameters Protocol
 *		- Activate Bit-timing Parameters Protocol
 *		- Store Configuration Protocol
 *		INQUIRY PROTOCOLS
 *		- Inguire Identity Vendor-Id. Protocol
 *		- Inguire Identity Product-Code Protocol
 *		- Inguire Identity Revision-Number Protocol
 *		- Inguire Identity Serial-Number Protocol
 *		- Inguire Identity Node-Id. Protocol
 *		IDENTIFICATION PROTOCOLS
 *		- Identify Remote Slaves
 *		- Identify Non-configured Remote Slaves
 *
 *
 *	-----------  Änderungshistorie  ------------------------------------------
 *
 *	$Log: cop_lss.c $
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */

#ifdef _DEBUG
 static char _id[] = "$Id: cop_lss.c 18 2009-02-07 14:53:44Z mars $ _DEBUG";
#else
 static char _id[] = "$Id: cop_lss.c 18 2009-02-07 14:53:44Z mars $";
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


/*	-----------  Variablen  --------------------------------------------------
 */

extern BYTE cop_baudrate;				// actual baudrate
extern LONG cop_error;					// last error code
extern BYTE cop_buffer[8];				// data buffer (8)
static WORD cop_timeout = LSS_TIMEOUT;	// time-out value


/*	-----------  Funktionen  -------------------------------------------------
 */

long lss_switch_mode_global(BYTE mode)
{
	// ---  LSS Switch Mode Global  ---
	cop_buffer[0] = (BYTE)0x04;			// command specifier
	cop_buffer[1] = (BYTE)mode;			// operation mode
	memset(&cop_buffer[2], 0x00, 6);	// (reserved)
	
	return cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer);
}

long lss_switch_mode_selective(DWORD vendor_id, DWORD product_code, DWORD revision_number, DWORD serial_number)
{
	short length;						// data length code
	
	// ---  LSS Switch Mode Selective: Vendor-Id  ---
	cop_buffer[0] = (BYTE)0x40;			// command specifier
	cop_buffer[1] = LOLOBYTE(vendor_id);// vendor-id (LSB)
	cop_buffer[2] = LOHIBYTE(vendor_id);//  -"-
	cop_buffer[3] = HILOBYTE(vendor_id);//  -"-
	cop_buffer[4] = HIHIBYTE(vendor_id);// vendor-id (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Switch Mode Selective: Product-Code  ---
	cop_buffer[0] = (BYTE)0x41;			// command specifier
	cop_buffer[1] = LOLOBYTE(product_code);// product-code (LSB)
	cop_buffer[2] = LOHIBYTE(product_code);//  -"-
	cop_buffer[3] = HILOBYTE(product_code);//  -"-
	cop_buffer[4] = HIHIBYTE(product_code);// product-code (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Switch Mode Selective: Revision-Number  ---
	cop_buffer[0] = (BYTE)0x42;			// command specifier
	cop_buffer[1] = LOLOBYTE(revision_number);// revision-number (LSB)
	cop_buffer[2] = LOHIBYTE(revision_number);//  -"-
	cop_buffer[3] = HILOBYTE(revision_number);//  -"-
	cop_buffer[4] = HIHIBYTE(revision_number);// revision-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Switch Mode Selective: Serial-Number  ---
	cop_buffer[0] = (BYTE)0x43;			// command specifier
	cop_buffer[1] = LOLOBYTE(serial_number);// serial-number (LSB)
	cop_buffer[2] = LOHIBYTE(serial_number);//  -"-
	cop_buffer[3] = HILOBYTE(serial_number);//  -"-
	cop_buffer[4] = HIHIBYTE(serial_number);// serial-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x44)	// command specifier?
				return cop_error = COPERR_FORMAT;
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_configure_node_id(BYTE node_id)
{
	short length;						// data length code
	
	// ---  LSS Configure Node-Id  ---
	cop_buffer[0] = (BYTE)0x11;			// command specifier
	cop_buffer[1] = (BYTE)node_id;		// node-id
	memset(&cop_buffer[2], 0x00, 6);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x11)	// command specifier?
				return cop_error = COPERR_FORMAT;
			if(cop_buffer[1] != 0x00)	// error code?
				return cop_error = (long)cop_buffer[1];
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_configure_bit_timing(BYTE baudrate)
{
	short length;						// data length code
	
	// ---  LSS Configure Bit-timing  ---
	cop_buffer[0] = (BYTE)0x13;		// command specifier
	cop_buffer[1] = (BYTE)0x00;		// table selector
	cop_buffer[2] = (BYTE)baudrate;	// table index
	memset(&cop_buffer[3], 0x00, 5);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x13)	// command specifier?
				return cop_error = COPERR_FORMAT;
			if(cop_buffer[1] != 0x00)	// error code?
				return cop_error = (long)cop_buffer[1];
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_activate_bit_timing(WORD switch_delay)
{
	// ---  LSS Activate Bit-timing  ---
	cop_buffer[0] = (BYTE)0x15;			// command specifier
	cop_buffer[1] = LOBYTE(switch_delay);//switch delay (LSB)
	cop_buffer[2] = HIBYTE(switch_delay);//switch delay (MSB)
	memset(&cop_buffer[3], 0x00, 5);	// (reserved)

	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) == COPERR_NOERROR) {
		can_start_timer((WORD)(2 * switch_delay));
		while(!can_is_timeout());		// 2 * switch delay time!
	}
	return cop_error;
}

long lss_store_configuration(void)
{
	short length;						// data length code
	
	// ---  LSS Store Configuration  ---
	cop_buffer[0] = (BYTE)0x17;		// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x17)	// command specifier?
				return cop_error = COPERR_FORMAT;
			if(cop_buffer[1] != 0x00)	// error code?
				return cop_error = (long)cop_buffer[1];
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_inquire_vendor_id(DWORD *vendor_id)
{
	short length;						// data length code
	
	if(vendor_id == NULL)				// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Vendor-Id  ---
	cop_buffer[0] = (BYTE)0x5A;			// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x5A)	// command specifier?
				return cop_error = COPERR_FORMAT;
			LOLOBYTE(*vendor_id) = cop_buffer[1];// vendor-id (LSB)
			LOHIBYTE(*vendor_id) = cop_buffer[2];//  -"-
			HILOBYTE(*vendor_id) = cop_buffer[3];//  -"-
			HIHIBYTE(*vendor_id) = cop_buffer[4];// vendor-id (MSB)
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_inquire_product_code(DWORD *product_code)
{
	short length;						// data length code
	
	if(product_code == NULL)     		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Product-Code  ---
	cop_buffer[0] = (BYTE)0x5B;			// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x5B)	// command specifier?
				return cop_error = COPERR_FORMAT;
			LOLOBYTE(*product_code) = cop_buffer[1];// product-code (LSB)
			LOHIBYTE(*product_code) = cop_buffer[2];//  -"-
			HILOBYTE(*product_code) = cop_buffer[3];//  -"-
			HIHIBYTE(*product_code) = cop_buffer[4];// product-code (MSB)
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_inquire_revision_number(DWORD *revision_number)
{
	short length;						// data length code
	
	if(revision_number == NULL) 		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Revision-Number  ---
	cop_buffer[0] = (BYTE)0x5C;			// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x5C)	// command specifier?
				return cop_error = COPERR_FORMAT;
			LOLOBYTE(*revision_number) = cop_buffer[1];// revision-number (LSB)
			LOHIBYTE(*revision_number) = cop_buffer[2];//  -"-
			HILOBYTE(*revision_number) = cop_buffer[3];//  -"-
			HIHIBYTE(*revision_number) = cop_buffer[4];// revision-number (MSB)
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_inquire_serial_number(DWORD *serial_number)
{
	short length;						// data length code
	
	if(serial_number == NULL)   		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Serial-Number  ---
	cop_buffer[0] = (BYTE)0x5D;			// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x5D)	// command specifier?
				return cop_error = COPERR_FORMAT;
			LOLOBYTE(*serial_number) = cop_buffer[1];// serial-number (LSB)
			LOHIBYTE(*serial_number) = cop_buffer[2];//  -"-
			HILOBYTE(*serial_number) = cop_buffer[3];//  -"-
			HIHIBYTE(*serial_number) = cop_buffer[4];// serial-number (MSB)
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_inquire_node_id(BYTE *node_id)
{
	short length;						// data length code
	
	if(node_id == NULL)   				// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Node-Id  ---
	cop_buffer[0] = (BYTE)0x5E;			// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)

	// 1. Configure transmit message object for LSS master
	if((cop_error = can_config(CANBUF_TX, LSS_MASTER, CANMSG_TRANSMIT)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		return cop_error;
	}
	// 2. Configure receive message object for LSS slave 
	if((cop_error = can_config(CANBUF_RX, LSS_SLAVE, CANMSG_RECEIVE)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 2. Transmit the LSS master message
	if((cop_error = can_transmit(CANBUF_TX, 8, cop_buffer)) != CANERR_NOERROR) {
		can_delete(CANBUF_TX);
		can_delete(CANBUF_RX);
		return cop_error;
	}
	// 3. Start timer for LSS time-out
	can_start_timer(cop_timeout);

	// 4. Wait until slave message is received
	do	{
		if((cop_error = can_receive(CANBUF_RX, &length, cop_buffer)) == COPERR_NOERROR)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			if(length != 8)				// 8 byte received?
				return cop_error = COPERR_LENGTH;
			if(cop_buffer[0] != 0x5E)	// command specifier?
				return cop_error = COPERR_FORMAT;
		   *node_id = cop_buffer[1];	// node-id
			return cop_error;
		}
		else if(cop_error != COPERR_RX_EMPTY)
		{
			can_delete(CANBUF_TX);
			can_delete(CANBUF_RX);
			return cop_error;
		}
	}	while(!can_is_timeout());
	// 4. A time-out occurred!
	can_delete(CANBUF_TX);
	can_delete(CANBUF_RX);
	return cop_error = COPERR_TIMEOUT;
}

long lss_identify_remote_slaves(DWORD vendor_id, DWORD product_code, DWORD revision_number_low, DWORD revision_number_high, DWORD serial_number_low, DWORD serial_number_high)
{
	// ---  LSS Identify Remote Slaves: Vendor-Id  ---
	cop_buffer[0] = (BYTE)0x46;			// command specifier
	cop_buffer[1] = LOLOBYTE(vendor_id);// vendor-id (LSB)
	cop_buffer[2] = LOHIBYTE(vendor_id);//  -"-
	cop_buffer[3] = HILOBYTE(vendor_id);//  -"-
	cop_buffer[4] = HIHIBYTE(vendor_id);// vendor-id (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Product-Code  ---
	cop_buffer[0] = (BYTE)0x47;			// command specifier
	cop_buffer[1] = LOLOBYTE(product_code);// product-code (LSB)
	cop_buffer[2] = LOHIBYTE(product_code);//  -"-
	cop_buffer[3] = HILOBYTE(product_code);//  -"-
	cop_buffer[4] = HIHIBYTE(product_code);  // product-code (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Revision-Number  ---
	cop_buffer[0] = (BYTE)0x48;			// command specifier
	cop_buffer[1] = LOLOBYTE(revision_number_low);// revision-number (LSB)
	cop_buffer[2] = LOHIBYTE(revision_number_low);//  -"-
	cop_buffer[3] = HILOBYTE(revision_number_low);//  -"-
	cop_buffer[4] = HIHIBYTE(revision_number_low);// revision-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Revision-Number  ---
	cop_buffer[0] = (BYTE)0x49;			// command specifier
	cop_buffer[1] = LOLOBYTE(revision_number_high);// revision-number (LSB)
	cop_buffer[2] = LOHIBYTE(revision_number_high);//  -"-
	cop_buffer[3] = HILOBYTE(revision_number_high);//  -"-
	cop_buffer[4] = HIHIBYTE(revision_number_high);// revision-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Serial-Number  ---
	cop_buffer[0] = (BYTE)0x4A;			// command specifier
	cop_buffer[1] = LOLOBYTE(serial_number_low);// serial-number (LSB)
	cop_buffer[2] = LOHIBYTE(serial_number_low);//  -"-
	cop_buffer[3] = HILOBYTE(serial_number_low);//  -"-
	cop_buffer[4] = HIHIBYTE(serial_number_low);// serial-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	if((cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Serial-Number  ---
	cop_buffer[0] = (BYTE)0x4B;			// command specifier
	cop_buffer[1] = LOLOBYTE(serial_number_high);// serial-number (LSB)
	cop_buffer[2] = LOHIBYTE(serial_number_high);//  -"-
	cop_buffer[3] = HILOBYTE(serial_number_high);//  -"-
	cop_buffer[4] = HIHIBYTE(serial_number_high);// serial-number (MSB)
	memset(&cop_buffer[5], 0x00, 3);	// (reserved)
	
	return cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer);
}

long lss_identify_non_configured_remote_slaves(void)
{
	// ---  LSS Identify Non-configured Remote Slaves  ---
	cop_buffer[0] = (BYTE)0x4C;		// command specifier
	memset(&cop_buffer[1], 0x00, 7);	// (reserved)
	
	return cop_error = cop_transmit(LSS_MASTER, 8, cop_buffer);
}

WORD lss_timeout(WORD milliseconds)
{
	WORD last_value = cop_timeout;		// copy old time-out value
	cop_timeout = milliseconds;			// set new time-out value
	return last_value;					// return old time-out value
}

LPSTR lss_version(void)
{
	return (LPSTR)_id;					// Revision number
}

/*	--------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047-470, Fax: +49-69-7912-33292, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
