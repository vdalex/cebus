/*---------------------------------------------------------------------------*
 * gl_vars.c                                                                 *
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
#include "macs.h"

T_Flags			    Macs_Flags  = {0,0,0,0,0,0,0,0};	//bits 0-7: 0=false, 1=true
T_Flags2			Macs_Flags2 = {0,0,0,0,0,0,0,0};	//bits 0-7: 0=false, 1=true
T_Ack_Service		Ack_Service = NONE;
T_State             MACS_State;
T_Mac_Event         MACS_Event;
T_Packet_Type		RCV_Type;

/* 3.5.28 XMIT_Type
XMIT_Type identifies the frame type which is being transmitted or is ready for transmission.
XMIT_Type may take on the values IACK, ACK_DATA, UNACK_DATA, FAILURE,
ADR_ACK_DATA, ADR_IACK, and ADR_UNACK_DATA. XMIT_Type is undefined initially, and
a default value is not important. */
T_Packet_Type		XMIT_Type;

T_Confirm_Result	Result;
T_Sym				Sym;

Byte                Current_Preamble;
Byte                Running_Sum_Sym_One;

Byte				Busy_Wait;
Byte				Ch_Access_Num;
Byte				CH_Access_Period;
Byte				Good_XMIT_Count;
Byte				MPQBW; // Min/Priority/Queue/Busy_Wait
Byte				Num_Retries;
Byte				Quiet;
Byte				Random_Wait;
Byte				Restart_Count;

Byte				Retry_Count;

/* 3.5.25 Wait_Time
Wait_Time is the number of unit symbol times which the MAC must wait for a given event.
The default value for Wait_Time is 0. */
Byte				Wait_Time;
Word				Max_Retrans_Time;

/* Implementation defines */
//#define NB_FRAME_MAX 10

/* Implementation variables */
T_Frame Tx_Frame[2];
T_Frame Rx_Frame[2];

// Read
Word Valid_Received_Frame_Count;
Word Fcs_Error_Received_Frame_Count;
Word Errored_Frame_Count;
Word Failed_Transmission_Count;
Word Successful_Transmission_Count;

// Read/Write
Word Individual_Node_Address;
Word Group_Addresses;
Word Max_Restart;
Word Unack_Ch_Access_Period;
Word Ack_Ch_Access_Period;

/*
3.5.21 Retrans_Time
This variable defines the amount of time allowed for multiple transmissions of a frame.
Retrans_Time may not exceed the constant value MAX_RETRANS_TIME.
(MAX_RETRANS_TIME is the default value for Retrans_Time.) Note that there are two
separate Retrans_Time variables which handle Unacknowledged Service and Acknowledged
Service independently.
For simplicity, the state machine description does not distinguish between the two.
If an unacknowledged service data request is being service by the MAC, it can be safely
assumed that the appropriate Retrans_Time variable is being used. The same is true for
acknowledged service. The timer mechanism used to measure elapsed time against
Retrans_Time must have an accuracy of 10% . */
// Byte				Retrans_Time;
Word Unack_Retrans_Time;
Word Ack_Retrans_Time;
