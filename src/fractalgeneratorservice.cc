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
 * Copyright (C) 2002-2021 by Thomas Dreibholz
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

#include <complex>
#include <iostream>

#include "fractalgeneratorservice.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"


#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 303)
#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif


// ###### Constructor #######################################################
FractalGeneratorServer::FractalGeneratorServer(
   int                                                     rserpoolSocketDescriptor,
   FractalGeneratorServer::FractalGeneratorServerSettings* settings)
   : TCPLikeServer(rserpoolSocketDescriptor)
{
   Settings                   = *settings;
   LastSendTimeStamp          = 0;
   LastCookieTimeStamp        = 0;
   DataPacketsAfterLastCookie = 0;
}


// ###### Destructor ########################################################
FractalGeneratorServer::~FractalGeneratorServer()
{
}


// ###### Create a FractalGenerator thread ##################################
TCPLikeServer* FractalGeneratorServer::fractalGeneratorServerFactory(int sd, void* userData, const uint32_t peIdentifier)
{
   return(new FractalGeneratorServer(sd, (FractalGeneratorServerSettings*)userData));
}


// ###### Print parameters ##################################################
void FractalGeneratorServer::fractalGeneratorPrintParameters(const void* userData)
{
   const FractalGeneratorServerSettings* settings = (const FractalGeneratorServerSettings*)userData;

   puts("Fractal Generator Parameters:");
   printf("   Transmit Timeout        = %ums\n", (unsigned int)settings->TransmitTimeout);
   printf("   Data Max Time           = %ums\n", (unsigned int)(settings->DataMaxTime / 1000ULL));
   printf("   Cookie Max Time         = %ums\n", (unsigned int)(settings->CookieMaxTime / 1000ULL));
   printf("   Cookie Max Packets      = %u Packets\n", (unsigned int)settings->CookieMaxPackets);
   printf("   Failure After           = %u Packets\n", (unsigned int)settings->FailureAfter);
   printf("   Test Mode               = %s\n", (settings->TestMode == true) ? "on" : "off");
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


// ###### Send cookie #######################################################
bool FractalGeneratorServer::sendCookie(const unsigned long long now)
{
   FGPCookie cookie;
   ssize_t   sent;

   memcpy((char*)&cookie.ID, FGP_COOKIE_ID, sizeof(cookie.ID));
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
                          (unsigned char*)&cookie, sizeof(cookie), 0,
                          Settings.TransmitTimeout);

   LastCookieTimeStamp        = now;
   DataPacketsAfterLastCookie = 0;

   if(unlikely(sent != sizeof(cookie))) {
      printTimeStamp(stdout);
      printf("Sending cookie on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   return(true);
}


// ###### Send data #########################################################
bool FractalGeneratorServer::sendData(const unsigned long long now)
{
   const size_t dataSize = getFGPDataSize(Data.Points);
   ssize_t      sent;

   Data.Header.Type   = FGPT_DATA;
   Data.Header.Flags  = 0x00;
   Data.Header.Length = htons(dataSize);
   Data.Points        = htonl(Data.Points);
   Data.StartX        = htonl(Data.StartX);
   Data.StartY        = htonl(Data.StartY);

   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                      &Data, dataSize, 0,
                      0, htonl(PPID_FGP), 0, 0, 0,
                      Settings.TransmitTimeout);

   DataPackets++;
   DataPacketsAfterLastCookie++;
   Data.Points        = 0;
   Data.StartX        = 0;
   Data.StartY        = 0;
   LastSendTimeStamp  = now;

   if(unlikely(sent != (ssize_t)dataSize)) {
      printTimeStamp(stdout);
      printf("Sending Data on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   return(true);
}


// ###### Set point and advance x position ##################################
EventHandlingResult FractalGeneratorServer::advanceX(const unsigned int color)
{
   if(Data.Points == 0) {
      Data.StartX = Status.CurrentX;
      Data.StartY = Status.CurrentY;
   }
   Data.Buffer[Data.Points] = htonl(color);
   Data.Points++;

   // ====== Send Data ======================================================
   if(unlikely(Data.Points >= FGD_MAX_POINTS)) {
      const unsigned long long now = getMicroTime();

      if(unlikely(!sendData(now))) {
         return(EHR_Abort);
      }

      // ====== Failure more ================================================
      if(unlikely((Settings.FailureAfter > 0) && (DataPackets >= Settings.FailureAfter))) {
         sendCookie(now);
         printTimeStamp(stdout);
         printf("Failure Tester on RSerPool socket %u -> Disconnecting after %u packets!\n",
                RSerPoolSocketDescriptor,
                (unsigned int)DataPackets);
         return(EHR_Shutdown);
      }

      // ====== Schedule cookie/data timer ==================================
      scheduleTimer();
   }


   // ====== Send Data/Cookie due to expired timer ==========================
   lock();
   const bool alerted = Alert;
   Alert = false;
   unlock();
   if(unlikely(alerted)) {
      const unsigned long long now = getMicroTime();

      // ====== Send Data ===================================================
      if(unlikely(LastSendTimeStamp + 1000000 < now)) {
         if(unlikely(!sendData(now))) {
            return(EHR_Abort);
         }
      }

      // ====== Send cookie =================================================
      if( (DataPacketsAfterLastCookie >= Settings.CookieMaxPackets) ||
          (LastCookieTimeStamp + Settings.CookieMaxTime < now) ) {
         if(unlikely(!sendCookie(now))) {
            return(EHR_Abort);
         }
      }

      // ====== Schedule cookie/data timer ==================================
      scheduleTimer();
   }


   // ====== Shutdown ====================================================
   if(unlikely(isShuttingDown())) {
      printf("Aborting session on RSerPool socket %u due to server shutdown!\n",
             RSerPoolSocketDescriptor);
      const unsigned long long now = getMicroTime();
      if(sendData(now)) {
         sendCookie(now);
      }
      return(EHR_Abort);
   }

   Status.CurrentX++;
   return(EHR_Okay);
}


// ###### Advance y position ################################################
void FractalGeneratorServer::advanceY()
{
   Status.CurrentY++;
   Status.CurrentX = 0;
}


// ###### Schedule timer ####################################################
void FractalGeneratorServer::scheduleTimer()
{
   const unsigned long long nextTimerTimeStamp =
      std::min(LastCookieTimeStamp + Settings.CookieMaxTime,
               LastSendTimeStamp + Settings.DataMaxTime);
   setAsyncTimer(nextTimerTimeStamp);
}


// ###### Handle cookie/transmission timer ##################################
void FractalGeneratorServer::asyncTimerEvent(const unsigned long long now)
{
   lock();
   Alert = true;
   unlock();
}


// ###### Calculate image ###################################################
EventHandlingResult FractalGeneratorServer::calculateImage()
{
   // ====== Update load ====================================================
   setLoad(1.0 / ServerList->getMaxThreads());

   // ====== Initialize =====================================================
   EventHandlingResult  result = EHR_Okay;
   double               stepX;
   double               stepY;
   std::complex<double> z;
   size_t               i;
   const unsigned int   algorithm = (Settings.TestMode) ? 0 : Status.Parameter.AlgorithmID;

   lock();
   Alert = false;
   unlock();
   DataPackets = 0;
   Data.Points = 0;
   const double c1real = networkToDouble(Status.Parameter.C1Real);
   const double c1imag = networkToDouble(Status.Parameter.C1Imag);
   const double c2real = networkToDouble(Status.Parameter.C2Real);
   const double c2imag = networkToDouble(Status.Parameter.C2Imag);
   const double n      = networkToDouble(Status.Parameter.N);
   stepX = (c2real - c1real) / Status.Parameter.Width;
   stepY = (c2imag - c1imag) / Status.Parameter.Height;
   LastCookieTimeStamp = LastSendTimeStamp = getMicroTime();

   if(!sendCookie(LastCookieTimeStamp)) {
      printTimeStamp(stdout);
      printf("Sending cookie (start) on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   scheduleTimer();


   // ====== Algorithms =====================================================
   switch(algorithm) {

      // ====== Mandelbrot ==================================================
      case FGPA_MANDELBROT:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               // ====== Calculate pixel ====================================
               const std::complex<double> c =
                  std::complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                       c1imag + ((double)Status.CurrentY * stepY));

               z = std::complex<double>(0.0, 0.0);
               for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                  z = z*z - c;
                  if(unlikely( (z.real() * z.real() + z.imag() * z.imag()) >= 2.0) ) {
                     break;
                  }
               }

               result = advanceX(i);
               if(unlikely(result != EHR_Okay)) {
                  goto finished;
               }
            }
            advanceY();
         }
        break;

      // ====== MandelbrotN =================================================
      case FGPA_MANDELBROTN:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               // ====== Calculate pixel ====================================
               const std::complex<double> c =
                  std::complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                       c1imag + ((double)Status.CurrentY * stepY));

               z = std::complex<double>(0.0, 0.0);
               for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                  z = pow(z, std::complex<double>(n)) - c;
                  if(unlikely( (z.real() * z.real() + z.imag() * z.imag()) >= 2.0) ) {
                     break;
                  }
               }

               result = advanceX(i);
               if(unlikely(result != EHR_Okay)) {
                  goto finished;
               }
            }
            advanceY();
         }
        break;

      // ====== Test Mode ===================================================
      default:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               result = advanceX((Status.CurrentX * Status.CurrentY) % 256);
               if(unlikely(result != EHR_Okay)) {
                  goto finished;
               }
            }
            advanceY();
         }
        break;

   }


   // ====== Send last Data packet ==========================================
   if(result == EHR_Okay) {
      const unsigned long long now = getMicroTime();
      if(Data.Points > 0) {
         if(unlikely(!sendData(now))) {
            return(EHR_Abort);
         }
      }

      Data.StartX = 0xffffffff;
      Data.StartY = 0xffffffff;
      if(unlikely(!sendData(now))) {
         return(EHR_Abort);
      }
   }

finished:
   setLoad(0.0);
   return(result);
}
