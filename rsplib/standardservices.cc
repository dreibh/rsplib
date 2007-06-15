/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#include "standardservices.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"


/*
   ##########################################################################
   #### ECHO                                                             ####
   ##########################################################################
*/

// ###### Constructor #######################################################
EchoServer::EchoServer()
{
}


// ###### Destructor ########################################################
EchoServer::~EchoServer()
{
}


// ###### Handle message ####################################################
EventHandlingResult EchoServer::handleMessage(rserpool_session_t sessionID,
                                              const char*        buffer,
                                              size_t             bufferSize,
                                              uint32_t           ppid,
                                              uint16_t           streamID)
{
   printTimeStamp(stdout);
   printf("Echo %u bytes on session %u\n", (unsigned int)bufferSize, sessionID);

   ssize_t sent;
   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                      buffer, bufferSize, 0,
                      sessionID, ppid, streamID,
                      0, 0, 0);
   return((sent == (ssize_t)bufferSize) ? EHR_Okay : EHR_Abort);
}



/*
   ##########################################################################
   #### DISCARD                                                          ####
   ##########################################################################
*/

// ###### Constructor #######################################################
DiscardServer::DiscardServer()
{
}


// ###### Destructor ########################################################
DiscardServer::~DiscardServer()
{
}


// ###### Handle message ####################################################
EventHandlingResult DiscardServer::handleMessage(rserpool_session_t sessionID,
                                                 const char*        buffer,
                                                 size_t             bufferSize,
                                                 uint32_t           ppid,
                                                 uint16_t           streamID)
{
   printTimeStamp(stdout);
   printf("Discard %u bytes on session %u\n", (unsigned int)bufferSize, sessionID);
   return(EHR_Okay);
}



/*
   ##########################################################################
   #### DAYTIME                                                          ####
   ##########################################################################
*/

// ###### Constructor #######################################################
DaytimeServer::DaytimeServer()
{
}


// ###### Destructor ########################################################
DaytimeServer::~DaytimeServer()
{
}


// ###### Handle notification ###############################################
void DaytimeServer::handleNotification(const union rserpool_notification* notification)
{
   if((notification->rn_header.rn_type == RSERPOOL_SESSION_CHANGE) &&
      (notification->rn_session_change.rsc_state == RSERPOOL_SESSION_ADD)) {
      char                     daytime[128];
      char                     microseconds[64];
      const unsigned long long microTime = getMicroTime();
      const time_t             timeStamp = microTime / 1000000;
      const struct tm*         timeptr   = localtime(&timeStamp);
      strftime((char*)&daytime, sizeof(daytime), "%A, %d-%B-%Y %H:%M:%S", timeptr);
      snprintf((char*)&microseconds, sizeof(microseconds), ".%06d\n",
               (unsigned int)(microTime % 1000000));
      safestrcat((char*)&daytime, microseconds, sizeof(daytime));

      ssize_t sent;
      sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                         (char*)&daytime, strlen(daytime), 0,
                         notification->rn_session_change.rsc_session,
                         0x00000000, 0,
                         0, 0, 0);
      rsp_sendmsg(RSerPoolSocketDescriptor,
                  NULL, 0, 0,
                  notification->rn_session_change.rsc_session,
                  0x00000000, 0,
                  0, SCTP_EOF, 0);
   }
}



/*
   ##########################################################################
   #### CHARACTER GENERATOR                                              ####
   ##########################################################################
*/

// ###### Constructor #######################################################
CharGenServer::CharGenServer(int rserpoolSocketDescriptor)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
}


// ###### Destructor ########################################################
CharGenServer::~CharGenServer()
{
}


// ###### Handle notification ###############################################
EventHandlingResult CharGenServer::initializeSession()
{
   unsigned char buffer[1024];
   size_t        i;

   for(i = 0;i < sizeof(buffer);i++) {
      buffer[i] = (unsigned char)(30 + (i % (128 - 30)));
   }
   while(rsp_sendmsg(RSerPoolSocketDescriptor,
                     (char*)&buffer, sizeof(buffer), 0,
                     0, 0, 0, 0, 0, 3600000000ULL) > 0) {
      // puts("sent data!");
   }

   return(EHR_Shutdown);
}


// ###### Create a CharGenServer thread #####################################
TCPLikeServer* CharGenServer::charGenServerFactory(int sd, void* userData)
{
   return(new CharGenServer(sd));
}



/*
   ##########################################################################
   #### PING PONG                                                        ####
   ##########################################################################
*/

#include "pingpongpackets.h"


// ###### Constructor #######################################################
PingPongServer::PingPongServer(int rserpoolSocketDescriptor,
                               PingPongServer::PingPongServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings = *settings;
   Replies  = 0;
   ReplyNo  = 1;
}


// ###### Destructor ########################################################
PingPongServer::~PingPongServer()
{
}


// ###### Create a PingServer thread ########################################
TCPLikeServer* PingPongServer::pingPongServerFactory(int sd, void* userData)
{
   return(new PingPongServer(sd, (PingPongServer::PingPongServerSettings*)userData));
}


// ###### Handle cookie echo ################################################
EventHandlingResult PingPongServer::handleCookieEcho(const char* buffer,
                                                     size_t      bufferSize)
{
   const struct PPPCookie* cookie = (const struct PPPCookie*)buffer;
   if( (bufferSize != sizeof(PPPCookie)) ||
       (strncmp((const char*)&cookie->ID, PPP_COOKIE_ID, sizeof(cookie->ID))) ) {
      puts("Received bad cookie echo!");
      return(EHR_Abort);
   }
   ReplyNo = ntoh64(cookie->ReplyNo);
   return(EHR_Okay);
}


// ###### Handle message ####################################################
EventHandlingResult PingPongServer::handleMessage(const char* buffer,
                                                  size_t      bufferSize,
                                                  uint32_t    ppid,
                                                  uint16_t    streamID)
{
   ssize_t sent;

   if(bufferSize >= (ssize_t)sizeof(PingPongCommonHeader)) {
      const Ping* ping = (const Ping*)buffer;
      if(ping->Header.Type == PPPT_PING) {
         if(ntohs(ping->Header.Length) >= (ssize_t)sizeof(struct Ping)) {
            /* ====== Answer Ping by Pong ================================ */
            size_t dataLength = ntohs(ping->Header.Length) - sizeof(Ping);
            char pongBuffer[sizeof(struct Pong) + dataLength];
            Pong* pong = (Pong*)&pongBuffer;

            pong->Header.Type   = PPPT_PONG;
            pong->Header.Flags  = 0x00;
            pong->Header.Length = htons(sizeof(Pong) + dataLength);
            pong->MessageNo     = ping->MessageNo;
            pong->ReplyNo       = hton64(ReplyNo);
            memcpy(&pong->Data, &ping->Data, dataLength);

            sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                               (char*)pong, sizeof(Pong) + dataLength, 0,
                               0, htonl(PPID_PPP), 0, 0, 0, 0);
            if(sent > 0) {
               ReplyNo++;
               Replies++;

               /* ====== Send cookie ===================================== */
               struct PPPCookie cookie;
               strncpy((char*)&cookie.ID, PPP_COOKIE_ID, sizeof(cookie.ID));
               cookie.ReplyNo = hton64(ReplyNo);
               sent = rsp_send_cookie(RSerPoolSocketDescriptor,
                                      (unsigned char*)&cookie, sizeof(cookie),
                                      0, 0);
               if(sent > 0) {
                  /* ====== Failure tester =============================== */
                  if((Settings.FailureAfter > 0) && (Replies >= Settings.FailureAfter)) {
                     printTimeStamp(stdout);
                     printf("Failure Tester on RSerPool socket %u -> Disconnecting after %u replies!\n",
                            RSerPoolSocketDescriptor,
                            (unsigned int)Replies);
                     return(EHR_Abort);
                  }
                  return(EHR_Okay);
               }
            }
         }
      }
   }
   return(EHR_Abort);
}
