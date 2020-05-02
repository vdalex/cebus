/*---------------------------------------------------------------------------*
 * intf.c                                                                    *
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

// 3.3.15 PH_CC_DATA.request(Sym)
void PH_CC_DATA_req(T_Sym Sym)
{
    PlseOut(PH_RQ_CC_DATA + Sym);
}

/* Common code for the MA_*_DATA_req */
void MA_Packet_req(T_Frame *pFr, T_Deliv Flags)
{
    if (Flags.Include_Source || Flags.Addressed_Service)
    {
        pFr->SA = Individual_Node_Address;
        pFr->SHC = Group_Addresses;
    }

    pFr->CTL.Priority = Flags.Priority;
    pFr->CTL.Service_Class = Flags.Service_Class;
}

// 1.1.3.2.1 MA_DATA.request
void MA_DATA_req(T_Frame *pFr, T_Deliv Flags)
{
    MA_Packet_Req(pFr, Flags);

    if (Flags.Addressed_Service)
        pFr->CTL.Type = ADR_UNACK_DATA;
    else
        pFr->CTL.Type = UNACK_DATA;

	MACS_Event = MA_DATA_REQ;
}

// 1.1.4.2.1 MA_ACK_DATA.request
void MA_ACK_DATA_req(T_Frame *pFr, T_Deliv Flags)
{
    MA_Packet_Req(pFr, Flags);

    if (Flags.Addressed_Service)
        pFr->CTL.Type = ADR_ACK_DATA;
    else
        pFr->CTL.Type = ACK_DATA;

	MACS_Event = MA_ACK_DATA_REQ;
}

// 1.2.3.2.1 MA_SM_DATA.request
void MA_SM_DATA_req(T_Frame *pFr, T_Deliv Flags)
{
/* 1.2.3.2.1.2
Because the MSDU is privileged at the
level of the Data Link Layer, the house code of the destination must be the same as the local
house code. Thus, the Source House Code field may be Null */

    if (Destination_HC == Group_Addresses)
    {
	    MA_Packet_Req(pFr, Flags);

        if (Flags.Addressed_Service)
            pFr->Type = ADR_UNACK_DATA;
        else
            pFr->CTL.Type = UNACK_DATA;

        MACS_Event = MA_SM_DATA_REQ;
	}
	else {/* FIXME hot potato code */}
}

// 1.2.4.2.1 MA_SM_ACK_DATA.request
void MA_SM_ACK_DATA_req(T_Frame *pFr, T_Deliv *Flags)
{
    if (pFr->DHC == Group_Addresses)
    {
	    MA_Packet_Req(pFr, Flags);

        if (Flags.Addressed_Service)
            pFr->CTL.Type = ADR_ACK_DATA;
        else
            pFr->CTL.Type = ACK_DATA;

        MACS_Event = MA_SM_ACK_DATA_REQ;
	}
	else {/* FIXME hot potato code */}
}

// 1.1.3.2.3 MA_DATA.confirm
void MA_DATA_conf(T_Confirm_Result Result)
{
	MacsOut(MA_CF_DATA || Result);
}

// 1.1.4.2.3 MA_ACK_DATA.confirm
void MA_ACK_DATA_conf(T_Confirm_Result Result)
{
	MacsOut(MA_CF_ACK_DATA || Result);
}

// 1.2.3.2.3 MA_SM_DATA.confirm
void MA_SM_DATA_conf(T_Confirm_Result Result)
{
	IncTxStatCounter(Result);
	MacsOut(MA_CF_SM_DATA || Result);
}

// 1.2.4.2.3 MA_SM_ACK_DATA.confirm
void MA_SM_ACK_DATA_conf(T_Confirm_Result Result)
{
	IncTxStatCounter(Result);
	MacsOut(MA_CF_SM_ACK_DATA || Result);
}

void MA_DATA_OR_ACK_DATA_conf(T_Confirm_Result Result)
{
	IncTxStatCounter(Result);

    if ((XMIT_Type == ACK_DATA) || (XMIT_Type == ADR_ACK_DATA))
        MA_ACK_DATA_conf(Result);
    else
        MA_DATA_conf(Result);
}

//1.1.3.2.2 MA_DATA.indication
void MA_DATA_ind(T_Frame pFr)
{
	MacsOut(MA_IND_DATA);
	MaIndication(pFr);
    // MAC's receive frame buffer is cleared.
}

//1.1.4.2.2 MA_ACK_DATA.indication
void MA_ACK_DATA_ind(T_Frame pFr)
{
	MacsOut(MA_IND_ACK_DATA);
	MaIndication(pFr);
    // MAC's receive frame buffer is cleared.
}

//1.2.3.2.2 MA_SM_DATA.indication
void MA_SM_DATA_ind(T_Frame pFr)
{
	MacsOut(MA_IND_SM_DATA);
	MaIndication(pFr);
}

//1.2.4.2.2 MA_SM_ACK_DATA.indication
void MA_SM_ACK_DATA_ind(T_Frame pFr)
{
	MacsOut(MA_IND_SM_ACK_DATA);
	MaIndication(pFr);
}

//1.2.5.2.1 MA_INITIALIZE_PROTOCOL.request
void MA_INITIALIZE_PROTOCOL_req(void)
{
	MACS_Event = MA_INIT_PROTOCOL_REQ;
}

//1.2.5.2.2 MA_INITIALIZE_PROTOCOL.confirm
Boolean MA_INIT_PROTOCOL_conf(T_Confirm_Result Result)
{
	MacsOut(MA_IND_ACK_DATA || Result);
}

/*
//1.2.5.2.3 MA_SET_VALUE.request
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
*/
/*
Set Value Request
0       primitive
1       nb of values
2,3     value LSB,MSB
4       variable
...
*/

/* Count: number of couples of parameters/variables */
void MA_SET_VALUE_req(char packet[]) // Variable,Value
{
    Word Value;
    int i, count;
    bool Result[16];
    T_Mac_Var Var, Param[16];

    count = packet[1];

    for (i = 0; i < count; i++)
    {
        Var = packet[4+3*i];
        Value = packet[2+3*i] + (packet[3+3*i] << 8); // LSB,MSB
        Param[i] = Var;
        Result[i] = TRUE;

        switch (Var)
          { // Read only
            // Read/Write
            case INDIVIDUAL_NODE_ADDRESS:
                Individual_Node_Address = Value;
                /* 2.3.1 Preamble (PRE) */
                Running_Sum_Sym_One = ((Individual_Node_Address >> 8) + (Individual_Node_Address & 0xff)) & 0xff;
                break;
            case GROUP_ADDRESSES:
                Group_Addresses = Value;
                break;
            case MAX_RESTART:
                Max_Restart = Value;
                break;
            case CH_ACCESS_NUM:
                Ch_Access_Num = Value;
                break;
            case UNACK_CH_ACCESS_PERIOD:
                Unack_Ch_Access_Period = Value;
                break;
            case UNACK_RETRANS_TIME:
                Unack_Retrans_Time = Value;
                break;
            case ACK_CH_ACCESS_PERIOD:
                Ack_Ch_Access_Period = Value;
                break;
            case ACK_RETRANS_TIME:
                Ack_Retrans_Time = Value;
                break;
            case MAX_RETRANS_TIME:
                Max_Retrans_Time = Value;
                break;
            case NUM_RETRIES:
                Num_Retries = Value;
                break;
            default:
                Result[i] = FALSE;
        }
    }

    //1.2.5.2.4 MA_SET_VALUE.confirm
	MacsOut(MA_CF_SET_VALUE);
    for (i = 0; i < count; i++)
    {
        MacsOut(2 * count + 1);
        MacsOut(Result[i]);
        MacsOut(Param[i]);
    }
}

//1.2.5.2.5 MA_READ_VALUE.request
/* The variable VALID_RECEIVED_FRAME_COUNT contains the number of valid frames received
since the last read of this variable.
The variable FCS_ERROR_RECEIVED_FRAME_COUNT contains the number of frames
received with FCS errors since the last read of this variable. Other errored frames are
counted in ERRORED_FRAME_COUNT.
The variable FAILED_TRANSMISSION_COUNT contains the number of unsuccessful
transmissions from both acknowledged and unacknowledged service. Any of the failure
parameters used in the MA_DATA.confirm, MA_ACK_DATA.confirm, MA_SM_DATA.confirm,
and MA_SM_ACK_DATA.confirm service primitives contribute to the value of
FAILED_TRANSMISSION_COUNT. The count is set to 0 each time the variable is read by the
Layer System Management.
The variable SUCCESSFUL_TRANSMISSION_COUNT contains the total number of successful
transmissions using both types of service. The count is set to 0 after each time the variable is
read.
Individual and group addresses are stored in the parameters named accordingly.
The
parameters
MAX_RESTART,
CH_ACCESS_NUM,
UNACK_CH_ACCESS_PERIOD,
ACK_CH_ACCESS_PERIOD,
UNACK_RETRANS_TIME,
ACK_RETRANS_TIME,
MAX_RETRANS_TIME, and NUM_RETRIES govern channel access.
Support of these variables is not required by the standard. Their use is optional and may be
implemented at the manufacturer’s discretion. */
void MA_READ_VALUE_req(T_Mac_Var Var)
{
word Result;

    switch (Var)
      { // Read only
        case VALID_RECEIVED_FRAME_COUNT:
            Result = Valid_Received_Frame_Count;
            Valid_Received_Frame_Count = 0;
            break;
        case FCS_ERROR_RECEIVED_FRAME_COUNT:
            Result = Fcs_Error_Received_Frame_Count;
            Fcs_Error_Received_Frame_Count = 0;
            break;
        case ERRORED_FRAME_COUNT:
            Result = Errored_Frame_Count;
            Errored_Frame_Count = 0;
            break;
        case FAILED_TRANSMISSION_COUNT:
            Result = Failed_Transmission_Count;
            Failed_Transmission_Count = 0;
            break;
        case SUCCESSFUL_TRANSMISSION_COUNT:
            Result = Successful_Transmission_Count;
            Successful_Transmission_Count = 0;
            break;

// Read/Write
        case INDIVIDUAL_NODE_ADDRESS:
            Result = Individual_Node_Address;
            break;
        case GROUP_ADDRESSES:
            Result = Group_Addresses;
            break;
        case MAX_RESTART:
            Result = Max_Restart;
            break;
        case CH_ACCESS_NUM:
            Result = Ch_Access_Num;
            break;
        case UNACK_CH_ACCESS_PERIOD:
            Result = Unack_Ch_Access_Period;
            break;
        case UNACK_RETRANS_TIME:
            Result = Unack_Retrans_Time;
            break;
        case ACK_CH_ACCESS_PERIOD:
            Result = Ack_Ch_Access_Period;
            break;
        case ACK_RETRANS_TIME:
            Result = Ack_Retrans_Time;
            break;
        case MAX_RETRANS_TIME:
            Result = Max_Retrans_Time;
            break;
        case NUM_RETRIES:
            Result = Num_Retries;
            break;
        default:
            MA_READ_VALUE_conf(0,FALSE);
            return;
        ;
    }
    MA_READ_VALUE_conf(Result,TRUE);
}

//1.2.5.2.6 MA_READ_VALUE.confirm
void MA_READ_VALUE_conf(Word Result, bool Success)
{
	MacsOut(MA_CF_READ_VALUE);
	MacsWordOut(Result);
	MacsOut(Success);
}

//1.2.5.2.7 LSM_EVENT.indication
void LSM_EVENT_ind(T_Mac_Event Value)
{
    MACS_Event = Value;
}

//1.2.5.2.8 MA_FAILURE_REPORT.indication
void MA_FAILURE_REPORT_ind(T_Confirm_Result Result)
{
	MacsOut(MA_IND_FAIL_REPORT || Result);
}
