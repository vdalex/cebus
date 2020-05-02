/*---------------------------------------------------------------------------*
 * plse.h                                                                    *
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

#define CRC_16 0x8005

/* implementation */
#define PH_RQ_NOP									0x00
#define PH_RQ_CC_DATA								0x04
#define PH_RQ_INITIALIZE_PROTOCOL					0x20
#define PH_RQ_SET_VALUE								0x40
#define PH_RQ_READ_VALUE							0x50

#define PH_CC_DATA_IND_SYM_1						0x09
#define PH_CC_DATA_IND_SYM_0						0x08
#define PH_CC_DATA_IND_EOF							0x0A
#define PH_CC_DATA_IND_EOP							0x0B
#define PH_CC_DATA_IND_PEOF							0x0C
#define PH_CC_DATA_CONF_SUCCESS						0x04
#define PH_CC_DATA_CONF_FAILURE_COLLISION			0x05
#define PH_CC_DATA_CONF_FAILURE_OTHER				0x06
#define PH_CC_STATUS_IND_CHANNEL_QUIET				0x10
#define PH_CC_STATUS_IND_CHANNEL_ACTIVE				0x11
#define PH_CC_STATUS_IND_GOOD_FRAME					0x12
#define PH_CC_STATUS_IND_BAD_FRAME					0x13
#define PH_INITIALIZE_PROTOCOL_CONFIRM_SUCCESS		0x20
#define PH_INITIALIZE_PROTOCOL_CONFIRM_OTHER		0x21
#define PH_EVENT_IND_MEDIUM_RESET					0x14
#define PH_EVENT_IND_JABBER_RECEIVED				0x15
#define PH_FAILURE_REPORT_IND_PH_FAILURE			0x16
#define PH_FAILURE_REPORT_IND_MEDIUM_FAILURE		0x17

typedef uint16_t Word;
typedef uint8_t Byte;
typedef uint8_t Boolean;

typedef enum E_Sym {SYM_1,SYM_0,SYM_EOF,SYM_EOP,SYM_PEOF} T_Sym;

typedef enum E_Medium_State {SUP,INF,SUP1,SUP2} T_Medium_State;
typedef enum E_Plse_State {IDLE,XMIT_PRE_SYM,RCV_PRE_SYM,XMIT_SYM,RCV_SYM,XMIT_CRC,RCV_CRC,RESET_WAIT} T_Plse_State;

typedef enum E_Confirm_Result {
	SUCCESS,
	FAILURE_COLLISION,
	FAILURE_OTHER
} T_Confirm_Result;

typedef enum E_Indication_Result {
    CHANNEL_QUIET,
    CHANNEL_ACTIVE,
    GOOD_FRAME,
    BAD_FRAME,
    PH_FAILURE,
    MEDIUM_FAILURE,
    MEDIUM_RESET,
    JABBER_RECEIVED
} T_Indication_Result;

typedef enum E_Plse_Event {
    NO_EVENT,
    SYM_TIMER_EXP,
    M_STATE_INDICATION,
    PH_CC_DATA_REQUEST,
    PH_INITIALIZE_PROTOCOL_REQUEST,
    EV_PH_FAILURE,
    EV_MEDIUM_FAILURE,
    EV_MEDIUM_RESET
} T_Plse_Event;

void MacsOut(char value);
void PH_RESET(void);
void PH_CC_DATA_REQ(T_Sym Sym);
void PH_INITIALIZE_PROTOCOL_REQ(void);
void Ma_State_Indication(T_Medium_State Medium_State, T_Medium_State Medium_Phase);
void GetEvents(void);
void PlseStateExec(void);

void SkipTxPreamble(void);

extern void PlOut(Byte value);
