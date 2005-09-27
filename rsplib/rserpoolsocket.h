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

#ifndef RSERPOOLSOCKET_H
#define RSERPOOLSOCKET_H

#include "tdtypes.h"
#include "rserpool.h"
#include "poolhandle.h"
#include "identifierbitmap.h"
#include "sessionstorage.h"
#include "notificationqueue.h"
#include "leaflinkedredblacktree.h"
#include "threadsafety.h"
#include "timer.h"
#include "tagitem.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


struct PoolElement
{
   struct PoolHandle   Handle;
   uint32_t            Identifier;
   struct ThreadSafety Mutex;

   struct rsp_loadinfo LoadInfo;

   struct Timer        ReregistrationTimer;
   unsigned int        RegistrationLife;
   unsigned int        ReregistrationInterval;

   bool                HasControlChannel;
};

struct RSerPoolSocket
{
   struct LeafLinkedRedBlackTreeNode Node;

   int                               Descriptor;
   struct ThreadSafety               Mutex;

   int                               SocketDomain;
   int                               SocketType;
   int                               SocketProtocol;
   int                               Socket;

   struct PoolElement*               PoolElement;        /* PE mode                      */
   struct SessionStorage             SessionSet;         /* UDP-like PU mode and PE mode */
   struct Session*                   ConnectedSession;   /* TCP-like PU mode             */

   struct NotificationQueue          Notifications;

   struct IdentifierBitmap*          SessionAllocationBitmap;
   char*                             MessageBuffer;
};

#define RSERPOOL_MESSAGE_BUFFER_SIZE 65536


void rserpoolSocketPrint(const void* node, FILE* fd);
int rserpoolSocketComparison(const void* node1, const void* node2);

struct RSerPoolSocket* getRSerPoolSocketForDescriptor(int sd);
bool waitForRead(struct RSerPoolSocket* rserpoolSocket,
                 unsigned long long     timeout);
void deletePoolElement(struct PoolElement* poolElement, struct TagItem* tags);
void reregistrationTimer(struct Dispatcher* dispatcher,
                         struct Timer*      timer,
                         void*              userData);
bool doRegistration(struct RSerPoolSocket* rserpoolSocket,
                    bool                   waitForRegistrationResult);


#ifdef __cplusplus
}
#endif

#endif
