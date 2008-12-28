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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
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

#ifndef SESSIONCONTROL_H
#define SESSIONCONTROL_H

#include "tdtypes.h"
#include "session.h"
#include "rserpoolsocket.h"

#ifdef __cplusplus
extern "C" {
#endif


struct Session* addSession(struct RSerPoolSocket* rserpoolSocket,
                           const sctp_assoc_t     assocID,
                           const bool             isIncoming,
                           const unsigned char*   poolHandle,
                           const size_t           poolHandleSize,
                           struct TagItem*        tags);
void deleteSession(struct RSerPoolSocket* rserpoolSocket,
                   struct Session*        session);
struct Session* findSession(struct RSerPoolSocket* rserpoolSocket,
                            rserpool_session_t     sessionID,
                            sctp_assoc_t           assocID);


ssize_t getCookieEchoOrNotification(struct RSerPoolSocket* rserpoolSocket,
                                    void*                  buffer,
                                    size_t                 bufferLength,
                                    struct rsp_sndrcvinfo* rinfo,
                                    int*                   msg_flags,
                                    const bool             isPreRead);

bool handleNotification(struct RSerPoolSocket*         rserpoolSocket,
                        const union sctp_notification* notification);
void handleControlChannelMessage(struct RSerPoolSocket* rserpoolSocket,
                                 const sctp_assoc_t     assocID,
                                 char*                  buffer,
                                 size_t                 bufferSize);
bool handleControlChannelAndNotifications(struct RSerPoolSocket* rserpoolSocket);

bool sendCookieEcho(struct RSerPoolSocket* rserpoolSocket,
                    struct Session*        session);


#ifdef __cplusplus
}
#endif

#endif
