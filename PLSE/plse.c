/*---------------------------------------------------------------------------*
 * plse.c                                                                    *
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
#include <stdio.h>

#include "plse.h"

/* 5.4 Variable Description.
5.4.1
CRC_COUNT
This variable is used to count the bits of the CRC as they are transmitted and received. */
int CRC_COUNT;

/* 5.4.1
CRC_REGISTER
This variable corresponds to the 16-bit CRC register. */
uint16_t CRC_REGISTER;

/* 5.4.1
LAST_RCV
This variable is used to track the state of the last received UST. */
/* 5.4.1
LAST_XMIT
This variable is used to track the state of the last transmitted UST. */
T_Medium_State LAST_RCV, LAST_XMIT;

/* 5.4.1 SYM
SYM is a variable containing the current symbol being transmitted. */
T_Sym SYM;

/* 5.4.1 SYM_TIME
The SYM_TIME variable is the defined pulse width (in unit symbol times) for the transmission
of each symbol. A ONE (1) symbol is 1 unit symbol time, a ZERO (0) symbol is 2 unit symbol
times, an EOF symbol is 3 unit symbol times, and an EOP symbol is 4 unit symbol times. */
const int SYM_TIME[5] = {1,2,3,4,8};

/* 5.4.2 SYM_TIMER
Is the timer function associated with reception and transmission of symbols (and also keeps
track of elapsed quiet time on the channel for the MAC Sublayer). */
int SYM_TIMER;

int SYM_TIMER_EXPIRATION;

/* 5.4.3
UST_COUNT
This variable is used to count USTs of a given symbol both during transmission and reception. */
int UST_COUNT;

/* 5.2.14 RCVD_STATE
This variable stores the current state being received. */
/* 5.4.1 XMIT_STATE
XMIT_STATE is a variable containing the current medium state (Inferior or Superior) being
transmitted. */
T_Medium_State XMIT_STATE,RCVD_STATE;
T_Medium_State FIRST_THETA,RECEIVED_THETA;

T_Plse_State Plse_State;
T_Plse_Event Plse_Event;

uint16_t Jabber_Ctr;

void SkipTxPreamble(void)
{
    Plse_State = XMIT_SYM;
}

/*------------------------------------------------------------------------*/
void GetEvents(void)
{
	if (SYM_TIMER >= SYM_TIMER_EXPIRATION)
		Plse_Event = SYM_TIMER_EXP;
}

/*------------------------------------------------------------------------*/
void M_STATE_REQUEST(T_Medium_State Medium_State)
{
char c;

    switch (Medium_State)
    {
        case INF:
            c = ' ';
            break;
        case SUP1:
            c = '1';
            break;
        case SUP2:
            c = '2';
            break;
        default:
            c = 'X';
    }

    PlOut(c);
}

// called by an interrupt
void Ma_State_Indication(T_Medium_State Medium_State, T_Medium_State Medium_Phase)
{
    RCVD_STATE = Medium_State; /* SUP or INF */
    RECEIVED_THETA = Medium_Phase; /* SUP1 or SUP2 */

    if (RCVD_STATE == SUP) printf("SUP "); else printf("INF ");
    if (RECEIVED_THETA == SUP1) printf("SUP1\n"); else printf("SUP2\n");
    Plse_Event = M_STATE_INDICATION;
}

/*------------------------------------------------------------------------*/
/* 3.2.1 PH_CC_DATA.request */
void PH_CC_DATA_REQ(T_Sym Sym)
{
    SYM = Sym;
    Plse_Event = PH_CC_DATA_REQUEST;
}

/* 4.2.1 PH_INITIALIZE_PROTOCOL.request */
void PH_INITIALIZE_PROTOCOL_REQ(void)
{
    Plse_Event = PH_INITIALIZE_PROTOCOL_REQUEST;
}

/*------------------------------------------------------------------------*/
/* 3.2.2 PH_CC_DATA.indication */
void PH_CC_DATA_INDICATION(T_Sym Sym)
{
    switch (Sym)
    {
        case SYM_1:
			MacsOut(PH_CC_DATA_IND_SYM_1);
            //printf("PH_CC_DATA_IND: SYM_1\n");
            break;
        case SYM_0:
			MacsOut(PH_CC_DATA_IND_SYM_0);
            //printf("PH_CC_DATA_IND: SYM_0\n");
            break;
        case SYM_EOF:
			MacsOut(PH_CC_DATA_IND_EOF);
            //printf("PH_CC_DATA_IND: EOF\n");
            break;
/* 3.5.7 The PH_CC_DATA.indication with the EOP and the
PH_CC_STATUS.indication (with a Status value of GOOD_FRAME or BAD_FRAME) will be
passed up to the MAC Sublayer at the same time. */
        // Implementation: Not used since we always send a status indication
        // about good or bad frame, only after receiving a EOP.
//        case SYM_EOP:
//			MacsOut(PH_CC_DATA_IND_EOP);
            //printf("PH_CC_DATA_IND: EOP\n");
//            break;
        case SYM_PEOF:
			MacsOut(PH_CC_DATA_IND_PEOF);
			//printf("PH_CC_DATA_IND: PEOF\n");
            break;
        default:
        ;
    }
}

/* 3.2.3 PH_CC_DATA.confirm */
void PH_CC_DATA_CONFIRM(T_Confirm_Result Result)
{
    switch (Result)
    {
        case SUCCESS:
			MacsOut(PH_CC_DATA_CONF_SUCCESS);
            //printf("PH_CC_DATA_CONF: SUCCESS\n");
            break;
        case FAILURE_COLLISION:
			MacsOut(PH_CC_DATA_CONF_FAILURE_COLLISION);
            //printf("PH_CC_DATA_CONF: FAILURE_COLLISION\n");
            break;
        case FAILURE_OTHER:
			MacsOut(PH_CC_DATA_CONF_FAILURE_OTHER);
            //printf("PH_CC_DATA_CONF: FAILURE_OTHER\n");
            break;
        default:
        ;
    }
}

/* 3.2.4 PH_CC_STATUS.indication */
void PH_CC_STATUS_INDICATION(T_Indication_Result Result)
{
    switch (Result)
    {
        case CHANNEL_QUIET:
			MacsOut(PH_CC_STATUS_IND_CHANNEL_QUIET);
            //printf("PH_CC_STATUS_IND: CHANNEL_QUIET\n");
            break;
        case CHANNEL_ACTIVE:
			MacsOut(PH_CC_STATUS_IND_CHANNEL_ACTIVE);
            //printf("PH_CC_STATUS_IND: CHANNEL_ACTIVE\n");
            break;

/* 3.5.7 The PH_CC_DATA.indication with the EOP and the
PH_CC_STATUS.indication (with a Status value of GOOD_FRAME or BAD_FRAME) will be
passed up to the MAC Sublayer at the same time. */
        case GOOD_FRAME:
			MacsOut(PH_CC_STATUS_IND_GOOD_FRAME);
            //printf("PH_CC_STATUS_IND: GOOD_FRAME\n");
            break;
        case BAD_FRAME:
			MacsOut(PH_CC_STATUS_IND_BAD_FRAME);
            //printf("PH_CC_STATUS_IND: BAD_FRAME\n");
            break;
        default:
        ;
    }
}

/* 4.2.2 PH_INITIALIZE_PROTOCOL.confirm */
void PH_INITIALIZE_PROTOCOL_CONFIRM(T_Confirm_Result Result) // request
{
    switch (Result)
    {
        case SUCCESS:
			MacsOut(PH_INITIALIZE_PROTOCOL_CONFIRM_SUCCESS);
            //printf("PH_INITIALIZE_PROTOCOL_CONFIRM: SUCCESS\n");
            break;
        default:
			MacsOut(PH_INITIALIZE_PROTOCOL_CONFIRM_OTHER);
            //printf("PH_INITIALIZE_PROTOCOL_CONFIRM: OTHER\n");
            break;
        ;
    }
}

/* 4.2.3 PH_SET_VALUE.request
4.2.3.1 Purpose.
The PH_SET_VALUE.request primitive is used by the Layer System Management to set the
value of SE Sublayer variables. */
//void PH_SET_VALUE_req

/* 4.2.4 PH_SET_VALUE.confirm
4.2.4.1 Purpose.
The PH_SET_VALUE.confirm primitive is generated in response to a PH_SET_VALUE.request
primitive to indicate the results of the request. */

/* 4.2.3 PH_READ_VALUE.request
4.2.3.1 Purpose.
The PH_READ_VALUE.request primitive is used by the Layer System Management to read the
value of PL/RF SE Sublayer variables. */

/* 4.2.4 PH_READ_VALUE.confirm
4.2.4.1 Purpose.
The PH_READ_VALUE.confirm primitive is generated in response to a
PH_READ_VALUE.request primitive to indicate the results of the request. */

/* PH_EVENT.indication
4.2.5.1 Purpose.
This primitive is used to notify the Layer System Management that a significant event has
occurred in the SE Sublayer. */
void PH_EVENT_INDICATION(T_Indication_Result Result)
{
    switch (Result)
    {
        case MEDIUM_RESET:
			MacsOut(PH_EVENT_IND_MEDIUM_RESET);
            //printf("PH_EVENT_IND: MEDIUM_RESET\n");
            break;
        case JABBER_RECEIVED:
			MacsOut(PH_EVENT_IND_JABBER_RECEIVED);
            //printf("PH_EVENT_IND: JABBER_RECEIVED\n");
            break;
        default:
        ;
    }
}


// MEDIUM_RESET and JABBER_RECEIVED

/* Callback */

/* 4.2.6 LSM_EVENT.indication
4.2.6.1 Purpose.
This primitive is used to notify the SE Sublayer that a significant event has occurred either in
the Layer System Management or in another layer. */

// MAC_FAILURE and MAC_RESET

/* 4.2.7 PH_FAILURE_REPORT.indication */
void PH_FAILURE_REPORT_INDICATION(T_Indication_Result Result)
{
    switch (Result)
    {
        case PH_FAILURE:
			MacsOut(PH_FAILURE_REPORT_IND_PH_FAILURE);
            //printf("PH_FAILURE_REPORT_IND: PH_FAILURE\n");
            break;
        case MEDIUM_FAILURE:
			MacsOut(PH_FAILURE_REPORT_IND_MEDIUM_FAILURE);
            //printf("PH_FAILURE_REPORT_IND: MEDIUM_FAILURE\n");
            break;
        default:
        ;
    }
}
/*------------------------------------------------------------------------*/
T_Medium_State SECOND_THETA(T_Medium_State in)
{
    if (in == SUP1)
        return SUP2;
    else
        return SUP1;
}

void START_SYM_TIMER(int time)
{
    SYM_TIMER = 0;
    SYM_TIMER_EXPIRATION = time;
}

void PH_RESET(void)
{
    Plse_State = IDLE;
    Plse_Event = NO_EVENT;
    SYM_TIMER = 0;
    UST_COUNT = 0;
    LAST_RCV = 0;
    LAST_XMIT = 0;
    CRC_REGISTER = 0;
    Jabber_Ctr = 0;
}

int get_symbol_ust_count(T_Sym Sym)
{
    return SYM_TIME[(int) Sym];
}

void SHIFT_1_TO_CRC(void)
{
    CRC_REGISTER = (CRC_REGISTER << 1) ^ CRC_16;
}

void SHIFT_0_TO_CRC(void)
{
    CRC_REGISTER = (CRC_REGISTER << 1);
}

/* 4.2.5.2 ...
JABBER_RECEIVED indicates that another transmitter on the medium has transmitted
a SUPERIOR state continuously for 1000 unit symbol times and is therefore in error.
For each additional 1000 unit symbol times that the "jabber" is received ... */
void JabberIncrementAndCheck(void)
{
    Jabber_Ctr++;
    if (Jabber_Ctr == 1000)
    {
        PH_EVENT_INDICATION(JABBER_RECEIVED);
        Jabber_Ctr = 0;
    }
}

/*------------------------------------------------------------------------*/
void Idle(void)
{
    switch (Plse_Event)
    {
        case M_STATE_INDICATION:
            if (RCVD_STATE == SUP)
            {
                PH_CC_STATUS_INDICATION(CHANNEL_ACTIVE);
                UST_COUNT = 1;
                FIRST_THETA = RECEIVED_THETA;
                LAST_RCV = SUP;
                START_SYM_TIMER(128);
                Plse_State = RCV_PRE_SYM;
            }
            break;

        case SYM_TIMER_EXP:
            PH_CC_STATUS_INDICATION(CHANNEL_QUIET);
            Jabber_Ctr = 0;
            START_SYM_TIMER(100);
            break;

        case PH_CC_DATA_REQUEST: //(not EOF)
            UST_COUNT = get_symbol_ust_count(SYM);
            M_STATE_REQUEST(SUP1);
            START_SYM_TIMER(114);
            Plse_State = XMIT_PRE_SYM;
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void ReceivePreamble(void)
{
    switch (Plse_Event)
    {
        case M_STATE_INDICATION:
            printf("RP state ind\n");
            if (RCVD_STATE == FIRST_THETA)
            {
                if (LAST_RCV == SUP)
                {
                    if (SYM_TIMER >= 107)
                    {
                        printf("RP_1\n");
                        UST_COUNT++;
                        START_SYM_TIMER(128);
                    }
                    else if (SYM_TIMER > 93) // && SYM_TIMER < 107
                    {
                        printf("RP_2\n");
                        UST_COUNT++;
                        START_SYM_TIMER(114);
                    } else printf("RP_2B\n");
                }
                else /* LAST_RCV = INF */
                {
                    if ((SYM_TIMER > 93) && (SYM_TIMER < 107))
                    {
                        printf("RP_3A\n");
                        PH_CC_DATA_INDICATION(UST_COUNT);
                        UST_COUNT = 1;
                        LAST_RCV = SUP;
                        START_SYM_TIMER(128);
                    }
                    else printf("RP_3B\n");
                }
            }
            else // not(FIRST_THETA)
            {
                if (LAST_RCV == SUP)
                {
                    if ((SYM_TIMER > 93) && (SYM_TIMER < 107))
                    {
                        if ((UST_COUNT > 2) && (UST_COUNT <= 8))
                        {
                            printf("RP_4\n");
                            PH_CC_DATA_INDICATION(SYM_PEOF);
                            UST_COUNT = 1;
                            LAST_RCV = SECOND_THETA(FIRST_THETA);
                            SHIFT_0_TO_CRC();
                            Plse_State = RCV_SYM;
                        }
                    } else printf("RP_4B\n");
                } else printf("RP_4C\n");
            }
            break;

        case SYM_TIMER_EXP:
            printf("RP sym timer expired\n");
            if (LAST_RCV == SUP)
            {
                printf("RP_5\n");
                PH_CC_DATA_INDICATION(UST_COUNT);
                UST_COUNT = 1;
                LAST_RCV = INF;
                START_SYM_TIMER(114);
            }
            else /* LAST_RCV = INF */
            {
                Jabber_Ctr = 0;

                if (UST_COUNT > 2) // M_STATE_INFERIOR≥2
                {
                    printf("RP_6\n");
                    // PH_CC_DATA_INDICATION(SYM_EOP);
                    PH_CC_STATUS_INDICATION(BAD_FRAME);
                    Plse_State = IDLE;
                }
                else
                {
                    printf("RP_7\n");
                    UST_COUNT++;
                    START_SYM_TIMER(114);
                }
            }
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(1);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void ReceiveSymbol(Plse_Event)
{
    switch (Plse_Event)
    {
        case M_STATE_INDICATION:
            if (RCVD_STATE == FIRST_THETA)
            {
                if (LAST_RCV == FIRST_THETA) /* Same state; count UST */
                {
                    JabberIncrementAndCheck();
                    UST_COUNT++;
                }
                else /* Input toggled state */
                {
                    Jabber_Ctr = 0;

                    PH_CC_DATA_INDICATION(UST_COUNT);
                    UST_COUNT = 1;
                    LAST_RCV = FIRST_THETA;
                }
                SHIFT_1_TO_CRC();
            }
            else /* SECOND_THETA */
            {
                if (LAST_RCV == FIRST_THETA) /* Input toggled state */
                {
                    Jabber_Ctr = 0;

                    PH_CC_DATA_INDICATION(UST_COUNT);
                    UST_COUNT = 1;
                    LAST_RCV = SECOND_THETA(FIRST_THETA);
                }
                else /* Same state; count UST */
                {
                    JabberIncrementAndCheck();
                    UST_COUNT++;
                }
                SHIFT_0_TO_CRC();
            }

            if (UST_COUNT == 4)
            {
                UST_COUNT = 1;
                CRC_COUNT = 15;
                Plse_State = RCV_CRC;
            }
            else
            {
                START_SYM_TIMER(114);
            }
            break;

        case SYM_TIMER_EXP:
            // M_STATE_INFERIOR
            // PH_CC_DATA_INDICATION(SYM_EOP);
            Jabber_Ctr = 0;
            PH_CC_STATUS_INDICATION(BAD_FRAME);
            Plse_State = IDLE;
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void ReceiveCrc(void)
{
    switch (Plse_Event)
    {
        case M_STATE_INDICATION:
            if (CRC_COUNT >= 0)
            {
                if (RCVD_STATE == FIRST_THETA)
                {
                    if ((SYM_TIMER > 93) && (SYM_TIMER < 107))
                    {
                        SHIFT_1_TO_CRC();
                        CRC_COUNT--;
                    }
                }
                else /* SECOND_THETA */
                {
                    if ((SYM_TIMER > 93) && (SYM_TIMER < 107))
                    {
                        SHIFT_0_TO_CRC();
                        CRC_COUNT--;
                    }
                }
                START_SYM_TIMER(114); // added to get state inferior FIXME
            }
            else
            {
                /* CRC_COUNT < 0 */
                if (CRC_REGISTER == 0)
                {
                    // PH_CC_DATA_INDICATION(SYM_EOP); /* Implicit */
                    PH_CC_STATUS_INDICATION(GOOD_FRAME);
                }
                else
                {
                    // PH_CC_DATA_INDICATION(SYM_EOP); /* Implicit */
                    PH_CC_STATUS_INDICATION(BAD_FRAME);
                }
                Plse_State = IDLE;
            }
            break;

        case SYM_TIMER_EXP:
            // M_STATE_INFERIOR:
            // PH_CC_DATA_INDICATION(SYM_EOP); /* Implicit */
            PH_CC_STATUS_INDICATION(BAD_FRAME);
            Plse_State = IDLE;
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void TransmitPreamble(void)
{
    switch (Plse_Event)
    {
        case SYM_TIMER_EXP:
            if (UST_COUNT > 1)
            {
                if (LAST_XMIT == SUP)
                {
                    if (SYM == SYM_EOF)
                    {
                        // printf("TP_1A\n");
                        UST_COUNT--;
                        START_SYM_TIMER(100);
                        M_STATE_REQUEST(SUP1);
                    }
                    else
                    {
                        // printf("TP_1B\n");
                        UST_COUNT--;
                        START_SYM_TIMER(114);
                        M_STATE_REQUEST(SUP1);
                    }
                }
                else // LAST_XMIT = INF
                {
                    // printf("TP_2\n");
                    UST_COUNT--;
                    START_SYM_TIMER(114);
                }
            }
            else /* UST_COUNT = 1 */
            {
                if (SYM == SYM_EOF)
                {
                    // printf("TP_3\n");
                    LAST_XMIT = SUP1;
                    PH_CC_DATA_CONFIRM(SUCCESS);
                    Plse_State = XMIT_SYM;
                }
                else
                {
                    // printf("TP_4\n");
                    PH_CC_DATA_CONFIRM(SUCCESS);
                }
            }
            break;

        case PH_CC_DATA_REQUEST: //(SYM)
            if (SYM == SYM_EOF)
            {
                // printf("TP_5\n");
                UST_COUNT = 8;
                LAST_XMIT = SUP;
                M_STATE_REQUEST(SUP1);
                START_SYM_TIMER(100);
            }
            else
            {
                if (LAST_XMIT == SUP)
                {
                    // printf("TP_6\n");
                    UST_COUNT = get_symbol_ust_count(SYM);
                    LAST_XMIT = INF;
                    M_STATE_REQUEST(INF); // Added
                    START_SYM_TIMER(114);
                }
                else // LAST_XMIT = INF
                {
                    // printf("TP_7\n");
                    UST_COUNT = get_symbol_ust_count(SYM);
                    LAST_XMIT = SUP;
                    M_STATE_REQUEST(SUP1);
                    START_SYM_TIMER(114);
                }
            }
            break;

        case M_STATE_INDICATION:
            if ((RCVD_STATE == SUP) && (LAST_XMIT == INF))
            {
                PH_CC_DATA_CONFIRM(FAILURE_COLLISION);
                UST_COUNT = 1;
                FIRST_THETA = RECEIVED_THETA;
                LAST_RCV = SUP;
                START_SYM_TIMER(128);
                Plse_State = RCV_PRE_SYM;
            }
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            if (!SYM_TIMER_EXP) PH_CC_DATA_CONFIRM(FAILURE_OTHER);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void TransmitSymbol(void)
{
    switch (Plse_Event)
    {
        case SYM_TIMER_EXP:
            if (UST_COUNT > 1)
            {
                if (LAST_XMIT == SUP1)
                {
                    // printf("TS_3\n");
                    UST_COUNT--;
                    START_SYM_TIMER(100);
                    M_STATE_REQUEST(SUP1);
                    SHIFT_1_TO_CRC();
                }
                else // LAST_XMIT = SUP2
                {
                    // printf("TS_4\n");
                    UST_COUNT--;
                    START_SYM_TIMER(100);
                    M_STATE_REQUEST(SUP2);
                    SHIFT_0_TO_CRC();
                }
            }
            else /* UST_COUNT = 1 */
                if (SYM == SYM_EOP)
                {
                    // printf("TS_5\n");
                    CRC_COUNT = 15;
                    Plse_State = XMIT_CRC;

                    // next iteration of main loop will recheck timer for expiration
                    // and regenerate a SYM_TIMER_EXP event for the state XMIT_CRC.
                }
                else
                    PH_CC_DATA_CONFIRM(SUCCESS);
            break;

        case PH_CC_DATA_REQUEST: //(SYM)
            if (LAST_XMIT == SUP1)
            {
                UST_COUNT = get_symbol_ust_count(SYM);
                // printf("TS_1: UST_COUNT = %d\n",UST_COUNT);
                LAST_XMIT = SUP2;
                START_SYM_TIMER(100);
                M_STATE_REQUEST(SUP2);
                SHIFT_0_TO_CRC();
            }
            else // LAST_XMIT = SUP2
            {
                UST_COUNT = get_symbol_ust_count(SYM);
                // printf("TS_2: UST_COUNT = %d\n",UST_COUNT);
                LAST_XMIT = SUP1;
                START_SYM_TIMER(100);
                M_STATE_REQUEST(SUP1);
                SHIFT_1_TO_CRC();
            }
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            if (!SYM_TIMER_EXP) PH_CC_DATA_CONFIRM(FAILURE_OTHER);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void TransmitCrc(void)
{
    //printf("TC_1: CRC_COUNT = %d; CRC_REGISTER = %0X4\n",CRC_COUNT,CRC_REGISTER);
    /* We are ready to transmit the first CRC bit, since the previous EOP has
    generated the last SYM_TIMER_EXP. */

    switch (Plse_Event)
    {
        case SYM_TIMER_EXP:
            if (CRC_COUNT >= 0)
            {
                // Test bit CRC_COUNT of CRC_REGISTER
                if (CRC_REGISTER && (1 << CRC_COUNT))
                {
                    CRC_COUNT--;
                    START_SYM_TIMER(100);
                    M_STATE_REQUEST(SUP1);
                }
                else
                {
                    CRC_COUNT--;
                    START_SYM_TIMER(100);
                    M_STATE_REQUEST(SUP2);
                }
            }
            else /* CRC_COUNT < 0 */
            {
                PH_CC_DATA_CONFIRM(SUCCESS);
                START_SYM_TIMER(100);
            }
            break;

        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
            PH_FAILURE_REPORT_INDICATION(PH_FAILURE);
            Plse_State = RESET_WAIT;
            break;

        case EV_MEDIUM_FAILURE:
            PH_FAILURE_REPORT_INDICATION(MEDIUM_FAILURE);
            if (!SYM_TIMER_EXP) PH_CC_DATA_CONFIRM(FAILURE_OTHER);
            Plse_State = RESET_WAIT;
            break;

        default:
        ;
    }
}

void ResetWait(void)
{
    switch (Plse_Event)
    {
        case PH_INITIALIZE_PROTOCOL_REQUEST:
            PH_RESET();
            PH_INITIALIZE_PROTOCOL_CONFIRM(SUCCESS);
            START_SYM_TIMER(100);
            Plse_State = IDLE;
            break;

        case EV_PH_FAILURE:
        case EV_MEDIUM_FAILURE:
            break;

        case EV_MEDIUM_RESET:
            if (MEDIUM_FAILURE);
                PH_EVENT_INDICATION(MEDIUM_RESET);
            break;

        default:
        ;
    }
}

void PlseStateExec(void)
{
    switch (Plse_State)
    {
        case IDLE:
            printf("State = IDLE\n");
            Idle();
            break;
        case XMIT_PRE_SYM:
            printf("State = TX_PREAMBLE\n");
            TransmitPreamble();
            break;
        case XMIT_SYM:
            printf("State = TX_SYMBOL\n");
            TransmitSymbol();
            break;
        case XMIT_CRC:
            printf("State = TX_CRC\n");
            TransmitCrc();
            break;
        case RCV_PRE_SYM:
            printf("State = RX_PREAMBLE\n");
            ReceivePreamble();
            break;
        case RCV_SYM:
            printf("State = RX_SYMBOL\n");
            ReceiveSymbol();
            break;
        case RCV_CRC:
            printf("State = RX_CRC\n");
            ReceiveCrc();
            break;
        case RESET_WAIT:
            printf("State = RESET_WAIT\n");
            ResetWait();
            break;
        default:
            printf("State = NO_EVENT\n");
        ;
    }
    Plse_Event = NO_EVENT;
}
