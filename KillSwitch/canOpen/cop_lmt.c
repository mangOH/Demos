/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_lmt.c 1.1 2005/10/09 19:17:00 vogt Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *				 
 *	Zweck     :  CANopen Master - LMT - Layer Management Services.
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
 *	CANopen Master LMT - Layer Management Services.
 *
 *		Implements the Layer Management Services and Protocols (LMT) according
 *		to CiA DS-205 (Part 1 and 2 of February, 1996).
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
 *		- Inguire Manufacturer Name Protocol
 *		- Inguire Product Name Protocol
 *		- Inguire Identity Serial-Number Protocol
 *		IDENTIFICATION PROTOCOLS
 *		- Identify Remote Slaves
 *
 *
 *	-----------  Änderungshistorie  ------------------------------------------
 *
 *	$Log: cop_lmt.c $
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */

#ifdef _DEBUG
 static char _id[] = "$Id: cop_lmt.c 18 2009-02-07 14:53:44Z mars $ _DEBUG";
#else
 static char _id[] = "$Id: cop_lmt.c 18 2009-02-07 14:53:44Z mars $";
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
static WORD cop_timeout = LMT_TIMEOUT;	// time-out value


/*	-----------  Funktionen  -------------------------------------------------
 */

long lmt_switch_mode_global(BYTE mode)
{
	// ---  LSS Switch Mode Global  ---
	cop_buffer[0] = (BYTE)0x04;			// command specifier
	cop_buffer[1] = (BYTE)mode;			// operation mode
	memset(&cop_buffer[2], 0x00, 6);	// (reserved)
	
	return cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer);
}

long lmt_switch_mode_selective(char *manufacturer_name, char *product_name, char *serial_number)
{
	// ---  LSS Switch Mode Selective: Manufacturer-Name  ---
	cop_buffer[0] = (BYTE)0x01;		// command specifier
	memcpy(&cop_buffer[1], manufacturer_name, 7);
	
	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Switch Mode Selective: Product-Name  ---
	cop_buffer[0] = (BYTE)0x02;		// command specifier
	memcpy(&cop_buffer[1], product_name, 7);
	
	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Switch Mode Selective: Serial-Number  ---
	cop_buffer[0] = (BYTE)0x03;		// command specifier
	cop_buffer[1] = (BYTE)(serial_number[0] - '0') << 4;
	cop_buffer[1] |=(BYTE)(serial_number[1] - '0');
	cop_buffer[2] = (BYTE)(serial_number[2] - '0') << 4;
	cop_buffer[2] |=(BYTE)(serial_number[3] - '0');
	cop_buffer[3] = (BYTE)(serial_number[4] - '0') << 4;
	cop_buffer[3] |=(BYTE)(serial_number[5] - '0');
	cop_buffer[4] = (BYTE)(serial_number[6] - '0') << 4;
	cop_buffer[4] |=(BYTE)(serial_number[7] - '0');
	cop_buffer[5] = (BYTE)(serial_number[8] - '0') << 4;
	cop_buffer[5] |=(BYTE)(serial_number[9] - '0');
	cop_buffer[6] = (BYTE)(serial_number[10] - '0') << 4;
	cop_buffer[6] |=(BYTE)(serial_number[11] - '0');
	cop_buffer[7] = (BYTE)(serial_number[12] - '0') << 4;
	cop_buffer[7] |=(BYTE)(serial_number[13] - '0');
	
	return cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer);
}

long lmt_configure_node_id(BYTE node_id)
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

long lmt_configure_bit_timing(BYTE table, BYTE baudrate)
{
	short length;						// data length code
	
	// ---  LSS Configure Bit-timing  ---
	cop_buffer[0] = (BYTE)0x13;			// command specifier
	cop_buffer[1] = (BYTE)table;		// table selector
	cop_buffer[2] = (BYTE)baudrate;		// table index
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

long lmt_activate_bit_timing(WORD switch_delay)
{
	// ---  LSS Activate Bit-timing  ---
	cop_buffer[0] = (BYTE)0x15;			// command specifier
	cop_buffer[1] = LOBYTE(switch_delay);//switch delay (LSB)
	cop_buffer[2] = HIBYTE(switch_delay);//switch delay (MSB)
	memset(&cop_buffer[3], 0x00, 5);	// (reserved)

	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) == COPERR_NOERROR) {
		can_start_timer((WORD)(2 * switch_delay));
		while(!can_is_timeout());		// 2 * switch delay time!
	}
	return cop_error;
}

long lmt_store_configuration(void)
{
	short length;						// data length code
	
	// ---  LSS Configure Bit-timing  ---
	cop_buffer[0] = (BYTE)0x17;			// command specifier
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

long lmt_inquire_manufacturer_name(char *manufacturer_name)
{
	short length;						// data length code
	
	if(manufacturer_name == NULL)		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Manufacturer-Name  ---
	cop_buffer[0] = (BYTE)0x24;			// command specifier
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
			if(cop_buffer[0] != 0x24)	// command specifier?
				return cop_error = COPERR_FORMAT;
			manufacturer_name[0] = cop_buffer[1];
			manufacturer_name[1] = cop_buffer[2];
			manufacturer_name[2] = cop_buffer[3];
			manufacturer_name[3] = cop_buffer[4];
			manufacturer_name[4] = cop_buffer[5];
			manufacturer_name[5] = cop_buffer[6];
			manufacturer_name[6] = cop_buffer[7];
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

long lmt_inquire_product_name(char *product_name)
{
	short length;						// data length code
	
	if(product_name == NULL)     		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Product-Name  ---
	cop_buffer[0] = (BYTE)0x25;			// command specifier
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
			if(cop_buffer[0] != 0x25)	// command specifier?
				return cop_error = COPERR_FORMAT;
			product_name[0] = cop_buffer[1];
			product_name[1] = cop_buffer[2];
			product_name[2] = cop_buffer[3];
			product_name[3] = cop_buffer[4];
			product_name[4] = cop_buffer[5];
			product_name[5] = cop_buffer[6];
			product_name[6] = cop_buffer[7];
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

long lmt_inquire_serial_number(char *serial_number)
{
	short length;						// data length code
	
	if(serial_number == NULL)   		// null pointer assignment
		return cop_error = COPERR_FATAL;
	
	// ---  LSS Inquire Serial-Number  ---
	cop_buffer[0] = (BYTE)0x26;			// command specifier
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
			if(cop_buffer[0] != 0x26)	// command specifier?
				return cop_error = COPERR_FORMAT;
			serial_number[0] = (cop_buffer[1] >> 4) + '0';
			serial_number[1] = (cop_buffer[1] & 15) + '0';
			serial_number[2] = (cop_buffer[2] >> 4) + '0';
			serial_number[3] = (cop_buffer[2] & 15) + '0';
			serial_number[4] = (cop_buffer[3] >> 4) + '0';
			serial_number[5] = (cop_buffer[3] & 15) + '0';
			serial_number[6] = (cop_buffer[4] >> 4) + '0';
			serial_number[7] = (cop_buffer[4] & 15) + '0';
			serial_number[8] = (cop_buffer[5] >> 4) + '0';
			serial_number[9] = (cop_buffer[5] & 15) + '0';
			serial_number[10] = (cop_buffer[6] >> 4) + '0';
			serial_number[11] = (cop_buffer[6] & 15) + '0';
			serial_number[12] = (cop_buffer[7] >> 4) + '0';
			serial_number[13] = (cop_buffer[7] & 15) + '0';
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

long lmt_identify_remote_slaves(char *manufacturer_name, char *product_name, char *serial_number_low, char *serial_number_high)
{
	// ---  LSS Identify Remote Slaves: Manufacturer-Name  ---
	cop_buffer[0] = (BYTE)0x05;			// command specifier
	memcpy(&cop_buffer[1], manufacturer_name, 7);
	
	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Product-Name  ---
	cop_buffer[0] = (BYTE)0x06;			// command specifier
	memcpy(&cop_buffer[1], product_name, 7);
	
	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Serial-Number (low) ---
	cop_buffer[0] = (BYTE)0x07;			// command specifier
	cop_buffer[1] = (BYTE)serial_number_low[0] << 4;
	cop_buffer[1] |=(BYTE)serial_number_low[1];
	cop_buffer[2] = (BYTE)serial_number_low[2] << 4;
	cop_buffer[2] |=(BYTE)serial_number_low[3];
	cop_buffer[3] = (BYTE)serial_number_low[4] << 4;
	cop_buffer[3] |=(BYTE)serial_number_low[5];
	cop_buffer[4] = (BYTE)serial_number_low[6] << 4;
	cop_buffer[4] |=(BYTE)serial_number_low[7];
	cop_buffer[5] = (BYTE)serial_number_low[8] << 4;
	cop_buffer[5] |=(BYTE)serial_number_low[9];
	cop_buffer[6] = (BYTE)serial_number_low[10] << 4;
	cop_buffer[6] |=(BYTE)serial_number_low[11];
	cop_buffer[7] = (BYTE)serial_number_low[12] << 4;
	cop_buffer[7] |=(BYTE)serial_number_low[13];
	
	if((cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer)) != COPERR_NOERROR)
		return cop_error;
	
	// ---  LSS Identify Remote Slaves: Serial-Number (high) ---
	cop_buffer[0] = (BYTE)0x08;			// command specifier
	cop_buffer[1] = (BYTE)serial_number_high[0] << 4;
	cop_buffer[1] |=(BYTE)serial_number_high[1];
	cop_buffer[2] = (BYTE)serial_number_high[2] << 4;
	cop_buffer[2] |=(BYTE)serial_number_high[3];
	cop_buffer[3] = (BYTE)serial_number_high[4] << 4;
	cop_buffer[3] |=(BYTE)serial_number_high[5];
	cop_buffer[4] = (BYTE)serial_number_high[6] << 4;
	cop_buffer[4] |=(BYTE)serial_number_high[7];
	cop_buffer[5] = (BYTE)serial_number_high[8] << 4;
	cop_buffer[5] |=(BYTE)serial_number_high[9];
	cop_buffer[6] = (BYTE)serial_number_high[10] << 4;
	cop_buffer[6] |=(BYTE)serial_number_high[11];
	cop_buffer[7] = (BYTE)serial_number_high[12] << 4;
	cop_buffer[7] |=(BYTE)serial_number_high[13];
	
	return cop_error = cop_transmit(LMT_MASTER, 8, cop_buffer);
}

WORD lmt_timeout(WORD milliseconds)
{
	WORD last_value = cop_timeout;		// copy old time-out value
	cop_timeout = milliseconds;			// set new time-out value
	return last_value;					// return old time-out value
}

LPSTR lmt_version(void)
{
	return (LPSTR)_id;					// Revision number
}

/*	--------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047-470, Fax: +49-69-7912-33292, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
