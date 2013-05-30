/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2013 by Thomas Dreibholz
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
#include "simpleredblacktree.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


struct Session
{
   struct SimpleRedBlackTreeNode SessionIDNode;
   struct SimpleRedBlackTreeNode AssocIDNode;

   sctp_assoc_t                  AssocID;
   rserpool_session_t            SessionID;

   struct PoolHandle             Handle;
   uint32_t                      ConnectedPE;
   bool                          IsIncoming;
   bool                          IsFailed;
   unsigned long long            ConnectionTimeStamp;

   void*                         Cookie;
   size_t                        CookieSize;
   void*                         CookieEcho;
   size_t                        CookieEchoSize;

   unsigned long long            ConnectTimeout;
   unsigned long long            HandleResolutionRetryDelay;

   struct TagItem*               Tags;

   char                          StatusText[128];
};


#ifdef __cplusplus
}
#endif

#endif
