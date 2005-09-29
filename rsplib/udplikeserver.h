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

#ifndef UDPLIKESERVER_H
#define UDPLIKESERVER_H

#include "rserpool.h"
#include "cpprspserver.h"


struct TagItem;

class UDPLikeServer
{
   public:
   UDPLikeServer();
   virtual ~UDPLikeServer();

   virtual void poolElement(const char*          programTitle,
                            const char*          poolHandle,
                            struct rsp_info*     info,
                            struct rsp_loadinfo* loadinfo,
                            struct TagItem*      tags = NULL);

   protected:
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID) = 0;
   virtual EventHandlingResult handleCookieEcho(rserpool_session_t sessionID,
                                                const char*        buffer,
                                                size_t             bufferSize);
   virtual EventHandlingResult handleNotification(const union rserpool_notification* notification);

   protected:
   int RSerPoolSocketDescriptor;
};


#endif
