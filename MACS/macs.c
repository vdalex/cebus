/*---------------------------------------------------------------------------*
 * macs.c                                                                    *
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

void BeginTransmit()
{
    Assemble_Normal_Frame();
    Reset_Retrans_Timer(); // Restart_Retrans_Timer
#ifndef USE_SE_FCS
    Compute_FCS();
#endif
    Create_Preamble();
    Restart_Count = 1;
    Sym = Get_Next_Sym();
    PH_CC_DATA_req(Sym);
}

void Idle(void)
{
    switch (MACS_Event)
    {
        case PH_CC_ST_IND_CH_QUIET:
            break;

        case PH_CC_ST_IND_CH_ACTIVE:
            MACS_State = RCV;
            break;

        case MA_DATA_REQ:
            if (Macs_Flags2.Local_Busy)
            {
                Result = FAILURE_LOCAL_BUSY;
                MA_DATA_conf(Result);
            }
            else
            {
                BeginTransmit();
                MACS_State = XMIT;
            }
            break;

        case MA_ACK_DATA_REQ:
            if (Macs_Flags2.Local_Busy)
            {
                Result = FAILURE_LOCAL_BUSY;
                MA_ACK_DATA_conf(Result);
            }
            else
            {
                BeginTransmit();
                MACS_State = XMIT;
            }
            break;

        case NO_CH_QUIET_SENT:
            MA_FAILURE_REPORT_ind(PH_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            break;

        default:
        ;
    }
}

void QueueTransmit(void)
{
    Save_Current_Frame();
    Macs_Flags.Pending_Xmit = TRUE;
    if (Quiet >= MPQBW)
        Macs_Flags.Queued = FALSE;
    if (Busy_Wait > 0)
        Busy_Wait--;
    MACS_State = RCV;
}

/* 3.2.3 Higher_Priority
The priority of an MA_DATA.request or MA_ACK_DATA.request service primitive which arrives
during the XMIT_WAIT State is higher than the priority of the pending MA_DATA.request or
MA_ACK_DATA.request. As a result, the higher priority request may be substituted for the
lower priority request. */
void HigherPriority(void)
{
    Result = FAILURE_PRIORITY;
    if ((XMIT_Type == UNACK_DATA) || (XMIT_Type == ADR_UNACK_DATA))
        MA_DATA_conf(Result);
    else /* XMIT_Type = ACK_DATA, ADR_ACK_DATA */
        MA_ACK_DATA_conf(Result);
    Good_XMIT_Count = 0;
    Restart_Count = 0;
    Retry_Count = 0;
    Busy_Wait = 0;
    Assemble_Normal_Frame();
#ifndef USE_SE_FCS
    Compute_FCS();
#endif
    Create_Preamble();
    Macs_Flags.Pending_Xmit = TRUE;
    Reset_Retrans_Timer();
}

void TransmitWait(void)
{
    switch (MACS_Event)
    {
        case PH_CC_ST_IND_CH_QUIET:
            if (Quiet < Wait_Time)
                Quiet++;
            else
            {
                Sym = Get_Next_Sym();
                PH_CC_DATA_req(Sym);
                MACS_State = XMIT;
            }
            break;

        case PH_CC_ST_IND_CH_ACTIVE:
            switch (XMIT_Type)
            {
                case UNACK_DATA:
                case ACK_DATA:
                    if (Restart_Count < MAX_RESTART)
                        QueueTransmit();
                    else
                    {
                        Result = FAILURE_EXCESSIVE_COLLISION;
                        if (XMIT_Type == UNACK_DATA)
                          MA_DATA_conf(Result);
                        else
                          MA_ACK_DATA_conf(Result);
                        Busy_Wait = 0;
                        Restart_Count = 0;
                        if (Quiet >= MPQBW) Macs_Flags.Queued = FALSE;
                        MACS_State = RCV;
                    }
                    break;

                case ADR_UNACK_DATA:
                case ADR_ACK_DATA:
                    QueueTransmit();
                    break;
            }
            break;

        case MA_DATA_REQ:
        case MA_ACK_DATA_REQ:
            HigherPriority();
            MACS_State = IDLE_WAIT;
            break;

        case Retrans_Time_Expired:
            if (XMIT_Type == ADR_UNACK_DATA)
            {
                if (Good_XMIT_Count >= 1)
                    Result = SUCCESS;
                else /* Good_XMIT_Count < 1 */
                    Result = FAILURE_EXCESSIVE_COLLISIONS;
                MA_DATA_conf(Result);
            }
            else /* XMIT_Type = ADR_ACK_DATA */
            {
                Result = FAILURE_EXCESSIVE_COLLISIONS;
                MA_ACK_DATA_conf(Result);
            }
            Wait_Time = CONST_CA_WAIT_TIME - Quiet;
            Ack_Service = NONE;
            Good_XMIT_Count = 0;
            Restart_Count = 0;
            Retry_Count = 0;
            Busy_Wait = 0;

            MACS_State = IDLE_WAIT;
            break;

        case NO_CH_QUIET_SENT:
            MA_FAILURE_REPORT_ind(PH_FAILURE);
            Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break;

TransmitFailureCommonCode:

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:
    }
}

void AckServiceNoneAction(void)
{
    Macs_Flags.Queued = TRUE;
    Restart_Count = 0;
    Wait_Time = 6;
    Quiet = 1;
    Save_Current_Frame();
    Ack_Service = SOURCE;
    MACS_State = AS_WAIT;
}

void AckServiceSourceAction(void)
{
    Retry_Count = Retry_Count + 1;
    Wait_Time = 10;
    Quiet = 1;
    MACS_State = AS_WAIT;
}

void AckServiceIackFailDestAction(void)
{
    Wait_Time = 10;
    Quiet = 1;
    MACS_State = AS_WAIT;
}

void XmitConfirmSuccess(void)
{
    if not(Sym == EOP)
    {
        Sym = Get_Next_Sym();
        PH_CC_DATA_req(Sym);
        MACS_State = XMIT;
    }
    else
        switch (XMIT_Type)
        {
            case UNACK_DATA:
                Result = SUCCESS;
                MA_DATA_conf(Result);
                Wait_Time = CONST_CA_WAIT_TIME;
                Macs_Flags.Queued = TRUE;
                Restart_Count = 0;
                Quiet = 1;
                MACS_State = IDLE_WAIT;
                break;

            case ADR_UNACK_DATA:
                Good_XMIT_Count++;
                Macs_Flags.Queued = TRUE;
                Restart_Count = 0;
                Quiet = 1;
                if (Good_XMIT_Count < Ch_Access_Num)
                {
                    Save_Current_Frame();
                    Macs_Flags.Pending_Xmit = TRUE;
                }
                else /* Good_XMIT_Count >= Ch_Access_Num */
                {
                    Result = SUCCESS;
                    MA_DATA_conf(Result);
                    Good_XMIT_Count = 0;
                    Wait_Time = CONST_CA_WAIT_TIME;
                }
                MACS_State = IDLE_WAIT;
                break;

            case ACK_DATA:
                switch (Ack_Service)
                {
                    case NONE:
                        AckServiceNoneAction();
                        break;

                    case SOURCE:
                        AckServiceSourceAction();
                        break;

                    default:
                    ;
                }

            case ADR_ACK_DATA:
                switch (Ack_Service)
                {
                    case NONE:
                        AckServiceNoneAction();
                        break;

                    case SOURCE:
                        AckServiceSourceAction();
                        break;

                    case DESTINATION:
                        AckServiceIackFailDestAction();
                        break;

                    default:
                    ;
                }

            case IACK:
            case FAILURE:
                if (Ack_Service == DESTINATION)
                    AckServiceIackFailDestAction();
                break;

            default:
            ;
        }
}

void SaveFrame(void)
{
    Save_Current_Frame();
    Macs_Flags.Pending_Xmit = TRUE;
    Macs_Flags.Queued = FALSE;
    if (Busy_Wait > 0)
        Busy_Wait = Busy_Wait - 1;
    MACS_State = RCV;
}

void XmitFailureCollision()
{
    switch (Ack_Service)
    {
        case NONE:
            switch (XMIT_Type)
            {
                case UNACK_DATA:
                case ACK_DATA:
                    if (Restart_Count < MAX_RESTART)
                        SaveFrame();
                    else
                        Result = FAILURE_EXCESSIVE_COLLISION;
                        if (XMIT_Type == UNACK_DATA)
                            MA_DATA_conf(Result);
                        else /* XMIT_Type = ACK_DATA */
                            MA_ACK_DATA_conf(Result);
                        Busy_Wait = 0;
                        Restart_Count = 0;
                        Macs_Flags.Queued = FALSE;
                        MACS_State = RCV;
                    break;

                case ADR_UNACK_DATA:
                case ADR_ACK_DATA:
                    SaveFrame();
                    break;
                default:
                ;
            }

        case SOURCE:
            Macs_Flags.Retry_Backoff = TRUE;
            MACS_State = RCV;
            break;

        case DESTINATION:
            /* No Action - Continue to transmit an IACK, Failure Packet, or Addressed IACK */
            MACS_State = XMIT;
            break;
        default:
        ;
    }
}

void Transmit(void)
{
    switch (MACS_Event)
    {
        case PH_CC_DATA_CONF_SUCCESS:
            XmitConfirmSuccess();
            break;

        case PH_CC_DATA_CONF_FAILURE_COLLISION:
            XmitFailureCollision();
            break;

/* The events MA_DATA.request and MA_ACK_DATA.request may occur during the transmission
of an IACK. The frame may be encoded and marked as a pending frame, but the current
transmission cannot be interrupted.

A Higher Priority MA_DATA.request or MA_ACK_DATA.request may occur during the XMIT State.
The frame may be encoded and marked as a pending frame, but the current transmission
cannot be interrupted.

The event Pending_XMIT may be true during the XMIT State (in the case that a requested
frame is pending during the transmission of an IACK or a normal frame already in progress).
However, Pending_XMIT is not an event/condition that needs to be serviced during the
XMIT State. */

        case PH_CC_DATA_CONF_FAILURE_OTHER:
            Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            Restart_Count = 0;
            Wait_Time = CONST_CA_WAIT_TIME;
            Quiet = 1;
            MACS_State = IDLE_WAIT;
            break;

        case No_PH_Confirm_Sent:
            MA_FAILURE_REPORT_ind(PH_FAILURE);
            Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break;

TransmitFailureCommonCode:

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:
        ;
    }
}

void As_Wait_Failure_No_Acknowledge(void)
{
    Result = FAILURE_NO_ACKNOWLEDGE;
    MA_ACK_DATA_conf(Result);
    Busy_Wait = 0;
    Wait_Time = CONST_CA_WAIT_TIME - Quiet;
    Retry_Count = 0;
    Ack_Service = NONE;
}

void As_Wait(void)
{
    switch (MACS_Event)
    {
        case PH_CC_ST_IND_CH_ACTIVE:
            MACS_State = RCV;
            break;

        case PH_CC_ST_IND_CH_QUIET:
            if (Quiet < Wait_Time)
                Quiet++;
            else /* Quiet >= Wait_Time */
                switch (Ack_Service)
                {
                    case SOURCE:
                        if (Retry_Count < Num_Retries)
                            Sym = Get_Next_Sym();
                            PH_CC_DATA_req(Sym);
                            MACS_State = XMIT;
                        else
                            switch (XMIT_Type)
                            {
                                case ADR_ACK_DATA:
                                    Save_Current_Frame();
                                    Macs_Flags.Pending_Xmit = TRUE;
                                    if (Busy_Wait > 0) Busy_Wait--;
                                    Retry_Count = 0;
                                    Ack_Service = NONE;
                                    MACS_State = IDLE_WAIT;
                                    break;

                                case ACK_DATA:
                                    As_Wait_Failure_No_Acknowledge();
                                    MACS_State = IDLE_WAIT;
                                    break;

                                default:
                                ;
                            }
                        break

                    case DESTINATION:
                        Ack_Service = NONE;
                        Wait_Time = CONST_CA_WAIT_TIME - Quiet;
                        MACS_State = IDLE_WAIT;
                        break;

                    default:
                    ;
                }
            break;

        case Retrans_Time_Expired:
            if (XMIT_Type == ADR_ACK_DATA) and (Ack_Service == SOURCE)
            {
                As_Wait_Failure_No_Acknowledge();
                Good_XMIT_Count = 0;
                MACS_State = IDLE_WAIT;
            }
            break;

/* The events MA_DATA.request and MA_ACK_DATA.request may occur when Ack_Service
 = Destination during the AS_WAIT State. The frame may be encoded and marked as
 a pending frame, but the current Ack_Service transmission cannot be interrupted.
A Higher Priority MA_DATA.request or MA_ACK_DATA.request may occur during the
AS_WAIT State. Any frames which are already pending must be discarded. The frame
may be encoded and marked as a pending frame, but the Ack_Service transmission
cannot be interrupted.
The event Pending_XMIT may be true during the AS_Wait State. However, Pending_XMIT
does not interrupt an Ack_Service transmission and, therefore, is not an
event/condition that needs to be serviced during the AS_Wait State. */

        case NO_CH_QUIET_SENT:
            MA_FAILURE_REPORT_ind(PH_FAILURE);
            if ((Ack_Service == SOURCE) || Macs_Flags.Pending_Xmit)
                Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            if ((Ack_Service == SOURCE) || Macs_Flags.Pending_Xmit)
                Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:;
    }
}

void ReceivedBadFrame(void)
{
	Errored_Frame_Count++; // Statistical counters
    Discard_Frame();
    Ack_Service = NONE;
    Wait_Time = CONST_CA_WAIT_TIME - Quiet;
    MACS_State = IDLE_WAIT;
}

void ReceivedFrameConfirm(T_Confirm_Result Result)
{
    MA_ACK_DATA_conf(Result);
    Discard_Frame();
    Busy_Wait = 0;
    Good_XMIT_Count = 0;
    Wait_Time = CONST_CA_WAIT_TIME;
}

void GoodFrameAddressMatch(void)
{
    switch (RCV_Type)
    {
        case IACK:
        case FAILURE:
        case ADR_IACK:
            ReceivedBadFrame();
            break;

        case UNACK:
            MA_DATA_ind(pRx_Current_Fr);
            Ack_Service = NONE;
            Wait_Time = CONST_CA_WAIT_TIME;
            MACS_State = IDLE_WAIT;
            break;

        case ADR_UNACK:
            if (CheckDuplicate())
                Discard_Frame();
            else
            {
                Update_Sequence_Nb_Addr_List();
                MA_DATA_ind(pRx_Current_Fr);
            }
            Ack_Service = NONE;
            Wait_Time = CONST_CA_WAIT_TIME;
            MACS_State = IDLE_WAIT;
            break;

        case ACK_DATA OR ADR_ACK_DATA:
            if (CheckDuplicate())
            {
                Discard_Frame();
                if (RCV_Type == ACK_DATA)
                    Assemble_Ack_Frame(IACK,NONE);
                else /* RCV_Type = ADR_ACK_DATA */
                    Assemble_Ack_Frame(ADR_IACK,SUCCESS);
            }
            else /* Not a duplicate */
            {
                // (RCV_Assoc_List_Full OR Other Local_Busy Condition)
                if (Macs_Flags.RCV_List_Full || Macs_Flags2.Local_Busy)
                    /* Receiving MAC cannot accept received frame */
                    if (RCV_Type == ACK_DATA);
                        Assemble_Ack_Frame(FAILURE,REMOTE_BUSY);
                    else /* RCV_Type = ADR_ACK_DATA */
                        Assemble_Ack_Frame(ADR_IACK,FAILURE_REMOTE_BUSY);

#if 0
                else if (MAC cannot accept received frame for other reason) // FIXME
                    if (RCV_Type == ACK_DATA)
                        Assemble_Ack_Frame (FAILURE,FAILURE_REMOTE_REJECT);
                    else /* RCV_Type = ADR_ACK_DATA */
                        Assemble_Ack_Frame(ADR_IACK,FAILURE_REMOTE_REJECT);
#endif
                else /* Receiving MAC can accept received frame */
                    if (RCV_Type == ACK_DATA)
                        Assemble_Ack_Frame (IACK,NONE)
                    else /* RCV_Type = ADR_ACK_DATA */
                        Assemble_Ack_Frame(ADR_IACK,SUCCESS);
                    if (Ack_Service == NONE)
                        Ack_Service = DESTINATION;
                        /* Otherwise, Ack_Service is already DESTINATION */

                    Update_Sequence_Nb_Addr_List();
	                MA_DATA_ind(pRx_Current_Fr);
            }
            Sym = Get_Next_Sym();
            PH_CC_DATA_req(Sym);
            MACS_State = XMIT; /* To Transmit Acknowledgment Frame */
            break;

        default:
        ;
    }
}

void ReceivedFrameDelayed(void)
{
    if (Good_XMIT_Count < Ch_Access_Num)
    {
        Busy_Wait = 12;
        Save_Current_Frame();
        Macs_Flags.Pending_Xmit = TRUE;
    }
    else /* Good_XMIT_Count >= Ch_Access_Num */
    {
        Result = FAILURE_NO_ACKNOWLEDGE:
        ReceivedFrameConfirm(Result);
    }
}

/* Process received frame */
void ProcessReceivedFrame()
{
T_Ack_Info_Field AckOrFailResult;

    if (Good_Frame)
	{
		Valid_Received_Frame_Count++; // Statistical counters

        switch (Ack_Service)
        {
            case NONE:
            case DESTINATION:
                if (Macs_Flags.Address_Match)
				{
                    GoodFrameAddressMatch();
				}
                else
				{
                    ReceivedBadFrame();
				}
                break;

            case SOURCE: /* Expecting an IACK, FAILURE packet, or Addressed IACK */
                switch (RCV_Type)
                {
					case IACK:
					case ADR_IACK:
                        AckOrFailResult = AckOrFailureGetInfo();
                        switch (AckOrFailResult)
                        {
                            case SUCCESS:
                                Result = SUCCESS;
                                ReceivedFrameConfirm(Result);
                                break;

                            case FAILURE_REMOTE_REJECT:
                                if (Macs_Flags.Address_Match)
                                    Result = FAILURE_REMOTE_REJECT;
                                else
                                    Result = FAILURE_NO_ACKNOWLEDGE;
                                ReceivedFrameConfirm(Result);
                                break;

                            case FAILURE_REMOTE_BUSY:
                                if (Macs_Flags.Address_Match)
                                    ReceivedFrameDelayed();
                                else
                                {
                                    Result = FAILURE_NO_ACKNOWLEDGE;
                                    ReceivedFrameConfirm(Result);
                                }
                                break;

                            default:
                                Result = FAILURE_OTHER; /* FIXME */
                                ReceivedFrameConfirm(Result);
                        }
                        break;

                    case FAILURE:
                        AckOrFailResult = AckOrFailureGetInfo();
                        switch (AckOrFailResult)
                        {
                            case FAILURE_REMOTE_REJECT:
                                Result = FAILURE_REMOTE_REJECT;
                                ReceivedFrameConfirm(Result);
                                break;

                            case FAILURE_REMOTE_BUSY:
                                ReceivedFrameDelayed();
                                break;

                            default:
                                Result = FAILURE_OTHER; /* FIXME */
                                ReceivedFrameConfirm(Result);
                        }
                        break;

                    default: /* RCV_Type = UNACK_DATA, ACK_DATA, ADR_UNACK_DATA,
                                ADR_ACK_DATA, OR ADR_IACK/Not_Address_Match
                                - Another node talked out of turn */
                        Result = FAILURE_NO_ACKNOWLEDGE;
                        ReceivedFrameConfirm(Result);
                }
                Retry_Count = 0;
                Macs_Flags.Retry_Backoff = FALSE;
                Quiet = 1;
                Ack_Service = NONE;
                MACS_State = IDLE_WAIT;
                break;

            default:
            ;
        }
	}
    else /* bad frame */
	{
		Fcs_Error_Received_Frame_Count++; // Statistical counters

        /* Received Noise, Bad IACK, Bad Failure Packet, Bad Addressed IACK, or Bad Frame */
        switch (Ack_Service)
        {
            case NONE:
            case DESTINATION:
                ReceivedBadFrame();
                break;

            default:
                if ((Retry_Count < Num_Retries) && !Macs_Flags.Retry_Backoff)
                {
                    Discard_Frame();
                    Sym = Get_Next_Sym();
                    PH_CC_DATA_req(Sym);
                    MACS_State = XMIT; /* To transmit an immediate retry */
                }
                else
                { /* Other cases of bad received frame */
                    if (Good_XMIT_Count < Ch_Access_Num)
                    {
                        Save_Current_Frame();
                        Macs_Flags.Pending_Xmit = TRUE;
                        if (Busy_Wait > 0)
                            Busy_Wait--;
                    }
                    else /* Good_XMIT_Count >= Ch_Access_Num */
                    {
                        if ((Retry_Count < Num_Retries) && Macs_Flags.Retry_Backoff)
                            Result = FAILURE_OTHER;
                        else /* Retry_Count = Num_Retries */
                            Result = FAILURE_NO_ACKNOWLEDGE;
                        MA_ACK_DATA_conf(Result);
                        Good_XMIT_Count = 0;
                        Wait_Time = CONST_CA_WAIT_TIME;
                        Busy_Wait = 0;
                    }
                    Discard_Frame();
                    Retry_Count = 0;
                    Macs_Flags.Retry_Backoff = FALSE;
                    Quiet = 1;
                    Ack_Service = NONE;
                    MACS_State = IDLE_WAIT;
                } /* Cases for received Bad frame */
        }
	}
} /* Process Received Frame */

void Receive(void)
{
    switch (MACS_Event)
    {
        /* *** For Symbol Encoding Sublayers other than PL and RF: */
#ifndef USE_PLSE
        case PH_CC_ST_IND_BIT_ERROR:
            Discard_Frame();
            /* Begin receiving next frame */
            break;
#endif

/* The events MA_DATA.request and MA_ACK_DATA.request may occur during the RCV
State. The frame may be encoded and marked as a pending frame, but the current
reception cannot be interrupted. A Higher Priority MA_DATA.request or
MA_ACK_DATA.request may occur during RCV State. Any frames already pending must
be discarded. The frame may be encoded and marked as a pending frame, but the
current reception cannot be interrupted. The event Pending_XMIT may be true
during the RCV State. However, Pending_XMIT is not an event/condition that
needs to be serviced during the RCV State. */

        case PH_CC_DATA_IND:
/* 3.5.7 The PH_CC_DATA.indication with the EOP and the
PH_CC_STATUS.indication (with a Status value of GOOD_FRAME or BAD_FRAME) will be
passed up to the MAC Sublayer at the same time. */
            if (Sym == EOP)
                ProcessReceivedFrame();
            else
            {
                Save_Symbol(Sym);
                if (Sym == EOF) //First EOF Symbol
                    Quiet = 1;
            }
            break

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            if (Macs_Flags.Pending_Xmit)
            {
                Result = FAILURE_OTHER;
                MA_DATA_OR_ACK_DATA_conf(Result);
            }
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:
        ;
    }
}

void DataRequest(void)
{
    if (Macs_Flags2.Local_Busy)
    {
        Result = FAILURE_LOCAL_BUSY;
        MA_DATA_OR_ACK_DATA_conf(Result);
        Restart_Count = 0;
        Wait_Time = CONST_CA_WAIT_TIME - Quiet;
        MACS_State = IDLE_WAIT;
    }
    else
    {
        Assemble_Normal_Frame();
        Reset_Retrans_Timer();
#ifndef USE_SE_FCS
        Compute_FCS();
#endif
        Create_Preamble();

        CalculateWaitTime(Priority);
        Restart_Count = 1;
        MACS_State = XMIT_WAIT;
    }
}

void Idle_Wait(void)
{
    switch (MACS_Event)
    {
        case PH_CC_ST_IND_CH_QUIET:
            if (Quiet >= Wait_Time)
            {
                Macs_Flags.Queued = FALSE;
                MACS_State = IDLE;
            }
            else
                Quiet++;
            break;

        case PH_CC_ST_IND_CH_ACTIVE:
            MACS_State = RCV;
            break;

        case MA_DATA_REQ:
        case MA_ACK_DATA_REQ:
            DataRequest();
            break

        case NO_CH_QUIET_SENT:
            MA_FAILURE_REPORT_ind(PH_FAILURE);
            if (Macs_Flags.Pending_Xmit)
                Result = FAILURE_OTHER;
            MA_DATA_OR_ACK_DATA_conf(Result);
            MACS_State = RESET_WAIT;
            break

        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            if (Macs_Flags.Pending_Xmit)
            {
                Result = FAILURE_OTHER;
                MA_DATA_OR_ACK_DATA_conf(Result);
            }
            MACS_State = RESET_WAIT;
            break;

        case MAC_FAILURE:
            MA_FAILURE_REPORT_ind(MAC_FAILURE);
            MACS_State = RESET_WAIT;
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:
            if (Macs_Flags.Pending_Xmit)
            {
                if (Macs_Flags2.Local_Busy)
                {
                    Result = FAILURE_LOCAL_BUSY;
                    MA_DATA_OR_ACK_DATA_conf(Result);
                    Restart_Count = 0;
                    Macs_Flags.Pending_Xmit = FALSE;
                    Wait_Time = CONST_CA_WAIT_TIME - Quiet;
                }
                else
                {
                    Restart_Count++;
                    Macs_Flags.Pending_Xmit = FALSE;

                    CalculateWaitTime();
                    MACS_State = XMIT_WAIT;
                }
            }
    }
}

void Reset_Wait(void)
{
    switch (MACS_Event)
    {
        case LSM_EVENT_IND_MEDIUM_FAILURE:
        case LSM_EVENT_IND_PH_FAILURE:
            break;

        case LSM_EVENT_IND_MEDIUM_RESET:
            if (Macs_Flags2.Medium_Failure)
            {
                MACS_State = IDLE;
                Macs_Flags2.Medium_Failure = FALSE; // Added
            }
            break;

        case LSM_EVENT_IND_PH_RESET:
            if (Macs_Flags2.PH_Failure)
            {
                MACS_State = IDLE;
                Macs_Flags2.PH_Failure = FALSE; // Added
            }
            break;

        case MA_INIT_PROTOCOL_REQ:
            Reset_MAC();
            MA_INIT_PROTOCOL_conf(SUCCESS);
            MACS_State = IDLE;
            break;

        default:
        ;
    }
}

void MACS_loop(void)
{
    switch (MACS_State)
    {
        case IDLE:
            Idle();
            break;
        case XMIT_WAIT:
            TransmitWait();
            break;
        case XMIT:
            Transmit();
            break;
        case AS_WAIT:
            As_Wait();
            break;
        case RCV:
            Receive();
            break;
        case IDLE_WAIT:
            Idle_Wait();
            break;
        case RESET_WAIT:
            Reset_Wait();
            break;
        default:
        ;
    }
    MACS_Event = NO_EVENT;
}

