/*
 * fractalpoolelement.cc: Fractal Generator PE Example
 * Copyright (C) 2003 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <complex>

#include "thread.h"
#include "mutex.h"

#include "rsplib.h"
#include "rsplib-tags.h"
#include "loglevel.h"
#include "timeutilities.h"
#include "netutilities.h"
#include "breakdetector.h"

#include "fractalgeneratorexample.h"


class ServiceThread : public TDThread
{
   public:
   ServiceThread();
   ~ServiceThread();

   inline bool hasFinished() {
      return(Session == NULL);
   }

   static void handleCookieEcho(void* userData, void* cookieData, const size_t cookieSize);

   private:
   void run();

   unsigned int       ID;
   bool               IsNewSession;

   public:
   SessionDescriptor* Session;
};


ServiceThread::ServiceThread()
{
   static unsigned int IDCounter = 0;
   ID           = ++IDCounter;
   Session      = NULL;
   IsNewSession = true;
   std::cout << "Created thread " << ID << "..." << std::endl;
}

ServiceThread::~ServiceThread()
{
   std::cout << "Stopping thread " << ID << "..." << std::endl;
   if(Session) {
      rspDeleteSession(Session);
      Session = NULL;
   }
   waitForFinish();
   std::cout << "Thread " << ID << " has been stopped." << std::endl;
}

void ServiceThread::handleCookieEcho(void* userData, void* cookieData, const size_t cookieSize)
{
   ServiceThread*          serviceThread = (ServiceThread*)userData;
   FractalGeneratorCookie* cookie        = (FractalGeneratorCookie*)cookieData;

   if(serviceThread->IsNewSession) {
      if( (cookieSize >= sizeof(struct FractalGeneratorCookie)) &&
          (!(strncmp(cookie->ID, FG_COOKIE_ID, sizeof(FG_COOKIE_ID)))) ) {
         /* Here should be *at least* checks for a Message Authentication Code
            (MAC), timestamp (out-of-date cookie, e.g. a replay attack) and
            validity and plausibility of the values set from the cookie! */

      }
      else {
         puts("Cookie is invalid!");
      }
      serviceThread->IsNewSession = false;
   }
   else {
      puts("CookieEcho is only allowed before first transaction!");
   }
}

void ServiceThread::run()
{
   // ====== Handle command =================================================
   struct TagItem                   tags[16];
   struct FractalGeneratorParameter parameter;
   struct FractalGeneratorData      data;
   ssize_t                          received;
   unsigned long long               lastCookieTimeStamp;
   unsigned long long               lastSendTimeStamp;

   size_t                           width;
   size_t                           height;
   std::complex<double>             c1;
   std::complex<double>             c2;
   size_t                           maxIterations;
   unsigned int                     algorithmID;
   size_t                           x, y, i;

   double                           stepX;
   double                           stepY;
   std::complex<double>             z;


   for(;;) {
      tags[0].Tag  = TAG_RspIO_Timeout;
      tags[0].Data = (tagdata_t)5000000;
      tags[1].Tag  = TAG_DONE;
      received = rspSessionRead(Session, (char*)&parameter, sizeof(parameter), (struct TagItem*)&tags);
      if(received == sizeof(parameter)) {
         IsNewSession = false;

         width         = ntohl(parameter.Width);
         height        = ntohl(parameter.Height);
         c1            = std::complex<double>(parameter.C1Real, parameter.C1Imag);
         c2            = std::complex<double>(parameter.C2Real, parameter.C2Imag);
         maxIterations = ntohl(parameter.MaxIterations);
         algorithmID   = ntohl(parameter.AlgorithmID);
         std::cout << "Start: w=" << width << " h=" << height
                  << " c1=" << c1
                  << " c2=" << c2
                  << " maxIterations=" << maxIterations
                  << " algorithmID=" << algorithmID << std::endl;

         data.Points = 0;
         stepX = (c2.real() - c1.real()) / width;
         stepY = (c2.imag() - c1.imag()) / height;
         lastCookieTimeStamp = lastSendTimeStamp = getMicroTime();
         for(y = 0;y < height;y++) {
            for(x = 0;x < width;x++) {
               const std::complex<double> c =
                  std::complex<double>(c1.real() + ((double)x * stepX),
                                       c1.imag() + ((double)y * stepY));
               z = std::complex<double>(0.0, 0.0);

               for(i = 0;i < maxIterations;i++) {
                  z = z*z - c;
                  if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                     break;
                  }
               }

               if(data.Points == 0) {
                  data.StartX = x;
                  data.StartY = y;
               }
               data.Buffer[data.Points] = i;
               data.Points++;

               if( (data.Points >= FGD_MAX_POINTS) ||
                  (lastSendTimeStamp + 1000000 < getMicroTime()) ) {
                  if(rspSessionWrite(Session,
                                    &data,
                                    getFractalGeneratorDataSize(data.Points),
                                    NULL) < 0) {
                     std::cerr << "Writing data failed!" << std::endl;
                     goto finish;
                     break;
                  }
                  lastSendTimeStamp = getMicroTime();
                  data.Points = 0;
               }
               if(lastCookieTimeStamp + 1000000 < getMicroTime()) {
                  FractalGeneratorCookie cookie;
                  strncpy((char*)&cookie.ID, FG_COOKIE_ID, sizeof(FG_COOKIE_ID));
                  cookie.Parameter.Width         = htonl(width);
                  cookie.Parameter.Height        = htonl(height);
                  cookie.Parameter.MaxIterations = htonl(maxIterations);
                  cookie.Parameter.AlgorithmID   = htonl(algorithmID);
                  cookie.Parameter.C1Real        = c1.real();
                  cookie.Parameter.C2Real        = c2.real();
                  cookie.Parameter.C1Imag        = c1.imag();
                  cookie.Parameter.C2Imag        = c2.imag();
                  cookie.CurrentX                = htonl(x);
                  cookie.CurrentY                = htonl(y);
                  if(rspSessionSendCookie(Session,
                                          (unsigned char*)&cookie,
                                          sizeof(cookie), NULL) == 0) {
                     std::cerr << "Writing cookie failed!" << std::endl;
                     goto finish;
                     break;
                  }
                  lastCookieTimeStamp = getMicroTime();
               }
            }
         }

         if(data.Points > 0) {
            if(rspSessionWrite(Session,
                                 &data,
                                 getFractalGeneratorDataSize(data.Points),
                                 NULL) < 0) {
               std::cerr << "Writing data failed!" << std::endl;
               goto finish;
            }
            data.Points = 0;
         }

         data.StartX = ~0;
         data.StartY = ~0;
         data.Points = 0;
         if(rspSessionWrite(Session,
                              &data,
                              getFractalGeneratorDataSize(data.Points),
                              NULL) < 0) {
            std::cerr << "Writing data failed!" << std::endl;
            goto finish;
         }
      }
      else {
         std::cerr << "Invalid parameter message received!" << std::endl;
         goto finish;
      }
   }

   // ====== Shutdown connection ==========================================
finish:
   rspDeleteSession(Session);
   Session = NULL;
}




class ServiceThreadList
{
   public:
   ServiceThreadList();
   ~ServiceThreadList();
   void add(ServiceThread* thread);
   void remove(ServiceThread* thread);
   void removeFinished();
   void removeAll();

   struct ThreadListEntry {
      ThreadListEntry* Next;
      ServiceThread*   Object;
   };
   ThreadListEntry* ThreadList;
};

ServiceThreadList::ServiceThreadList()
{
   ThreadList = NULL;
}

ServiceThreadList::~ServiceThreadList()
{
   removeAll();
}

void ServiceThreadList::removeFinished()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      ThreadListEntry* next = entry->Next;
      if(entry->Object->hasFinished()) {
         remove(entry->Object);
      }
      entry = next;
   }
}

void ServiceThreadList::removeAll()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      remove(entry->Object);
      entry = ThreadList;
   }
}

void ServiceThreadList::add(ServiceThread* thread)
{
   ThreadListEntry* entry = new ThreadListEntry;
   entry->Next   = ThreadList;
   entry->Object = thread;
   ThreadList    = entry;
}

void ServiceThreadList::remove(ServiceThread* thread)
{
   ThreadListEntry* entry = ThreadList;
   ThreadListEntry* prev  = NULL;
   while(entry != NULL) {
      if(entry->Object == thread) {
         if(prev == NULL) {
            ThreadList = entry->Next;
         }
         else {
            prev->Next = entry->Next;
         }
         delete entry->Object;
         entry->Object = NULL;
         delete entry;
         return;
      }
      prev  = entry;
      entry = entry->Next;
   }
}




/* ###### rsplib main loop thread ########################################### */
static bool RsplibThreadStop = false;
static void* rsplibMainLoop(void* args)
{
   struct timeval timeout;
   while(!RsplibThreadStop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 50000;
      rspSelect(0, NULL, NULL, NULL, &timeout);
   }
   return(NULL);
}


int main(int argc, char** argv)
{
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   pthread_t                     rsplibThread;
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];

   card64                        start                          = getMicroTime();
   card64                        stop                           = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "FractalGeneratorPool";
   unsigned int                  policyType                     = PPT_ROUNDROBIN;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   int                           n;
   int                           result;


   start = getMicroTime();
   stop  = 0;
   for(n = 1;n < argc;n++) {
      if(!(strcmp(argv[n], "-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strncmp(argv[n], "-stop=" ,6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[n][6]));
      }
      else if(!(strncmp(argv[n], "-port=" ,6))) {
         port = atol((char*)&argv[n][6]);
      }
      else if(!(strncmp(argv[n], "-ph=" ,4))) {
         poolHandle = (char*)&argv[n][4];
      }
      else if(!(strncmp(argv[n], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[n][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[n][8]);
         if(policyParameterWeight > 0xffffff) {
            policyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[n][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-loaddeg=" ,9))) {
         policyParameterLoadDegradation = atol((char*)&argv[n][9]);
         if(policyParameterLoadDegradation > 0xffffff) {
            policyParameterLoadDegradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[n], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[n][8]);
         if(policyParameterWeight < 1) {
            policyParameterWeight = 1;
         }
      }
      else if(!(strncmp(argv[n], "-policy=" ,8))) {
         if((!(strcmp((char*)&argv[n][8], "roundrobin"))) || (!(strcmp((char*)&argv[n][8], "rr")))) {
            policyType = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[n][8], "wrr")))) {
            policyType = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastused"))) || (!(strcmp((char*)&argv[n][8], "lu")))) {
            policyType = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "lud")))) {
            policyType = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[n][8], "rlu")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rlud")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastused"))) || (!(strcmp((char*)&argv[n][8], "plu")))) {
            policyType = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "plud")))) {
            policyType = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[n][8], "rplu")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[n][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[n][8], "rplud")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[n][8], "random"))) || (!(strcmp((char*)&argv[n][8], "rand")))) {
            policyType = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[n][8], "weightedrandom"))) || (!(strcmp((char*)&argv[n][8], "wrand")))) {
            policyType = PPT_WEIGHTED_RANDOM;
         }
         else {
            printf("ERROR: Unknown policy type \"%s\"!\n" , (char*)&argv[n][8]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-log" ,4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[n], "-nameserver=" ,12))) {
         /* Process this later */
      }
      else {
         printf("Bad argument \"%s\"!\n" ,argv[n]);
         printf("Usage: %s {-nameserver=Nameserver address(es)} {-ph=Pool Handle} {-sctp} {-port=local port} {-stop=seconds} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight} \n" ,
                argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      finishLogging();
      exit(1);
   }

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-nameserver=" ,12))) {
         if(rspAddStaticNameServer((char*)&argv[n][12]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad name server setting: %s\n", argv[n]);
            exit(1);
         }
      }
   }


   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }

   tags[0].Tag  = TAG_PoolPolicy_Type;
   tags[0].Data = policyType;
   tags[1].Tag  = TAG_PoolPolicy_Parameter_Load;
   tags[1].Data = policyParameterLoad;
   tags[2].Tag  = TAG_PoolPolicy_Parameter_LoadDegradation;
   tags[2].Data = policyParameterLoadDegradation;
   tags[3].Tag  = TAG_PoolPolicy_Parameter_Weight;
   tags[3].Data = policyParameterWeight;
   tags[4].Tag  = TAG_PoolElement_SocketType;
   tags[4].Data = type;
   tags[5].Tag  = TAG_PoolElement_SocketProtocol;
   tags[5].Data = protocol;
   tags[6].Tag  = TAG_PoolElement_LocalPort;
   tags[6].Data = port;
   tags[7].Tag  = TAG_PoolElement_ReregistrationInterval;
   tags[7].Data = 45000;
   tags[8].Tag  = TAG_PoolElement_RegistrationLife;
   tags[8].Data = (3 * 45000) + 5000;
   tags[9].Tag  = TAG_UserTransport_HasControlChannel;
   tags[9].Data = (tagdata_t)true;
   tags[10].Tag  = TAG_END;

   poolElement = rspCreatePoolElement((unsigned char*)poolHandle, strlen(poolHandle), tags);
   if(poolElement != NULL) {
      puts("Fractal Generator Pool Element - Version 1.0");
      puts("--------------------------------------------\n");

#ifndef FAST_BREAK
      installBreakDetector();
#endif

      ServiceThreadList stl;
      while(!breakDetected()) {
         /* ====== Clean-up session list ================================= */
         stl.removeFinished();

         /* ====== Call rspSessionSelect() =============================== */
         /* Wait for activity. Note: rspSessionSelect() is called in the
            background thread. Therefore, it is *not* necessary here! */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 0;
         tags[1].Tag  = TAG_DONE;
         pedArray[0]       = poolElement;
         pedStatusArray[0] = RspSelect_Read;
         result = rspSessionSelect(NULL,
                                   NULL,
                                   0,
                                   (struct PoolElementDescriptor**)&pedArray,
                                   (unsigned int*)&pedStatusArray,
                                   1,
                                   500000,
                                   (struct TagItem*)&tags);

         /* ====== Handle results of ext_select() =========================== */
         if(result > 0) {
            if(pedStatusArray[0] & RspSelect_Read) {
               ServiceThread* serviceThread = new ServiceThread;
               if(serviceThread) {
                  tags[0].Tag  = TAG_TuneSCTP_MinRTO;
                  tags[0].Data = 200;
                  tags[1].Tag  = TAG_TuneSCTP_MaxRTO;
                  tags[1].Data = 500;
                  tags[2].Tag  = TAG_TuneSCTP_InitialRTO;
                  tags[2].Data = 250;
                  tags[3].Tag  = TAG_TuneSCTP_Heartbeat;
                  tags[3].Data = 100;
                  tags[4].Tag  = TAG_TuneSCTP_PathMaxRXT;
                  tags[4].Data = 3;
                  tags[5].Tag  = TAG_TuneSCTP_AssocMaxRXT;
                  tags[5].Data = 12;
                  tags[6].Tag  = TAG_RspSession_ReceivedCookieEchoCallback;
                  tags[6].Data = (tagdata_t)ServiceThread::handleCookieEcho;
                  tags[7].Tag  = TAG_RspSession_ReceivedCookieEchoUserData;
                  tags[7].Data = (tagdata_t)serviceThread;
                  tags[8].Tag  = TAG_UserTransport_HasControlChannel;
                  tags[8].Data = 1;
                  tags[9].Tag  = TAG_DONE;
                  serviceThread->Session = rspAcceptSession(pedArray[0], (struct TagItem*)&tags);
                  if(serviceThread->Session) {
                     std::cout << "Adding new client." << std::endl;
                     stl.add(serviceThread);
                     serviceThread->start();
                  }
                  else {
                     std::cerr << "ERROR: Unable to accept new session!" << std::endl;
                     delete serviceThread;
                  }
               }
            }
         }

         if(result < 0) {
            perror("rspSessionSelect() failed");
            break;
         }

         /* ====== Check auto-stop timer ==================================== */
         if((stop > 0) && (getMicroTime() >= stop)) {
            puts("Auto-stop time reached -> exiting!");
            break;
         }
      }

      std::cerr << "Closing sessions..." << std::endl;
      stl.removeAll();
      std::cerr << "Removing Pool Element..." << std::endl;
      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      puts("ERROR: Unable to create pool element!");
      exit(1);
   }

   std::cerr << "Removing main thread..." << std::endl;
   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
