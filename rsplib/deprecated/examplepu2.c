/*
 *  $Id: examplepu2.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Example Pool User
 *
 */


#include "tdtypes.h"
#include "utilities.h"
#include "netutilities.h"
#include "dispatcher.h"
#include "timer.h"
#include "asapinstance.h"
#include "pool.h"
#include "poolelement.h"
#include "asapconnection.h"
#include "breakdetector.h"

#include <ext_socket.h>



/* Exit immediately on Ctrl-C. No clean shutdown. */
// #define FAST_BREAK



struct ExamplePoolUser
{
   struct Dispatcher*     StateMachine;
   struct ASAPInstance*   ASAP;
   struct ASAPConnection* Connection;
   struct PoolHandle*     Handle;
   card64                 InBytes;
   card64                 OutBytes;
   card64                 OutCalls;
};


struct ExamplePoolUser* epuNew(const char* poolName);
void epuDelete(struct ExamplePoolUser* epu);
static void epuEchoInputHandler(struct Dispatcher* dispatcher,
                                int                fd,
                                int                eventMask,
                                void*              userData);
static void epuEchoOutputHandler(struct ASAPConnection* asapConnection,
                                 int                    eventMask,
                                 void*                  userData);




struct ExamplePoolUser* epuNew(const char* poolName)
{
   struct ExamplePoolUser* epu;
   enum ASAPError          result;

   epu = (struct ExamplePoolUser*)malloc(sizeof(struct ExamplePoolUser));
   if(epu == NULL) {
      puts("ERROR: Out of memory!");
      epuDelete(epu);
   }

   epu->StateMachine = NULL;
   epu->ASAP         = NULL;
   epu->Connection   = NULL;
   epu->InBytes      = 0;
   epu->OutBytes     = 0;
   epu->OutCalls     = 0;

   epu->StateMachine = dispatcherNew(dispatcherDefaultLock,dispatcherDefaultUnlock,NULL);
   if(epu->StateMachine == NULL) {
      puts("ERROR: Unable to create dispatcher!");
      epuDelete(epu);
   }

   epu->ASAP = asapNew(epu->StateMachine);
   if(epu->ASAP == NULL) {
      puts("ERROR: Unable to create ASAP instance!");
      epuDelete(epu);
   }

   dispatcherAddFDCallback(epu->StateMachine,STDIN_FILENO,FDCE_Read|FDCE_Exception,epuEchoInputHandler,(void*)epu);

   epu->Handle = poolHandleNewASCII(poolName);
   if(epu->Handle == NULL) {
      puts("ERROR: Unable to create pool handle!");
      epuDelete(epu);
   }

   result = asapConnect(epu->ASAP,
                        epu->Handle,
                        0,
                        &epu->Connection,
                        FDCE_Read|FDCE_Exception,
                        epuEchoOutputHandler,
                        (void*)epu);
   if(result != ASAP_Okay) {
      printf("ERROR: Unable to connect to pool \"%s\": Error code #%d!\n",poolName,result);
      epuDelete(epu);
   }

#ifndef FAST_BREAK
   installBreakDetector();
#endif

   return(epu);
}


void epuDelete(struct ExamplePoolUser* epu)
{
   if(epu) {
      if(epu->Connection) {
         asapDisconnect(epu->ASAP,epu->Connection);
         epu->Connection = NULL;
      }
      if(epu->Handle) {
         poolHandleDelete(epu->Handle);
         epu->Handle = NULL;
      }
      if(epu->ASAP) {
         asapDelete(epu->ASAP);
         epu->ASAP = NULL;
      }
      if(epu->StateMachine) {
         dispatcherDelete(epu->StateMachine);
         epu->StateMachine = NULL;
      }
      free(epu);
   }
   exit(0);
}



static void epuEchoInputHandler(struct Dispatcher* dispatcher,
                                int               fd,
                                int               eventMask,
                                void*             userData)
{
   struct ExamplePoolUser* epu = (struct ExamplePoolUser*)userData;
   char                    buffer[8192];
   struct msghdr           msg;
   struct iovec            io;
   ssize_t                 received;
   size_t                  lineNumberLength;

   if(fd == STDIN_FILENO) {
      setNonBlocking(fd);

      snprintf((char*)&buffer,sizeof(buffer),"%09Ld ",++epu->OutCalls);
      lineNumberLength = strlen(buffer);

      fgets((char*)&buffer[lineNumberLength],sizeof(buffer) - lineNumberLength,stdin);
      if(buffer[lineNumberLength] == 0x00) {
         sendBreak(true);
      }
      else {
         received = strlen(buffer);
         msg.msg_name       = NULL;
         msg.msg_namelen    = 0;
         msg.msg_control    = NULL;
         msg.msg_controllen = 0;
         msg.msg_iov        = &io;
         msg.msg_iovlen     = 1;
#ifdef __APPLE__
         msg.msg_flags      = 0;
#else
         msg.msg_flags      = MSG_NOSIGNAL;
#endif
         io.iov_base = buffer;
         io.iov_len  = received;

         if(asapConnectionWrite(epu->Connection,&msg)) {
            epu->OutBytes += received;
         }
      }
   }
}


static void epuEchoOutputHandler(struct ASAPConnection* asapConnection,
                                 int                    eventMask,
                                 void*                  userData)
{
   struct ExamplePoolUser* epu = (struct ExamplePoolUser*)userData;
   char                    name[256];
   char                    buffer[8193];
   struct msghdr           msg;
   struct iovec            io;
   ssize_t                 received;
   static card64           counter = 1;

   msg.msg_name       = &name;
   msg.msg_namelen    = sizeof(name);
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_iov        = &io;
   msg.msg_iovlen     = 1;
#ifdef __APPLE__
   msg.msg_flags      = MSG_DONTWAIT;
#else
   msg.msg_flags      = MSG_NOSIGNAL|MSG_DONTWAIT;
#endif
   io.iov_base = buffer;
   io.iov_len  = sizeof(buffer) - 1;

   received = asapConnectionRead(epu->Connection,&msg);
   if(received > 0) {
      epu->InBytes += received;

      buffer[received] = 0x00;
      if(msg.msg_namelen == 0) {
         *((PoolElementHandle*)&name) = UNDEFINED_POOL_ELEMENT_IDENTIFIER;
      }
      printf("[in=%Ld out=%Ld] P%Ld.%Ld> %s",
             epu->InBytes, epu->OutBytes,
             (card64)*((PoolElementHandle*)&name),counter++,buffer);
   }
}


int main(int argc, char** argv)
{
   struct ExamplePoolUser* epu;
   int                     n;
   int                     result;
   struct timeval          timeout;
   fd_set                  readfdset;
   fd_set                  writefdset;
   fd_set                  exceptfdset;

   if(argc < 2) {
      printf("Usage: %s [Pool Name] {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
      exit(1);
   }


   if(argc > 2) {
      for(n = 2;n <argc;n++) {
         if(!(strncmp(argv[n],"-log",4))) {
            if(initLogging(argv[n]) == false) {
               exit(1);
            }
         }
         else {
            puts("Bad arguments!");
            exit(1);
         }
      }
   }
   beginLogging();


   epu = epuNew(argv[1]);
   if(epu) {

      puts("Example Pool User - Version 1.0");
      puts("-------------------------------\n");

      while(!breakDetected()) {
         dispatcherGetSelectParameters(epu->StateMachine, &n, &readfdset, &writefdset, &exceptfdset, &timeout);
         timeout.tv_sec  = min(0,timeout.tv_sec);
         timeout.tv_usec = min(500000,timeout.tv_usec);
         result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
         if(errno == EINTR) {
            break;
         }
         if(result < 0) {
            perror("select() failed");
            epuDelete(epu);
         }
         dispatcherHandleSelectResult(epu->StateMachine, result, &readfdset, &writefdset, &exceptfdset);
      }

      puts("\nTerminated!");
      epuDelete(epu);
   }

   finishLogging();
   return(0);
}
