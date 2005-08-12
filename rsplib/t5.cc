/*
 * calcapppoolelement.cc
 *
 * Copyright (C) 2005 by Thomas Dreibholz
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


using namespace std;


struct CalcAppMessage { int x; }; // ?????????ß

class SessionSet
{
   public:
   SessionSet();
   ~SessionSet();

   bool addSession(SessionDescriptor* session);
   void removeSession(SessionDescriptor* session);
   void removeAll();
   size_t getSessions();
   void initSelectArray(SessionDescriptor** sdArray,
                        unsigned int*       statusArray,
                        const unsigned int  status);
   void handleEvents(SessionDescriptor* session,
                     const unsigned int sessionEvents);

   private:
   struct SessionSetEntry {
      SessionSetEntry*  Next;
      SessionDescriptor* Session;

      // ---- Hier Request-spez. Daten einbauen
      // ----
   };

   SessionSetEntry* find(SessionDescriptor* session);

   void handleCookieEcho(SessionSetEntry* sessionListEntry,
                         CalcAppMessage*  message,
                         const size_t     size);
   void handleCommand(SessionSetEntry* sessionListEntry,
                      CalcAppMessage*  message,
                      const size_t     size);


   SessionSetEntry* FirstSession;
   size_t           Sessions;
};

SessionSet::SessionSet()
{
   FirstSession = NULL;
   Sessions     = 0;
}

SessionSet::~SessionSet()
{
   removeAll();
}

void SessionSet::removeAll()
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      removeSession(sessionSetEntry->Session);
      sessionSetEntry = FirstSession;
   }
   CHECK(Sessions == 0);
}

bool SessionSet::addSession(SessionDescriptor* session)
{
   SessionSetEntry* sessionSetEntry = new SessionSetEntry;
   if(sessionSetEntry) {
      sessionSetEntry->Next    = FirstSession;
      sessionSetEntry->Session = session;
      FirstSession              = sessionSetEntry;
      Sessions++;
      return(true);
   }
   return(false);
}

void SessionSet::removeSession(SessionDescriptor* session)
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   SessionSetEntry* prev  = NULL;
   while(sessionSetEntry != NULL) {
      if(sessionSetEntry->Session == session) {
         if(prev == NULL) {
            FirstSession = sessionSetEntry->Next;
         }
         else {
            prev->Next = sessionSetEntry->Next;
         }
         rspDeleteSession(sessionSetEntry->Session);
         sessionSetEntry->Session = NULL;
         delete sessionSetEntry;
         CHECK(Sessions > 0);
         Sessions--;
         return;
      }
      prev  = sessionSetEntry;
      sessionSetEntry = sessionSetEntry->Next;
   }
}

SessionSet::SessionSetEntry* SessionSet::find(SessionDescriptor* session)
{
   SessionSetEntry* sessionSetEntry = FirstSession;
   while(sessionSetEntry != NULL) {
      if(sessionSetEntry->Session == session) {
         return(sessionSetEntry);
      }
      sessionSetEntry = sessionSetEntry->Next;
   }
   return(NULL);
}

size_t SessionSet::getSessions()
{
   return(Sessions);
}

void SessionSet::initSelectArray(SessionDescriptor** sdArray,
                                 unsigned int*      statusArray,
                                 const unsigned int status)
{
   const size_t count = getSessions();
   size_t       i;

   SessionSetEntry* sessionSetEntry = FirstSession;
   for(i = 0;i < count;i++) {
      CHECK(sessionSetEntry != NULL);
      sdArray[i]     = sessionSetEntry->Session;
      statusArray[i] = status;
      sessionSetEntry = sessionSetEntry->Next;
   }
}


void SessionSet::handleEvents(SessionDescriptor* session,
                              const unsigned int sessionEvents)
{
   char           buffer[4096];
   struct TagItem tags[16];
   ssize_t        received;

   SessionSetEntry* sessionSetEntry = find(session);
   CHECK(sessionSetEntry != NULL);


   tags[0].Tag  = TAG_RspIO_MsgIsCookieEcho;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_RspIO_Timeout;
   tags[1].Data = 0;
   tags[2].Tag  = TAG_DONE;
   received = rspSessionRead(sessionSetEntry->Session, (char*)&buffer, sizeof(buffer), (struct TagItem*)&tags);
   if(received > 0) {
      printf("recv=%d\n",received);
      if(tags[0].Data != 0) {
         handleCookieEcho(sessionSetEntry, (struct CalcAppMessage*)&buffer, received);
      }
      else {
         handleCommand(sessionSetEntry, (struct CalcAppMessage*)&buffer, received);
      }
   }
}


void SessionSet::handleCookieEcho(SessionSetEntry* sessionListEntry,
                                  CalcAppMessage*  message,
                                  const size_t     size)
{
   puts("COOKIE ECHO!");
}


void SessionSet::handleCommand(SessionSetEntry* sessionListEntry,
                               CalcAppMessage*  message,
                               const size_t     size)
{
   if(size < sizeof(CalcAppMessage)) {
      cerr << "ERROR: Invalid message size" << endl;
      removeSession(sessionListEntry->Session);
      return;
   }

   printf("cmd: %d\n",size);

   removeSession(sessionListEntry->Session);

}


int main(int argc, char** argv)
{
   uint32_t                      identifier        = 0;
   unsigned int                  reregInterval     = 5000;
   struct PoolElementDescriptor* poolElement;
   struct TagItem                tags[16];
   struct PoolElementDescriptor* pedArray[1];
   unsigned int                  pedStatusArray[FD_SETSIZE];
   card64                        start                          = getMicroTime();
   card64                        stop                           = 0;
   int                           type                           = SOCK_STREAM;
   int                           protocol                       = IPPROTO_SCTP;
   unsigned short                port                           = 0;
   char*                         poolHandle                     = "CalcAppPool";
   unsigned int                  policyType                     = PPT_RANDOM;
   unsigned int                  policyParameterWeight          = 1;
   unsigned int                  policyParameterLoad            = 0;
   unsigned int                  policyParameterLoadDegradation = 0;
   int                           i;
   int                           result;

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
      else if(!(strncmp(argv[i], "-registrar=" ,11))) {
         /* Process this later */
      }
      else {
         cerr << "Bad argument \"" << argv[i] << "\"!"  << endl;
         cerr << "Usage: " << argv[0]
              << " {-registrar=Registrar address(es)} {-ph=Pool Handle} {-sctp} {-port=local port} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off} "
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

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[i][11]) != RSPERR_OKAY) {
            cerr << "ERROR: Bad registrar setting: "
                 << argv[i] << "!" << endl;
            exit(1);
         }
      }
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
      cout << "CalcApp Pool Element - Version 1.0" << endl
           << "----------------------------------" << endl << endl;

#ifndef FAST_BREAK
      installBreakDetector();
#endif

      SessionSet sessionList;
      while(!breakDetected()) {
         /* ====== Call rspSessionSelect() =============================== */
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 1;  // !!!!!!
         tags[1].Tag  = TAG_DONE;
         pedArray[0]       = poolElement;
         pedStatusArray[0] = RspSelect_Read;
         const size_t sessions = sessionList.getSessions();
         struct SessionDescriptor* sessionDescriptorArray[sessions];
         unsigned int              sessionStatusArray[sessions];
         sessionList.initSelectArray((struct SessionDescriptor**)&sessionDescriptorArray,
                                     (unsigned int*)&sessionStatusArray,
                                     RspSelect_Read);
         result = rspSessionSelect((struct SessionDescriptor**)&sessionDescriptorArray,
                                   (unsigned int*)&sessionStatusArray,
                                   sessions,
                                   (struct PoolElementDescriptor**)&pedArray,
                                   (unsigned int*)&pedStatusArray,
                                   1,
                                   500000,
                                   (struct TagItem*)&tags);

         /* ====== Handle results of ext_select() =========================== */
         if(result > 0) {
            if(pedStatusArray[0] & RspSelect_Read) {
               /*
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
               */
               tags[0].Tag  = TAG_DONE;
               struct SessionDescriptor* session = rspAcceptSession(pedArray[0], (struct TagItem*)&tags);
               if(session) {
                  if(!sessionList.addSession(session)) {
                     rspDeleteSession(session);
                  }
               }
               else {
                  cerr << "ERROR: Unable to accept new session!" << endl;
               }
            }
            for(size_t i = 0;i < sessions;i++) {
               if(sessionStatusArray[i] & RspSelect_Read) {
                  sessionList.handleEvents(sessionDescriptorArray[i],
                                           sessionStatusArray[i]);
               }
            }
         }

         if(result < 0) {
            perror("rspSessionSelect() failed");
            break;
         }

         /* ====== Check auto-stop timer ==================================== */
         if((stop > 0) && (getMicroTime() >= stop)) {
            cerr << "Auto-stop time reached -> exiting!" << endl;
            break;
         }
      }

      cout << "Closing sessions..." << endl;
      sessionList.removeAll();
      cout << "Removing Pool Element..." << endl;
      rspDeletePoolElement(poolElement, NULL);
   }
   else {
      cerr << "ERROR: Unable to create pool element!" << endl;
      exit(1);
   }

   rspCleanUp();
   finishLogging();
   cout << endl << "Terminated!" << endl;
   return(0);
}

