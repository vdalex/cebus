/*---------------------------------------------------------------------------*
 * sbrout.c                                                                  *
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

/* Get_Next_Sym()/Save_Sym() variables */
T_Packet_Field Tx_Field;
T_Packet_Field Rx_Field;
int Tx_Bit_Ctr;
int Rx_Bit_Ctr;
int Tx_Data_Index;
int Rx_Data_Index;


void CalculateWaitTime(T_Priority Priority)
{
    // 3.3.21 Set_Min/Priority/Queue/Busy_Wait
/*
Set_Min/Priority/Queue/Busy_Wait
Set the value of the variable Min/Priority/Queue/Busy_Wait. The value of this variable is the
delay before transmission (in unit symbol times) due to the minimum wait time for the
network (10 USTs), the present frame's priority, the value of the Queued variable, and the
current value of the Busy_Wait variable. The values for each of the variable combinations are
listed below:
Priority    Queued Value    Delay
High        FALSE           10 + Busy_Wait
High        TRUE            14 + Busy_Wait
Standard    FALSE           14 + Busy_Wait
Standard    TRUE            18 + Busy_Wait
Deferred    FALSE           18 + Busy_Wait
Deferred    TRUE            22 + Busy_Wait
*/
    MPQBW = Busy_Wait;

    switch (Priority)
    {
        case HIGH:
            MPQBW += 10;
            break;
        case STANDARD:
            MPQBW += 14;
            break;
        case DEFERRED:
            MPQBW += 18;
            break;
    }

    if (Macs_Flags.Queued)
        MPQBW += 4;

    // 3.3.22 Set_Random_Wait
/*
Set the value of the variable Random_Wait. The value of the variable is either 0, 1, 2, or 3
unit symbol times. The value is chosen at random and corresponds to an additional delay
prior to frame transmission. The value of the random delay is obtained from the sum of the
third and fourth least significant bits of the current Preamble value and the least significant
two bits of the local address. Carries are discarded in this summation.
*/
    Random_Wait = 0;
    if (Current_Preamble & 0x08) Random_Wait++;
    if (Current_Preamble & 0x10) Random_Wait++;
    Random_Wait += Individual_Node_Address & 0x03;
    Random_Wait &= 0x03;

    Wait_Time = MPQBW + Random_Wait - Quiet;
}

/* 3.3.2 Assemble_Ack_Frame
Assemble the fields to form the appropriate type of Acknowledge MAC frame: IACK, FAILURE
(Remote_Busy), FAILURE (Remote_Reject), ADR_IACK (Success), ADR_IACK (Remote_Busy), or
ADR_IACK (Remote_Reject). */

/* typedef enum E_Ack_Info_Field{
	SUCCESS = 0,
	FAILURE_REMOTE_BUSY = 0x0001,
	FAILURE_REMOTE_BUSY_RCV_LIST = 0x0010,
	FAILURE_REMOTE_REJECT = 0x0080,
	FAILURE_REMOTE_REJECT_EXTENDED = 0x0090
} T_Ack_Info_Field; */
/*
Assemble_Ack_Frame(IACK);
Assemble_Ack_Frame(ADR_IACK(SUCCESS));
Assemble_Ack_Frame(FAILURE(REMOTE_BUSY))
Assemble_Ack_Frame(ADR_IACK(FAIL_REMOTE_BUSY))
Assemble_Ack_Frame(FAILURE(REMOTE_REJECT))
Assemble_Ack_Frame(ADR_IACK(FAIL_REMOTE_REJECT))
Assemble_Ack_Frame(IACK)
Assemble_Ack_Frame(ADR_IACK(SUCCESS));
*/
void Assemble_Ack_Frame(T_Frame *p_Tx, T_Packet_Type Type,T_Ack_Info_Field Info) /* FIXME input parameter what type */
{
/* FIXME incomplete */
}

/* 3.3.3 Assemble_Normal_Frame
Assemble the appropriate fields to form a Normal MAC frame. */
void Assemble_Normal_Frame(T_Frame *p_Tx)
{
    /* Initialize the Get_Next_Sym() variables */
    Tx_Field = F_PREAMBLE;
    Tx_Bit_Ctr = 8;
    Tx_Data_Index = 0;

    XMIT_Type = p_Tx->CTL.Type;

/* FIXME incomplete */
}

/* 3.3.4 Check_for Duplicate
Check the received frame against the previously received frame to determine whether it is a
duplicate. For ADR_UNACK_DATA and ADR_ACK_DATA frames, the Source Address, Source
House Code, and one-bit sequence number are compared. */
bool CheckDuplicate(T_Frame *pCurrent_Fr, T_Frame *pPrev_Fr)
{
    switch(pCurrent_Fr->CTL.Type)
    {
        case ADR_UNACK_DATA:
        case ADR_ACK_DATA:
            return (pCurrent_Fr->SA == pPrev_Fr->SA) &&
                   (pCurrent_Fr->SHC == pPrev_Fr->SHC) &&
                   (pCurrent_Fr->CTL.Sequence_Nb == pPrev_Fr->CTL.Sequence_Nb);

/* An ACK_DATA frame may be identified as a duplicate and discarded based on time of arrival
only. However, it is recommended that ACK_DATA frames arriving within the time window for
immediates retries be compared against the previously received frame. Comparison may
proceed in the following steps:
a) Compare the frame structure (number of fields and field sizes).
b) If Source Address and Source House Code are present in the received frames,
compare those fields.
c) If Source Address and Source House Code are not present in the received frames,
compare Information fields. */

        case ACK_DATA:
            /* FIXME incomplete */
            break;
        default:
        ;
    }
}

#ifndef USE_SE_FCS
/* 3.3.5 Compute_FCS
Compute the 8-bit Frame Check Sequence (FCS) value, and append it to the end of the MAC
frame. (Because the Power Line and RF Symbol Encoding Sublayers provide error detection,
FCS computation is not carried out for these media.) */
void Compute_FCS(void)
{
    /* Calculate the 2's complement of the sum of all bytes, */
}
#endif

/* 3.3.6 Create_Preamble
Obtain the random 8-bit Preamble value. */
void Create_Preamble(void)
{
    /* 2.3.1 Preamble (PRE).
    Each MAC frame shall contain a fixed length, 8-bit Preamble field. The value of the Preamble
    field is a pseudo-random bit sequence which allows for contention resolution. As a possible
    method for generating a pseudo-random value, a running sum may be kept of all "one"
    symbols that have been transmitted since "power-up" of the network. This process is
    accomplished through the following algorithm:
    The 8-bit running sum value is preset at power-up to the sum of the 8-bit subfields of the
    source address as provided by Layer System Management. Carries are discarded.
    As each "ONE" symbol ("ZERO" symbols are not counted) is transmitted, the running sum is
    incremented. Carries from this summation are also discarded.
    The current value of the running sum at the time of packet transmission is supplied to the
    packet's Preamble field.
    The Preamble field is sent least significant bit first without leading zero suppression and is
    terminated with the EOF symbol to delimit the Preamble from the remainder of the frame.
    For an immediate retransmission (retry) of a Normal MAC frame, the Preamble field may be
    omitted. This modification may be used in conjunction with an extended length initial EOF
    (i.e., the first EOF of the frame) on RF and Power Line media. This option may also be used
    for the Preamble of an Acknowledge MAC frame. */

    Current_Preamble = Running_Sum_Sym_One;
}

/* 3.3.18 Reset_Retrans_Timer
Reset the timer which measures the elapsed time against the value Retrans_Time. This action
is performed after a normal frame is assembled. Specifically, this event occurs when any new
data request is received during the IDLE State or IDLE_WAIT State and when a new data
request with a higher priority is received by the MAC Sublayer during the XMIT_WAIT State.
Resetting the timer allows the MAC to have the full amount of time (Retrans_Time) to transmit
all copies of the new, higher priority frame. */
void Reset_Retrans_Timer(void)
{
/* FIXME incomplete */
}

/* 3.3.7 Discard_Frame
Discard the data symbols of the frame received so far. This action is carried out when a "bit
error" is detected in reception. A bit error occurs during contention or noise and indicates
that the frame will probably contain errors. Since error correction is not used, a frame
subjected to bit errors should be discarded. */
void Discard_Frame(void)
{
    /* Initialize the Save_Sym() variables */
    Rx_Field = F_PREAMBLE;
    Rx_Bit_Ctr = 8;
}

/* 3.3.19 Save_Current_Frame
Save the frame currently in the MAC frame transmission buffer for another transmission
attempt. */
void Save_Current_Frame(void)
{
/* FIXME incomplete */
}

/* 3.3.17 Reset_MAC
The MAC Sublayer resets itself in response to an MA_INITIALIZE_PROTOCOL.request from the
Layer System Management. Such a reset may occur from any MAC state. The reset puts the
MAC Sublayer into its IDLE State and sets all variables to their default values. */
void Reset_MAC(void)
{
    Valid_Received_Frame_Count = 0;
    Fcs_Error_Received_Frame_Count = 0;
    Errored_Frame_Count = 0;
    Failed_Transmission_Count = 0;
    Successful_Transmission_Count = 0;

    // Read/Write
    Individual_Node_Address = 0;
    Group_Addresses = 0;
    Max_Restart = 1;
    Unack_Ch_Access_Period = 1;
    Unack_Retrans_Time = 1;
    Ack_Ch_Access_Period = 1;
    Ack_Retrans_Time = 1;

    Busy_Wait = 0;
    Ch_Access_Num = 1;
    CH_Access_Period = Retrans_Time; // default
    Good_XMIT_Count = 0;
    Num_Retries = 1;
    Quiet = 1;
    Restart_Count = 0;
    Retry_Count = 0;
    Wait_Time = 0;

    /* Initialize the Get_Next_Sym() variables */
    Tx_Field = F_PREAMBLE;
    Tx_Bit_Ctr = 8;
    Tx_Data_Index = 0;
    /* Initialize the Save_Sym() variables */
    Rx_Field = F_PREAMBLE;
    Rx_Bit_Ctr = 8;
    Rx_Data_Index = 0;
}

/*
3.3.23 Substitute_Higher_Priority_Req
During the XMIT_WAIT State, an MA_DATA.request or MA_ACK_DATA.request service
primitive is received which has a higher priority than the pending request. The pending
request is discarded and replaced by the new request. In this manner, the higher priority
request is granted access to the channel first. */
void Substitute_Higher_Priority_Req(void)
{}

void IncTxStatCounter(T_Confirm_Result Result)
{
	if (Result == SUCCESS) Successful_Transmission_Count++;
	else Failed_Transmission_Count++; // Statistical counters
}

void MaIndication(T_Frame *pFr)
{
	MacsOut(pFR->CTL);
	MacsWordOut(pFR->SA);
	MacsWordOut(pFR->SHC);
	MacsWordOut(pFR->DA);
	MacsWordOut(pFR->DHC);
	MacsOut(pFR->Len);
	for (integer i = 0; i < Len; i++)
	{
		MacsOut(pFr->Data[i]);
	}
}

void Update_Sequence_Nb_Addr_List(void)
{
}

T_Ack_Info_Field AckOrFailureGetInfo()
{

}

/* 3.3.8 Get_Next_Sym(Sym)
Set the value of the variable Sym to the value of the next symbol to be transmitted. */

/* Transmission is accomplished by passing the eight bits of the Preamble LSB first to the
Physical Layer. No leading zero suppression is used on the Preamble. After the Preamble
octet, an EOF symbol is passed. The Control field is passed LSB first with leading zero
suppression, followed by an EOF symbol. All address and house code fields are transmitted
LSB first as 16 bit fields with leading zero suppression. If a "1" bit occurs in the upper octet,
no zeros are suppressed in the lower octet. Each of these fields is followed by an EOF symbol.
The Information field is transmitted in the order it is received from the Logical Link Control
Sublayer. No leading zero suppression is used on the Information field of a normal frame.
(Note, however, that leading zero suppression is used on the Information field of an IACK
frame.) An EOF symbol terminates the Information field. The FCS field is transmitted LSB
first with leading zero suppression and is terminated with an EOP symbol. */

/* {F_PREAMBLE,F_CONTROL,F_ADDR,F_HOUSE,F_INFO,F_FCS} T_Packet_Field; */

/* FIXME implement LZS */
T_Sym GetNextBitFromByte(Byte data, int next_field_length, bool lzs)
{
T_Sym Result;

    if (Tx_Bit_Ctr == 0)
    {
#ifdef USE_SE_FCS
        if (Tx_Field == F_INFO)
        {
        /* FIXME get data length */
        /* if Tx_Data_Index == last byte */
            Result = SYM_EOP;

        /* else */
            /* Result = SYM_EOF; */
        }
        else
            Result = SYM_EOF;
#else
        if (Tx_Field == F_FCS) Result = SYM_EOP; else Result = SYM_EOF;
#endif
        Tx_Bit_Ctr = next_field_length;
        Tx_Field++;
    }
    else
    {
        /* ctr  8 7 6 5 4 3 2 1 0 */
        /* bit  0 1 2 3 4 5 6 7 */
        if ((data >> (Tx_Bit_Ctr - 8)) & 1) Result = SYM_1; else Result = SYM_0;
        Tx_Bit_Ctr--;
    }
    return Result;
}

/* FIXME implement LZS */
T_Sym GetNextBitFromWord(Word data, int next_field_length, bool lzs)
{
T_Sym Result;

    if (Tx_Bit_Ctr == 0)
    {
        Result = SYM_EOF;
        Tx_Bit_Ctr = next_field_length;
        Tx_Field++;
    }
    else
    {
        /* ctr  16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1 0 */
        /* bit   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
        if ((data >> (Tx_Bit_Ctr - 16)) & 1) Result = SYM_1; else Result = SYM_0;
        Tx_Bit_Ctr--;
    }
    return Result;
}

T_Sym Get_Next_Sym(void)
{
T_Sym Result;

    switch(Tx_Field)
    {
        /* Preamble - EOF no LZS */
        case F_PREAMBLE:
            Result = GetNextBitFromByte(p_Tx_Fr->PRE, 8, FALSE);
            break;
        /* Control  - EOF LZS */
        case F_CONTROL:
            Result = GetNextBitFromByte(p_Tx_Fr->CTL,16, TRUE);
            break;
        /* Address  - EOF LZS */
        case F_ADDR:
            Result = GetNextBitFromWord(p_Tx_Fr->DA,16, TRUE);
            break;
        /* House    - EOF LZS */
        case F_HOUSE:
            Result = GetNextBitFromWord(p_Tx_Fr->DHC, 8, TRUE);
            break;
        /* Info     - EOF/EOP no LZS except for IACK */
        case F_INFO:
        /* FIXME IACK */
            Result = GetNextBitFromByte(p_Tx_Fr->Data[Tx_Data_Index], 8, FALSE);
            break;
#ifndef USE_SE_FCS
        /* FCS      - EOP LZS */
        case F_FCS:
            Result = GetNextBitFromByte(p_Tx_Fr->FCS, 8, TRUE);
            break;
#endif
        default:
        ;
    }
    if (Result == SYM_ONE) Running_Sum_Sym_One++;
    return Result;
}

/* FIXME not complete */
void PutNextSymToByte(T_Sym Sym, Byte data, int next_field_length)
{
//	if (Sym == SYM_EOF)
    if (Rx_Bit_Ctr == 0)
    {
#ifdef USE_SE_FCS
        if (Rx_Field == F_INFO) Result = SYM_EOP; else Result = SYM_EOF;
#else
        if (Rx_Field == F_FCS) Result = SYM_EOP; else Result = SYM_EOF;
#endif
        Rx_Bit_Ctr = next_field_length;
        Rx_Field++;
    }
    else
    {
        /* ctr  8 7 6 5 4 3 2 1 0 */
        /* bit  0 1 2 3 4 5 6 7 */
        if ((data >> (Rx_Bit_Ctr - 8)) & 1) Result = SYM_1; else Result = SYM_0;
        Rx_Bit_Ctr--;
    }
}

void Save_Symbol(T_Sym Sym)
{
    switch(Rx_Field)
    {
        /* Preamble - EOF no LZS */
        case F_PREAMBLE:
            PutNextSymToByte(Sym, p_Rx->PRE, 8);
            break;
        /* Control  - EOF LZS */
        case F_CONTROL:
            PutNextSymToByte(Sym, p_Rx->CTL, 16);
            break;
        /* Address  - EOF LZS */
        case F_ADDR:
            PutNextSymToWord(Sym, p_Rx->DA, 16);
            break;
        /* House    - EOF LZS */
        case F_HOUSE:
            PutNextSymToWord(Sym, p_Rx->DHC, 8);
            break;
        /* Info     - EOF/EOP no LZS except for IACK */
        case F_INFO:
            PutNextSymToByte(Sym, p_Rx->Data[Rx_Data_Index], 8);
        /* FIXME IACK */
            break;
#ifndef USE_SE_FCS
        /* FCS      - EOP LZS */
        case F_FCS:
            PutNextSymToByte(Sym, p_Rx->FCS, 8);
            break;
#endif
        default:
        ;
    }
}

