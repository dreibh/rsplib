/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2019 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef RSERPOOLSOCKET_H
#define RSERPOOLSOCKET_H

#include "tdtypes.h"
#include "rserpool-internals.h"
#include "poolhandle.h"
#include "identifierbitmap.h"
#include "sessionstorage.h"
#include "notificationqueue.h"
#include "simpleredblacktree.h"
#include "threadsafety.h"
#include "netutilities.h"
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
   bool                InDaemonMode;
};

struct RSerPoolSocket
{
   struct SimpleRedBlackTreeNode Node;

   int                           Descriptor;
   struct ThreadSafety           Mutex;

   int                           SocketDomain;
   int                           SocketType;
   int                           SocketProtocol;
   int                           Socket;
   struct MessageBuffer*         MsgBuffer;

   struct PoolElement*           PoolElement;        /* PE mode                      */
   struct Session*               ConnectedSession;   /* TCP-like PU mode             */
   struct SessionStorage         SessionSet;         /* UDP-like PU mode and PE mode */
   struct TuneSCTPParameters     AssocParameters;
   struct ThreadSafety           SessionSetMutex;
   bool                          WaitingForFirstMsg;

   struct NotificationQueue      Notifications;

   struct IdentifierBitmap*      SessionAllocationBitmap;
   char*                         MessageBuffer;
};

#define RSERPOOL_MESSAGE_BUFFER_SIZE 65536


void rserpoolSocketPrint(const void* node, FILE* fd);
int rserpoolSocketComparison(const void* node1, const void* node2);

struct RSerPoolSocket* getRSerPoolSocketForDescriptor(int sd);
bool waitForRead(struct RSerPoolSocket* rserpoolSocket,
                 int                    timeout);
void deletePoolElement(struct PoolElement* poolElement,
                       int                 flags,
                       struct TagItem*     tags);
void reregistrationTimer(struct Dispatcher* dispatcher,
                         struct Timer*      timer,
                         void*              userData);
bool doRegistration(struct RSerPoolSocket* rserpoolSocket,
                    bool                   waitForRegistrationResult);


#ifdef __cplusplus
}
#endif

#endif
