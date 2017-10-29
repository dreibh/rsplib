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
 * Copyright (C) 2002-2018 by Thomas Dreibholz
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

#ifndef UDPLIKESERVER_H
#define UDPLIKESERVER_H

#include "rserpool-internals.h"
#include "cpprspserver.h"


struct TagItem;

class UDPLikeServer
{
   public:
   UDPLikeServer();
   virtual ~UDPLikeServer();

   virtual EventHandlingResult initializeService(int sd, const uint32_t peIdentifier);
   virtual void finishService(int sd, EventHandlingResult initializeResult);

   virtual void poolElement(const char*          programTitle,
                            const char*          poolHandle,
                            struct rsp_info*     info,
                            struct rsp_loadinfo* loadinfo,
                            const sockaddr*      localAddressSet = NULL,
                            const size_t         localAddresses  = 0,
                            unsigned int         reregInterval   = 30000,
                            unsigned int         runtimeLimit    = 0,
                            const bool           quiet           = false,
                            const bool           daemonMode      = false,
                            struct TagItem*      tags            = NULL);

   double getLoad() const;
   void setLoad(double load);

   protected:
   virtual bool waitForAction(unsigned long long timeout);
   virtual void printParameters();
   void startTimer(unsigned long long timeStamp);
   void stopTimer();

   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
   virtual EventHandlingResult handleCookieEcho(rserpool_session_t sessionID,
                                                const char*        buffer,
                                                size_t             bufferSize);
   virtual void handleNotification(const union rserpool_notification* notification);
   virtual void handleTimer();

   protected:
   int RSerPoolSocketDescriptor;

   private:
   unsigned long long NextTimerTimeStamp;
   unsigned int       Load;
};


#endif
