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
#ifdef ENABLE_CSP
#include "componentstatusprotocol.h"
#endif

#include "fractalgeneratorpackets.h"


using namespace std;


class ServiceThread : public TDThread
{
   public:
   ServiceThread(const size_t failureAfter, const bool testMode);
   ~ServiceThread();

   inline bool hasFinished() {
      return(Session == NULL);
   }

   private:
   void run();
   bool handleParameter(const FGPParameter* parameter,
                        const size_t        length);
   bool handleCookieEcho(const FGPCookie* cookie,
                         const size_t     length);
   bool sendCookie();
   bool sendData(FGPData* data);

   unsigned int           ID;
   bool                   IsNewSession;
   bool                   Shutdown;
   FractalGeneratorStatus Status;
   unsigned long long     LastCookieTimeStamp;
   unsigned long long     LastSendTimeStamp;

   public:
   SessionDescriptor*  Session;
   size_t              FailureAfter;
   bool                TestMode;
};


// ###### Constructor #######################################################
ServiceThread::ServiceThread(const size_t failureAfter, const bool testMode)
{
   static unsigned int IDCounter = 0;
   ID                  = ++IDCounter;
   Session             = NULL;
   IsNewSession        = true;
   Shutdown            = false;
   FailureAfter        = failureAfter;
   TestMode            = testMode;
   LastSendTimeStamp   = 0;
   LastCookieTimeStamp = 0;
   cout << "Created thread " << ID << "..." << endl;
}


// ###### Destructor ########################################################
ServiceThread::~ServiceThread()
{
   cout << "Stopping thread " << ID << "..." << endl;
   Shutdown = true;
   waitForFinish();
   if(Session) {
      rspDeleteSession(Session);
      Session = NULL;
   }
   cout << "Thread " << ID << " has been stopped." << endl;
}


// ###### Handle parameter message ##########################################
bool ServiceThread::handleParameter(const FGPParameter* parameter,
                                    const size_t        size)
{
   if(size < sizeof(struct FGPParameter)) {
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
   cout << "Parameters: w=" << Status.Parameter.Width
        << " h=" << Status.Parameter.Width
        << " c1=" << complex<double>(networkToDouble(Status.Parameter.C1Real), networkToDouble(Status.Parameter.C1Imag))
        << " c2=" << complex<double>(networkToDouble(Status.Parameter.C2Real), networkToDouble(Status.Parameter.C2Imag))
        << " n=" << networkToDouble(Status.Parameter.N)
        << " maxIterations=" << Status.Parameter.MaxIterations
        << " algorithmID=" << Status.Parameter.AlgorithmID << endl;
   if((Status.Parameter.Width > 65536)           ||
      (Status.Parameter.Height > 65536)          ||
      (Status.Parameter.MaxIterations > 1000000)) {
      cerr << "Bad parameters!" << endl;
      return(false);
   }
   return(true);
}


// ###### Handle cookie echo ################################################
bool ServiceThread::handleCookieEcho(const FGPCookie* cookie,
                                     const size_t     size)
{
   if(IsNewSession == false) {
      cerr << "Bad cookie: session is not new!" << endl;
      return(false);
   }
   if(size != sizeof(struct FGPCookie)) {
      cerr << "Bad cookie: invalid size!" << endl;
      return(false);
   }
   if(strncmp(cookie->ID, FGP_COOKIE_ID, sizeof(cookie->ID))) {
      cerr << "Bad cookie: invalid ID!" << endl;
      return(false);
   }
   IsNewSession = false;

   if(!handleParameter(&cookie->Parameter, sizeof(cookie->Parameter))) {
      return(false);
   }
   Status.CurrentX = ntohl(cookie->CurrentX);
   Status.CurrentY = ntohl(cookie->CurrentY);
   cout << "Cookie: x=" << Status.CurrentX
        << " y=" << Status.CurrentY << endl;
   return(true);
}


// ###### Send cookie #######################################################
bool ServiceThread::sendCookie()
{
   FGPCookie cookie;
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
   if(rspSessionSendCookie(Session,
                           (unsigned char*)&cookie,
                           sizeof(cookie), NULL) == 0) {
      cerr << "Writing cookie failed!" << endl;
      return(false);
   }
   LastCookieTimeStamp = getMicroTime();
   return(true);
}


// ###### Send data #########################################################
bool ServiceThread::sendData(FGPData* data)
{
   const size_t dataSize = getFGPDataSize(data->Points);
   for(size_t i = 0;i < data->Points;i++) {
      data->Buffer[i] = htonl(data->Buffer[i]);
   }
   data->Header.Type   = FGPT_DATA;
   data->Header.Flags  = 0x00;
   data->Header.Length = htons(dataSize);
   data->Points = htonl(data->Points);
   data->StartX = htonl(data->StartX);
   data->StartY = htonl(data->StartY);

   struct TagItem tags[16];
   tags[0].Tag  = TAG_RspIO_SCTP_PPID;
   tags[0].Data = PPID_FGP;
   tags[1].Tag  = TAG_DONE;
   if(rspSessionWrite(Session,
                      data,
                      dataSize,
                      (TagItem*)&tags) < 0) {
      cerr << "Writing data failed!" << endl;
      return(false);
   }
   data->Points = 0;
   data->StartX = 0;
   data->StartY = 0;
   LastSendTimeStamp = getMicroTime();
   return(true);
}


// ###### Service thread main loop ##########################################
void ServiceThread::run()
{
   char            buffer[4096];
   struct TagItem  tags[16];
   struct FGPData  data;
   ssize_t         received;
   double          stepX;
   double          stepY;
   complex<double> z;
   size_t          i;
   size_t          dataPackets = 0;

   while(!Shutdown) {
      tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
      tags[0].Data = 0;
      tags[1].Tag  = TAG_RspIO_Timeout;
      tags[1].Data = (tagdata_t)2000000;
      tags[2].Tag  = TAG_DONE;
      received = rspSessionRead(Session, (char*)&buffer, sizeof(buffer), (struct TagItem*)&tags);
      if(received > 0) {
         if(tags[0].Data != 0) {
            if(!handleCookieEcho((struct FGPCookie*)&buffer, received)) {
               goto finish;
            }
         }
         else {
            if((received >= (ssize_t)sizeof(FGPCommonHeader)) &&
               (((FGPCommonHeader*)&buffer)->Type == FGPT_PARAMETER) &&
               (!handleParameter((struct FGPParameter*)&buffer, received))) {
               Status.CurrentX = 0;
               Status.CurrentY = 0;
               goto finish;
            }
         }

         IsNewSession = false;
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
            cerr << "Sending cookie (start) failed!" << endl;
            goto finish;
         }
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               if(!TestMode) {
                  const complex<double> c =
                     complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                    c1imag + ((double)Status.CurrentY * stepY));

                  // ====== Algorithms =========================================
                  switch(Status.Parameter.AlgorithmID) {
                     case FGPA_MANDELBROT:
                        z = complex<double>(0.0, 0.0);
                        for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                           z = z*z - c;
                           if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                              break;
                           }
                        }
                     break;
                     case FGPA_MANDELBROTN:
                        z = complex<double>(0.0, 0.0);
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
               // ===========================================================

               if(data.Points == 0) {
                  data.StartX = Status.CurrentX;
                  data.StartY = Status.CurrentY;
               }
               data.Buffer[data.Points] = i;
               data.Points++;

               if( (data.Points >= FGD_MAX_POINTS) ||
                  (LastSendTimeStamp + 1000000 < getMicroTime()) ) {
                  if(!sendData(&data)) {
                     cerr << "Sending data failed!" << endl;
                     goto finish;
                  }

                  dataPackets++;
                  if((FailureAfter > 0) && (dataPackets > FailureAfter)) {
                     sendCookie();
                     cerr << "Failure Tester -> Disconnect after "
                          << dataPackets << " packets!" << endl;
                     goto finish;
                  }
               }

               Status.CurrentX++;
            }
            Status.CurrentX = 0;
            Status.CurrentY++;

            if(LastCookieTimeStamp + 1000000 < getMicroTime()) {
               if(!sendCookie()) {
                  cerr << "Sending cookie failed!" << endl;
                  goto finish;
               }
            }
            if(Shutdown) {
               cerr << "Aborting session due to server shutdown!" << endl;
               goto finish;
            }
         }

         if(data.Points > 0) {
            if(!sendData(&data)) {
               cerr << "Sending data (last block) failed!" << endl;
               goto finish;
            }
         }

         data.StartX = 0xffffffff;
         data.StartY = 0xffffffff;
         if(!sendData(&data)) {
            cerr << "Sending data (finalizer) failed!" << endl;
            goto finish;
         }
      }
      else {
         goto finish;
      }
   }

   // ====== Shutdown connection ==========================================
finish:
   cerr << "Thread-Ende!" << endl;
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


// ###### Constructor #######################################################
ServiceThreadList::ServiceThreadList()
{
   ThreadList = NULL;
}


// ###### Destructor ########################################################
ServiceThreadList::~ServiceThreadList()
{
   removeAll();
}


// ###### Remove finished sessions ##########################################
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


// ###### Remove all sessions ###############################################
void ServiceThreadList::removeAll()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      remove(entry->Object);
      entry = ThreadList;
   }
}


// ###### Add session #######################################################
void ServiceThreadList::add(ServiceThread* thread)
{
   ThreadListEntry* entry = new ThreadListEntry;
   entry->Next   = ThreadList;
   entry->Object = thread;
   ThreadList    = entry;
}


// ###### Remove session ####################################################
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




// ###### rsplib main loop thread ###########################################
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



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   uint32_t                      identifier        = 0;
   unsigned int                  reregInterval     = 5000;
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   pthread_t                     rsplibThread;
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];
   card64                        start                          = getMicroTime();
   card64                        stop                           = 0;
   size_t                        failureAfter                   = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "FractalGeneratorPool";
   unsigned int                  policyType                     = PPT_RANDOM;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   bool                          testMode                       = false;
   int                           i;
   int                           result;
#ifdef ENABLE_CSP
   struct CSPReporter            cspReporter;
   uint64_t                      cspIdentifier;
   unsigned int                  cspReportInterval = 0;
   union sockaddr_union          cspReportAddress;
#endif

#ifdef ENABLE_CSP
   string2address("127.0.0.1:2960", &cspReportAddress);
#endif
   start = getMicroTime();
   stop  = 0;
   for(i = 1;i < argc;i++) {
      if(!(strcmp(argv[i], "-sctp"))) {
         protocol = IPPROTO_SCTP;
      }
      else if(!(strncmp(argv[i], "-stop=" ,6))) {
         stop = start + ((card64)1000000 * (card64)atol((char*)&argv[i][6]));
      }
      else if(!(strncmp(argv[i], "-port=" ,6))) {
         port = atol((char*)&argv[i][6]);
      }
      else if(!(strncmp(argv[i], "-ph=" ,4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         identifier = atol((char*)&argv[i][12]);
      }
      else if(!(strcmp(argv[i], "-testmode"))) {
         testMode = true;
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-cspreportinterval=", 19))) {
         cspReportInterval = atol((char*)&argv[i][19]);
      }
      else if(!(strncmp(argv[i], "-cspreportaddress=", 18))) {
         if(!string2address((char*)&argv[i][18], &cspReportAddress)) {
            fprintf(stderr,
                    "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                    (char*)&argv[i][18]);
            exit(1);
         }
         if(cspReportInterval <= 0) {
            cspReportInterval = 250000;
         }
      }
#else
      else if((!(strncmp(argv[i], "-cspreportinterval=", 19))) ||
              (!(strncmp(argv[i], "-cspreportaddress=", 18)))) {
         fprintf(stderr, "ERROR: CSP support not compiled in! Ignoring argument %s\n", argv[i]);
         exit(1);
      }
#endif
      else if(!(strncmp(argv[i], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[i][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[i][8]);
         if(policyParameterWeight > 0xffffff) {
            policyParameterWeight = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-load=" ,6))) {
         policyParameterLoad = atol((char*)&argv[i][6]);
         if(policyParameterLoad > 0xffffff) {
            policyParameterLoad = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-loaddeg=" ,9))) {
         policyParameterLoadDegradation = atol((char*)&argv[i][9]);
         if(policyParameterLoadDegradation > 0xffffff) {
            policyParameterLoadDegradation = 0xffffff;
         }
      }
      else if(!(strncmp(argv[i], "-weight=" ,8))) {
         policyParameterWeight = atol((char*)&argv[i][8]);
         if(policyParameterWeight < 1) {
            policyParameterWeight = 1;
         }
      }
      else if(!(strncmp(argv[i], "-policy=" ,8))) {
         if((!(strcmp((char*)&argv[i][8], "roundrobin"))) || (!(strcmp((char*)&argv[i][8], "rr")))) {
            policyType = PPT_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedroundrobin"))) || (!(strcmp((char*)&argv[i][8], "wrr")))) {
            policyType = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastused"))) || (!(strcmp((char*)&argv[i][8], "lu")))) {
            policyType = PPT_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "leastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "lud")))) {
            policyType = PPT_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastused"))) || (!(strcmp((char*)&argv[i][8], "rlu")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rlud")))) {
            policyType = PPT_RANDOMIZED_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastused"))) || (!(strcmp((char*)&argv[i][8], "plu")))) {
            policyType = PPT_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "priorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "plud")))) {
            policyType = PPT_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastused"))) || (!(strcmp((char*)&argv[i][8], "rplu")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED;
         }
         else if((!(strcmp((char*)&argv[i][8], "randomizedpriorityleastuseddegradation"))) || (!(strcmp((char*)&argv[i][8], "rplud")))) {
            policyType = PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION;
         }
         else if((!(strcmp((char*)&argv[i][8], "random"))) || (!(strcmp((char*)&argv[i][8], "rand")))) {
            policyType = PPT_RANDOM;
         }
         else if((!(strcmp((char*)&argv[i][8], "weightedrandom"))) || (!(strcmp((char*)&argv[i][8], "wrand")))) {
            policyType = PPT_WEIGHTED_RANDOM;
         }
         else {
            cerr << "ERROR: Unknown policy type \"" << (char*)&argv[i][8] << "\"!" << endl;
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-rereginterval=" ,15))) {
         reregInterval = atol((char*)&argv[i][15]);
         if(reregInterval < 250) {
            reregInterval = 250;
         }
      }
      else if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-failureafter=" ,14))) {
         failureAfter = atol((char*)&argv[i][14]);
      }
      else if(!(strncmp(argv[i], "-registrar=" ,11))) {
         /* Process this later */
      }
      else {
         cerr << "Bad argument \"" << argv[i] << "\"!"  << endl;
         cerr << "Usage: " << argv[0]
              << " {-registrar=Registrar address(es)} {-ph=Pool Handle} {-sctp} {-port=local port} {-stop=seconds} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} {-policy=roundrobin|rr|weightedroundrobin|wee|leastused|lu|leastuseddegradation|lud|random|rd|weightedrandom|wrd} {-load=load} {-weight=weight} {-failureafter=Packets}";
#ifdef ENABLE_CSP
         cerr << "{-cspreportaddress=Address} {-cspreportinterval=Microseconds} ";
#endif
         cerr << " {-identifier=PE Identifier}"
              << endl;
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      cerr << "ERROR: Unable to initialize rsplib!" << endl;
      finishLogging();
      exit(1);
   }
#ifdef ENABLE_CSP
   cspIdentifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, identifier);
   if(cspReportInterval > 0) {
      cspReporterNewForRspLib(&cspReporter, cspIdentifier, &cspReportAddress, cspReportInterval);
   }
#endif

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[i][11]) != RSPERR_OKAY) {
            cerr << "ERROR: Bad registrar setting: "
                 << argv[i] << "!" << endl;
            exit(1);
         }
      }
   }


   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      cerr << "ERROR: Unable to create rsplib main loop thread!" << endl;
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
   tags[7].Data = reregInterval;
   tags[8].Tag  = TAG_PoolElement_RegistrationLife;
   tags[8].Data = (3 * 45000) + 5000;
   tags[9].Tag  = TAG_PoolElement_Identifier;
   tags[9].Data = identifier;
   tags[10].Tag  = TAG_UserTransport_HasControlChannel;
   tags[10].Data = (tagdata_t)true;
   tags[11].Tag  = TAG_END;

   poolElement = rspCreatePoolElement((unsigned char*)poolHandle, strlen(poolHandle), tags);
   if(poolElement != NULL) {
      cout << "Fractal Generator Pool Element - Version 1.0" << endl
           << "--------------------------------------------" << endl << endl
           << "Failure After: " << failureAfter << " packets" << endl
           << "Test Mode:     " << (testMode ? "on" : "off") << endl << endl;

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
               ServiceThread* serviceThread = new ServiceThread(failureAfter, testMode);
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
                  tags[6].Tag  = TAG_UserTransport_HasControlChannel;
                  tags[6].Data = 1;
                  tags[7].Tag  = TAG_DONE;
                  serviceThread->Session = rspAcceptSession(pedArray[0], (struct TagItem*)&tags);
                  if(serviceThread->Session) {
                     cout << "Adding new client." << endl;
                     stl.add(serviceThread);
                     serviceThread->start();
                  }
                  else {
                     cerr << "ERROR: Unable to accept new session!" << endl;
                     delete serviceThread;
                  }
               }
            }
         }

         if(result < 0) {
            if(errno != EINTR) {
               perror("rspSessionSelect() failed");
            }
            break;
         }

         /* ====== Check auto-stop timer ==================================== */
         if((stop > 0) && (getMicroTime() >= stop)) {
            cerr << "Auto-stop time reached -> exiting!" << endl;
            break;
         }
      }

      cout << "Closing sessions..." << endl;
      stl.removeAll();
      cout << "Removing Pool Element..." << endl;
      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      cerr << "ERROR: Unable to create pool element!" << endl;
      exit(1);
   }

   cout << "Removing main thread..." << endl;
   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);
#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      cspReporterDelete(&cspReporter);
   }
#endif
   rspCleanUp();
   finishLogging();
   cout << endl << "Terminated!" << endl;
   return(0);
}
