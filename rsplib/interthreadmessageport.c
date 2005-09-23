/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "tdtypes.h"
#include "interthreadmessageport.h"


/* ###### Constructor #################################################### */
void interThreadMessagePortNew(struct InterThreadMessagePort* itmPort)
{
   threadSignalNew(&itmPort->Signal);
   doubleLinkedRingListNew(&itmPort->Queue);
}


/* ###### Destructor ##################################################### */
void interThreadMessagePortDelete(struct InterThreadMessagePort* itmPort)
{
   CHECK(itmPort->Queue.Node.Next == itmPort->Queue.Head);
   doubleLinkedRingListDelete(&itmPort->Queue);
   threadSignalDelete(&itmPort->Signal);
}


/* ###### Lock ########################################################### */
void interThreadMessagePortLock(struct InterThreadMessagePort* itmPort)
{
    threadSignalLock(&itmPort->Signal);
}


/* ###### Unlock ######################################################### */
void interThreadMessagePortUnlock(struct InterThreadMessagePort* itmPort)
{
   threadSignalUnlock(&itmPort->Signal);
}


/* ###### Get first message ############################################## */
struct InterThreadMessageNode* interThreadMessagePortGetFirstMessage(struct InterThreadMessagePort* itmPort)
{
   struct DoubleLinkedRingListNode* node;
   node = itmPort->Queue.Node.Next;
   if(node != itmPort->Queue.Head) {
      return((struct InterThreadMessageNode*)node);
   }
   return(NULL);
}


/* ###### Get next message ############################################### */
struct InterThreadMessageNode* interThreadMessagePortGetNextMessage(struct InterThreadMessagePort* itmPort,
                                                                    struct InterThreadMessageNode*     message)
{
   struct DoubleLinkedRingListNode* node;
   node = message->Node.Next;
   if(node != itmPort->Queue.Head) {
      return((struct InterThreadMessageNode*)node);
   }
   return(NULL);
}


/* ###### Remove message ################################################# */
void interThreadMessagePortRemoveMessage(struct InterThreadMessagePort* itmPort,
                                         struct InterThreadMessageNode* message)
{
   doubleLinkedRingListRemNode(&message->Node);
   doubleLinkedRingListNodeDelete(&message->Node);
}


/* ###### Enqueue message ################################################ */
void interThreadMessagePortEnqueue(struct InterThreadMessagePort* itmPort,
                                   struct InterThreadMessageNode* message,
                                   struct InterThreadMessagePort* replyPort)
{
   doubleLinkedRingListNodeNew(&message->Node);
   message->ReplyPort = replyPort;

   threadSignalLock(&itmPort->Signal);
   doubleLinkedRingListAddTail(&itmPort->Queue, &message->Node);
   threadSignalFire(&itmPort->Signal);
   threadSignalUnlock(&itmPort->Signal);
}


/* ###### Dequeue message ################################################ */
struct InterThreadMessageNode* interThreadMessagePortDequeue(struct InterThreadMessagePort* itmPort)
{
   struct InterThreadMessageNode* message;
   threadSignalLock(&itmPort->Signal);
   message = interThreadMessagePortGetFirstMessage(itmPort);
   if(message) {
      doubleLinkedRingListRemNode(&message->Node);
      doubleLinkedRingListNodeDelete(&message->Node);
   }
   threadSignalUnlock(&itmPort->Signal);
   return(message);
}


/* ###### Wait for message ############################################### */
void interThreadMessagePortWait(struct InterThreadMessagePort* itmPort)
{
   threadSignalLock(&itmPort->Signal);
   while(interThreadMessagePortGetFirstMessage(itmPort) == NULL) {
      threadSignalWait(&itmPort->Signal);
   }
   threadSignalUnlock(&itmPort->Signal);
}


/* ###### Reply message ################################################## */
void interThreadMessageReply(struct InterThreadMessageNode* message)
{
   CHECK(message->ReplyPort != NULL);
   interThreadMessagePortEnqueue(message->ReplyPort, message, NULL);
}
