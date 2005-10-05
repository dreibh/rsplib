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
                      0, 0);
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
                         (char*)&daytime, strlen(daytime), SCTP_EOF,
                         notification->rn_session_change.rsc_session,
                         0x00000000, 0,
                         0, 0);
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
EventHandlingResult CharGenServer::initialize()
{
   unsigned char buffer[1024];
   size_t        i;

   for(i = 0;i < sizeof(buffer);i++) {
      buffer[i] = (unsigned char)(30 + (i % (256 - 30)));
   }
   while(rsp_sendmsg(RSerPoolSocketDescriptor,
                     (char*)&buffer, sizeof(buffer), 0,
                     0, 0, 0, 0, 3600000000ULL) > 0) {
      puts("sent data!");
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
                               0, htonl(PPID_PPP), 0, 0, 0);
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



/*
   ##########################################################################
   #### FRACTAL GENERATOR                                                ####
   ##########################################################################
*/

#include "fractalgeneratorpackets.h"

#include <complex>


// ###### Constructor #######################################################
FractalGeneratorServer::FractalGeneratorServer(int rserpoolSocketDescriptor,
                                               FractalGeneratorServer::FractalGeneratorServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings            = *settings;
   LastSendTimeStamp   = 0;
   LastCookieTimeStamp = 0;
}


// ###### Destructor ########################################################
FractalGeneratorServer::~FractalGeneratorServer()
{
}


// ###### Create a FractalGenerator thread ##################################
TCPLikeServer* FractalGeneratorServer::fractalGeneratorServerFactory(int sd, void* userData)
{
   return(new FractalGeneratorServer(sd, (FractalGeneratorServerSettings*)userData));
}


// ###### Send cookie #######################################################
bool FractalGeneratorServer::sendCookie()
{
   FGPCookie cookie;
   ssize_t   sent;

   strncpy((char*)&cookie.ID, FGP_COOKIE_ID, sizeof(cookie.ID));
   cookie.Parameter.Header.Type   = FGPT_PARAMETER;
   cookie.Parameter.Header.Flags  = 0x00;
   cookie.Parameter.Header.Length = htons(sizeof(cookie.Parameter.Header));
   cookie.Parameter.Width         = htonl(Status.Parameter.Width);
   cookie.Parameter.Height        = htonl(Status.Parameter.Height);
   cookie.Parameter.MaxIterations = htonl(Status.Parameter.MaxIterations);
   cookie.Parameter.AlgorithmID   = htonl(Status.Parameter.AlgorithmID);
   cookie.Parameter.C1Real        = Status.Parameter.C1Real;
   cookie.Parameter.C2Real        = Status.Parameter.C2Real;
   cookie.Parameter.C1Imag        = Status.Parameter.C1Imag;
   cookie.Parameter.C2Imag        = Status.Parameter.C2Imag;
   cookie.Parameter.N             = Status.Parameter.N;
   cookie.CurrentX                = htonl(Status.CurrentX);
   cookie.CurrentY                = htonl(Status.CurrentY);

   sent = rsp_send_cookie(RSerPoolSocketDescriptor,
                          (unsigned char*)&cookie, sizeof(cookie),
                          0, 0);

   LastCookieTimeStamp = getMicroTime();
   return(sent == sizeof(cookie));
}


// ###### Send data #########################################################
bool FractalGeneratorServer::sendData(FGPData* data)
{
   const size_t dataSize = getFGPDataSize(data->Points);
   ssize_t      sent;

   for(size_t i = 0;i < data->Points;i++) {
      data->Buffer[i] = htonl(data->Buffer[i]);
   }
   data->Header.Type   = FGPT_DATA;
   data->Header.Flags  = 0x00;
   data->Header.Length = htons(dataSize);
   data->Points = htonl(data->Points);
   data->StartX = htonl(data->StartX);
   data->StartY = htonl(data->StartY);

   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                        data, dataSize, 0,
                        0, htonl(PPID_FGP), 0, 0, 0);

   data->Points = 0;
   data->StartX = 0;
   data->StartY = 0;
   LastSendTimeStamp = getMicroTime();

   return(sent == (ssize_t)dataSize);
}


// ###### Handle parameter message ##########################################
bool FractalGeneratorServer::handleParameter(const FGPParameter* parameter,
                                             const size_t        size,
                                             const bool          insideCookie)
{
   if(size < sizeof(struct FGPParameter)) {
      printTimeStamp(stdout);
      printf("Received too short message on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   if(parameter->Header.Type != FGPT_PARAMETER) {
      printTimeStamp(stdout);
      printf("Received unknown message type %u on RSerPool socket %u\n",
             parameter->Header.Type,
             RSerPoolSocketDescriptor);
      return(false);
   }
   Status.Parameter.Width         = ntohl(parameter->Width);
   Status.Parameter.Height        = ntohl(parameter->Height);
   Status.Parameter.C1Real        = parameter->C1Real;
   Status.Parameter.C2Real        = parameter->C2Real;
   Status.Parameter.C1Imag        = parameter->C1Imag;
   Status.Parameter.C2Imag        = parameter->C2Imag;
   Status.Parameter.N             = parameter->N;
   Status.Parameter.MaxIterations = ntohl(parameter->MaxIterations);
   Status.Parameter.AlgorithmID   = ntohl(parameter->AlgorithmID);
   Status.CurrentX                = 0;
   Status.CurrentY                = 0;

   if(!insideCookie) {
      printTimeStamp(stdout);
      printf("Got Parameter on RSerPool socket %u:\nw=%u h=%u c1=(%lf,%lf) c2=(%lf,%lf), n=%f, maxIterations=%u, algorithmID=%u\n",
            RSerPoolSocketDescriptor,
            Status.Parameter.Width,
            Status.Parameter.Height,
            networkToDouble(Status.Parameter.C1Real),
            networkToDouble(Status.Parameter.C1Imag),
            networkToDouble(Status.Parameter.C2Real),
            networkToDouble(Status.Parameter.C2Imag),
            networkToDouble(Status.Parameter.N),
            Status.Parameter.MaxIterations,
            Status.Parameter.AlgorithmID);
   }

   if((Status.Parameter.Width > 65536)  ||
      (Status.Parameter.Height > 65536) ||
      (Status.Parameter.MaxIterations > 1000000)) {
      puts("Bad parameters!");
      return(false);
   }
   return(true);
}


// ###### Handle cookie echo ################################################
EventHandlingResult FractalGeneratorServer::handleCookieEcho(const char* buffer,
                                                             size_t      bufferSize)
{
   const struct FGPCookie* cookie = (const struct FGPCookie*)buffer;

   if(bufferSize != sizeof(struct FGPCookie)) {
      printTimeStamp(stdout);
      printf("Invalid size of cookie on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   if(strncmp(cookie->ID, FGP_COOKIE_ID, sizeof(cookie->ID))) {
      printTimeStamp(stdout);
      printf("Invalid cookie ID on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   if(!handleParameter(&cookie->Parameter, sizeof(cookie->Parameter), true)) {
      return(EHR_Abort);
   }

   Status.CurrentX = ntohl(cookie->CurrentX);
   Status.CurrentY = ntohl(cookie->CurrentY);
   printTimeStamp(stdout);
   printf("Got CookieEcho on RSerPool socket %u:\nx=%u y=%u\n",
          RSerPoolSocketDescriptor,
          Status.CurrentX, Status.CurrentY);

   return(calculateImage());
}


// ###### Handle message ####################################################
EventHandlingResult FractalGeneratorServer::handleMessage(const char* buffer,
                                                          size_t      bufferSize,
                                                          uint32_t    ppid,
                                                          uint16_t    streamID)
{
   if(!handleParameter((const FGPParameter*)buffer, bufferSize)) {
      return(EHR_Abort);
   }
   return(calculateImage());
}


// ###### Calculate image ####################################################
EventHandlingResult FractalGeneratorServer::calculateImage()
{
   // ====== Initialize =====================================================
   struct FGPData       data;
   double               stepX;
   double               stepY;
   std::complex<double> z;
   size_t               i;
   size_t               dataPackets = 0;

   data.Points = 0;
   const double c1real = networkToDouble(Status.Parameter.C1Real);
   const double c1imag = networkToDouble(Status.Parameter.C1Imag);
   const double c2real = networkToDouble(Status.Parameter.C2Real);
   const double c2imag = networkToDouble(Status.Parameter.C2Imag);
   const double n      = networkToDouble(Status.Parameter.N);
   stepX = (c2real - c1real) / Status.Parameter.Width;
   stepY = (c2imag - c1imag) / Status.Parameter.Height;
   LastCookieTimeStamp = LastSendTimeStamp = getMicroTime();

   if(!sendCookie()) {
      printTimeStamp(stdout);
      printf("Sending cookie (start) on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   // ====== Calculate image ================================================
   setLoad(1.0);
   while(Status.CurrentY < Status.Parameter.Height) {
      while(Status.CurrentX < Status.Parameter.Width) {
         // ====== Calculate pixel ==========================================
         if(!Settings.TestMode) {
            const std::complex<double> c =
               std::complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                    c1imag + ((double)Status.CurrentY * stepY));

            // ====== Algorithms ============================================
            switch(Status.Parameter.AlgorithmID) {
               case FGPA_MANDELBROT:
                  z = std::complex<double>(0.0, 0.0);
                  for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                     z = z*z - c;
                     if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                        break;
                     }
                  }
                break;
               case FGPA_MANDELBROTN:
                  z = std::complex<double>(0.0, 0.0);
                  for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                     z = pow(z, (int)rint(n)) - c;
                     if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                        break;
                     }
                  }
                break;
               default:
                  i = 0;
                break;
            }
         }
         else {
            i = (Status.CurrentX * Status.CurrentY) % 256;
         }
         // =================================================================

         if(data.Points == 0) {
            data.StartX = Status.CurrentX;
            data.StartY = Status.CurrentY;
         }
         data.Buffer[data.Points] = i;
         data.Points++;

         // ====== Send data ================================================
         if( (data.Points >= FGD_MAX_POINTS) ||
            (LastSendTimeStamp + 1000000 < getMicroTime()) ) {
            if(!sendData(&data)) {
               printTimeStamp(stdout);
               printf("Sending data on RSerPool socket %u failed!\n",
                      RSerPoolSocketDescriptor);
               return(EHR_Abort);
            }

            dataPackets++;
            if((Settings.FailureAfter > 0) && (dataPackets >= Settings.FailureAfter)) {
               sendCookie();
               printTimeStamp(stdout);
               printf("Failure Tester on RSerPool socket %u -> Disconnecting after %u packets!\n",
                      RSerPoolSocketDescriptor,
                      (unsigned int)dataPackets);
               return(EHR_Abort);
            }
         }

         Status.CurrentX++;
      }
      Status.CurrentX = 0;
      Status.CurrentY++;

      // ====== Send cookie =================================================
      if(LastCookieTimeStamp + 1000000 < getMicroTime()) {
         if(!sendCookie()) {
            printTimeStamp(stdout);
            printf("Sending cookie (start) on RSerPool socket %u failed!\n",
                  RSerPoolSocketDescriptor);
            return(EHR_Abort);
         }
      }
      if(isShuttingDown()) {
         printf("Aborting session on RSerPool socket %u due to server shutdown!\n",
               RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
   }

   if(data.Points > 0) {
      if(!sendData(&data)) {
         printf("Sending data (last block) on RSerPool socket %u failed!\n",
                  RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
   }

   data.StartX = 0xffffffff;
   data.StartY = 0xffffffff;
   if(!sendData(&data)) {
      printf("Sending data (end of data) on RSerPool socket %u failed!\n",
               RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   setLoad(0.0);
   return(EHR_Okay);
}
