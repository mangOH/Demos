/*	-- $Header: P:/CAN/I386/CANopen/Master/gen/api/RCS/cop_api.h 1.4 2009/02/05 13:02:49 saturn Sav $ --
 *
 *	Projekt   :  CAN - Controller Area Network.
 *
 *	Zweck     :  CANopen Master Interface (generic).
 *
 *	Copyright :  (c) 2005-2009 by UV Software, Friedrichshafen.
 *
 *	Compiler  :  Microsoft Visual C/C++ Compiler (Version 6.0)
 *
 *	Export    :  LONG cop_init(LONG board, LPVOID param, BYTE baudrate);
 *	             LONG cop_exit(void);
 *
 *	             LONG cop_reset(BYTE baudrate);
 *
 *	             LONG sdo_write(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data);
 *	             LONG sdo_read(BYTE node_id, WORD index, BYTE subindex, SHORT *length, BYTE *data, SHORT max);
 *	             WORD sdo_timeout(WORD milliseconds);
 *	             LONG sdo_write_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE value);
 *	             LONG sdo_read_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE *value);
 *	             LONG sdo_write_16bit(BYTE node_id, WORD index, BYTE subindex, WORD value);
 *	             LONG sdo_read_16bit(BYTE node_id, WORD index, BYTE subindex, WORD *value);
 *	             LONG sdo_write_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD value);
 *	             LONG sdo_read_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD *value);
 *
 *	             LONG nmt_start_remote_node(BYTE node_id);
 *	             LONG nmt_stop_remote_node(BYTE node_id);
 *	             LONG nmt_enter_preoperational(BYTE node_id);
 *	             LONG nmt_reset_node(BYTE node_id);
 *	             LONG nmt_reset_communication(BYTE node_id);
 *
 *	             LONG lss_switch_mode_global(BYTE lss_mode);
 *	             LONG lss_switch_mode_selective(DWORD vendor_id, DWORD product_code, \
 *	                                            DWORD revision_number, DWORD serial_number);
 *	             LONG lss_configure_node_id(BYTE node_id);
 *	             LONG lss_configure_bit_timing(BYTE baudrate);
 *	             LONG lss_activate_bit_timing(WORD switch_delay);
 *	             LONG lss_store_configuration(void);
 *	             LONG lss_inquire_vendor_id(DWORD *vendor_id);
 *	             LONG lss_inquire_product_code(DWORD *product_code);
 *	             LONG lss_inquire_revision_number(DWORD *revision_number);
 *	             LONG lss_inquire_serial_number(DWORD *serial_number);
 *	             LONG lss_inquire_node_id(BYTE *node_id);
 *	             LONG lss_identify_remote_slaves(DWORD vendor_id, DWORD product_code, \
 *	                                             DWORD revision_number_low, DWORD revision_number_high, \
 *	                                             DWORD serial_number_low, DWORD serial_number_high);
 *	             LONG lss_identify_non_configured_remote_slaves(void);
 *	             WORD lss_timeout(WORD milliseconds);
 *
 *	             LONG lmt_switch_mode_global(BYTE lmt_mode);
 *	             LONG lmt_switch_mode_selective(CHAR* manufacturer_name, CHAR* product_name, CHAR* serial_number);
 *	             LONG lmt_configure_node_id(BYTE node_id);
 *	             LONG lmt_configure_bit_timing(BYTE table, BYTE baudrate);
 *	             LONG lmt_activate_bit_timing(WORD switch_delay);
 *	             LONG lmt_store_configuration(void);
 *	             LONG lmt_inquire_manufacturer_name(CHAR *manufacturer_name);
 *	             LONG lmt_inquire_product_name(CHAR *product_name);
 *	             LONG lmt_inquire_serial_number(CHAR *serial_number);
 *	             LONG lmt_identify_remote_slaves(CHAR* manufacturer_name, CHAR* product_name, CHAR* serial_number_low, \
 *	                                                                                          CHAR* serial_number_high);
 *	             WORD lmt_timeout(WORD milliseconds);
 *
 *	             LONG cop_transmit(LONG cob_id, SHORT length, BYTE *data);
 *	             LONG cop_request(LONG cob_id, SHORT *length, BYTE *data);
 *	             LONG cop_status(BYTE *status, BYTE *load);
 *
 *	             LONG cop_queue_read(LONG *cob_id, SHORT *length, BYTE *data);
 *	             LONG cop_queue_clear(void);
 *	             LONG cop_queue_status(BYTE *status, BYTE *load);
 *
 *	             LPSTR cop_hardware(void);
 *	             LPSTR cop_software(void);
 *	             LPSTR cop_version(void);
 *	             LPSTR sdo_version(void);
 *	             LPSTR lss_version(void);
 *	             LPSTR lmt_version(void);
 *
 *	Include   :  can_defs.h, windows.h or default.h
 *
 *	Autor     :  Uwe Vogt, UV Software.
 *
 *	E-Mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  Schnittstellenbeschreibung  ---------------------------------
 *
 *	CANopen Master for generic CAN Interfaces.
 *
 *		Supported CAN Controller API:
 *		- IXXAT Virtual CAN Interface (VCI V2): _CAN_CONTROLLER=100
 *		- IXXAT canAnalyzer/32 Client (CAC): _CAN_CONTROLLER=800
 *		- PEAK PCAN-Light Interface: _CAN_CONTROLLER=200
 *		- [To Be Continued]
 *
 *	CANopen Master API - Data Link Layer.
 *
 *		Implements the CANopen Data Link Layer for accessing the CAN Controller
 *		and common communication functions for the CANopen Master interface.
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
 *	$Log: cop_api.h $
 *	Revision 1.4  2009/02/05 13:02:49  saturn
 *	Header default.h included for linux gcc environment.
 *
 *	Revision 1.3  2006/10/05 10:19:49  vogt
 *	The multiplexor is checked now with the SDO confirmation.
 *	If the multiplexor did not match the frame is skipped.
 *
 *	Revision 1.2  2005/10/24 13:11:47  vogt
 *	New error codes added.
 *
 *	Revision 1.1  2005/10/09 19:17:00  vogt
 *	Initial revision
 *
 */

#ifndef __COP_API_H
#define __COP_API_H

#ifdef _COPAPI_EXPORTS
	#define COPAPI	__declspec(dllexport)
#else
  #ifndef _COPAPI_EXTERN
	#define COPAPI	__declspec(dllimport)
  #else
	#define COPAPI	extern
  #endif
#endif
#ifdef __cplusplus
extern "C" {
#endif

/*	-----------  Include-Dateien  --------------------------------------------
 */

#include "can_defs.h"					// CAN definitions and options
#ifndef _WIN32
#include "default.h"					// C extensions and useful stuff
#else
#include <windows.h>					// Master include file for Windows
#endif

/*	-----------  Definitionen  -----------------------------------------------
 */

#ifndef _COP_DEFS
#define _COP_DEFS
/*	- - - - - -  Baud rate index (CANopen Master)  - - - - - - - - - - - - - -
 */
 #define COPBDR_1000			0		// Baud rate: 1000 kBit/s
 #define COPBDR_800				1		// Baud rate: 800 kBit/s
 #define COPBDR_500				2		// Baud rate: 500 kBit/s
 #define COPBDR_250				3		// Baud rate: 250 kBit/s
 #define COPBDR_125				4		// Baud rate: 125 kBit/s
 #define COPBDR_100				5		// Baud rate: 100 kBit/s
 #define COPBDR_50				6		// Baud rate: 50 kBit/s
 #define COPBDR_20				7		// Baud rate: 20 kBit/s
 #define COPBDR_10				8		// Baud rate: 10 kBit/s

/*	- - - - - -  Error Codes (CAN/CANopen Communication)   - - - - - - - - - -
 */
 #define COPERR_NOERROR			 0		// No error
 #define COPERR_BOFF			-1		// Busoff status
 #define COPERR_EWRN			-2 		// Error warning status
 #define COPERR_BERR			-3		// Bus error
 #define COPERR_OFFLINE			-9		// Not started
 #define COPERR_ONLINE			-8		// Already started
 #define COPERR_MSG_LST			-10		// Message lost (overrun)
 #define COPERR_LEC_STUFF		-11		// Stuff error
 #define COPERR_LEC_FORM		-12		// Form error
 #define COPERR_LEC_ACK			-13		// Acknowledge error
 #define COPERR_LEC_BIT1		-14		// Recessive bit error
 #define COPERR_LEC_BIT0		-15		// Dominant bit error
 #define COPERR_LEC_CRC			-16		// Checksum error
 #define COPERR_TX_BUSY			-20		// Transmitter busy
 #define COPERR_RX_EMPTY		-30		// Receiver empty
 #define COPERR_QUE_OVR			-40		// Queue overrun
 #define COPERR_TIMEOUT			-50		// Time-out error
 #define COPERR_LENGTH			-60		// Invalid length
 #define COPERR_FORMAT			-70		// Format error
 #define COPERR_STATUS			-80		// Illegal status
 #define COPERR_NODE_ID			-90		// Illegal node-id
 #define COPERR_BAUDRATE		-91		// Illegal baudrate
 #define COPERR_ILLPARA			-93		// Illegal parameter
 #define COPERR_NULLPTR			-94		// Null-pointer assignment
 #define COPERR_NOTINIT			-95		// Not initialized
 #define COPERR_YETINIT			-96		// Already initialized
 #define COPERR_NOTSUPP			-98		// Not supported
 #define COPERR_FATAL			-99		// Other errors
 #define COPERR_ABORTED			0x4		// Transfer aborted

/*	- - - - - -  SDO Abort Codes according to CiA DS-301 Version 4.02  - - - -
 */
 #define SDOERR_WRONG_TOGGLEBIT 		0x05030000L
 #define SDOERR_PROTOCOL_TIMEOUT		0x05040000L
 #define SDOERR_UNKNOWN_SPECIFIER		0x05040001L
 #define SDOERR_INVALID_BLK_SIZE 		0x05040002L
 #define SDOERR_INVALID_SEQ_NUM  		0x05040003L
 #define SDOERR_CRC_ERROR          		0x05040004L
 #define SDOERR_OUT_OF_MEMORY			0x05040005L
 #define SDOERR_OBJECT_ACCESS			0x06010000L
 #define SDOERR_WRITE_ONLY_OBJECT		0x06010001L
 #define SDOERR_READ_ONLY_OBJECT		0x06010002L
 #define SDOERR_OBJECT_NOT_EXISTS		0x06020000L
 #define SDOERR_OBJECT_NOT_MAPABLE		0x06040041L
 #define SDOERR_PDO_TOO_LONG    		0x06040042L
 #define SDOERR_INVALID_VALUE   		0x06040043L
 #define SDOERR_INCOMPATIBILITY 		0x06040047L
 #define SDOERR_HARDWARE_ACCESS 		0x06060000L
 #define SDOERR_UNKNOWN_DATATYPE		0x06070010L
 #define SDOERR_TYPE_LENGTH_TOO_HIGH	0x06070012L
 #define SDOERR_TYPE_LENGTH_TOO_LOW 	0x06070013L
 #define SDOERR_SUBINDEX_NOT_EXISTS		0x06090011L
 #define SDOERR_RANGE_EXCEEDED   		0x06090030L
 #define SDOERR_RANGE_OVERFLOW  		0x06090031L
 #define SDOERR_RANGE_UNDERFLOW 		0x06090032L
 #define SDOERR_RANGE_MAX_LESS_MIN		0x06090033L
 #define SDOERR_GENERAL_ERROR			0x08000000L
 #define SDOERR_SERVICE_ERROR			0x08000020L
 #define SDOERR_LOCAL_ERROR				0x08000021L
 #define SDOERR_DEVICE_ERROR   			0x08000022L
 #define SDOERR_DYNAMIC_DICTIONARY		0x08000023L

/*	- - - - - -  LSS Error codes according to CiA DSP-305 Version 1.0  - - - -
 */
 #define LSSERR_ILLEGAL_NODE_ID			0x01
 #define LSSERR_BIT_TIMING_ERROR		0x01
 #define LSSERR_STORE_NOT_SUPPORTED		0x01
 #define LSSERR_STORAGE_ACCESS_ERROR	0x02
 #define LSSERR_IMPLEMENTATION_ERROR	0xFF
#endif
										// ---	SDO Definitions  ---
#define  SDO_CLIENT				0x600	// COB-Id of Default Client-SDO
#define  SDO_SERVER				0x580	// COB-Id of Default Server-SDO
#define  SDO_TIMEOUT			500		// Time-out value for SDO protocol
										// ---	NMT Definitions  ---
#define  NMT_MASTER				0x000	// COB-Id of NMT-Master
#define  NMT_SLAVE				0x700	// COB-Id of NMT-Slave
#define  NMT_ALL				0x0		// All NMT-Slave devices
										// ---	LSS Definitions  ---
#define  LSS_MASTER				0x7E5	// COB-Id of LSS-Master
#define  LSS_SLAVE				0x7E4	// COB-Id of LSS-Slave
#define  LSS_TIMEOUT			500		// Time-out value for LSS protocol
#define  LSS_OPERATION			0		// LSS operation mode
#define  LSS_CONFIGURATION		1		// LSS configuration mode
										// ---	LMT Definitions  ---
#define  LMT_MASTER				0x7E5	// COB-Id of LMT-Master
#define  LMT_SLAVE				0x7E4	// COB-Id of LMT-Slave
#define  LMT_TIMEOUT			500		// Time-out value for LMT protocol
#define  LMT_OPERATION			0		// LMT operation mode
#define  LMT_CONFIGURATION		1		// LMT configuration mode
										// ---	CAN Message Buffers  ---
#define  CANBUF_TX				0		// Message buffer for transmit objects
#define  CANBUF_RX				1		// Message buffer for receive objects
#define  CANRTR_FACTOR			10		// Increased time-out for RTR-frames


/*	-----------  Typen  ------------------------------------------------------
 */


/*	-----------  Variablen  --------------------------------------------------
 */

#ifdef _CAN_BOARD_LIST
 extern CAN_BOARD can_board[CAN_BOARDS];// list of CAN Interface boards
 extern BYTE      can_baudrate;			// index to the bit-timing table
#endif

/*	-----------  Prototypen  -------------------------------------------------
 */

COPAPI LONG cop_init(LONG board, LPVOID param, BYTE baudrate);
/*
 *  function:   initializes the CANopen Master API and starts the communication
 *              via CAN with the selected baudrate.
 *
 *	            For a list of available CAN Interface boards see 'can_defs.h'.
 *
 *  parameter:  board: type of the CAN Controller interface.
 *	            param: pointer to board-specific parameters.
 *	            baudrate: index (0,..,8) to the bit-timing table.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG cop_exit(void);
/*
 *  function:   stops the communication via CAN and closes the CANopen Master API.
 *
 *  parameter:  (none)
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG cop_reset(BYTE baudrate);
/*
 *  function:   stops the communication via CAN and restarts the communication
 *              via CAN with the selected baudrate..
 *
 *  parameter:  baudrate: index (0,..,8) to the bit-timing table.
 *
 *  result:     0 if successful, or a negative value on error.
 */

/*	- - - - - -	 SDO - Service Data Object   - - - - - - - - - - - - - - - - -
 */
COPAPI LONG sdo_write(BYTE node_id, WORD index, BYTE subindex, SHORT length, BYTE *data);
/*
 *  function:   writes data of arbitrary length to the selected node at object
 *              index and subindex. The data must be encoded according to the
 *              CiA DS-301 Encoding Rules.
 *
 *              The function implements the SDO-Download protocol according to
 *              the CiA DS-301 Communication Profile. Depending on the length
 *              of the data either the segmented or the expedited protocol is
 *              used.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              length of the object data.
 *              data: pointer to data buffer.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_read(BYTE node_id, WORD index, BYTE subindex, SHORT *length, BYTE *data, SHORT max);
/*
 *  function:   reads data of arbitrary length from the selected node at object
 *              index and subindex. The data is encoded according to the CiA
 *              DS-301 Encoding Rules.
 *
 *              The function implements the SDO-Upload protocol according to
 *              the CiA DS-301 Communication Profile. Depending on the length
 *              of the data either the segmented or the expedited protocol is
 *              used.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              length of the object data.
 *              data: pointer to data buffer.
 *              max: length of the data buffer.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI WORD sdo_timeout(WORD milliseconds);
/*
 *  function:   sets the time-out value in milliseconds for the SDO
 *              peer-to-peer communication (max. time for a request/
 *              confirmation transfer).
 *
 *  parameter:  time-out (0,...,65535) in milliseconds.
 *
 *  result:     last time-out value in milliseconds.
 */

COPAPI LONG sdo_write_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE value);
/*
 *  function:   writes an 8-bit value to the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Download protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 8-bit value to be written.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_read_8bit(BYTE node_id, WORD index, BYTE subindex, BYTE *value);
/*
 *  function:   reads an 8-bit value from the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Upload protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 8-bit value read from the node.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_write_16bit(BYTE node_id, WORD index, BYTE subindex, WORD value);
/*
 *  function:   writes an 16-bit value to the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Download protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 16-bit value to be written.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_read_16bit(BYTE node_id, WORD index, BYTE subindex, WORD *value);
/*
 *  function:   reads an 16-bit value from the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Upload protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 16-bit value read from the node.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_write_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD value);
/*
 *  function:   writes an 32-bit value to the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Download protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 32-bit value to be written.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

COPAPI LONG sdo_read_32bit(BYTE node_id, WORD index, BYTE subindex, DWORD *value);
/*
 *  function:   reads an 32-bit value from the selected node at object index
 *              and subindex.
 *
 *              The function implements the expedited SDO-Upload protocol
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node.
 *              index of the object dictionary.
 *              subindex of the object entry.
 *              value: 32-bit value read from the node.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the SDO Abort Code from the node (as a positive value).
 */

/*	 - - - - -  NMS - Network Management Services  - - - - - - - - - - - - - -
 */
COPAPI LONG nmt_start_remote_node(BYTE node_id);
/*
 *  function:   sends the NMT Start_remote_node command to the selected node.
 *
 *              The function implements the NMT broadcast message command
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node or 0 for all nodes.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG nmt_stop_remote_node(BYTE node_id);
/*
 *  function:   sends the NMT Stop_remote_node command to the selected node.
 *
 *              The function implements the NMT broadcast message command
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node or 0 for all nodes.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG nmt_enter_preoperational(BYTE node_id);
/*
 *  function:   sends the NMT Enter_preoperational command to the selected node.
 *
 *              The function implements the NMT broadcast message command
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node or 0 for all nodes.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG nmt_reset_node(BYTE node_id);
/*
 *  function:   sends the NMT Reset_node command to the selected node.
 *
 *              The function implements the NMT broadcast message command
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node or 0 for all nodes.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG nmt_reset_communication(BYTE node_id);
/*
 *  function:   sends the NMT Reset_communication command to the selected
 *              node.
 *
 *              The function implements the NMT broadcast message command
 *              according to the CiA DS-301 Communication Profile.
 *
 *  parameter:  node_id (1,..,127) of the node or 0 for all nodes.
 *
 *  result:     0 if successful, or a negative value on error.
 */

/*	 - - - - -  LSS - Layer Setting Services   - - - - - - - - - - - - - - - -
 */
COPAPI LONG lss_switch_mode_global(BYTE lss_mode);
/*
 *  function:   switches all nodes to the selected LSS mode.
 *
 *              The function implements the LSS Switch_mode_global command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *  parameter:  mode: 0 = operation mode, 1 = configuration mode.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_switch_mode_selective(DWORD vendor_id, DWORD product_code, \
                                      DWORD revision_number, DWORD serial_number);
/*
 *  function:   switches the node with matching LSS address to the LSS
 *              configuration mode.
 *
 *              The function implements the LSS Switch_mode_selective command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node with matching LSS address!
 *
 *  parameter:  vendor_id (as part of the LSS address).
 *              product_code (as part of the LSS address).
 *              revision_number (as part of the LSS address).
 *              serial_number (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_configure_node_id(BYTE node_id);
/*
 *  function:   assignes the selected node-id to the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Configure_node_id command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  node_id (1,...,127).
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LSS Error Code from the node (as a positive value).
 */

COPAPI LONG lss_configure_bit_timing(BYTE baudrate);
/*
 *  function:   assignes the selected baudrate to the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Configure_bit_timing command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  index (0,..,8) to the bit-timing table.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LSS Error Code from the node (as a positive value).
 */

COPAPI LONG lss_activate_bit_timing(WORD switch_delay);
/*
 *  function:   activates the assigned baudrate of all nodes which are in LSS 
 *              configuration mode and waits for switch_delay milliseconds
 *              before return. Afterwards the baudrate of the master
 *              can/should be changed to the baudrate of the slaves.
 *
 *              The function implements the LSS Activate_bit_timing command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              All nodes should be in LSS configuration mode!
 *
 *  parameter:  switch_delay in milliseconds.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_store_configuration(void);
/*
 *  function:   forces the node which is in LSS configuration mode to store
 *              the assigned LSS parameters (node-id and baudrate).
 *
 *              The function implements the LSS Store_configuration command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  (none)
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LSS Error Code from the node (as a positive value).
 */

COPAPI LONG lss_inquire_vendor_id(DWORD *vendor_id);
/*
 *  function:   retrieves the vendor-id from the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Inquire_vendor_id command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  vendor_id (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_inquire_product_code(DWORD *product_code);
/*
 *  function:   retrieves the product-code from the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Inquire_product_code command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  product_code (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_inquire_revision_number(DWORD *revision_number);
/*
 *  function:   retrieves the revision-number from the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Inquire_revision_number command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  revision_number (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_inquire_serial_number(DWORD *serial_number);
/*
 *  function:   retrieves the serial-number from the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Inquire_serial_number command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  serial_number (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_inquire_node_id(BYTE *node_id);
/*
 *  function:   retrieves the node-id from the node which is in LSS
 *              configuration mode.
 *
 *              The function implements the LSS Inquire_node_id command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *              There have to be only one node in LSS configuration mode!
 *
 *  parameter:  node-id (1,..,127 or 255).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_identify_remote_slaves(DWORD vendor_id, DWORD product_code, \
                                       DWORD revision_number_low, DWORD revision_number_high, \
                                       DWORD serial_number_low, DWORD serial_number_high);
/*
 *  function:   requests all nodes to identify themself.
 *
 *              The function implements the LSS Identify_remote_slaves command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *  parameter:  vendor_id (as part of the LSS address).
 *              product_code (as part of the LSS address).
 *              revision_number (as part of the LSS address).
 *              serial_number (as part of the LSS address).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lss_identify_non_configured_remote_slaves(void);
/*
 *  function:   requests all nodes with no node-id to identify themself.
 *
 *              The function implements the LSS Identify_remote_slaves command
 *              according to CiA DSP-305 (Layer Setting Services).
 *
 *  parameter:  (none)
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI WORD lss_timeout(WORD milliseconds);
/*
 *  function:   sets the time-out value in milliseconds for the LSS
 *              peer-to-peer communication (max. time for a request/
 *              confirmation transfer).
 *
 *  parameter:  time-out (0,...,65535) in milliseconds.
 *
 *  result:     last time-out value in milliseconds.
 */

/*	 - - - - -  LMT - Layer Management Services- - - - - - - - - - - - - - - -
 */
COPAPI LONG lmt_switch_mode_global(BYTE lmt_mode);
/*
 *  function:   switches all nodes to the selected LMT mode.
 *
 *              The function implements the LMT Switch_mode_global command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *  parameter:  mode: 0 = operation mode, 1 = configuration mode.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_switch_mode_selective(CHAR* manufacturer_name, CHAR* product_name, CHAR* serial_number);
/*
 *  function:   switches the node with matching LMT address to the LMT
 *              configuration mode.
 *
 *              The function implements the LMT Switch_mode_selective command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node with matching LMT address!
 *
 *  parameter:  manufacturer_name (7 alpha-numerical characters).
 *              product_name (7 alpha-numerical characters).
 *              serial_number (14 digits).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_configure_node_id(BYTE node_id);
/*
 *  function:   assignes the selected node-id to the node which is in LMT
 *              configuration mode.
 *
 *              The function implements the LMT Configure_node_id command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  node_id (1,...,127).
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LMT Error Code from the node (as a positive value).
 */

COPAPI LONG lmt_configure_bit_timing(BYTE table, BYTE baudrate);
/*
 *  function:   assignes the selected baudrate to the node which is in LMT
 *              configuration mode.
 *
 *              The function implements the LMT Configure_bit_timing command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  table: selector (0=CiA) of the bit-timing table.
 *              baudrate: index (0,..,8) to the bit-timing table.
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LMT Error Code from the node (as a positive value).
 */

COPAPI LONG lmt_activate_bit_timing(WORD switch_delay);
/*
 *  function:   activates the assigned baudrate of all nodes which are
 *              in LMT configuration mode and waits for switch_delay
 *              milliseconds before return. Afterwards the baudrate of the
 *              master can/should be changed to the baudrate of the slaves.
 *
 *              The function implements the LMT Activate_bit_timing command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              All nodes should be in LMT configuration mode!
 *
 *  parameter:  switch_delay in milliseconds.
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_store_configuration(void);
/*
 *  function:   forces the node which is in LMT configuration mode to store
 *              the assigned LMT parameters (node-id and baudrate).
 *
 *              The function implements the LMT Store_configuration command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  (none)
 *
 *  result:     0 if successful, or a negative value on a communication error,
 *              or the LMT Error Code from the node (as a positive value).
 */

COPAPI LONG lmt_inquire_manufacturer_name(CHAR *manufacturer_name);
/*
 *  function:   retrieves the manufacturer-name from the node which is in
 *              LMT configuration mode.
 *
 *              The function implements the LMT Inquire_manufacturer_name
 *              command according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  manufacturer_name (7 alpha-numerical characters).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_inquire_product_name(CHAR *product_name);
/*
 *  function:   retrieves the product-name from the node which is in LMT
 *              configuration mode.
 *
 *              The function implements the LMT Inquire_product_name command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  product_name (7 alpha-numerical characters).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_inquire_serial_number(CHAR *serial_number);
/*
 *  function:   retrieves the serial-number from the node which is in LMT 
 *             configuration mode.
 *
 *              The function implements the LMT Inquire_serial_number command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *              There have to be only one node in LMT configuration mode!
 *
 *  parameter:  serial_number (14 digits).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI LONG lmt_identify_remote_slaves(CHAR* manufacturer_name, CHAR* product_name, CHAR* serial_number_low, \
                                                                                    CHAR* serial_number_high);
/*
 *  function:   requests all nodes to identify themself.
 *
 *              The function implements the LMT Identify_remote_slaves command
 *              according to CiA DS-205-1/-2 (Layer Management).
 *
 *  parameter:  manufacturer_name (7 alpha-numerical characters).
 *              product_name (7 alpha-numerical characters).
 *              serial_number_low (14 digits).
 *              serial_number_high (14 digits).
 *
 *  result:     0 if successful, or a negative value on error.
 */

COPAPI WORD lmt_timeout(WORD milliseconds);
/*
 *  function:   sets the time-out value in milliseconds for the LMT
 *              peer-to-peer communication (max. time for a request/
 *              confirmation transfer).
 *
 *  parameter:  time-out (0,...,65535) in milliseconds.
 *
 *  result:     last time-out value in milliseconds.
 */

/*	 - - - - -  CAN - Layer 2 Functions  - - - - - - - - - - - - - - - - - - -
 */
COPAPI LONG cop_transmit(LONG cob_id, SHORT length, BYTE *data);
/*
 *  function  :  transmits a message with the selected 11-bit identifier
 *               and max. 8 data byte.
 *
 *               The function can be used for transmitting PDOs.
 *
 *  parameter :  cob_id (11-bit identifier) of the message.
 *               length (0,..,8) of the message data.
 *               data: pointer to the message data.
 *
 *  result    :  0 if successful, or a negative value on error.
 */

COPAPI LONG cop_request(LONG cob_id, SHORT *length, BYTE *data);
/*
 *  function  :  requests max. 8 data bytes form a remote node with the
 *               selected 11-bit identifier.
 *
 *               The function can be used for requesting PDOs.
 *
 *  parameter :  cob_id (11-bit identifier) of the message.
 *               length (0,..,8) of the received data.
 *               data: pointer to a buffer for the received data.
 *
 *  result    :  0 if successful, or a negative value on error.
 */

COPAPI LONG cop_status(BYTE *status, BYTE *load);
/*
 *  function  :  retrieves the status of the last operation (error code).
 *
 *  parameter :  status - CAN controller status register (pointer or NULL).
 *                            Bit 7: CAN controller stopped
 *                            Bit 6: Busoff status
 *                            Bit 5: Error warning status
 *                            Bit 4: Bus error (LEC)
 *                            Bit 3: Transmitter busy
 *                            Bit 2: Receiver empty
 *                            Bit 1: Message lost
 *                            Bit 0: Event-queue overrun
 *	             load   - CAB bus-load (pointer or NULL).
 *
 *  result    :  0 if no error occured during the last operation, or one of the
 *               following values: COPERR_BOFF       - on busoff status
 *                                 COPERR_BERR       - on bus error
 *                                 COPERR_MSG_LST    - on message loss
 *                                 COPERR_LEC_STUFF  - on stuff error
 *                                 COPERR_LEC_FORM   - on form error
 *                                 COPERR_LEC_ACK    - on acknowledge error
 *                                 COPERR_LEC_BIT1   - on recessive bit error
 *                                 COPERR_LEC_BIT0   - on dominant bit error
 *                                 COPERR_LEC_CRC    - on checksum error
 *                                 COPERR_TX_BUSY    - on transmitter busy
 *                                 COPERR_RX_EMPTY   - on receiver empty
 *                                 COPERR_TIMEOUT    - on time-out
 *                                 COPERR_LENGTH     - on invalid lenght
 *                                 COPERR_FORMAT     - on format error
 *                                 COPERR_STATUS     - on illegal status
 *                                 COPERR_NODE_ID    - on illegal node-id
 *                                 COPERR_BAUDRATE   - on illegal baud rate
 *                                 COPERR_ILLPARA    - on illegal parameter
 *                                 COPERR_NULLPTR    - on Null-pointer assignment
 *                                 COPERR_NOTINIT    - if not initialized
 *                                 COPERR_YETINIT    - if already initialized
 *                                 COPERR_NOTSUPP    - if not supported
 *                                 COPERR_FATAL      - on other errors
 *                                 COPERR_SDO_...    - if a SDO Abort received
 *                                 COPERR_LSS_...    - if a LSS Error received
 */

/*	 - - - - -  CAN - Event-queue  - - - - - - - - - - - - - - - - - - - - - -
 */
COPAPI LONG cop_queue_read(LONG *cob_id, SHORT *length, BYTE *data);
/*
 *	function  :  reads the first received identifier and data from
 *               the event-queue if any, or returns CANQUE_EMPTY if
 * 		         the queue is empty.
 *
 *               The function can be used for RPDOs, EMCY, etc.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

COPAPI LONG cop_queue_clear(void);
/*
 *	function  :  deletes all received messages from the queue and sets
 *               the state of the queue to empty.
 *
 *	parameter :  (none)
 *
 *	result    :  0 if successful, or a negative value on error.
 */

COPAPI LONG cop_queue_status(BYTE *status, BYTE *load);
/*
 *	function  :  return the error status of the event-queue and resets
 *               it to error-free.
 *
 *  parameter :  status - CAN controller status register (pointer or NULL).
 *                            Bit 7: CAN controller stopped
 *                            Bit 6: Busoff status
 *                            Bit 5: Error warning status
 *                            Bit 4: Bus error (LEC)
 *                            Bit 3: Transmitter busy
 *                            Bit 2: Receiver empty
 *                            Bit 1: Message lost
 *                            Bit 0: Event-queue overrun
 *	             load   - CAB bus-load (pointer or NULL).
 *
 *	result    :  0 if error-free, or one of the following negative values
 *               if not error-free: CANQUE_MSG_LST  - message lost
 *                                  CANQUE_OVERRUN  - queue overrun
 */

/*	 - - - - -  API - Version Information  - - - - - - - - - - - - - - - - - -
 */
COPAPI LPSTR cop_hardware(void);
/*
 *	function  :  retrieves the hardware version of the CAN Controller
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */

COPAPI LPSTR cop_software(void);
/*
 *	function  :  retrieves the firmware version of the CAN Controller
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */

COPAPI LPSTR cop_version(void);
COPAPI LPSTR sdo_version(void);
COPAPI LPSTR lss_version(void);
COPAPI LPSTR lmt_version(void);
/*
 *	function  :  retrieves version information of the CANopen Master API
 *	             as a zero-terminated string.
 *
 *	parameter :  (none)
 *
 *	result    :  pointer to a zero-terminated string, or NULL on error.
 */

#ifdef __cplusplus
}
#endif
#endif	// __COP_API_H

/*	--------------------------------------------------------------------------
 *	Uwe Vogt,  UV Software,  Steinaecker 28,  88048 Friedrichshafen,  Germany
 *	Fon: +49-7541-6041530, Fax. +49-1803-551809359, Cell fon: +49-170-3801903
 *	E-Mail: uwe.vogt@uv-software.de, Internet URL: http://www.uv-software.de/
 */
