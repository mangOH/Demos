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
 *	export    :  short can_init(long board, void *param);
 *	             short can_exit(void);
 *
 *	             short can_start(BYTE baudrate);
 *	             short can_reset(void);
 *
 *	             short can_status(BYTE *status);
 *	             short can_busload(BYTE *load, BYTE *status);
 *
 *	             short can_config(short index, long cob_id, WORD service);
 *	             short can_delete(short index);
 *
 *	             short can_transmit(short index, short length, BYTE *data);
 *	             short can_update(short index, int length, BYTE *data);
 *	             short can_busy(short index);
 *
 *	             short can_receive(short index, short *length, BYTE *data);
 *	             short can_receive_id(short index, short *length, BYTE *data, long *cob_id);
 *	             short can_data(short index);
 *
 *	             short can_queue_get_message(long *cob_id, short *length, BYTE *data);
 *	             short can_queue_enable(void);
 *	             short can_queue_disable(void);
 *	             short can_queue_empty(void);
 *	             short can_queue_clear(void);
 *	             short can_queue_status(void);
 *
 *	             short can_start_timer(WORD timeout);
 *	             short can_is_timeout(void);
 *
 *	             LPSTR can_hardware(void);
 *	             LPSTR can_software(void);
 *	             LPSTR can_version(void);
 *
 *	includes  :  default.h, can_ctrl.h
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
 *	Note: Only the message buffers 0 to 14 can be used:
 *	      - Message buffer 0 to 13 for transmission.
 *	      - Message buffer 0 to 14 for reception.
 *	      - Message buffer 14 is used for an event
 *	        queue with option _CAN_EVENT_QUEUE.
 *
 *
 *	-----------  history  ---------------------------------------------------
 *
 *	$Log$
 */

#ifndef __CAN_CTRL_H
#define __CAN_CTRL_H


/*  -----------  includes  -------------------------------------------------
 */

#include "default.h"
#include "can_defs.h"


/*  -----------  defines  --------------------------------------------------
 */

#ifndef _CAN_DEFS
 #define CANBDR_1000				 0	// Baud rate: 1000 kBit/s
 #define CANBDR_800					 1	// Baud rate:  800 kBit/s
 #define CANBDR_500					 2	// Baud rate:  500 kBit/s
 #define CANBDR_250					 3	// Baud rate:  250 kBit/s
 #define CANBDR_125					 4	// Baud rate:  125 kBit/s
 #define CANBDR_100					 5	// Baud rate:  100 kBit/s
 #define CANBDR_50					 6	// Baud rate:   50 kBit/s
 #define CANBDR_20					 7	// Baud rate:   20 kBit/s
 #define CANBDR_10					 8	// Baud rate:   10 kBit/s

 #define CANMSG_TRANSMIT			 1	// Transmit message object
 #define CANMSG_UPDATE				 2	// Update for remote request
 #define CANMSG_RECEIVE				 3	// Receive message object
 #define CANMSG_REQUEST				 4	// Request message object
 #define CAN_REQUEST(length)		 ((int)CANMSG_REQUEST | (((int)length) << 8))

 #define CANERR_NOERROR				 0	// No error!
 #define CANERR_BOFF				-1	// CAN - Busoff status
 #define CANERR_EWRN				-2 	// CAN - Error warning status
 #define CANERR_BERR				-3	// CAN - Bus error
 #define CANERR_OFFLINE				-9	// CAN - Not started
 #define CANERR_ONLINE				-8	// CAN - Already started
 #define CANERR_MSG_LST				-10	// CAN - Message lost
 #define CANERR_LEC_STUFF			-11	// LEC - Stuff error
 #define CANERR_LEC_FORM			-12	// LEC - Form error
 #define CANERR_LEC_ACK				-13	// LEC - Acknowledge error
 #define CANERR_LEC_BIT1			-14	// LEC - Recessive bit error
 #define CANERR_LEC_BIT0			-15	// LEC - Dominant bit error
 #define CANERR_LEC_CRC				-16	// LEC - Checksum error
 #define CANERR_TX_BUSY				-20	// USR - Transmitter busy
 #define CANERR_RX_EMPTY			-30	// USR - Receiver empty
 #define CANERR_TIMEOUT				-50	// USR - Time-out
 #define CANERR_BAUDRATE			-91	// USR - Illegal baudrate
 #define CANERR_ILLPARA				-93	// USR - Illegal parameter
 #define CANERR_NULLPTR				-94	// USR - Null-pointer assignement
 #define CANERR_NOTINIT				-95	// USR - Not initialized
 #define CANERR_YETINIT				-96	// USR - Already initialized
 #define CANERR_NOTSUPP				-98	// USR - Not supported
 #define CANERR_FATAL				-99	// USR - Other errors
								
 #define CANQUE_NOERROR				 0	// No error!
 #define CANQUE_MSG_LST				-10	// CAN - Message lost
 #define CANQUE_EMPTY				-30	// USR - Queue empty
 #define CANQUE_OVERRUN				-40	// USR - Queue overrun
#endif

/*  -----------  types  ----------------------------------------------------
 */

#ifndef _CAN_STATE
 typedef union _can_state				// CAN Status-register:
 {
   unsigned char byte;					//   byte access
#ifdef _BIG_ENDIAN
   struct {								//   bit access (MSB first):
     unsigned char can_stopped : 1;		//     CAN controller stopped
     unsigned char bus_off : 1;			//     Busoff status
     unsigned char warning_level : 1;	//     Error warning status
     unsigned char bus_error : 1;		//     Bus error (LEC)
     unsigned char transmitter_busy : 1;//     Transmitter busy
     unsigned char receiver_empty : 1;	//     Receiver empty
     unsigned char message_lost : 1;	//     Message lost
     unsigned char queue_overrun : 1;	//     Event-queue overrun
   } b;
#else
   struct {								//   bit access (LSB first):
     unsigned char queue_overrun : 1;	//     Event-queue overrun
     unsigned char message_lost : 1;	//     Message lost
     unsigned char receiver_empty : 1;	//     Receiver empty
     unsigned char transmitter_busy : 1;//     Transmitter busy
     unsigned char bus_error : 1;		//     Bus error (LEC)
     unsigned char warning_level : 1;	//     Error warning status
     unsigned char bus_off : 1;			//     Busoff status
     unsigned char can_stopped : 1;		//     CAN controller stopped
   } b;
#endif
 } CAN_STATE;
#endif

/*  -----------  variables  ------------------------------------------------
 */

extern BYTE can_baudrate;				// index to the bit-timing table


/*  -----------  prototypes  -----------------------------------------------
 */

short can_init(long board, void *param);
/*
 *	function  :  initializes the on-chip CAN controller and sets the operation
 *	             status to 'stopped'. The bits INIT and CCE of the CAN control
 *	             register are set, no communication is possible in this state,
 *	             all message objects are deleted, but can be configured.
 *
 *	parameter :  board		- type of the CAN Controller interface.
 *	             param		- pointer to board-specific parameters.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_exit(void);
/*
 *	function  :  stops any operation of the CAN controller and sets the operation
 *	             status to 'offline'. The bit INIT of the CAN control register
 *	             is set, no communication is possible.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_start(BYTE baudrate);
/*
 *	function  :  initializes the bit-timing register of the CAN controller with
 *	             the parameters of the bit-timing table selected by the baudrate
 *	             index and sets the operation status to 'running'. The bits INIT
 *	             and CCE of the CAN control register are reset, the communication
 *	             via CAN is started.
 *
 *  parameter :  baudrate	- index (0,..,8) to the bit-timing table.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_reset(void);
/*
 *	function  :  stops any operation of the CAN controller and sets the operation
 *	             status to 'stopped'. The bits INIT and CCE of the CAN control
 *	             register are set, no communication is possible in this state.
 *	             Message objects can be configured.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_status(BYTE *status);
/*
 *	function  :  
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_busload(BYTE *load, BYTE *status);
/*
 *	function  :  
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_config(short index, long cob_id, WORD service);
/*
 *	function  :  configures the message object selected by index with an 11-bit
 *		         identifier for transmission or reception.
 *               The parameter service specifies the type of message object:
 *			     1. Transmit Message Object:
 *	                The message object is configured for the transmission of data.
 *	                The transmission is started by a call to can_transmit and can
 *	                be monitored by calling can_busy.
 *	             2. Update Object for Remote Request
 *	                The message object is configured to hold data which can be
 *	                requested by a remote node. The data are stored by a call to
 *	                can_update and the transmission can be monitored by calling
 *	                can_busy.
 *	             3. Receive Message Object
 *	                The message object is configured for receiving data. The state
 *	                of the reception can be monitored by calling can_data and the
 *	                received data can be read by a call to can_receive.
 *	             4. RTR Message Object
 *	                The message object is configured for receiving data via a RTR
 *	                frame from a remote node. The state of the reception can be
 *	                monitored by calling can_data and the received data can be
 *	                read by a call to can_receive.
 *	             For the configured message object the bits MSGVAL and CPUUPD of
 *	             the message control register are set. Except for RTR message
 *	             object, the bit CPUUPD is cleared and bit TXRQ is set to start
 *	             the transmission of the RTR frame.
 *
 *	             The message object 14 can not be configured for transmission!
 *
 *  parameter :  index (0,..,14) of a message object.
 *	             cob_id (11-bit identifier) of the message object.
 *	             service: CANMSG_TRANSMIT - for a transmit message object
 *		       	          CANMSG_UPDATE   - update object for remote request
 *				          CANMSG_RECEIVE  - for a receive message object
 *				          CANMSG_REQUEST  - for a RTR message object
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_delete(short index);
/*
 *	function  :  deletes the message object selected by index. The bit MSGVAL
 *	             of the message control register is cleared.
 *
 *  parameter :  index (0,..,14) of a message object.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_transmit(short index, short length, BYTE *data);
/*
 *	function  :  transmits the data of the message object selected by
 *               index if no transmission is pending. The bit CPUUPD of
 *               the message control register is cleared and the bits
 *               NEWDAT and TXRQ are set to start the transmission.
 *
 *               The message object must be configured for transmission!
 *
 *  parameter :  index (0,..,13) of a message object.
 *               length (0,..,8) of the message data.
 *               data: pointer to the message data.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_update(short index, int length, BYTE *data);
/*
 *	function  :  updates the data of the message object selected by index
 *               if no transmission is pending. The bit CPUUPD of the 
 *               message control register is cleared and the bit NEWDAT is
 *               set, so the data can be requested by a remote node.
 *
 *               The message object must be configured for transmission!
 *
 *  parameter :  index (0,..,13) of a message object.
 *               length (0,..,8) of the message data.
 *               data: pointer to the message data.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_busy(short index);
/*
 *	function  :  monitors the transmission of the message object selected by
 *	             index. Sets the bit NEWDAT of the message control register
 *	             if bit TXRQ is resetted after a transmission.
 *
 *               The message object must be configured for transmission!
 *
 *	             If a transmission is pending bit transmitter-busy in the
 *	             status-register will be set.
 *
 *  parameter :  index (0,..,13) of a message object.
 *
 *  result    :  non-zero if a transmission is pending, or 0 if not.
 */

short can_receive(short index, short *length, BYTE *data);
/*
 *	function  :  reads	the received data of the message object selected by
 *               index if any and clears the bits NEWDAT and MSGLST of the
 *               message control register. If bit MSGLST was set, unread
 *               data are overwritten by the received data. The function
 *               returns 0 but bit message-lost in the status-register
 *	             will be set.
 *
 *               The message object must be configured for reception!
 *               The function can not be applied to message object 14 if
 *               the event-queue is enabled!
 *
 *  parameter :  index (0,..,14) of a message object.
 *               length (0,..,8) of the received data.
 *               data: pointer to a buffer for the received data.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_receive_id(short index, short *length, BYTE *data, long *cob_id);
/*
 *	function  :  reads	the received data of the message object selected by
 *               index if any and clears the bits NEWDAT and MSGLST of the
 *               message control register. If bit MSGLST was set, unread
 *               data are overwritten by the received data. The function
 *               returns 0 but the bit message-lost in the status-register
 *	             will be set.
 *
 *               The message identifier is either 11-bit (standard) or
 *               29-bit (extended) identifier, depending on bit XTD of
 *               the message configuration register.
 *
 *               The message object must be configured for reception!
 *               The function can not be applied to message object 14 if
 *               the event-queue is enabled!
 *
 *  parameter :  index (0,..,14) of a message object.
 *               length (0,..,8) of the received data.
 *               data: pointer to a buffer for the received data.
 *               cob_id of the received message object.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_data(short index);
/*
 *	function  :  monitors the reception of the message object selected by
 *	             index. 
 *
 *               The message object must be configured for reception!
 *               The function can not be applied to message object 14 if
 *               the event-queue is enabled!
 *
 *  parameter :  index (0,..,14) of a message object.
 *
 *  result    :  non-zero if new data is received, or 0 if not.
 */

short can_queue_get_message(long *cob_id, short *length, BYTE *data);
/*
 *	function  :  reads the first received identifier and data from
 *               the event-queue if any, or returns CANQUE_EMPTY if
 * 		         the queue is empty.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_queue_enable(void);
/*
 *	function  :  enables the reception of messages from message object 14
 *               into the event-queue. The message object is configured
 *               for the reception of all 11-bit identifier, except for
 *               those for which a message object is actually allocated.
 *               The state of the reception can be monitored by calling
 *               can_queue_empty and received messages can be read by
 *               subsequent calls to can_queue_get_message.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_queue_disable(void);
/*
 *	function  :  disables the reception of messages into the event-queue.
 *               The queue will no be cleared.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_queue_empty(void);
/*
 *	function  :  checks if the queue is empty.
 *
 *	parameter :  (none)
 *
 *	result    :  non-zero if the queue is empty, or 0 if not.
 */

short can_queue_clear(void);
/*
 *	function  :  deletes all received messages from the queue and sets
 *               the state of the queue to empty.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_queue_status(void);
/*
 *	function  :  return the error status of the event-queue and resets
 *               it to error-free.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if error-free, or one of the following negative values
 *               if not error-free: CANQUE_MSG_LST  - message lost
 *                                  CANQUE_OVERRUN  - queue overrun
 */

short can_start_timer(WORD timeout);
/*
 *	function  :  starts a software timer for time-out supervision.
 *
 *	parameter :  timeout	- time interval in milliseconds.
 *
 *	result    :  0 if successful, or a negative value on error.
 */

short can_is_timeout(void);
/*
 *	function  :  retrievs the state of the software timer.
 *
 *	parameter :  (none)
 *
 *	result    :  none-zero if a time-out has occurred, or 0 otherwise.
 */

LPSTR can_hardware(void);
/*
 *	function  :  retrieves the hardware version of the CAN Controller
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */

LPSTR can_software(void);
/*
 *	function  :  retrieves the firmware version of the CAN Controller
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */

LPSTR can_version(void);
/*
 *	function  :  retrieves version information of the CAN Controller API
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */


#endif      // __CAN_CTRL_H

/*  -------------------------------------------------------------------------
 *	Uwe Vogt, UV Software, Muellerstrasse 12e, 88045 Friedrichshafen, Germany
 *	Fon: +49-7541-6047470, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
