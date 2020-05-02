/*---------------------------------------------------------------------------*
 * macs.h                                                                    *
 * Copyright (C) 2014  Jacques Pelletier                                     *
 *                                                                           *
 * This program is free software; you can redistribute it and *or            *
 * modify it under the terms of the GNU General Public License               *
 * as published by the Free Software Foundation; either version 2            *
 * of the License, or (at your option) any later version.                    *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software Foundation,   *
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           *
 *---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#define USE_SE_FCS

/* implementation */
// 1.1.3.2.1 MA_DATA.request
// 1.1.4.2.1 MA_ACK_DATA.request
// 1.2.3.2.1 MA_SM_DATA.request
// 1.2.4.2.1 MA_SM_ACK_DATA.request
// 1.2.5.2.1 MA_INITIALIZE_PROTOCOL.request
// 1.2.5.2.3 MA_SET_VALUE.request
// 1.2.5.2.5 MA_READ_VALUE.request

#define MA_RQ_DATA              0x01
#define MA_RQ_ACK_DATA          0x02
#define MA_RQ_SM_DATA           0x03
#define MA_RQ_SM_ACK_DATA       0x04
#define MA_RQ_INIT_PROTOCOL     0x05
#define MA_RQ_SET_VALUE         0x06
#define MA_RQ_READ_VALUE        0x07

// 1.1.3.2.3 MA_DATA.confirm
// 1.1.4.2.3 MA_ACK_DATA.confirm
// 1.2.3.2.3 MA_SM_DATA.confirm
// 1.2.4.2.3 MA_SM_ACK_DATA.confirm
// 1.2.5.2.2 MA_INITIALIZE_PROTOCOL.confirm
// 1.2.5.2.4 MA_SET_VALUE.confirm
// 1.2.5.2.6 MA_READ_VALUE.confirm

#define MA_CF_DATA              0x10
#define MA_CF_ACK_DATA          0x20
#define MA_CF_SM_DATA           0x30
#define MA_CF_SM_ACK_DATA       0x40
#define MA_CF_INIT_PROTOCOL     0x50
#define MA_CF_SET_VALUE         0x60
#define MA_CF_READ_VALUE        0x70

// 1.1.3.2.2 MA_DATA.indication
// 1.1.4.2.2 MA_ACK_DATA.indication
// 1.2.3.2.2 MA_SM_DATA.indication
// 1.2.4.2.2 MA_SM_ACK_DATA.indication

#define MA_IND_DATA             0x01
#define MA_IND_ACK_DATA         0x02
#define MA_IND_SM_DATA          0x03
#define MA_IND_SM_ACK_DATA      0x04

// 1.2.5.2.7 LSM_EVENT.indication
// SE
#define LSM_IND_EVENT_START                 0x80
#define LSM_IND_EVENT_MEDIUM_FAILURE        0x80
#define LSM_IND_EVENT_PH_FAILURE            0x81
#define LSM_IND_EVENT_MEDIUM_RESET          0x82
#define LSM_IND_EVENT_PH_RESET              0x83

// LLC
#define LSM_IND_EVENT_LLC_FAILURE           0x84
#define LSM_IND_EVENT_LLC_RESET             0x85
#define LSM_IND_EVENT_UPPER_LAYER_BUSY      0x86
#define LSM_IND_EVENT_UPPER_LAYER_NOT_BUSY  0x87

// 1.2.5.2.8 MA_FAILURE_REPORT.indication
#define MA_IND_FAIL_REPORT      0x90

typedef uint16_t Word;
typedef uint8_t Byte;
typedef uint8_t Boolean;

#define NB_RECV_ASSOC_LIST 5
#define NB_XMIT_ASSOC_LIST 5

// 3.4 Constant Description
#define CONST_CA_WAIT_TIME 26
//MAX_RESTART
// 7500 UST (0.75s)
#define CONST_MAX_RETRANS_TIME 7500

typedef enum E_Packet_Field {F_PREAMBLE,F_CONTROL,F_ADDR,F_HOUSE,F_INFO,F_FCS} T_Packet_Field;

// 3.1
typedef enum E_State {IDLE,XMIT_WAIT,XMIT,AS_WAIT,RCV,IDLE_WAIT,RESET_WAIT} T_State;
typedef enum E_Priority {HIGH,STANDARD,DEFERRED,RESERVED1} T_Priority;
typedef enum E_Service {BASIC,EXTENDED} T_Service;
typedef enum E_Ack_Service {SOURCE,DESTINATION,NONE} T_Ack_Service;
typedef enum E_Packet_Type {IACK,ACK_DATA,UNACK_DATA,RESERVED2,FAILURE,ADR_ACK_DATA,ADR_IACK,ADR_UNACK_DATA} T_Packet_Type;
typedef enum E_Sym {SYM_1,SYM_0,SYM_EOF,SYM_EOP} T_Sym;

typedef enum E_Mac_Event {
    NO_EVENT,

    PH_CC_ST_IND_CH_QUIET,  //PH_CC_STATUS.indication(CH_QUIET)
    PH_CC_ST_IND_CH_ACTIVE, //PH_CC_STATUS.indication(CH_ACTIVE)
    PH_CC_ST_IND_BIT_ERROR, //PH_CC_STATUS.indication(BIT_ERROR)
    PH_CC_DATA_IND, //PH_CC_DATA.indication
    PH_CC_DATA_CONF_SUCCESS, //PH_CC_DATA.confirm(SUCCESS)
    PH_CC_DATA_CONF_FAILURE_COLLISION, //PH_CC_DATA.confirm(FAILURE_COLLISION)
    PH_CC_DATA_CONF_FAILURE_OTHER, //PH_CC_DATA.confirm(FAILURE_OTHER)
    NO_CH_QUIET_SENT, //No_CH_QUIET_Sent

    MA_DATA_REQ, //MA_DATA.request
    MA_ACK_DATA_REQ, //MA_ACK_DATA.request
    MA_SM_DATA_REQ, //MA_SM_DATA.request
    MA_SM_ACK_DATA_REQ, //MA_SM_ACK_DATA.request

    MAC_FAILURE, //MAC_Failure
    MA_INIT_PROTOCOL_REQ, //MA_INIT_PROTOCOL.request

    Retrans_Time_Expired,

    /* PLSE */
    LSM_EVENT_IND_MEDIUM_FAILURE, //LSM_EVENT.indication(Medium_FAILURE)
    LSM_EVENT_IND_PH_FAILURE, //LSM_EVENT.indication(PH_FAILURE)
    LSM_EVENT_IND_MEDIUM_RESET, //LSM_EVENT.indication(Medium_Reset)
    LSM_EVENT_IND_PH_RESET, //LSM_EVENT.indication(PH_Reset)

    /* LLC */
    LSM_EVENT_IND_LLC_FAILURE,
    LSM_EVENT_IND_LLC_RESET,
    LSM_EVENT_IND_UPPER_LAYER_BUSY,
    LSM_EVENT_IND_UPPER_LAYER_NOT_BUSY
} T_Mac_Event;


typedef enum E_Confirm_Result {
	SUCCESS,
	FAILURE_NO_ACKNOWLEDGE,
	FAILURE_REMOTE_REJECT,
	FAILURE_REMOTE_REJECT_EXTENDED,
	FAILURE_EXCESSIVE_COLLISIONS,
	FAILURE_REMOTE_BUSY,
	FAILURE_REMOTE_BUSY_RCV_LIST,
	FAILURE_LOCAL_BUSY,
	FAILURE_LOCAL_BUSY_XMIT_LIST,
	FAILURE_OTHER,
	FAILURE_PRIORITY,
	FAILURE_MACS,
	FAILURE_PH
} T_Confirm_Result;

typedef enum E_Ack_Info_Field{
	SUCCESS = 0,
	FAILURE_REMOTE_BUSY = 0x0001,
	FAILURE_REMOTE_BUSY_RCV_LIST = 0x0010,
	FAILURE_REMOTE_REJECT = 0x0080,
	FAILURE_REMOTE_REJECT_EXTENDED = 0x0090,
	NONE = 0xFFFF   // not used
} T_Ack_Info_Field;

// 1.2.5.2.3.2
typedef enum E_Mac_Var
{
    // Read
    VALID_RECEIVED_FRAME_COUNT,
    FCS_ERROR_RECEIVED_FRAME_COUNT,
    ERRORED_FRAME_COUNT,
    FAILED_TRANSMISSION_COUNT,
    SUCCESSFUL_TRANSMISSION_COUNT,

    // Read/Write
	INDIVIDUAL_NODE_ADDRESS,
	GROUP_ADDRESSES,
	MAX_RESTART,
	CH_ACCESS_NUM,
	UNACK_CH_ACCESS_PERIOD,
	UNACK_RETRANS_TIME,
	ACK_CH_ACCESS_PERIOD,
	ACK_RETRANS_TIME,
	MAX_RETRANS_TIME,
	NUM_RETRIES
} T_Mac_Var;

typedef struct S_Assoc
{
	Word	SA;
	Word	SHC;
	Word	DA;
	Word	DHC;
	Byte	Sequence_Number;
	Word	Timer; // Rx: set to 9375 (0.9375s) Tx: set to 11250 (1.125s)
} T_Assoc;

typedef struct S_Frame
{
	Byte        PRE;
	T_Control   CTL;
	Word	    DA;
	Word	    DHC;
	Word	    SA;
	Word	    SHC;
	Byte	    Data[32];
	Byte		Len;

#ifndef USE_SE_FCS
	Byte     FCS;
#endif
} T_Frame;

typedef struct S_Deliv
{
	Boolean Include_Source: 1;
	Boolean	Addressed_Service: 1;
	T_Priority Priority: 2;
	T_Service Service_Class: 1;
} T_Deliv;

typedef struct S_Control
{
	T_Packet_Type Type: 3;
	T_Priority Priority: 2;
	Boolean Reserved: 1;
	T_Service Service_Class: 1;
	Boolean Sequence_Nb: 1;
} T_Control;

// 3.5 Variable Description
/*
3.2.18 Pending_XMIT
The boolean variable Pending_XMIT has a value of TRUE. When this event occurs, the most
recently accepted MA_DATA.request or MA_ACK_DATA.request is serviced.
3.5.12 Pending_XMIT
Pending_XMIT is a boolean variable that when set to TRUE puts the most recently accepted
MA_DATA.request or MA_ACK_DATA.request event back into the event list to be serviced. A
restart may be performed when frame transmission is deferred or aborted and the maximum
number of MAC restarts has not been reached. Pending_XMIT is initialized to the default
value of FALSE.
*/
typedef struct S_Flags
{
	Boolean Address_Match:1;
	Boolean Duplicate:1;
	Boolean Good_Frame:1;
	Boolean Pending_Xmit:1;
	Boolean Queued:1;
	Boolean RCV_List_Full:1;
	Boolean XMIT_List_Full:1;
	Boolean Retry_Backoff:1;
} T_Flags;

typedef struct S_Flags2
{
    Boolean Local_Busy:1;
    Boolean Mac_Failure:1;
    Boolean Medium_Failure:1;
    Boolean PH_Failure: 1;
    Boolean Reserved4: 1;
    Boolean Reserved5: 1;
    Boolean Reserved6: 1;
    Boolean Reserved7: 1;
} T_Flags2;

void MA_DATA_req(
	Word Destination,
	Word Destination_HC,
	T_Deliv Flags,
	Byte Data[],
	Byte Length);

void MA_ACK_DATA_req(
	Word Destination,
	Word Destination_HC,
	T_Deliv Flags,
	Byte Data[],
	Byte Length);

void MA_SM_DATA_req(
	Word Destination,
	Word Destination_HC,
	T_Deliv Flags,
	Byte Data[],
	Byte Length);

void MA_SM_ACK_DATA_req(
	Word Destination,
	Word Destination_HC,
	T_Deliv Flags,
	Byte Data[],
	Byte Length);

