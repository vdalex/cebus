/*---------------------------------------------------------------------------*
 * main.c                                                                    *
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>

#include "plse.h"

#define MAX_PACKET_SIZE 80

int Macs_2_Plse_Fd,Plse_2_Macs_Fd;
int Macs_2_Llc_Fd,Llc_2_Macs_Fd;

/* implementation */
void MacsOut(char value)
{
	write(Macs_2_Llc, &value, 1);
}

/* LSB 1st */
void MacsWordOut(Word value)
{
	MacsOut(value & 0xFF);
	MacsOut(value >> 8);
}

/*
Data request packets
0       primitive
1       data size in bytes (length of data field)
2,3     Destination LSB,MSB
4,5     Destination HC LSB,MSB
6       Flags
7-n     Data

Set Value Request
0       primitive
1       nb of values
2,3     value LSB,MSB
4       variable
...

Read Value Request
0       primitive
1       variable

Init Protocol
0       primitive

*/
void get_fields(char packet[],T_Frame *pFr, T_Deliv *Flags)
{
    pFr->DA = (packet[3] << 8) || packet[2];
    pFr->DHC = (packet[5] << 8) || packet[4];
    pFr->Len = packet[1];
    memcpy(&(pFr->Data[0]),&(packet[7]),packet[1]);
    *Flags = packet[6];
}

void MacsIn(char packet[]) /* packet */
{
Word Destination;
T_Frame *pFr;
T_Deliv Flags;

    pFr = &Tx_Frame[0];

    switch (packet[0])
    {
        case MA_RQ_DATA:
            get_fields(packet,pFr,&Flags);
            MA_DATA_req(pFr,Flags);
            break;
        case MA_RQ_ACK_DATA:
            get_fields(packet,pFr,&Flags);
            MA_ACK_DATA_req(pFr,Flags);
            break;
        case MA_RQ_SM_DATA:
            get_fields(packet,pFr,&Flags);
            MA_SM_DATA_req(pFr,Flags);
            break;
        case MA_RQ_SM_ACK_DATA:
            get_fields(packet,pFr,&Flags);
            MA_SM_ACK_DATA_req(pFr,Flags);
            break;
        case MA_RQ_INIT_PROTOCOL:
            MA_INITIALIZE_PROTOCOL_req();
            break;
        case MA_RQ_SET_VALUE:
            MA_SET_VALUE_req(packet);
            break;
        case MA_RQ_READ_VALUE:
            MA_READ_VALUE_req(packet[1]);
            break;

        /* LLC */
        case LSM_IND_EVENT_LLC_FAILURE:
            LSM_EVENT_ind(LSM_EVENT_IND_LLC_FAILURE);
            break;
        case LSM_IND_EVENT_LLC_RESET:
            LSM_EVENT_ind(LSM_EVENT_IND_LLC_RESET);
            break;

/*
    UPPER_LAYER_BUSY indicates to the MAC that a condition in the Network Layer, Application
    Layer, CAL Layer, or the user application itself exists, such that the node may not receive any
    new frames from the network. Reception of this service primitive value causes the MAC
    Sublayer to enter a LOCAL_BUSY state.
    UPPER_LAYER_NOT_BUSY indicates to the MAC that a busy condition in the Network Layer,
    Application Layer, CAL Layer, or the user application itself has gone away, so that the node
    may begin receiving new frames from the network. Reception of this service primitive value
    causes the MAC Sublayer to get out of its LOCAL_BUSY state.
*/
        case LSM_IND_EVENT_UPPER_LAYER_BUSY:
            Macs_Flags2.Local_Busy = TRUE;
            LSM_EVENT_ind(LSM_EVENT_IND_UPPER_LAYER_BUSY);
            break;
        case LSM_IND_EVENT_UPPER_LAYER_NOT_BUSY:
            Macs_Flags2.Local_Busy = FALSE;
            LSM_EVENT_ind(LSM_EVENT_IND_UPPER_LAYER_NOT_BUSY);
            break;

        default:
        ;
    }
}

void PlseOut(Byte value)
{
    write(Macs_2_Plse_Fd, &value, 1);
}

T_Mac_Event PlseIn(Byte value)
{
    switch (value)
    {
        case PH_CC_DATA_IND_SYM_1:
            event = PH_CC_DATA_IND;
            Sym = SYM_1;
            break;
        case PH_CC_DATA_IND_SYM_0:
            event = PH_CC_DATA_IND;
            Sym = SYM_0;
            break;
        case PH_CC_DATA_IND_EOF:
            event = PH_CC_DATA_IND;
            Sym = SYM_EOF;
            break;
/* 3.5.7 The PH_CC_DATA.indication with the EOP and the
PH_CC_STATUS.indication (with a Status value of GOOD_FRAME or BAD_FRAME) will be
passed up to the MAC Sublayer at the same time. */
        //case PH_CC_DATA_IND_EOP:
        case PH_CC_DATA_IND_PEOF:
            event = PH_CC_DATA_IND;
            Sym = SYM_PEOF;
            break;
        case PH_CC_DATA_CONF_SUCCESS:
            event = PH_CC_DATA_CONF_SUCCESS;
            break;
        case PH_CC_DATA_CONF_FAILURE_COLLISION:
            event = PH_CC_DATA_CONF_FAILURE_COLLISION;
            break;
        case PH_CC_DATA_CONF_FAILURE_OTHER:
            event = PH_CC_DATA_CONF_FAILURE_OTHER;
            break;
        case PH_CC_STATUS_IND_CHANNEL_QUIET:
            event = PH_CC_ST_IND_CH_QUIET:
            break;
        case PH_CC_STATUS_IND_CHANNEL_ACTIVE:
            event = PH_CC_ST_IND_CH_ACTIVE;
            break;
/* 3.5.7 The PH_CC_DATA.indication with the EOP and the
PH_CC_STATUS.indication (with a Status value of GOOD_FRAME or BAD_FRAME) will be
passed up to the MAC Sublayer at the same time. */
        case PH_CC_STATUS_IND_GOOD_FRAME:   // PH_CC_DATA_IND_EOP implicit
            Sym = SYM_EOP;
            Macs_Flags.Good_Frame = TRUE;
            event = PH_CC_DATA_IND;
            break;
        case PH_CC_STATUS_IND_BAD_FRAME:    // PH_CC_DATA_IND_EOP implicit
            Sym = SYM_EOP;
            Macs_Flags.Good_Frame = FALSE;
            event = PH_CC_DATA_IND;
            break;

#if 0
        /* FIXME */
        case PH_INITIALIZE_PROTOCOL_CONFIRM_SUCCESS:
            // event =
            break;
        case PH_INITIALIZE_PROTOCOL_CONFIRM_OTHER:
            // event =
            break;
        case PH_EVENT_IND_JABBER_RECEIVED:
            // event =
            break;
#endif

        case PH_EVENT_IND_MEDIUM_RESET:
            event = LSM_EVENT_IND_MEDIUM_RESET;
            break;
        case PH_EVENT_IND_PH_RESET:
            event = LSM_EVENT_IND_PH_RESET;
            break;
        case PH_FAILURE_REPORT_IND_PH_FAILURE:
            Macs_Flags2.PH_Failure = TRUE;
            event = LSM_EVENT_IND_PH_FAILURE;
            break;
        case PH_FAILURE_REPORT_IND_MEDIUM_FAILURE:
            Macs_Flags2.Medium_Failure = TRUE;
            event = LSM_EVENT_IND_MEDIUM_FAILURE;
            break;
        default:
            event = NO_EVENT;
    }
    return event;
}

int pipe_open(const char *file, int flag)
{
int fd;

    printf("Opening %s\n",file);

    if ((fd = open(file,flag)) < 0)
    {
        perror("Open");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void GetMacsEvent(void
{
      // Retrans_Time_Expired
      // NO_CH_QUIET_SENT
      // MAC_FAILURE
      MACS_Event = Retrans_Time_Expired;
}

int main(int argc, char *argv[])
{
Byte value;
char packet[MAX_PACKET_SIZE];

	Macs_2_Plse_Fd = pipe_open("plse2macs",O_RDONLY | O_NONBLOCK);
	Plse_2_Macs_Fd = pipe_open("macs2plse",O_WRONLY);

    Llc_2_Macs_Fd = pipe_open("llc2macs",O_RDONLY | O_NONBLOCK);
    Macs_2_Llc_Fd = pipe_open("macs2llc",O_WRONLY);

	printf("MACS: pipes opened\n");

	while(1)
	{
      if (read(Plse_2_Macs,&value,1) == 1)
        {
          MACS_Event = PlseIn(value);
        }
      else
        {
          if (read(Llc_2_Macs_Fd,packet,MAX_PACKET_SIZE) > 0)    // -1: error, 0: EOF
          {
            MacsIn(packet);
          }
          else
          {
            GetMacsEvents();
          }
        }
      MACS_loop();
	}

	if (Macs_2_Plse_Fd != -1) close(Macs_2_Plse_Fd);
	if (Plse_2_Macs_Fd != -1) close(Plse_2_Macs_Fd);
	if (Llc_2_Macs_Fd != -1) close(Llc_2_Macs_Fd);
	if (Macs_2_Llc_Fd != -1) close(Macs_2_Llc_Fd);

	return 0;
}
