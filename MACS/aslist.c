/*---------------------------------------------------------------------------*
 * aslist.c                                                                    *
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

/* 3.5.16 RCV_Assoc_List
The Receive Association List (RCV_Assoc_List) is a list of Data Link Layer associations
generated by the reception of Addressed Service frames (acknowledged or unacknowledged).
Each association in the list contains the following information:
    Source Address: MAC Address of the node from which the Addressed Service frame was received.
    Source House Code: House Code (system address or zone address) of the node from
        which the Addressed Service frame was received.
    Destination Address: MAC Address (individual or group address) to which the
        Addressed Service frame was sent. (This information is necessary because multiple
        addresses are supported by the receiving node -- individual MAC address, broadcast
        address, and possibly group addresses).
    Destination House Code: House Code (system address or zone address) to which the
        Addressed Service frame was sent. (This information is necessary because multiple
        house codes -- local house code and broadcast house code -- are supported by the
        receiving node).
    Sequence Number: One-bit sequence number obtained from the Control field of the
        received frame. If the same sequence number is received in subsequent frames from
        the same source node, the frames are identified as duplicates and are discarded. If a
        different sequence number is received from the same source node (i.e., if the sequence
        number toggles), the frame is identified as a new frame to be processed and passed up
        to the Network Layer. The new sequence number is put into the Receive Association
        List. If the association has “timed out” and been discarded by the time a new frame is
        received from the same source, the frame is accepted as new, a new association is
        established, and the received sequence number is placed in the association list.
    Timer: The timer governs how long an association will be maintained in the list.
        Upon reception of the first copy of a new frame, the timer is set to a value of
        1.25 x MAX_RETRANS_TIME = 0.9375 seconds.
        (MAX_RETRANS_TIME = 0.75 seconds)
        If the same frame is received again before the timer expires, it will be discarded as a
        duplicate. Note that all copies of the same frame will be received within a time interval
        no larger than MAX_RETRANS_TIME. These timing rules, in conjunction with the
        timing rules of the transmitter, ensure that the transmitting node will always maintain
        an association longer than the receiving node. In this manner, there will be no
        ambiguity about whether a received frame is new or a duplicate. Once the timer
        expires, the association is discarded, and space in the list for a new association
        becomes available. The timer mechanism must have an accuracy of 10% .

        In an implementation, the same physical timer could be used to track the age of
        multiple associations in the list. However, such details are left to the implementor.

Support for the reception of Addressed Service frames is mandatory. The minimum size of the
RCV_Assoc_List is one association. Support for two or three associations is recommended.
The maximum size of the list is not specified and remains the decision of the implementor.

Once the RCV_Assoc_List becomes full, the Data Link Layer variable RCV_List_Full is set to
TRUE, and the Data Link Layer cannot receive any new Addressed Service frames from the
network. Once an association in the list “times out”, it is removed from the list, and
RCV_List_Full is set to FALSE. Note that when the list is full, a received frame is not rejected
with “REMOTE_BUSY” or “REMOTE_BUSY_RCV_LIST” until the frame has been checked
against the list. If the frame is a duplicate of a previously received frame, it is acknowledged
with “SUCCESS” and then discarded.

The above description specifies the information which must be maintained in the list and the
behavior of the Data Link Layer in relation to that information. This specification does not
imply that any particular data structures be used to implement the list in a product. */

T_Assoc			    RCV_Assoc_List[NB_RX_ASSOC_LIST];		// 3 recommended, 1 minimum
int                 RCV_AL_Free_Count = NB_RX_ASSOC_LIST;

//if (RCV_AL_Free_Count == 0) Macs_Flags.RCV_List_Full = TRUE;

/* 3.5.26 XMIT_Assoc_List
The Transmit Association List (XMIT_Assoc_List) is a list of Data Link Layer associations
generated by the transmission of Addressed Service frames (acknowledged or
unacknowledged). Each association in the list contains the following information:
    Destination Address: MAC Address (individual or group address) of the node to which
        the Addressed Service frame was sent.
    Destination House Code: House Code (system address or zone address) of the node
        to which the Addressed Service frame was sent.
    Sequence Number: One-bit sequence number transmitted to the destination node in
        the Control field of the frame. This sequence number must remain the same for all
        subsequent transmissions of the same frame to the same destination node. The
        sequence number must be toggled for transmission of a new frame to the same
        destination. If the association has “timed out” (and has been discarded) before a new
        frame is sent to the same destination, a new association must be established, and the
        value of the sequence number is arbitrary.
    Timer: The timer governs how long an association will be maintained in the list.
        Upon transmission of the last symbol of the frame, the timer is set to a value of
        1.5 x MAX_RETRANS_TIME = 1.125 seconds.
        (MAX_RETRANS_TIME = 0.75 seconds)
        If the same frame is retransmitted, the timer is reset to 1.125 seconds at the end of
        each transmission. In this manner, the association will be maintained by the
        transmitting node until 1.125 seconds has elapsed after the last transmission of the
        frame. Note that all transmissions of the same frame must take place within the time
        interval RETRANS_TIME (whose maximum value is MAX_RETRANS_TIME). These
        timing rules, in conjunction with the timing rules of the receiver, ensure that the
        transmitting node will always maintain an association longer than the receiving node.
        Once the timer expires, the association is discarded, and space in the list for a new
        association becomes available. The timer mechanism must have an accuracy of 10% .
        In an implementation, the same physical timer could be used to track the age of
        multiple associations in the list. However, such details are left to the implementor.
        This association information is maintained on a “per Source Address” basis. That is, a
        separate Transmit Association List must be maintained for each different Source
        Address/House Code used by the node. If only one Source Address/House Code is ever used,
        only one list is required.
        Support for the transmission of Addressed Service frames is optional. However, if the service
        is supported, the minimum size of the XMIT_Assoc_List one is association. Support for two or
        three associations is recommended. The maximum size of the list is not specified and
        remains the decision of the implementor.
        Once the XMIT_Assoc_List becomes full, the Data Link Layer variable XMIT_List_Full is set to
        TRUE, and the Data Link Layer cannot accept subsequent requests for Addressed Service
        transmission from the Network Layer. Once an association in the list “times out”, it is
        removed from the list, and XMIT_List_Full is set to FALSE. */

/* Upon reception of the first copy of a new frame, the timer is set to a value of
        1.25 x MAX_RETRANS_TIME = 0.9375 seconds. */
void CreateRxAsList(T_Assoc p_As_List, T_Frame p_Fr)
{
    p_As_List->SA  = p_Fr->SA;
    p_As_List->SHC = p_Fr->SHC;
    p_As_List->DA  = p_Fr->DA;
    p_As_List->DHC = p_Fr->DHC;
    p_As_List->Sequence_Number = p_Fr->CTL.Sequence_Nb;
    p_As_List->Timer = 9375; // Rx: set to 0.9375s
}

void CreateTxAsList(T_Assoc p_Tx_As_List, T_Frame Tx_Fr)
{
    p_Tx_As_List->DA = Tx_Fr->DA;
    p_Tx_As_List->DHC = Tx_Fr->DHC;
    p_Tx_As_List->Sequence_Number = Tx_Fr->CTL.Sequence_Nb;
    p_Tx_As_List->Timer = 11250; // Tx: set to 1.125s
}

/* 3.5.27 XMIT_List_Full
This variable is a boolean that when set to TRUE indicates that the Transmit Association List
(XMIT_Assoc_List) contains the maximum number of Addressed Service associations. This
maximum size is not specified and is implementation-specific.
However, a full XMIT_Assoc_List is a “Local_Busy” condition which is independent of any implementation.
This condition causes the Data Link Layer to reject any new requests from the Network Layer
for transmission of Addressed Service frames. Unaddressed frames (both acknowledged and
unacknowledged) may be transmitted by a Data Link Layer with a full XMIT_Assoc_List. The
default value of this variable is FALSE. */

T_Assoc             XMIT_Assoc_List[NB_TX_ASSOC_LIST];
int                 XMIT_AL_Free_Count = NB_TX_ASSOC_LIST;

//if (XMIT_AL_Free_Count == 0) Macs_Flags.XMIT_List_Full = TRUE;

void Association_List_Timer_Tick(void) // Each 0.0005s
{
    for (i = 0; i < NB_TX_ASSOC_LIST; i++)
    {
        Word time = XMIT_Assoc_List[i].Timer;
        if (time > 0) time -= 5;
        if (time <= 0)
        {
            time = 0;
            Macs_Flags.XMIT_List_Full = FALSE;
            if (XMIT_AL_Free_Count != NB_TX_ASSOC_LIST) XMIT_AL_Free_Count++;
        }
        XMIT_Assoc_List[i].Timer = time;
    }

    for (i = 0; i < NB_RX_ASSOC_LIST; i++)
    {
        Word time = RCV_Assoc_List[i].Timer;
        if (time > 0) time -= 5;
        if (time <= 0)
        {
            time = 0;
            Macs_Flags.RCV_List_Full = FALSE;
            if (RCV_AL_Free_Count != NB_RX_ASSOC_LIST) RCV_AL_Free_Count++;
        }
        RCV_Assoc_List[i].Timer = time;
    }
}

