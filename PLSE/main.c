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

int Macs_2_Plse_Fd,Plse_2_Macs_Fd;
int Pl_2_Plse_Fd,Plse_2_Pl_Fd;

extern int SYM_TIMER;
extern int SYM_TIMER_EXPIRATION;
extern int UST_COUNT;
extern uint16_t Jabber_Ctr;

/* implementation */
void MacsOut(char value)
{
	write(Plse_2_Macs_Fd, &value, 1);
}

void MacsIn(char value)
{
    switch (value)
      {
        case PH_RQ_NOP:
            break;
        case PH_RQ_CC_DATA:     // SYM_1
            PH_CC_DATA_REQ(SYM_1);
            break;
        case PH_RQ_CC_DATA + 1: // SYM_0
            PH_CC_DATA_REQ(SYM_0);
            break;
        case PH_RQ_CC_DATA + 2: // EOF
            PH_CC_DATA_REQ(SYM_EOF);
            break;
        case PH_RQ_CC_DATA + 3: // EOP
            PH_CC_DATA_REQ(SYM_EOP);
            break;
//        case PH_RQ_CC_DATA + 4: // PEOF
//            PH_CC_DATA_REQ(SYM_PEOF);
            break;
        case PH_RQ_INITIALIZE_PROTOCOL:
            PH_INITIALIZE_PROTOCOL_REQ();
            break;
        case PH_RQ_SET_VALUE:
        case PH_RQ_READ_VALUE:
            break;
        case 0x70:
            SkipTxPreamble();
            break;
        default:
            MacsOut(0xFF);
      }
}

void PlOut(Byte value)
{
    write(Plse_2_Pl_Fd, &value, 1);
}

void PlIn(Byte value)
{
T_Medium_State Medium_State, Medium_Phase;

    switch (value)
      {
        case ' ':
            Medium_State = INF;
            break;
        case '1':
            Medium_State = SUP;
            Medium_Phase = SUP1;
            break;
        case '2':
            Medium_State = SUP;
            Medium_Phase = SUP2;
            break;
        default:
        ;
      }
    Ma_State_Indication(Medium_State, Medium_Phase);
}

int pipe_open(const char *file, int flag)
{
int fd;

    printf("Opening %s\n",file);

    if ((fd = open(file,flag)) < 0)
    {
        perror("Open"); //ing %s\n",file);
        exit(EXIT_FAILURE);
    }
    return fd;
}

int main(int argc, char *argv[])
{
Byte value;
unsigned int Step_Timer;

	Macs_2_Plse_Fd = pipe_open("macs2plse",O_RDONLY | O_NONBLOCK);
	Plse_2_Macs_Fd = pipe_open("plse2macs",O_WRONLY);

    Pl_2_Plse_Fd = pipe_open("pl2plse",O_RDONLY | O_NONBLOCK);
	Plse_2_Pl_Fd = pipe_open("plse2pl",O_WRONLY);

	printf("PLSE: pipes opened\n");

    PH_RESET();

	while(1)
	{
      printf("SYM_TIMER = %d; SYM_TIMER_EXPIRATION = %d\n", SYM_TIMER,SYM_TIMER_EXPIRATION);
      printf("UST_COUNT = %d\n",UST_COUNT);
      printf("Jabber_Ctr = %d\n", Jabber_Ctr);

      if (read(Pl_2_Plse_Fd,&value,1) == 1)
        {
          PlIn(value);
        }
      else
        {
          if (read(Macs_2_Plse_Fd,&value,1) == 1)
          {
            MacsIn(value);
          }
          else
          {
            if (scanf("%u",&Step_Timer) == 1)
            {
                SYM_TIMER += Step_Timer;
            }
            GetEvents();
          }
        }
      PlseStateExec();
	}

	if (Macs_2_Plse_Fd != -1) close(Macs_2_Plse_Fd);
	if (Plse_2_Macs_Fd != -1) close(Plse_2_Macs_Fd);
	if (Pl_2_Plse_Fd != -1) close(Pl_2_Plse_Fd);
	if (Plse_2_Pl_Fd != -1) close(Plse_2_Pl_Fd);

	return 0;
}
