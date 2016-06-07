/*	-- $Header$ --
 *
 *	projekt   :  CAN - Controller Area Network
 *
 *	purpose   :  CAN Controller Interface (socketCAN)
 *
 *	copyright :  (c) 2007-2009, UV Software, Friedrichshafen
 *
 *	compiler  :  GCC - GNU C Compiler (Linux Kernel 2.6)
 *
 *	export    :  (see header file)
 *
 *	includes  :  can_ctrl.h (can_defs.h)
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	CAN Controller Interface.
 *
 *	For berliOS socketCAN (v2.17) over PEAK PCAN-USB Dongle. For information
 *	about socketCAN see "http://socketcan.berlios.de/".
 *
 *
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log$
 */

static char _id[] = "$Id: can_ctrl.c 29 2009-02-11 10:27:50Z saturn $";


/*  -----------  includes  -------------------------------------------------
 */

#include "can_ctrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>


/*  -----------  defines  --------------------------------------------------
 */

#ifndef OK
#define OK						  0		// Positives Ergebnis, heiÃt kein Fehler!
#endif
#define CAN_ERROR			     -10000	// socketCAN Fehler ('errno' gesetzt)
#define CAN_FATAL			     -99	// Schwerwiegender Fehler

#ifdef _CAN_EVENT_QUEUE				// CAN event-queue
#ifndef CAN_EVENT_QUEUE_SIZE
#define CAN_EVENT_QUEUE_SIZE	  16384 // Size of queue for message object 15
#endif
#if     CAN_EVENT_QUEUE_SIZE < 2
 #error The size of the rx queue have to be greater than 1!
#endif
#define NEXT(index)			   (((index) + 1) % CAN_EVENT_QUEUE_SIZE)
#define CLEAR()					  head = tail
#define EMPTY()					 (head == tail)
#define OVERRUN()				 (NEXT(head) == tail)
#endif

/*  -----------  types  ----------------------------------------------------
 */

typedef struct _msg_obj					// message object:
{
	BYTE  control;						//   transmit, receive or remote
	DWORD cob_id;						//   COB-Id. (11-bit or 29-bit)
	short count;						//   number of received messgaes
	short length;						//   number of received data bytes
	BYTE  data[8];						//   received data bytes (0,..,8)
	DWORD time_stamp;					//   time-stamp in [ms]
}	MSG_OBJ;
typedef struct _can_que					// queue item:
{
	DWORD cob_id;						//   COB-Id. of the message
	short length;						//   lenght of the message
	BYTE  data[8];						//   data of the message
	DWORD time_stamp;					//   time-stamp in [ms]
}	MSG_QUE;


/*  -----------  prototypes  -----------------------------------------------
 */

static int can_read_queue(int count);	// read RCV queue
static int can_read_socket(struct can_frame *msg);


/*  -----------  variables  ------------------------------------------------
 */

BYTE can_baudrate = -1;					// index to the bit-timing table

static int   fd = -1;					// file descriptor (itÂ´s a socket)
static char  ifname[IFNAMSIZ] = "";		// interface name
static int   family = PF_CAN;			// protocol family
static int   type = SOCK_RAW;			// communication semantics
static int   protocol = CAN_RAW;		// protocol to be used with the socket
static char  hardware[256];				// hardware version of the CAN interface board
static char  software[256];				// software version of the PCAN-Light interface
static int   init = FALSE;				// initialization flag of interface

static const WORD bit_timing[9] = {		// bit-timing table:
	1000,								//   1000 Kbps
	/* n/a */ 0,						//    800 Kbps
	500,								//    500 Kbps
	250,								//    250 Kbps
	125,								//    125 Kbps
	100,								//    100 Kbps
	50,									//     50 Kbps
	20,									//     20 Kbps
	10									//     10 Kbps
};
static  CAN_STATE can_state = {0x80};	// 8-bit status register
static  __u64 llUntilStop = 0;			// variable for time-out

static  MSG_OBJ msg_buf[15];			// message buffer (15x)
#ifdef _CAN_EVENT_QUEUE
 static MSG_QUE msg_que[CAN_EVENT_QUEUE_SIZE];
 static int head = 0, tail = 0;			// queue for message object 15
 static int queue_error = CANERR_NOERROR;
 static int queue_enabled = FALSE;
#endif
static BYTE  que_load = 0x00;			// queue load


/*  -----------  functions  ------------------------------------------------
 */

short can_init(long board, void *param)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	
	if(init)							// must not be initialized!
		return CANERR_YETINIT;
	switch(board)						// supported CAN boards
	{
	case CAN_NETDEV:					//   socketCAN interface
		if(param == NULL)				//     null-pointer assignement?
			return CANERR_NULLPTR;		//       error!
		
		strncpy(ifname, ((struct _can_param*)param)->ifname, IFNAMSIZ);
		family = ((struct _can_param*)param)->family;
		type = ((struct _can_param*)param)->type;
		protocol = ((struct _can_param*)param)->protocol;
		
		if((fd = socket(family, type, protocol)) < 0)
		{
			return CANERR_SOCKET;
		}
		strcpy(ifr.ifr_name, ifname);
		ioctl(fd, SIOCGIFINDEX, &ifr);

		addr.can_family = family;
		addr.can_ifindex = ifr.ifr_ifindex;

		if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			return CANERR_SOCKET;
    	}
    	//@ToDo: reset CAN controller?
    	//       (not supported on berliOS)
    	//@ToDo: set filter for all messages
    	//
    	//@ToDo: set filter for error frames
    	//
		break;
	default:							//   unknown CAN board
		return CANERR_ILLPARA;
	}
	msg_buf[0].control = 0;				// clear message object 1
	msg_buf[1].control = 0;				// clear message object 2
	msg_buf[2].control = 0;				// clear message object 3
	msg_buf[3].control = 0;				// clear message object 4
	msg_buf[4].control = 0;				// clear message object 5
	msg_buf[5].control = 0;				// clear message object 6
	msg_buf[6].control = 0;				// clear message object 7
	msg_buf[7].control = 0;				// clear message object 8
	msg_buf[8].control = 0;				// clear message object 9
	msg_buf[9].control = 0;				// clear message object 10
	msg_buf[10].control = 0;			// clear message object 11
	msg_buf[11].control = 0;			// clear message object 12
	msg_buf[12].control = 0;			// clear message object 13
	msg_buf[13].control = 0;			// clear message object 14
	msg_buf[14].control = 0;			// clear message object 15
	#ifdef _CAN_EVENT_QUEUE
	 can_queue_enable();				// enbale event-queue
	#endif
	can_state.byte = 0x80;				// CAN controller not started yet!
	init = TRUE;						// set initialization flag
	return OK;
}

short can_exit(void)
{
	if(init)							// must be initialized!
	{
		close(fd);						//   close the socket
	}
	can_state.byte |= 0x80;				// CAN controller in INIT state
	strcpy(ifname, "");					// interface name
	family = PF_CAN;					// protocol family
	type = SOCK_RAW;					// communication semantics
	protocol = CAN_RAW;					// protocol to be used with the socket
	init = FALSE;						// clear initialization flag
	return OK;
}

short can_start(BYTE baudrate)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(!can_state.b.can_stopped)		// must be stopped!
		return CANERR_ONLINE;
	if(/*(baudrate < CANBDR_1000) ||*/ (CANBDR_10 < baudrate) || (CANBDR_800 == baudrate))
		return CANERR_BAUDRATE;
	//@ToDo: set baud rate!
	//       (not supported on berliOS)
	//@ToDo: start CAN controller?
	//       (not supported on berliOS)
	can_baudrate = baudrate;			// index to the bit-timing table
	can_state.b.can_stopped = 0;	 	// CAN controller started!
	return OK;
}

short can_reset(void)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(!can_state.b.can_stopped) {		// CAN started, then reset
		if(can_baudrate == CANBDR_800)	//   800 kBit/s not supported!
			return CANERR_BAUDRATE;		//     ==> error!
    	//@ToDo: reset CAN controller?
    	//       (not supported on berliOS)
	}
	can_state.b.can_stopped = 1;	 	// CAN controller stopped!
	return OK;
}
short can_status(BYTE *status)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
//@ToDo: status
return CANERR_NOTSUPP;
	if(status)							// status-register
	  *status = can_state.byte;
	return OK;
}

short can_busload(BYTE *load, BYTE *status)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
//@ToDo: status
return CANERR_NOTSUPP;
	if(status)							// status-register
	  *status = can_state.byte;
	if(load)							// queue-load
	  *load = que_load;
	return OK;
}

short can_config(short index, long cob_id, WORD service)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(index < 0 || 14 < index)			// message object 1 .. 15
		return CANERR_ILLPARA;
	if(cob_id < 0 || 0x7FF < cob_id)	// standard (11-bit) identifier
		return CANERR_ILLPARA;
	if((service & 0x00FF) < CANMSG_TRANSMIT ||  CANMSG_REQUEST < (service & 0x00FF))
		return CANERR_ILLPARA;
	if((service & 0x00FF) < CANMSG_RECEIVE && index >= 14)
		return CANERR_ILLPARA;
	#ifdef _PCAN_EVENT_QUEUE
	 if(queue_enabled && index >= 14)	// 15 used as FIFO?
		return CANERR_ILLPARA;
	#endif
	if((service & 0xFF00) > 0x0800)		// max. 8 data bytes
		return CANERR_ILLPARA;
	can_read_queue(CAN_RCV_QUEUE_READ);	//read CAN messages(!)

	msg_buf[index].control = (BYTE)(service & 0x00FF);
	msg_buf[index].length = (short)(service & 0xFF00) >> 8;
	msg_buf[index].cob_id = (DWORD)(cob_id);
	msg_buf[index].count  = (short)(0);
	memset(msg_buf[index].data, 0x00, 8);

#ifdef __TO_DO__
	if((msg_buf[index].control == CANMSG_REQUEST) &&
	   !can_state.b.can_stopped)
	{
		can_msg.MSGTYPE = MSGTYPE_RTR;	// request remote frame
		can_msg.ID = (DWORD)(msg_buf[index].cob_id);
		can_msg.LEN = (BYTE)(msg_buf[index].length);
							 msg_buf[index].count++;
							 msg_buf[index].time_stamp = -1;
		if((rc = PCAN_Write(&can_msg)) != CAN_ERR_OK)
		{
			if((rc & CAN_ERR_QXMTFULL)){//   transmit queue full?
				can_state.b.transmitter_busy = 1;
				return CANERR_TX_BUSY;	//     transmitter busy!
			}
			else if((rc & CAN_ERR_XMTFULL)){//transmission pending?
				can_state.b.transmitter_busy = 1;
				return CANERR_TX_BUSY;	//     transmitter busy!
			}
			if(rc > 255)				//   PCAN specific error?
				return pcan_error(rc);
		}
		can_state.b.transmitter_busy = 0;//message transmitted!
		msg_buf[index].count = 0;
	}
#endif
	return CANERR_NOERROR;				// OK!
}

short can_delete(short index)
{
	if(index < 0 || 14 < index)			// message object 1 .. 15
		return CANERR_ILLPARA;
	msg_buf[index].control = 0;			// reset the message object
	msg_buf[index].count = 0;
	return CANERR_NOERROR;				// OK!
}

short can_transmit(short index, short length, BYTE *data)
{
	struct can_frame frame;
	int nbytes;

	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(can_state.b.can_stopped)			// must be running!
		return CANERR_OFFLINE;
	if(index < 0 || 13 < index)			// message object 1 .. 14
		return CANERR_ILLPARA;
	if(length < 0 || 8 < length)		// data length 0 .. 8
		return CANERR_ILLPARA;
	if(data == NULL)					// null-pointer assignment!
		data = msg_buf[index].data;
	if((msg_buf[index].control != CANMSG_TRANSMIT) &&
	   (msg_buf[index].control != CANMSG_UPDATE))
		return CANERR_ILLPARA;

	frame.can_id = (DWORD)(msg_buf[index].cob_id);
	frame.can_dlc = (BYTE)(msg_buf[index].length = length);
						   msg_buf[index].count++;
						   msg_buf[index].time_stamp = -1;
	memcpy(frame.data, data, length);

	if((nbytes = write(fd, &frame, sizeof(struct can_frame))) != sizeof(struct can_frame))
	{
		//@ToDo: evaluate result
		can_state.b.transmitter_busy = 1;//   transmitter busy!
		return CANERR_TX_BUSY;
	}
	can_state.b.transmitter_busy = 0;	// message transmitted!
	msg_buf[index].count = 0;
	return CANERR_NOERROR;				// OK!
}

short can_update(short index, int length, BYTE *data)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(can_state.b.can_stopped)			// must be running!
		return CANERR_OFFLINE;
	if(index < 0 || 13 < index)			// message object 1 .. 14
		return CANERR_ILLPARA;
	if(length < 0 || 8 < length)		// data length 0 .. 8
		return CANERR_ILLPARA;
	if(data == NULL)					// null-pointer assignment!
		data = msg_buf[index].data;
	if((msg_buf[index].control != CANMSG_UPDATE) &&
	   (msg_buf[index].control != CANMSG_TRANSMIT))
		return CANERR_ILLPARA;
	return CANERR_NOTSUPP;				// NOT SUPPORTED!
}

short can_busy(short index)
{
	if(!init)							// must be initialized!
		return FALSE;
	if(can_state.b.can_stopped)			// must be running!
		return FALSE;
	if(index < 0 || 13 < index)			// message object 1 .. 14
		return FALSE;
	if(msg_buf[index].control != CANMSG_TRANSMIT)
		return FALSE;
	return msg_buf[index].count;		// transmitter busy?
}

short can_receive(short index, short *length, BYTE *data)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(can_state.b.can_stopped)			// must be running!
		return CANERR_OFFLINE;
	if(index < 0 || 14 < index)			// message object 1 .. 15
		return CANERR_ILLPARA;
	if(length == NULL || data == NULL)	// null pointer assignment
		return CANERR_NULLPTR;
	#ifdef _CAN_EVENT_QUEUE
	 if(queue_enabled && index >= 14)	// 15 used as FIFO?
		return CANERR_ILLPARA;
	#endif
	if((msg_buf[index].control != CANMSG_RECEIVE) &&
	   (msg_buf[index].control != CANMSG_REQUEST))
		return CANERR_ILLPARA;
	can_read_queue(CAN_RCV_QUEUE_READ);//read CAN messages

	if(!msg_buf[index].count) {			// no message read?
		can_state.b.receiver_empty = 1;		
		return CANERR_RX_EMPTY;			//   receiver empty!
	}
   *length =     msg_buf[index].length;	// data length code
	memcpy(data, msg_buf[index].data, msg_buf[index].length);
	can_state.b.receiver_empty = 0;		// message read!
	can_state.b.message_lost |= (msg_buf[index].count > 1);
	msg_buf[index].count = 0;
	return CANERR_NOERROR;				// OK!
}

short can_receive_id(short index, short *length, BYTE *data, long *cob_id)
{
	if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	if(can_state.b.can_stopped)			// must be running!
		return CANERR_OFFLINE;
	if(index < 0 || 14 < index)			// message object 1 .. 15
		return CANERR_ILLPARA;
	if(length == NULL || data == NULL)	// null pointer assignment
		return CANERR_NULLPTR;
	#ifdef _CAN_EVENT_QUEUE
	 if(queue_enabled && index >= 14)	// 15 used as FIFO?
		return CANERR_ILLPARA;
	#endif
	if((msg_buf[index].control != CANMSG_RECEIVE) &&
	   (msg_buf[index].control != CANMSG_REQUEST))
		return CANERR_ILLPARA;
	can_read_queue(CAN_RCV_QUEUE_READ);//read CAN messages

	if(!msg_buf[index].count) {			// no message read?
		can_state.b.receiver_empty = 1;		
		return CANERR_RX_EMPTY;			//   receiver empty!
	}
   *cob_id = (long)msg_que[tail].cob_id;// COB-identifier ($error: 06-10-04)
   *cob_id =     msg_buf[index].cob_id; // COB-identifier
   *length =     msg_buf[index].length;	// data length code
	memcpy(data, msg_buf[index].data, msg_buf[index].length);
	can_state.b.receiver_empty = 0;		// message read!
	can_state.b.message_lost |= (msg_buf[index].count > 1);
	msg_buf[index].count = 0;
	return CANERR_NOERROR;				// OK!
}

short can_data(short index)
{
	if(!init)							// must be initialized!
		return FALSE;
	if(can_state.b.can_stopped)			// must be running!
		return FALSE;
	if(index < 0 || 14 < index)			// message object 1 .. 15
		return FALSE;
	#ifdef _CAN_EVENT_QUEUE
	 if(queue_enabled && index >= 14)	// 15 used as FIFO?
		return FALSE;
	#endif
	if((msg_buf[index].control != CANMSG_RECEIVE) &&
	   (msg_buf[index].control != CANMSG_REQUEST))
		return FALSE;
	can_read_queue(CAN_RCV_QUEUE_READ);	//read CAN messages

	if(msg_buf[index].count)			// new data received?
		return TRUE;
	else								// receiver still empty!
		return FALSE;
}

short can_queue_get_message(long *cob_id, short *length, BYTE *data)
{
	#ifdef _CAN_EVENT_QUEUE
	 if(!init)							// must be initialized!
		return CANERR_NOTINIT;
	 if(can_state.b.can_stopped)		// must be running!
		return CANERR_OFFLINE;
	 if(!cob_id || !length || !data)	// null-pointer assignment!
		return CANERR_NULLPTR;
	 can_read_queue(CAN_RCV_QUEUE_READ);//read CAN messages

	 if(EMPTY())						// queue empty?
		return queue_error = CANQUE_EMPTY;
	*cob_id = (long)msg_que[tail].cob_id;// COB-identifier
	*length =       msg_que[tail].length;// data length code
	 memcpy(data, msg_que[tail].data, msg_que[tail].length);
	 tail = NEXT(tail);
	 return queue_error = CANQUE_NOERROR;// message de-queued!
	#else
	 if(!cob_id || !length || !data)	// null-pointer assignment!
		return CANERR_NULLPTR;
	 return CANQUE_EMPTY;
	#endif
}

short can_queue_enable(void)
{
	#ifdef _CAN_EVENT_QUEUE
	 if(!queue_enabled) {
		CLEAR();						// clear queue
		// Queue is initialized!
		queue_error = CANQUE_NOERROR;
		queue_enabled = TRUE;
	 }
	 return CANERR_NOERROR;
	#else
	 return CANERR_NOTSUPP;
	#endif
}

short can_queue_disable(void)
{
	#ifdef _CAN_EVENT_QUEUE
	 queue_enabled = FALSE;				// queue disabled
	 return CANERR_NOERROR;
	#else
	 return CANERR_NOTSUPP;
	#endif
}

short can_queue_empty(void)
{
	#ifdef _CAN_EVENT_QUEUE
	 return EMPTY();					// queue empty?
	#else
	 return TRUE;
	#endif
}

short can_queue_clear(void)
{
	#ifdef _CAN_EVENT_QUEUE
	 if(EMPTY())						// queue empty?
		return CANQUE_EMPTY;
	 CLEAR();							// clear queue
	 return CANQUE_NOERROR;
	#else
	 return CANQUE_EMPTY;
	#endif
}

short can_queue_status(void)
{
	#ifdef _CAN_EVENT_QUEUE
	 register int error = queue_error;	// copy last error code

	 queue_error = CANQUE_NOERROR;		// clear last error code
	 return error;
	#else
	 return CANQUE_EMPTY;
	#endif
}

short can_start_timer(WORD timeout)
{
	struct timeval tv;					// timer value
	gettimeofday(&tv, NULL);			// current time
	
	llUntilStop = ((__u64)tv.tv_sec * (__u64)1000000) + (__u64)tv.tv_usec \
	            + ((__u64)timeout   * (__u64)1000);
	
	return OK;
}

short can_is_timeout(void)
{
	__u64 llNow;						// 64-bit value
	struct timeval tv;					// timer value
	gettimeofday(&tv, NULL);			// current time
	
	llNow = ((__u64)tv.tv_sec * (__u64)1000000) + (__u64)tv.tv_usec;

	if(llNow < llUntilStop)
		return FALSE;
	else
		return TRUE;
}

LPSTR can_hardware(void)
{
	sprintf(hardware, "interface=\"%s\", family=%d, type=%d, protocol=%d", ifname, family, type, protocol);
	return (char*)hardware;				// hardware version
}

LPSTR can_software(void)
{
	sprintf(software, "berliOS socketCAN (http://socketcan.berlios.de/)");
	return (char*)software;				// software version
}

LPSTR can_version(void)
{
	return (LPSTR)_id;					// revision number
}

/*  -----------  local functions  ------------------------------------------
 */

static int can_read_socket(struct can_frame *msg)
{
	fd_set rdfs;
	struct timeval timeo;
	
    FD_ZERO(&rdfs);
    FD_SET(fd, &rdfs);
    
    timeo.tv_sec  = 0;
    timeo.tv_usec = 0;

    if(select(fd+1, &rdfs, NULL, NULL, &timeo) < 0)
    {
    	return 0;
    }
	if(!FD_ISSET(fd, &rdfs))
    {
    	return 0;
    }
	if(read(fd, msg, sizeof(struct can_frame)) != sizeof(struct can_frame))
	{
		return 0;
	}
	return 1;
}

static int can_read_queue(int count)
{
	struct can_frame can_msg;			// the message
	int   i, n = 0;						// buffer index

	if(!init)							// must be initialized!
		return 0;
	if(count) {							// read n messages
		for(n = 0; n < count; n++) {
			if(can_read_socket(&can_msg)) {
				if((can_msg.can_id & (CAN_EFF_FLAG | CAN_ERR_FLAG)) == 0x00000000) {
					#ifdef _CAN_EVENT_QUEUE
					 for(i = 0; i <= 13; i++) {
					#else
					 for(i = 0; i <= 14; i++) {
					#endif
						if((msg_buf[i].control == CANMSG_RECEIVE ||
							msg_buf[i].control == CANMSG_REQUEST) &&
						   (msg_buf[i].cob_id == (can_msg.can_id & CAN_SFF_MASK))) {
							memcpy(msg_buf[i].data, can_msg.data, can_msg.can_dlc);
							msg_buf[i].length = can_msg.can_dlc;
							msg_buf[i].count++;
							msg_buf[i].time_stamp = -1;
							break;
						}
		
					}
					#ifdef _CAN_EVENT_QUEUE
					 if(queue_enabled && i == 14) {
						memcpy(msg_que[head].data, can_msg.data, can_msg.can_dlc);
						msg_que[head].length = can_msg.can_dlc;
						msg_que[head].cob_id = (can_msg.can_id & CAN_SFF_MASK);
						msg_que[head].time_stamp = -1;
						head = NEXT(head);		//     message enqueued
						if(OVERRUN()) {			//     on queue overrun:
							tail = NEXT(tail);	//       delet oldest message
							queue_error = CANQUE_OVERRUN;
						}
						can_state.b.queue_overrun = (queue_error == CANQUE_OVERRUN);
					 }
					#endif
				}
				else if((can_msg.can_id & CAN_ERR_FLAG) == CAN_ERR_FLAG) {
					/* *** **
					can_state.b.bus_off = (can_msg.DATA[3] & CAN_ERR_BUSOFF) != CAN_ERR_OK;
					can_state.b.bus_error = (can_msg.DATA[3] & CAN_ERR_BUSHEAVY) != CAN_ERR_OK;
					can_state.b.warning_level = (can_msg.DATA[3] & CAN_ERR_BUSLIGHT) != CAN_ERR_OK;
					can_state.b.message_lost |= (can_msg.DATA[3] & CAN_ERR_OVERRUN) != CAN_ERR_OK;
					** *** */
				}
			}
			else {
				que_load = (BYTE)(((long)n * 100L) / (long)count);
				return n;
			}
		}
		que_load = (BYTE)(((long)n * 100L) / (long)count);
		return n;
	}
	else {							// read all messages
		while(can_read_socket(&can_msg)) {
			if((can_msg.can_id & (CAN_EFF_FLAG | CAN_ERR_FLAG)) == 0x00000000) {
				#ifdef _CAN_EVENT_QUEUE
				 for(i = 0; i <= 13; i++) {
				#else
				 for(i = 0; i <= 14; i++) {
				#endif
					if((msg_buf[i].control == CANMSG_RECEIVE ||
						msg_buf[i].control == CANMSG_REQUEST) &&
					   (msg_buf[i].cob_id == (can_msg.can_id & CAN_SFF_MASK))) {
						memcpy(msg_buf[i].data, can_msg.data, can_msg.can_dlc);
						msg_buf[i].length = can_msg.can_dlc;
						msg_buf[i].count++;
						msg_buf[i].time_stamp = -1;
						break;
					}
	
				}
				#ifdef _CAN_EVENT_QUEUE
				 if(queue_enabled && i == 14) {
					memcpy(msg_que[head].data, can_msg.data, can_msg.can_dlc);
					msg_que[head].length = can_msg.can_dlc;
					msg_que[head].cob_id = (can_msg.can_id & CAN_SFF_MASK);
					msg_que[head].time_stamp = -1;
					head = NEXT(head);		//     message enqueued
					if(OVERRUN()) {			//     on queue overrun:
						tail = NEXT(tail);	//       delet oldest message
						queue_error = CANQUE_OVERRUN;
					}
					can_state.b.queue_overrun = (queue_error == CANQUE_OVERRUN);
				 }
				#endif
				n++;
			}
			else if((can_msg.can_id & CAN_ERR_FLAG) == CAN_ERR_FLAG) {
				/* *** **
				can_state.b.bus_off = (can_msg.DATA[3] & CAN_ERR_BUSOFF) != CAN_ERR_OK;
				can_state.b.bus_error = (can_msg.DATA[3] & CAN_ERR_BUSHEAVY) != CAN_ERR_OK;
				can_state.b.warning_level = (can_msg.DATA[3] & CAN_ERR_BUSLIGHT) != CAN_ERR_OK;
				can_state.b.message_lost |= (can_msg.DATA[3] & CAN_ERR_OVERRUN) != CAN_ERR_OK;
				** *** */
			}
		}
		que_load = (BYTE)(((long)n * 100L) / (long)CAN_RCV_QUEUE_SIZE);
		return n;
	}
}

/*  -------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047470, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
