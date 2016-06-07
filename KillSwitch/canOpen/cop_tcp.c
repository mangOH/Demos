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
 *	export    :  (see header file)
 *
 *	includes  :  cop_tcp.h (default.h), can_defs.h, cop_api.h, base64.h
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
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log$
 */

static char _id[] = "$Id: cop_tcp.c 34 2009-02-25 20:31:10Z saturn $";


/*  -----------  includes  -------------------------------------------------
 */

#include "cop_tcp.h"

#include "can_defs.h"
#include "cop_api.h"
#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


/*  -----------  defines  --------------------------------------------------
 */

#define WHITESPACE(x)		(' ' == (x) || (x) == '\t')
#define VISIBLE(x)			(' ' <= (x) && (x) <= '\177')
#define DECIMAL(x)			('0' <= (x) && (x) <= '9')
#define OCTAL(x)			('0' <= (x) && (x) <= '7')
#define HEXADECIMAL(x)		('0' <= (x) && (x) <= '9')||('a' <= (x) && (x) <= 'f')||('A' <= (x) && (x) <= 'F')
#define BASE64(x)			('0' <= (x) && (x) <= '9')||('a' <= (x) && (x) <= 'z')||('A' <= (x) && (x) <= 'Z')||((x) == '+')||((x) == '/')

/* *** **
#define SDO_BOOLEAN			0x1
#define SDO_INTEGER8		0x2
#define SDO_INTEGER16		0x3
#define SDO_INTEGER32		0x4
#define SDO_UNSIGNED8		0x5
#define SDO_UNSIGNED16		0x6
#define SDO_UNSIGNED32		0x7
#define SDO_REAL32			0x8
#define SDO_VISIBLE_STRING	0x9
#define SDO_OCTET_STRING	0xA
#define SDO_UNICODE_STRING	0xB
#define SDO_TIME_OF_DAY		0xC
#define SDO_TIME_DIFFERENCE	0xD
#define SDO_DOMAIN			0xF
#define SDO_INTEGER24		0x10
#define SDO_REAL64			0x11
#define SDO_INTEGER40		0x12
#define SDO_INTEGER48		0x13
#define SDO_INTEGER56		0x14
#define SDO_INTEGER64		0x15
#define SDO_UNSIGNED24		0x16
#define SDO_UNSIGNED40		0x18
#define SDO_UNSIGNED48		0x19
#define SDO_UNSIGNED56		0x1A
#define SDO_UNSIGNED64		0x1B
** *** */

#define ERROR_NOT_SUPPORTED	100
#define ERROR_SYNTAX		101
#define ERROR_NOT_PROCESSED	102
#define ERROR_TIMEOUT		103
#define ERROR_EXIT			900
#define ERROR_FATAL			999


/*  -----------  keywords  -------------------------------------------------
 */

#define BOOLEAN				0
#define COMMUNICATION		1
#define COPYRIGHT			2
#define DISABLE				3
#define DOMAIN				4
#define EMCY				5
#define ENABLE				6
#define ERROR				7
#define EVENT				8
#define EXECUTE				9
#define EXIT				10
#define FIRMWARE			11
#define GUARDING			12
#define HARDWARE			13
#define HEARTBEAT			14
#define ID					15
#define INFO				16
#define INIT				17
#define INTEGER8			18
#define INTEGER16			19
#define INTEGER24			20
#define INTEGER32			21
#define INTEGER40			22
#define INTEGER48			23
#define INTEGER56			24
#define INTEGER64			25
#define NETWORK				26
#define NODE				27
#define OCTET_STRING		28
#define OK					29
#define PDO					30
#define PREOPERATIONAL		31
#define READ				32
#define REAL32				33
#define REAL64				34
#define RECEIVE				35
#define RESET				36
#define RESTORE				37
#define RPDO				38
#define RTR					39
#define SDO_TIME_OUT		40
#define SEND				41
#define SET					42
#define SOFTWARE			43
#define START				44
#define STOP				45
#define STORE				46
#define SYNC				47
#define TIME_OF_DAY			48
#define TIME_DIFFERENCE		49
#define TPDO				50
#define UNICODE_STRING		51
#define UNSIGNED8			52
#define UNSIGNED16			53
#define UNSIGNED24			54
#define UNSIGNED32			55
#define UNSIGNED40			56
#define UNSIGNED48			57
#define UNSIGNED56			58
#define UNSIGNED64			59
#define USER				60
#define VERSION				61
#define VISIBLE_STRING		62
#define WAIT				63
#define WRITE				64


/*  -----------  types  ----------------------------------------------------
 */


/*  -----------  prototypes  -----------------------------------------------
 */

static int read_object(unsigned long nr, unsigned char net, unsigned char node, char *request, char *response, int nbyte);
static int write_object(unsigned long nr, unsigned char net, unsigned char node, char *request, char *response, int nbyte);
static int send_message(unsigned long nr, unsigned char net, char *request, char *response, int nbyte);
static int recv_message(unsigned long nr, unsigned char net, char *response, int nbyte);

static int make_string(char *buffer, int nbyte);
static int make_base64(char *buffer, int length, int nbyte);
static int make_error(char *buffer, int nbyte, unsigned long sequence, int code);

static int lookahead(char *line, int *pos);
static int character(char *line, int *pos);
static int token(char *line, int *pos);
static int compare(char *string, char *keyword);

static int ascii2unsigned8(char *line, int *pos, unsigned char *value);
static int ascii2unsigned16(char *line, int *pos, unsigned short *value);
static int ascii2unsigned32(char *line, int *pos, unsigned long *value);
static int ascii2integer8(char *line, int *pos, char *value);
static int ascii2integer16(char *line, int *pos, short *value);
static int ascii2integer32(char *line, int *pos, long *value);

static int ascii2domain(char *line, int *pos, unsigned char *buffer, int *length, int nbyte);
static int ascii2string(char *line, int *pos, char *buffer, int *length, int nbyte);

/*  -----------  variables  ------------------------------------------------
 */


/*  -----------  functions  ------------------------------------------------
 */

int cop_tcp_parse(char *request, COP_TCP_SETTINGS *settings, char *response, int nbyte)
{
	unsigned long sequence = 0;
	unsigned char net = 0, number1;
	unsigned char node = 0, number2;
	unsigned short timeout;
	unsigned short baudrate;
	unsigned short heartbeat;
	unsigned char master;
	time_t now = time(NULL);
	size_t prefix;
	int pos = 0, chr;
	
	if(!request || !response || !settings)
		return make_error(response, nbyte, sequence, ERROR_FATAL);
	/* scan the literal '[' */
	if((chr = lookahead(request, &pos)) == -1)
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	if((chr = character(request, &pos)) != '[')
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	/* scan the <sequence> */
	if((chr = lookahead(request, &pos)) == -1)
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	if(!ascii2unsigned32(request, &pos, &sequence))
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	/* scan the literal ']' */
	if((chr = lookahead(request, &pos)) == -1)
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	if((chr = character(request, &pos)) != ']')
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	/* scan the [[<net>] <node>] */
	if((chr = lookahead(request, &pos)) == -1)
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	if(DECIMAL(chr)) {
		if(!ascii2unsigned8(request, &pos, &number1))
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		if(DECIMAL(chr)) {
			if(!ascii2unsigned8(request, &pos, &number2))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			node = number2;
			net = number1;
		}
		else {
			node = number1;
			net = settings->net;
		}
	}
	else {
		node = settings->node;
		net = settings->net;
	}
	/* scan the command */
	switch(token(request, &pos)) {
	case DISABLE:
		/* token DISABLE read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan next token */
		switch(token(request, &pos)) {
		case GUARDING:
			//@ToDo: disable node guarding
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		case HEARTBEAT:
			//@ToDo: disable heartbeat
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		default:
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		}
		break;
	case ENABLE:
		/* token ENABLE read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan next token */
		switch(token(request, &pos)) {
		case GUARDING:
			//@ToDo: enable node guarding
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		case HEARTBEAT:
			//@ToDo: enable heartbeat
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		default:
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		}
		break;
	case INFO:
		/* token INFO read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan next token */
		switch(token(request, &pos)) {
		case VERSION:
			/* token VERSION read:  */
			snprintf(response, nbyte, "[%lu] 0xFFFFFFFF 1 4.0 20090207 1 1.1 1\r\n", sequence); // FIXME!
			break;
		case FIRMWARE:
			/* token FIRMWARE read:  */
			snprintf(response, nbyte, "[%lu] \"", sequence);
			prefix = strlen(response);
			snprintf(response, nbyte, "[%lu] \"%s", sequence, cop_software());
			make_string(&response[prefix], nbyte - prefix);
			break;
		case HARDWARE:
			/* token HARDWARE read:  */
			snprintf(response, nbyte, "[%lu] \"", sequence);
			prefix = strlen(response);
			snprintf(response, nbyte, "[%lu] \"%s", sequence, cop_hardware());
			make_string(&response[prefix], nbyte - prefix);
			break;
		case SOFTWARE:
			/* token SOFTWARE read:  */
			snprintf(response, nbyte, "[%lu] \"", sequence);
			prefix = strlen(response);
			snprintf(response, nbyte, "[%lu] \"%s", sequence, cop_version());
			make_string(&response[prefix], nbyte - prefix);
			break;
		case COPYRIGHT:
			/* token COPYRIGHT read:  */
			snprintf(response, nbyte, "[%lu] \"Copyright (C) 2008-%u UV Software, Friedrichshafen.\"\r\n", sequence, 1900+localtime(&now)->tm_year);
			break;
		default:
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		}
		break;
	case INIT:
		/* token INIT read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan the <baudrate> */
		if(!ascii2unsigned16(request, &pos, &baudrate))
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		switch(baudrate) {
		case 1000: baudrate = 0; break;
		case 800: baudrate = 1; break;
		case 500: baudrate = 2; break;
		case 250: baudrate = 3; break;
		case 125: baudrate = 4; break;
		case 100: baudrate = 5; break;
		case 50: baudrate = 6; break;
		case 20: baudrate = 7; break;
		case 10: baudrate = 8; break;
		}
		/* execute Initialize Gateway command */
		if(cop_reset((BYTE)baudrate) != COPERR_NOERROR) {
			return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
		}
		snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
		break;
	case PREOPERATIONAL:
		/* token PREOPERATIONAL read: execute NMT Enter Pre-operational command */
		if(nmt_enter_preoperational(node) != COPERR_NOERROR) {
			return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
		}
		snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
		break;
	case READ:
		/* token READ read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		if(DECIMAL(chr)) {
			/* execute Upload SDO command */
			return read_object(sequence, net, node, &request[pos], response, nbyte);
		}
		else {
			/* scan next token */
			switch(token(request, &pos)) {
			case ERROR:
				//@ToDo: read the f*cking manual!
				return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
			case PDO:
				//@ToDo: read the f*cking manual!
				return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
			default:
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			}
		}
		break;
	case RESET:
		/* token RESET read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan next token */
		switch(token(request, &pos)) {
		case NODE:
			/* token NODE read: execute NMT Reset Node command */
			if(nmt_reset_node(node) != COPERR_NOERROR) {
				return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
			}
			snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
			break;
		case COMMUNICATION:
			/* token NODE read: execute NMT Reset Communication command */
			if(nmt_reset_communication(node) != COPERR_NOERROR) {
				return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
			}
			snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
			break;
		default:
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		}
		break;
	case RECEIVE:
		/* token RECEIVE read: execute Receive Message command */
		return recv_message(sequence, net, response, nbyte);
	case RESTORE:
		/* token RESTORE read: we want not support it! */
		return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
	case SEND:
		/* token SEND read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* execute Send Message command */
		return send_message(sequence, net, &request[pos], response, nbyte);
		break;
	case SET:
		/* token SET read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan next token */
		switch(token(request, &pos)) {
		case HEARTBEAT:
			/* token HEARTBEAT read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* scan the <value> */
			if(!ascii2unsigned16(request, &pos, &heartbeat))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			//@ToDo: set heartbeat time of the local node!
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		case ID:
			/* token ID read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* scan the <value> */
			if(!ascii2unsigned8(request, &pos, &master))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			if(master < 1 || 127 < master)
				return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
			//@ToDo: set node-id. of the local node!
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		case NETWORK:
			/* token NETWORK read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* scan the <value> */
			if(!ascii2unsigned8(request, &pos, &net))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			if(net != 1)
				return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
			/* execute Set Default Network command */
			settings->net = net;
			snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
			break;
		case NODE:
			/* token NODE read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* scan the <value> */
			if(!ascii2unsigned8(request, &pos, &node))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			if(node < 1 || 127 < node)
				return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
			/* execute Set Default Node-Id. command */
			settings->node = node;
			snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
			break;
		case RPDO:
			/* token RPDO read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			//@ToDo: configure a Receive-PDO!
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		case SDO_TIME_OUT:
			/* token SDO_TIMEOUT read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* scan the <milliseconds> */
			if(!ascii2unsigned16(request, &pos, &timeout))
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			/* execute Configure SDO Time-out command */
			timeout = sdo_timeout(timeout);
			snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
			break;
		case TPDO:
			/* token TPDO read: */
			if((chr = lookahead(request, &pos)) == -1)
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			//@ToDo: configure a Transmit-PDO!
			return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
		default:
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		}
		break;
	case START:
		/* token START read: execute NMT Start Node command */
		if(nmt_start_remote_node(node) != COPERR_NOERROR) {
			return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
		}
		snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
		break;
	case STOP:
		/* token STOP read: execute NMT Stop Node command */
		if(nmt_stop_remote_node(node) != COPERR_NOERROR) {
			return make_error(response, nbyte, sequence, ERROR_NOT_PROCESSED);
		}
		snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
		break;
	case STORE:
		/* token STORE read: we want not support it! */
		return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
	case WAIT:
		/* token WAIT read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* scan the <milliseconds> */
		if(!ascii2unsigned16(request, &pos, &timeout))
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		/* execute Wait command */
		if(timeout < 1000)
			usleep((long)timeout * 1000L);
		else
			sleep((unsigned)timeout / 1000);
		snprintf(response, nbyte, "[%lu] OK\r\n", sequence);
		break;
	case WRITE:
		/* token WRITE read: */
		if((chr = lookahead(request, &pos)) == -1)
			return make_error(response, nbyte, sequence, ERROR_SYNTAX);
		if(DECIMAL(chr)) {
			/* execute Download SDO command */
			return write_object(sequence, net, node, &request[pos], response, nbyte);
		}
		else {
			/* scan next token */
			switch(token(request, &pos)) {
			case PDO:
				/* token PDO read: */
				if((chr = lookahead(request, &pos)) == -1)
					return make_error(response, nbyte, sequence, ERROR_SYNTAX);
				//@ToDo: read the f*cking manual!
				return make_error(response, nbyte, sequence, ERROR_NOT_SUPPORTED);
			default:
				return make_error(response, nbyte, sequence, ERROR_SYNTAX);
			}
		}
		break;
	default:
		return make_error(response, nbyte, sequence, ERROR_SYNTAX);
	}
	return 0;
}

int cop_tcp_sequence(char *string, unsigned long *sequence)
{
	int pos = 0, chr;
	
	if(!string || !sequence)
		return ERROR_FATAL;
	/* scan the literal '[' */
	if((chr = lookahead(string, &pos)) == -1)
		return ERROR_SYNTAX;
	if((chr = character(string, &pos)) != '[')
		return ERROR_SYNTAX;
	/* scan the <sequence> */
	if((chr = lookahead(string, &pos)) == -1)
		return ERROR_SYNTAX;
	if(!ascii2unsigned32(string, &pos, sequence))
		return ERROR_SYNTAX;
	/* scan the literal ']' */
	if((chr = lookahead(string, &pos)) == -1)
		return ERROR_SYNTAX;
	if((chr = character(string, &pos)) != ']')
		return ERROR_SYNTAX;
	/* sequence number read */
	return 0;
}

/*  -----------  local functions  ------------------------------------------
 */

static int read_object(unsigned long nr, unsigned char net, unsigned char node, char *request, char *response, int nbyte)
{
	unsigned short index;
	unsigned char subindex;
	CHAR int8; SHORT int16; LONG int32;
	BYTE uint8; WORD uint16; DWORD uint32;
	BYTE time_of_day[6];
	SHORT length = 0;
	size_t prefix;
	int pos = 0;
	long rc;
	
	/* scan the <index> */
	if(!ascii2unsigned16(request, &pos, &index))
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	/* scan the <sub-index> */
	if(!ascii2unsigned8(request, &pos, &subindex))
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	/* scan the <datatype> */
	switch(token(request, &pos)) {
	case INTEGER8:
		if((rc = sdo_read_8bit(node, index, subindex, (BYTE*)&int8)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] %+i\r\n", nr, int8);
		break;
	case INTEGER16:
		if((rc = sdo_read_16bit(node, index, subindex, (WORD*)&int16)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] %+i\r\n", nr, int16);
		break;
	case INTEGER32:
		if((rc = sdo_read_32bit(node, index, subindex, (DWORD*)&int32)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] %+li\r\n", nr, int32);
		break;
	case UNSIGNED8:
		if((rc = sdo_read_8bit(node, index, subindex, (BYTE*)&uint8)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] 0x%X\r\n", nr, uint8);
		break;
	case UNSIGNED16:
		if((rc = sdo_read_16bit(node, index, subindex, (WORD*)&uint16)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] 0x%X\r\n", nr, uint16);
		break;
	case UNSIGNED32:
		if((rc = sdo_read_32bit(node, index, subindex, (DWORD*)&uint32)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] 0x%lX\r\n", nr, uint32);
		break;
	case VISIBLE_STRING:
		snprintf(response, nbyte, "[%lu] \"", nr);
		prefix = strlen(response);
		if((rc = sdo_read(node, index, subindex, &length, (BYTE*)&response[prefix], nbyte - prefix - 1)) == COPERR_NOERROR)
			make_string(&response[prefix], nbyte - prefix);
		break;
	case OCTET_STRING:
	case DOMAIN:
		snprintf(response, nbyte, "[%lu] ", nr);
		prefix = strlen(response);
		if((rc = sdo_read(node, index, subindex, &length, (BYTE*)&response[prefix], nbyte - prefix - 1)) == COPERR_NOERROR)
			make_base64(&response[prefix], length, nbyte - prefix);
		break;
	case TIME_OF_DAY:
	case TIME_DIFFERENCE:
		if((rc = sdo_read(node, index, subindex, &length, (BYTE*)&time_of_day[0], sizeof(time_of_day))) == COPERR_NOERROR) {
			uint32 = (DWORD)time_of_day[0] << 0;
			uint32 |= (DWORD)time_of_day[1] << 8;
			uint32 |= (DWORD)time_of_day[2] << 16;
			uint32 |= (DWORD)time_of_day[3] << 24;
			uint16 = (WORD)time_of_day[4] << 0;
			uint16 |= (WORD)time_of_day[5] << 8;
			snprintf(response, nbyte, "[%lu] %u %lu\r\n", nr, uint16, uint32);
		}
		break;
	//@ToDo: To be continued...
	case UNICODE_STRING:
	case REAL32:
		return make_error(response, nbyte, nr, ERROR_NOT_SUPPORTED);
	default:
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	}
	if(rc < COPERR_NOERROR) {
		if(rc != COPERR_TIMEOUT)
			return make_error(response, nbyte, nr, ERROR_NOT_PROCESSED);
		else
			return make_error(response, nbyte, nr, ERROR_TIMEOUT);
	}
	else if(rc > COPERR_NOERROR) {
		snprintf(response, nbyte, "[%lu] Error: 0x%lX\r\n", nr, rc);
	}
	net = net;
	return rc;
}

static int write_object(unsigned long nr, unsigned char net, unsigned char node, char *request, char *response, int nbyte)
{
	unsigned short index;
	unsigned char subindex;
	CHAR int8; SHORT int16; LONG int32;
	BYTE uint8; WORD uint16; DWORD uint32;
	BYTE time_of_day[6];
	int pos = 0, off, len;
	long rc;
	
	/* scan the <index> */
	if(!ascii2unsigned16(request, &pos, &index))
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	/* scan the <sub-index> */
	if(!ascii2unsigned8(request, &pos, &subindex))
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	/* scan the <datatype> */
	switch(token(request, &pos)) {
	case INTEGER8:
		if(!ascii2integer8(request, &pos, &int8))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_8bit(node, index, subindex, (BYTE)int8)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case INTEGER16:
		if(!ascii2integer16(request, &pos, &int16))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_16bit(node, index, subindex, (WORD)int16)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case INTEGER32:
		if(!ascii2integer32(request, &pos, &int32))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_32bit(node, index, subindex, (DWORD)int32)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case UNSIGNED8:
		if(!ascii2unsigned8(request, &pos, &uint8))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_8bit(node, index, subindex, (BYTE)uint8)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case UNSIGNED16:
		if(!ascii2unsigned16(request, &pos, &uint16))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_16bit(node, index, subindex, (WORD)uint16)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case UNSIGNED32:
		if(!ascii2unsigned32(request, &pos, &uint32))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write_32bit(node, index, subindex, (DWORD)uint32)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case VISIBLE_STRING:
		off = pos;
		if(!ascii2string(request, &pos, (char*)&request[off], &len, nbyte - pos))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write(node, index, subindex, (SHORT)len, (BYTE*)&request[off])) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case OCTET_STRING:
	case DOMAIN:
		off = pos;
		if(!ascii2domain(request, &pos, (unsigned char*)&request[off], &len, nbyte - pos))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write(node, index, subindex, (SHORT)len, (BYTE*)&request[off])) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	case TIME_OF_DAY:
	case TIME_DIFFERENCE:
		if(!ascii2unsigned16(request, &pos, (unsigned short*)&time_of_day[4]))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if(!ascii2unsigned32(request, &pos, (unsigned long*)&time_of_day[0]))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		if((rc = sdo_write(node, index, subindex, (SHORT)6, (BYTE*)&time_of_day[0])) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
		break;
	//@ToDo: To be continued...
	case UNICODE_STRING:
	case REAL32:
		return make_error(response, nbyte, nr, ERROR_NOT_SUPPORTED);
	default:
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	}
	if(rc < COPERR_NOERROR) {
		if(rc != COPERR_TIMEOUT)
			return make_error(response, nbyte, nr, ERROR_NOT_PROCESSED);
		else
			return make_error(response, nbyte, nr, ERROR_TIMEOUT);
	}
	else if(rc > COPERR_NOERROR) {
		snprintf(response, nbyte, "[%lu] Error: 0x%lX\r\n", nr, rc);
	}
	net = net;
	return rc;
}

static int send_message(unsigned long nr, unsigned char net, char *request, char *response, int nbyte)
{
	LONG cob; BYTE length, data[8] = {0,0,0,0,0,0,0,0};
	SHORT dlc, i;
	char buffer[6];
	int pos = 0, chr;
	long rc;
	
	/* scan the <cob-id> */
	if(!ascii2unsigned32(request, &pos, (unsigned long*)&cob))
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	/* standard or rtr frame? */
	if((chr = lookahead(request, &pos)) == -1)
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	if(DECIMAL(chr)) {
		/* scan the <length> */
		if(!ascii2unsigned8(request, &pos, &length))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		dlc = (SHORT)length;
		/* scan the [<data-byte>...] */
		for(i = 0; i < dlc && i < 8; i++) {
			if(!ascii2unsigned8(request, &pos, &data[i]))
				return make_error(response, nbyte, nr, ERROR_SYNTAX);
		}
		/* transmit the request */
		if((rc = cop_transmit(cob, dlc, data)) == COPERR_NOERROR)
			snprintf(response, nbyte, "[%lu] OK\r\n", nr);
	}
	else if(token(request, &pos) == RTR) {
		/* scan the <length> */
		if(!ascii2unsigned8(request, &pos, &length))
			return make_error(response, nbyte, nr, ERROR_SYNTAX);
		dlc = (SHORT)length;
		/* request the request */
		if((rc = cop_request(cob, &dlc, data)) == COPERR_NOERROR) {
			snprintf(response, nbyte, "[%lu] %i", nr, dlc);
			for(i = 0; i < dlc && i < 8; i++) {
				snprintf(buffer, 6, " 0x%X", data[i]);
				strncat(response, buffer, nbyte-strlen(response));
			}
			strncat(response, "\r\n", nbyte-strlen(response));
		}
	}
	else {
		return make_error(response, nbyte, nr, ERROR_SYNTAX);
	}
	if(rc != COPERR_NOERROR) {
		return make_error(response, nbyte, nr, ERROR_NOT_PROCESSED);
	}
	net = net;
	return rc;
}

static int recv_message(unsigned long nr, unsigned char net, char *response, int nbyte)
{
	LONG cob; BYTE data[8] = {0,0,0,0,0,0,0,0};
	SHORT dlc, i;
	char buffer[6];
	long rc;

	if((rc = cop_queue_read(&cob, &dlc, data)) == COPERR_NOERROR) {
		snprintf(response, nbyte, "[%lu] 0x%03lX %i", nr, cob, dlc);
		for(i = 0; i < dlc && i < 8; i++) {
			snprintf(buffer, 6, " 0x%X", data[i]);
			strncat(response, buffer, nbyte-strlen(response));
		}
		strncat(response, "\r\n", nbyte-strlen(response));		
	}
	else if(rc == COPERR_RX_EMPTY) {
		snprintf(response, nbyte, "[%lu] OK\r\n", nr);
	}
	else {
		return make_error(response, nbyte, nr, ERROR_NOT_PROCESSED);
	}
	net = net;
	return rc;
}

static int make_string(char *buffer, int nbyte)
{
	int i, j, l;
	
	if(!buffer)
		return -1;
	l = strlen(buffer);
	for(i = 0; i < l; i++) {
		if(buffer[i] == '\"') {
			for(j = (l + 1 < nbyte)? l : l - 1, l = j + 1; i <= j; j--)
				buffer[j+1] = buffer[j];
			buffer[i++] = '\"';
			buffer[l] = '\0';
		}
	}
	if(l + 1 < nbyte) {
		buffer[l++] = '\"';
		buffer[l] = '\0';
	}
	if(l + 1 < nbyte) {
		buffer[l++] = '\r';
		buffer[l] = '\0';
	}
	if(l + 1 < nbyte) {
		buffer[l++] = '\n';
		buffer[l] = '\0';
	}
	return l;
}

static int make_base64(char *buffer, int length, int nbyte)
{
	int i, j, l, L;
	
	if(!buffer)
		return -1;

	l = length;
	L = (((l + 2) / 3) * 4);
	
	if(L >= nbyte)
		L = nbyte - 1;

	for(j = (((L - 1) / 4) * 4), i = ((j / 4) * 3); j >= 0 && i >= 0; j -= 4, i -=3) {
		base64_encode((unsigned char*)&buffer[i], ((l - i) < 3 ? (l - i) : 3), (unsigned char*)&buffer[j], ((L - j) < 4 ? (L - j) : 4));
	}
	buffer[L] = '\0';

	if(L + 1 < nbyte) {
		buffer[L++] = '\r';
		buffer[L] = '\0';
	}
	if(L + 1 < nbyte) {
		buffer[L++] = '\n';
		buffer[L] = '\0';
	}
	return L;
}

static int make_error(char *buffer, int nbyte, unsigned long sequence, int code)
{
	snprintf(buffer, nbyte, "[%lu] Error: %i\r\n", sequence, code);
	return code;
}

static int lookahead(char *line, int *pos)
{
	if(!line || !pos)
		return -1;
	while(line[*pos]) {
		if(!WHITESPACE(line[*pos]))
			return line[*pos];
		else
			*pos += 1;
	}
	return -1;
}

static int character(char *line, int *pos)
{
	int chr;
	
	if(!line || !pos)
		return -1;
	if((chr = line[*pos]))
		*pos += 1;
	else
		chr = -1;
	return chr;
}

static int token(char *line, int *pos)
{
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(compare(&line[*pos], "b")) {
		*pos += strlen("b");
		return BOOLEAN;
	}
	if(compare(&line[*pos], "communication")) {
		*pos += strlen("communication");
		return COMMUNICATION;
	}
	if(compare(&line[*pos], "comm")) {
		*pos += strlen("comm");
		return COMMUNICATION;
	}
	if(compare(&line[*pos], "copyright")) {
		*pos += strlen("copyright");
		return COPYRIGHT;
	}
	if(compare(&line[*pos], "disable")) {
		*pos += strlen("disable");
		return DISABLE;
	}
	if(compare(&line[*pos], "d")) {
		*pos += strlen("d");
		return DOMAIN;
	}
	if(compare(&line[*pos], "emcy")) {
		*pos += strlen("emcy");
		return EMCY;
	}
	if(compare(&line[*pos], "enable")) {
		*pos += strlen("enable");
		return ENABLE;
	}
	if(compare(&line[*pos], "error")) {
		*pos += strlen("error");
		return ERROR;
	}
	if(compare(&line[*pos], "event")) {
		*pos += strlen("event");
		return EVENT;
	}
	if(compare(&line[*pos], "execute")) {
		*pos += strlen("execute");
		return EXECUTE;
	}
	if(compare(&line[*pos], "exec")) {
		*pos += strlen("exec");
		return EXECUTE;
	}
	if(compare(&line[*pos], "exit")) {
		*pos += strlen("exit");
		return EXIT;
	}
	if(compare(&line[*pos], "e")) {
		*pos += strlen("e");
		return ERROR;
	}
	if(compare(&line[*pos], "firmware")) {
		*pos += strlen("firmware");
		return FIRMWARE;
	}
	if(compare(&line[*pos], "guarding")) {
		*pos += strlen("guarding");
		return GUARDING;
	}
	if(compare(&line[*pos], "hardware")) {
		*pos += strlen("hardware");
		return HARDWARE;
	}
	if(compare(&line[*pos], "heartbeat")) {
		*pos += strlen("heartbeat");
		return HEARTBEAT;
	}
	if(compare(&line[*pos], "i8")) {
		*pos += strlen("i8");
		return INTEGER8;
	}
	if(compare(&line[*pos], "i16")) {
		*pos += strlen("i16");
		return INTEGER16;
	}
	if(compare(&line[*pos], "i24")) {
		*pos += strlen("i24");
		return INTEGER24;
	}
	if(compare(&line[*pos], "i32")) {
		*pos += strlen("i32");
		return INTEGER32;
	}
	if(compare(&line[*pos], "i40")) {
		*pos += strlen("i40");
		return INTEGER40;
	}
	if(compare(&line[*pos], "i48")) {
		*pos += strlen("i48");
		return INTEGER48;
	}
	if(compare(&line[*pos], "i56")) {
		*pos += strlen("i56");
		return INTEGER56;
	}
	if(compare(&line[*pos], "i64")) {
		*pos += strlen("i64");
		return INTEGER64;
	}
	if(compare(&line[*pos], "id")) {
		*pos += strlen("id");
		return ID;
	}
	if(compare(&line[*pos], "info")) {
		*pos += strlen("info");
		return INFO;
	}
	if(compare(&line[*pos], "init")) {
		*pos += strlen("init");
		return INIT;
	}
	if(compare(&line[*pos], "network")) {
		*pos += strlen("network");
		return NETWORK;
	}
	if(compare(&line[*pos], "node")) {
		*pos += strlen("node");
		return NODE;
	}
	if(compare(&line[*pos], "ok")) {
		*pos += strlen("ok");
		return OK;
	}
	if(compare(&line[*pos], "os")) {
		*pos += strlen("os");
		return OCTET_STRING;
	}
	if(compare(&line[*pos], "pdo")) {
		*pos += strlen("pdo");
		return PDO;
	}
	if(compare(&line[*pos], "preoperational")) {
		*pos += strlen("preoperational");
		return PREOPERATIONAL;
	}
	if(compare(&line[*pos], "preop")) {
		*pos += strlen("preop");
		return PREOPERATIONAL;
	}
	if(compare(&line[*pos], "p")) {
		*pos += strlen("p");
		return PDO;
	}
	if(compare(&line[*pos], "r32")) {
		*pos += strlen("r32");
		return REAL32;
	}
	if(compare(&line[*pos], "r64")) {
		*pos += strlen("r64");
		return REAL64;
	}
	if(compare(&line[*pos], "read")) {
		*pos += strlen("read");
		return READ;
	}
	if(compare(&line[*pos], "receive")) {
		*pos += strlen("receive");
		return RECEIVE;
	}
	if(compare(&line[*pos], "recv")) {
		*pos += strlen("recv");
		return RECEIVE;
	}
	if(compare(&line[*pos], "restore")) {
		*pos += strlen("restore");
		return RESTORE;
	}
	if(compare(&line[*pos], "reset")) {
		*pos += strlen("reset");
		return RESET;
	}
	if(compare(&line[*pos], "rpdo")) {
		*pos += strlen("rpdo");
		return RPDO;
	}
	if(compare(&line[*pos], "rtr")) {
		*pos += strlen("rtr");
		return RTR;
	}
	if(compare(&line[*pos], "r")) {
		*pos += strlen("r");
		return READ;
	}
	if(compare(&line[*pos], "sdo_timeout")) {
		*pos += strlen("sdo_timeout");
		return SDO_TIME_OUT;
	}
	if(compare(&line[*pos], "set")) {
		*pos += strlen("set");
		return SET;
	}
	if(compare(&line[*pos], "send")) {
		*pos += strlen("send");
		return SEND;
	}
	if(compare(&line[*pos], "software")) {
		*pos += strlen("software");
		return SOFTWARE;
	}
	if(compare(&line[*pos], "start")) {
		*pos += strlen("start");
		return START;
	}
	if(compare(&line[*pos], "stop")) {
		*pos += strlen("stop");
		return STOP;
	}
	if(compare(&line[*pos], "store")) {
		*pos += strlen("store");
		return STORE;
	}
	if(compare(&line[*pos], "sync")) {
		*pos += strlen("sync");
		return SYNC;
	}
	if(compare(&line[*pos], "td")) {
		*pos += strlen("td");
		return TIME_DIFFERENCE;
	}
	if(compare(&line[*pos], "tpdo")) {
		*pos += strlen("tpdo");
		return TPDO;
	}
	if(compare(&line[*pos], "t")) {
		*pos += strlen("t");
		return TIME_OF_DAY;
	}
	if(compare(&line[*pos], "u8")) {
		*pos += strlen("u8");
		return UNSIGNED8;
	}
	if(compare(&line[*pos], "u16")) {
		*pos += strlen("u16");
		return UNSIGNED16;
	}
	if(compare(&line[*pos], "u24")) {
		*pos += strlen("u24");
		return UNSIGNED24;
	}
	if(compare(&line[*pos], "u32")) {
		*pos += strlen("u32");
		return UNSIGNED32;
	}
	if(compare(&line[*pos], "u40")) {
		*pos += strlen("u40");
		return UNSIGNED40;
	}
	if(compare(&line[*pos], "u48")) {
		*pos += strlen("u48");
		return UNSIGNED48;
	}
	if(compare(&line[*pos], "u56")) {
		*pos += strlen("u56");
		return UNSIGNED56;
	}
	if(compare(&line[*pos], "u64")) {
		*pos += strlen("u64");
		return UNSIGNED64;
	}
	if(compare(&line[*pos], "user")) {
		*pos += strlen("user");
		return USER;
	}
	if(compare(&line[*pos], "us")) {
		*pos += strlen("us");
		return UNICODE_STRING;
	}
	if(compare(&line[*pos], "version")) {
		*pos += strlen("version");
		return VERSION;
	}
	if(compare(&line[*pos], "vs")) {
		*pos += strlen("vs");
		return VISIBLE_STRING;
	}
	if(compare(&line[*pos], "wait")) {
		*pos += strlen("wait");
		return WAIT;
	}
	if(compare(&line[*pos], "write")) {
		*pos += strlen("write");
		return WRITE;
	}
	if(compare(&line[*pos], "w")) {
		*pos += strlen("w");
		return WRITE;
	}
	return -1;
}

static int compare(char *string, char *keyword)
{
	char *p1, *p2;
	
	if(!string || !keyword)
		return 0;
	for(p1 = string, p2 = keyword; *p2 != '\0'; p1++, p2++) {
		if(*p1 == '\0')
			return 0;
		if(tolower(*p1) != tolower(*p2))
			return 0;
	}
	return 1;
}

static int ascii2unsigned8(char *line, int *pos, unsigned char *value)
{
	unsigned char num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned char)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned char)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned char)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned char)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 2)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 2 && num > 037)
	       		n = 4;
			num = (num << 3) + (unsigned char)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 3)
			return 0;
		else
			return 1;
	}
	if(DECIMAL(line[*pos])) {
		/* decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 2 && num > 25)
	       		n = 4;
			if(n == 2 && num == 25 && line[*pos] > '5')
	       		n = 4;
			num = (num * 10) + (unsigned char)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 3)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2integer8(char *line, int *pos, char *value)
{
	char num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned char)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned char)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned char)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned char)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 2)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 2 && num > 037)
	       		n = 4;
			num = (num << 3) + (unsigned char)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 3)
			return 0;
		else
			return 1;
	}
	if(line[*pos] == '-') {
		*pos += 1;
		if(DECIMAL(line[*pos])) {
			/* negative decimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; DECIMAL(line[*pos]); *pos += 1, n++) {
				if(n == 2 && num > 12)
		       		n = 4;
				if(n == 2 && num == 12 && line[*pos] > '8')
		       		n = 4;
				num = (num * 10) + (unsigned char)(line[*pos] - '0');
			}
			if(value)
				*value = num * (-1);
			if(n > 3)
				return 0;
			else
				return 1;
		}
	}
	if(line[*pos] == '+') {
		*pos += 1;
	}
	if(DECIMAL(line[*pos])) {
		/* positive decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 2 && num > 12)
	       		n = 4;
			if(n == 2 && num == 12 && line[*pos] > '7')
	       		n = 4;
			num = (num * 10) + (unsigned char)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 3)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2unsigned16(char *line, int *pos, unsigned short *value)
{
	unsigned short num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned short)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned short)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned short)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned short)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 4)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 5 && num > 017777)
	       		n = 7;
			num = (num << 3) + (unsigned short)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 6)
			return 0;
		else
			return 1;
	}
	if(DECIMAL(line[*pos])) {
		/* decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 4 && num > 6553)
	       		n = 6;
			if(n == 4 && num == 6553 && line[*pos] > '5')
	       		n = 6;
			num = (num * 10) + (unsigned short)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 5)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2integer16(char *line, int *pos, short *value)
{
	short num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned short)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned short)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned short)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned short)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 4)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 5 && num > 017777)
	       		n = 7;
			num = (num << 3) + (unsigned short)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 6)
			return 0;
		else
			return 1;
	}
	if(line[*pos] == '-') {
		*pos += 1;
		if(DECIMAL(line[*pos])) {
			/* negative decimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; DECIMAL(line[*pos]); *pos += 1, n++) {
				if(n == 4 && num > 3276)
		       		n = 6;
				if(n == 4 && num == 3276 && line[*pos] > '8')
		       		n = 6;
				num = (num * 10) + (unsigned short)(line[*pos] - '0');
			}
			if(value)
				*value = num * (-1);
			if(n > 5)
				return 0;
			else
				return 1;
		}
	}
	if(line[*pos] == '+') {
		*pos += 1;
	}
	if(DECIMAL(line[*pos])) {
		/* positive decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 4 && num > 3276)
	       		n = 6;
			if(n == 4 && num == 3276 && line[*pos] > '7')
	       		n = 6;
			num = (num * 10) + (unsigned short)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 5)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2unsigned32(char *line, int *pos, unsigned long *value)
{
	unsigned long num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned long)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned long)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned long)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned long)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 8)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 10 && num > 03777777777UL)
	       		n = 12;
			num = (num << 3) + (unsigned long)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 11)
			return 0;
		else
			return 1;
	}
	if(DECIMAL(line[*pos])) {
		/* decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 9 && num > 429496729UL)
	       		n = 11;
			if(n == 9 && num == 429496729UL && line[*pos] > '5')
	       		n = 11;
			num = (num * 10) + (unsigned long)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 10)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2integer32(char *line, int *pos, long *value)
{
	long num = 0;
	int n = 0;
	
	if(value)
		*value = 0;
	if(!line || !pos)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '0') {
		num = (unsigned long)(line[*pos] - '0');
		*pos += 1;
		if(line[*pos] == 'x' || line[*pos] == 'X') {
			*pos += 1;
			/* hexadecimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; HEXADECIMAL(line[*pos]); *pos += 1, n++) {
				if(DECIMAL(line[*pos]))
					num = (num << 4) + (unsigned long)(line[*pos] - '0');
				else if('a' <= line[*pos] && line[*pos] <= 'f')
					num = (num << 4) + (unsigned long)(line[*pos] - 'a' + 10);
				else if('A' <= line[*pos] && line[*pos] <= 'F')
					num = (num << 4) + (unsigned long)(line[*pos] - 'A' + 10);
			}
			if(value)
				*value = num;
			if(n > 8)
				return 0;
			else
				return 1;
		}
		/* octal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; OCTAL(line[*pos]); *pos += 1, n++) {
			if(n == 10 && num > 03777777777UL)
	       		n = 12;
			num = (num << 3) + (unsigned long)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 11)
			return 0;
		else
			return 1;
	}
	if(line[*pos] == '-') {
		*pos += 1;
		if(DECIMAL(line[*pos])) {
			/* positive decimal number */
			for(; '0' == line[*pos]; *pos += 1)
				;
			for(; DECIMAL(line[*pos]); *pos += 1, n++) {
				if(n == 9 && num > 214748364L)
		       		n = 11;
				if(n == 9 && num == 214748364L && line[*pos] > '8')
		       		n = 11;
				num = (num * 10) + (unsigned long)(line[*pos] - '0');
			}
			if(value)
				*value = num * (-1L);
			if(n > 10)
				return 0;
			else
				return 1;
		}
	}
	if(line[*pos] == '+') {
		*pos += 1;
	}
	if(DECIMAL(line[*pos])) {
		/* positive decimal number */
		for(; '0' == line[*pos]; *pos += 1)
			;
		for(; DECIMAL(line[*pos]); *pos += 1, n++) {
			if(n == 9 && num > 214748364L)
	       		n = 11;
			if(n == 9 && num == 214748364L && line[*pos] > '7')
	       		n = 11;
			num = (num * 10) + (unsigned long)(line[*pos] - '0');
		}
		if(value)
			*value = num;
		if(n > 10)
			return 0;
		else
			return 1;
	}
	return 0;
}

static int ascii2domain(char *line, int *pos, unsigned char *buffer, int *length, int nbyte)
{
	int n;
	
	if(!line || !pos || !buffer || !length)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	for(n = 0, *length = 0; BASE64(line[*pos]) /*|| line[*pos] == '='*/; *pos += 1, n++) {
		if(n == 4) {
			*length += base64_decode((unsigned char*)&line[*pos-4], 4, &buffer[*length], (*length + 3) < nbyte ? 3 : nbyte - *length);
			n = 0;
		}
	}
	if(n == 4) {
		*length += base64_decode((unsigned char*)&line[*pos-4], 4, &buffer[*length], (*length + 3) < nbyte ? 3 : nbyte - *length);
	}
	if(n == 3) {
		*length += base64_decode((unsigned char*)&line[*pos-3], 3, &buffer[*length], (*length + 3) < nbyte ? 3 : nbyte - *length);
	}
	if(n == 2) {
		*length += base64_decode((unsigned char*)&line[*pos-2], 2, &buffer[*length], (*length + 2) < nbyte ? 2 : nbyte - *length);
	}
	if(n == 1) {
		*length += base64_decode((unsigned char*)&line[*pos-1], 1, &buffer[*length], (*length + 1) < nbyte ? 1 : nbyte - *length);
	}
	for(; line[*pos] == '='; *pos += 1)
		;
	return 1;
}

static int ascii2string(char *line, int *pos, char *buffer, int *length, int nbyte)
{
	if(!line || !pos || !buffer || !length)
		return 0;
	for(; WHITESPACE(line[*pos]); *pos += 1)
		;
	if(line[*pos] == '\"') {
		for(*pos += 1, *length = 0; ' ' <=  line[*pos] /*&& line[*pos] <= '\177'*/ && (line[*pos] != '\"' || line[*pos+1] == '\"'); *pos += 1) {
			if(line[*pos] == '\"' && line[*pos+1] == '\"')
				*pos += 1;
			if(*length + 1 < nbyte) {
				buffer[*length] = line[*pos];
				*length += 1;
			}
		}
	}
	else {
		for(*length = 0; ' ' <  line[*pos] /*&& line[*pos] <= '\177'*/; *pos += 1) {
			if(*length + 1 < nbyte) {
				buffer[*length] = line[*pos];
				*length += 1;
			}
		}
	}
	if(*length < nbyte) 
		buffer[*length] = '\0';
	return 1;
}

/*  -----------  command syntax  -------------------------------------------
 */

void cop_tcp_syntax(FILE *stream)
{
	fprintf(stream, "1. SDO access commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "1.1 Upload SDO command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<upload-sdo-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] (\"read\"|\'r\') <index> <sub-index> <datatype>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<upload-sdo-response> ::= \'[\'<sequence>\']\' <value> |\n");
	fprintf(stream, "                          \'[\'<sequence>\']\' \"Error:\" <sdo-abort-code> |\n");
	fprintf(stream, "                          \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "1.2 Download SDO command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<download-sdo-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] (\"write\"|\'w\') <index> <sub-index> <datatype> <value>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<download-sdo-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                            \'[\'<sequence>\']\' \"Error:\" <sdo-abort-code> |\n");
	fprintf(stream, "                            \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "1.3 Configure SDO timeout command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-sdo-timeout-request>  ::= \'[\'<sequence>\']\' [<net>] \"set\" \"sdo_timeout\" <milliseconds>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-sdo-timeout-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                               \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "2. PDO access commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "2.1 Configure RPDO command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-rpdo-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"set\" \"rpdo\" ...\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-rpdo-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "2.2 Configure TPDO command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-tpdo-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"set\" \"tpdo\" ...\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-tpdo-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "2.3 Read PDO data command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-pdo-request>  ::= \'[\'<sequence>\']\' [<net>] (\"read\"|\'r\') (\"pdo\"|\'p\') ...\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-pdo-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "3. CANopen NMT commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.1 Start node command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<start-node-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"start\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<start-node-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                          \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.2 Stop node command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<stop-node-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"stop\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<stop-node-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                         \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.3 Set node to pre-operational command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-pre-operational-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] (\"preoperational\"|\"preop\")\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-pre-operational-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                                   \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.4 Reset node command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<reset-node-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"reset\" \"node\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<reset-node-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                          \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.5 Reset communication command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<reset-communication-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"reset\" (\"communication\"|\"comm\")\n");
	fprintf(stream, "\n");
	fprintf(stream, "<reset-communication-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                                   \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.6 Enable node guarding command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<enable-guarding-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"enable\" \"guarding\" <guarding-time> <lifetime-factor>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<enable-guarding-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.7 Disable node guarding command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<disable-guarding-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"disable\" \"guarding\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<disable-guarding-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.8 Start heartbeat consumer command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<enable-heartbeat-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"enable\" \"heartbeat\" <heartbeat-time>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<enable-heartbeat-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "3.9 Stop heartbeat consumer command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<disable-heartbeat-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] \"disable\" \"heartbeat\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<disable-heartbeat-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "4. Device failure management commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "4.1 Read device error command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-error-request>  ::= \'[\'<sequence>\']\' [[<net>] <node>] (\"read\"|\'r\') \"error\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-error-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "5. CANopen interface configuration commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "5.1 Initialize gateway command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<initialize-request>  ::= \'[\'<sequence>\']\' [<net>] \"init\" <baudrate-index>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<initialize-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                          \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "5.2 Store configuration command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<store-configuration-request>  ::= \'[\'<sequence>\']\' [<net>] \"store\" [\"CFG\" | \"PDO\" | \"SDO\" | \"NMT\"]\n");
	fprintf(stream, "\n");
	fprintf(stream, "<store-configuration-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "5.3 Restore configuration command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<restore-configuration-request>  ::= \'[\'<sequence>\']\' [<net>] \"restore\" [\"CFG\" | \"PDO\" | \"SDO\" | \"NMT\"]\n");
	fprintf(stream, "\n");
	fprintf(stream, "<restore-configuration-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "5.4 Set heartbeat producer command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-heartbeat-request>  ::= \'[\'<sequence>\']\' [<net>] \"set\" \"heartbeat\" <heartbeat-time>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-heartbeat-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "5.5 Set node-id command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-id-request>  ::= \'[\'<sequence>\']\' [<net>] \"set\" \"id\" <node-id>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-id-response> ::= \'[\'<sequence>\']\' \"Error: 100\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "6. Gateway management commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "6.1 Set default network command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-network-request>  ::= \'[\'<sequence>\']\' \"set\" \"network\" <network>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-network-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                           \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "6.2 Set default node-id command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-node-request>  ::= \'[\'<sequence>\']\' [<net>] \"set\" \"node\" <node>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<set-node-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                        \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "6.3 Get version command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<get-version-request>  ::= \'[\'<sequence>\']\' \"info\" \"version\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<get-version-response> ::= \'[\'<sequence>\']\' <vendor-id> <product-code> <revision-number> <serial-number> <gateway-class> <protocol-version> <implementation-class> |\n");
	fprintf(stream, "                           \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "7. Vendor-specific commands\n");
	fprintf(stream, "\n");
	fprintf(stream, "7.1 Send CAN message command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<send-message-request>  ::= \'[\'<sequence>\']\' [<net>] \"send\" <cob-id> <length> {<value>}*\n");
	fprintf(stream, "\n");
	fprintf(stream, "<send-message-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                            \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "7.2 Request CAN message command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<remote-message-request>  ::= \'[\'<sequence>\']\' [<net>] \"send\" <cob-id> \"rtr\" <length>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<remote-message-response> ::= \'[\'<sequence>\']\' <length> {<value>}* |\n");
	fprintf(stream, "                              \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "7.3 Read CAN message queue command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-message-request>  ::= \'[\'<sequence>\']\' [<net>] \"recv\"\n");
	fprintf(stream, "\n");
	fprintf(stream, "<read-message-response> ::= \'[\'<sequence>\']\' <cob-id> <length> {<value>}* |\n");
	fprintf(stream, "                            \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                            \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "7.4 Wait command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<wait-request>  ::= \'[\'<sequence>\']\' [<net>] \"wait\" <milliseconds>\n");
	fprintf(stream, "\n");
	fprintf(stream, "<wait-response> ::= \'[\'<sequence>\']\' \"OK\" |\n");
	fprintf(stream, "                    \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "7.5 Get information command\n");
	fprintf(stream, "\n");
	fprintf(stream, "<get-information-request>  ::= \'[\'<sequence>\']\' [<net>] \"info\" (\"hardware\" | \"firmware\" | \"software\" | \"copyright\")\n");
	fprintf(stream, "\n");
	fprintf(stream, "<get-information-response> ::= \'[\'<sequence>\']\' <string> |\n");
	fprintf(stream, "                               \'[\'<sequence>\']\' \"Error:\" <error-code>\n");
	fprintf(stream, "\n");
	fprintf(stream, "8. Miscellaneous\n");
	fprintf(stream, "\n");
	fprintf(stream, "8.1 Supported data types\n");
	fprintf(stream, "\n");
	fprintf(stream, "\"i8\"  = 8-bit signed integer value\n");
	fprintf(stream, "\"i16\" = 16-bit signed integer value\n");
	fprintf(stream, "\"i32\" = 32-bit signed integer value\n");
	fprintf(stream, "\"u8\"  = 8-bit unsigned integer value\n");
	fprintf(stream, "\"u16\" = 16-bit unsigned integer value\n");
	fprintf(stream, "\"u32\" = 32-bit unsigned integer value\n");
	fprintf(stream, "\"t\"   = time of day: days milliseconds\n");
	fprintf(stream, "\"td\"  = time difference: days milliseconds\n");
	fprintf(stream, "\"vs\"  = visible string\n");
	fprintf(stream, "\"os\"  = octet string\n");
	fprintf(stream, "\"d\"   = domain\n");
	fprintf(stream, "\n");
	fprintf(stream, "8.2 Value encoding\n");
	fprintf(stream, "\n");
	fprintf(stream, "Numerical values have to be encoded according to ISO/IEC 9899 (ANSI C).\n");
	fprintf(stream, "A visible strings with whitespaces have to be enclosed with double quotes.\n");
	fprintf(stream, "A double quote within a visible string have to be escaped by second double quote.\n");
	fprintf(stream, "Values of type domain or octet string have to be encoded according to RfC 2045 (MIME).\n");
	fprintf(stream, "\n");
	fprintf(stream, "8.3 Error codes\n");
	fprintf(stream, "\n");
	fprintf(stream, "\"Error: 100\" = Request not supported\n");
	fprintf(stream, "\"Error: 101\" = Syntax error\n");
	fprintf(stream, "\"Error: 102\" = Request not executed\n");
	fprintf(stream, "\"Error: 103\" = Time-out occurred\n");
	fprintf(stream, "\"Error: 999\" = Fatal error\n");
	fprintf(stream, "\n");
	fprintf(stream, "9. Further information\n");
	fprintf(stream, "\n");
	fprintf(stream, "CiA DS-301, CANopen application layer and communication profile, version 4.02\n");
	fprintf(stream, "CiA DS-309, Interfacing CANopen with TCP/IP, part 1 and 3, version 1.1\n");
	fprintf(stream, "\n");
}

/*  -----------  revision control  -----------------------------------------
 */

char* cop_tcp_version()
{
	return (char*)_id;
}

/*  -------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
