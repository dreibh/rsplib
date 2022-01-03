/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2022 by Thomas Dreibholz
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

#ifndef SESSION_H
#define SESSION_H

#include "tdtypes.h"
#include "rserpool.h"
#include "poolhandle.h"
#include "tagitem.h"
#include "threadsafety.h"
#include "simpleredblacktree.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_STATUS_MAXLEN 128


#ifdef ENABLE_CSP
/*
 * The status information needs to be accessed in getSessionStatus() for all
 * sockets. It is necessary to keep a copy here, protected by a mutex. Then,
 * getSessionStatus() can avoid locking *all* sockets and session!
 */
struct SessionStatus
{
   struct ThreadSafety   Mutex;

   int                   Socket;
   sctp_assoc_t          AssocID;
   uint32_t              PPID;

   unsigned long long    ConnectionTimeStamp;
   uint32_t              ConnectedPE;
   bool                  IsIncoming;
   bool                  IsFailed;

   char                  StatusText[SESSION_STATUS_MAXLEN];
};
#endif


struct Session
{
   struct SimpleRedBlackTreeNode SessionIDNode;
   struct SimpleRedBlackTreeNode AssocIDNode;

   sctp_assoc_t                  AssocID;
   rserpool_session_t            SessionID;
   uint32_t                      PPID;

   struct PoolHandle             Handle;
   uint32_t                      ConnectedPE;
   bool                          IsIncoming;
   bool                          IsFailed;

   void*                         Cookie;
   size_t                        CookieSize;
   void*                         CookieEcho;
   size_t                        CookieEchoSize;

   unsigned long long            ConnectTimeout;
   unsigned long long            HandleResolutionRetryDelay;

   struct TagItem*               Tags;
   char                          StatusText[SESSION_STATUS_MAXLEN];

#ifdef ENABLE_CSP
   struct SessionStatus          Status;
#endif
};


#ifdef __cplusplus
}
#endif

#endif
