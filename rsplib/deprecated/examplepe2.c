/*
 *  $Id: examplepe2.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Example Pool Element
 *
 */


#include "tdtypes.h"
#include "utilities.h"
#include "netutilities.h"
#include "dispatcher.h"
#include "timer.h"
#include "asapinstance.h"
#include "breakdetector.h"
#include "localaddresses.h"

#include <ext_socket.h>


/* Exit immediately on Ctrl-C. No clean shutdown. */
// #define FAST_BREAK

#define MAX_LOCAL_ADDRESSES 128


struct ExamplePoolElement
{
   struct Dispatcher*    StateMachine;
   struct ASAPInstance*  ASAP;

   enum ASAPError        RegistrationResult;
   PoolElementIdentifier Identifier;
   struct PoolHandle*    Handle;
   struct PoolElement*   Element;
   struct PoolPolicy*    Policy;

   int                   Domain;
   int                   Protocol;
   int                   Type;
   int                   ListenSocket;

   GHashTable*           ClientTable;
};


struct ExamplePoolElement* epeNew(const char*    poolName,
                                  const char**   localAddressArray,
                                  const size_t   localAddresses,
                                  const int      type,
                                  const int      protocol,
                                  const uint8_t  policy,
                                  const uint32_t load,
                                  const uint32_t weight);
void epeDelete(struct ExamplePoolElement* epe);
void epeSocketHandler(struct Dispatcher* dispatcher,
                      int                fd,
                      int                eventMask,
                      void*              userData);



struct ExamplePoolElement* epeNew(const char*    poolName,
                                  const char**   localAddressArray,
                                  const size_t   localAddresses,
                                  const int      type,
                                  const int      protocol,
                                  const uint8_t  policy,
                                  const uint32_t load,
                                  const uint32_t weight)
{
   struct ExamplePoolElement* epe;
   struct sockaddr_storage    listenSocketLocalAddressArray[MAX_LOCAL_ADDRESSES];
   struct TransportAddress*   transportAddress;
   struct sockaddr_storage    localAddress;
   socklen_t                  localAddressLength;
   struct sockaddr_storage*   listenAddressArray = NULL;
   size_t                     listenAddresses    = 0;
   uint16_t                   port;
   size_t                     i;

   epe = (struct ExamplePoolElement*)malloc(sizeof(struct ExamplePoolElement));
   if(epe == NULL) {
      puts("ERROR: Out of memory!");
      epeDelete(epe);
   }

   epe->StateMachine       = NULL;
   epe->ASAP               = NULL;
   epe->ListenSocket       = -1;
   epe->ClientTable        = NULL;
   epe->RegistrationResult = ASAP_OutOfMemory;

   for(i = 0;i < localAddresses;i++) {
      if(i >= MAX_LOCAL_ADDRESSES) {
         puts("ERROR: Too many local addresses given!");
         epeDelete(epe);
      }
      if(string2address(localAddressArray[i],&listenSocketLocalAddressArray[i]) == false) {
         puts("ERROR: Bad local address. Use format <Address>[:<Port>].");
         epeDelete(epe);
      }
   }
   epe->Domain   = getFamily((struct sockaddr*)&listenSocketLocalAddressArray[0]);
   epe->Protocol = protocol;
   epe->Type     = type;

   epe->ListenSocket = ext_socket(epe->Domain,
                                  epe->Type,
                                  epe->Protocol);
   if(epe->ListenSocket < 0) {
      puts("ERROR: Unable to create application socket!");
      epeDelete(epe);
   }
   setNonBlocking(epe->ListenSocket);

   if(bindplus(epe->ListenSocket,(struct sockaddr_storage*)&listenSocketLocalAddressArray,localAddresses) == false) {
      perror("Unable to bind application socket");
      epeDelete(epe);
   }

   if(protocol == IPPROTO_SCTP) {
      localAddressLength = sizeof(localAddress);
      if(ext_getsockname(epe->ListenSocket,(struct sockaddr*)&localAddress,&localAddressLength) != 0) {
         puts("ERROR: Unable to socket's local address!");
         epeDelete(epe);
      }
      port = getPort((struct sockaddr*)&localAddress);
      if(gatherLocalAddresses(&listenAddressArray,&listenAddresses,ASF_Default) == false) {
         puts("ERROR: Unable to obtain local addresses!");
         epeDelete(epe);
      }
      for(i = 0;i < listenAddresses;i++) {
         setPort((struct sockaddr*)&listenAddressArray[i],port);
      }
   }
   else {
      listenAddresses = getAddressesFromSocket(epe->ListenSocket,&listenAddressArray);
      if(listenAddresses < 1) {
         puts("ERROR: Unable to obtain application socket's local addresses!");
         epeDelete(epe);
      }
   }

   epe->Policy  = poolPolicyNew(policy);
   if(epe->Policy == NULL) {
      puts("ERROR: Unable to create policy!");
      epeDelete(epe);
   }
   epe->Policy->Load   = load;
   epe->Policy->Weight = weight;

   epe->Identifier = getPoolElementIdentifier();
   epe->Element = poolElementNew(UNDEFINED_POOL_ELEMENT_HANDLE,
                                 epe->Identifier,
                                 epe->Policy);
   if(epe->Policy == NULL) {
      puts("ERROR: Unable to create pool element!");
      epeDelete(epe);
   }

   transportAddress = transportAddressNew(epe->Protocol,
                                          getPort((struct sockaddr*)&listenAddressArray[0]),
                                          listenAddressArray,
                                          listenAddresses);
   if(transportAddress == NULL) {
      puts("ERROR: Unable to create transport address!");
      epeDelete(epe);
   }
   poolElementAddTransportAddress(epe->Element,transportAddress);

   deleteAddressArray(listenAddressArray);
   listenAddressArray = NULL;

   epe->StateMachine = dispatcherNew(dispatcherDefaultLock,dispatcherDefaultUnlock,NULL);
   if(epe->StateMachine == NULL) {
      puts("ERROR: Unable to create dispatcher!");
      epeDelete(epe);
   }

   epe->ASAP = asapNew(epe->StateMachine);
   if(epe->ASAP == NULL) {
      puts("ERROR: Unable to create ASAP instance!");
      epeDelete(epe);
   }

   dispatcherAddFDCallback(epe->StateMachine, epe->ListenSocket,
                           FDCE_Read|FDCE_Exception,
                           epeSocketHandler, (void*)epe);

   epe->Handle = poolHandleNewASCII(poolName);
   if(epe->Handle == NULL) {
      puts("ERROR: Out of memory!");
      epeDelete(epe);
   }

   if(epe->Type == SOCK_DGRAM) {
      epe->ClientTable = NULL;
   }
   else {
      if(ext_listen(epe->ListenSocket,5) < 0) {
         perror("Unable to set application socket to listen mode");
         epeDelete(epe);
      }
      epe->ClientTable = g_hash_table_new(g_direct_hash,g_direct_equal);
      if(epe->ClientTable == NULL) {
         puts("ERROR: Out of memory!");
         epeDelete(epe);
      }
   }

   puts("************************************");
   poolHandlePrint(epe->Handle,stdout);
   puts("");
   poolElementPrint(epe->Element,stdout);
   puts("************************************");


   epe->RegistrationResult = asapRegister(epe->ASAP,
                                          epe->Handle,
                                          epe->Element);
   if(epe->RegistrationResult != ASAP_Okay) {
      printf("ERROR: Unable to register pool element: Error code #%d!\n",epe->RegistrationResult);
      epeDelete(epe);
   }

#ifndef FAST_BREAK
   installBreakDetector();
#endif
   return(epe);
}


static gboolean removeClient(gpointer key, gpointer value, gpointer userData)
{
   struct ExamplePoolElement* epe = (struct ExamplePoolElement*)userData;
   int                        fd  = (int)value;

   ext_close(fd);
   dispatcherRemoveFDCallback(epe->StateMachine,fd);

   printTimeStamp(stdout);
   printf("Removed client #%d.\n",fd);

   return(true);
}


void epeDelete(struct ExamplePoolElement* epe)
{
   if(epe) {
      if(epe->RegistrationResult == ASAP_Okay) {
         asapDeregister(epe->ASAP,
                        epe->Handle,
                        epe->Element->Identifier);
      }
      if(epe->ClientTable) {
         g_hash_table_foreach_remove(epe->ClientTable,removeClient,(gpointer)epe);
         g_hash_table_destroy(epe->ClientTable);
         epe->ClientTable = NULL;
      }
      if(epe->ASAP) {
         asapDelete(epe->ASAP);
         epe->ASAP = NULL;
      }
      if(epe->Element) {
         poolElementDelete(epe->Element);
         epe->Element = NULL;
      }
      if(epe->Handle) {
         poolHandleDelete(epe->Handle);
         epe->Handle = NULL;
      }
      if(epe->StateMachine) {
         dispatcherRemoveFDCallback(epe->StateMachine, epe->ListenSocket);
         dispatcherDelete(epe->StateMachine);
         epe->StateMachine = NULL;
      }
      if(epe->ListenSocket >= 0) {
         ext_close(epe->ListenSocket);
      }
      free(epe);
   }
   exit(0);
}


void epeSocketHandler(struct Dispatcher* dispatcher,
                      int                fd,
                      int                eventMask,
                      void*              userData)
{
   struct ExamplePoolElement* epe = (struct ExamplePoolElement*)userData;
   char                       buffer[16385];
   struct sockaddr_storage    address;
   socklen_t                  addressSize;
   ssize_t                    received;
   int                        sd;

   if(epe->ClientTable != NULL) {
      if(fd == epe->ListenSocket) {
         sd = ext_accept(epe->ListenSocket,NULL,0);
         if(sd >= 0) {
            dispatcherAddFDCallback(epe->StateMachine, sd,
                                    FDCE_Read|FDCE_Exception,
                                    epeSocketHandler, (void*)epe);
            g_hash_table_insert(epe->ClientTable,(gpointer)sd,(gpointer)sd);
            setNonBlocking(sd);

            printTimeStamp(stdout);
            printf("Added client #%d.\n",sd);
         }
      }
      else {
         if(g_hash_table_lookup(epe->ClientTable,(gpointer)fd) != NULL) {
            received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, 0,
                                    NULL, 0,
                                    NULL, NULL, NULL,
                                    0);
            do {
               if(received > 0) {

                  buffer[received] = 0x00;
                  printf("Echo> %s",buffer);

                  ext_send(fd,(char*)&buffer,received,0);
               }
               else {
                  dispatcherRemoveFDCallback(epe->StateMachine, fd);
                  g_hash_table_remove(epe->ClientTable,(gpointer)fd);
                  ext_close(fd);

                  printTimeStamp(stdout);
                  printf("Removed client #%d.\n",fd);
                  break;
               }
               received = recvfromplus(fd, (char*)&buffer, sizeof(buffer) - 1, 0,
                                       NULL, 0,
                                       NULL, NULL, NULL,
                                       0);
            } while(received >= 0);
         }
      }
   }

   else {
      received = ext_recvfrom(fd,
                              (char*)&buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&address, &addressSize);
      do {
         if(received > 0) {
            buffer[received] = 0x00;
            printf("Echo> %s",buffer);
            received = ext_sendto(fd,
                       (char*)&buffer, received, 0,
                       (struct sockaddr*)&address, addressSize);
         }
         received = ext_recvfrom(fd,
                                 (char*)&buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr*)&address, &addressSize);
      } while(received >= 0);
   }
}



int main(int argc, char** argv)
{
   struct ExamplePoolElement* epe;
   int                        n;
   int                        result;
   struct timeval             timeout;
   fd_set                     readfdset;
   fd_set                     writefdset;
   fd_set                     exceptfdset;
   card64                     start;
   card64                     stop;
   char*                      localAddressArray[MAX_LOCAL_ADDRESSES];
   size_t                     localAddresses;
   int                        protocol = IPPROTO_TCP;
   int                        type     = SOCK_STREAM;
   uint8_t                    policy   = PPT_WEIGHTED_ROUNDROBIN;
   uint32_t                   load     = 1;
   uint32_t                   weight   = 16384;

   if(argc < 2) {
      printf("Usage: %s [Pool Name] {{-local=address:port} ...} {-sctp} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
      exit(1);
   }

   start = getMicroTime();
   stop  = 0;
   localAddresses = 0;
   if(argc > 2) {
      for(n = 2;n < argc;n++) {
         if(!(strncmp(argv[n],"-local=",7))) {
            if(localAddresses < MAX_LOCAL_ADDRESSES) {
               localAddressArray[localAddresses] = (char*)&argv[n][7];
               localAddresses++;
            }
            else {
               puts("ERROR: Too many local addresses given!");
               exit(1);
            }
         }
         else if(!(strcmp(argv[n],"-sctp-udplike"))) {
            protocol = IPPROTO_SCTP;
            type     = SOCK_DGRAM;
         }
         else if(!(strcmp(argv[n],"-sctp-tcplike"))) {
            protocol = IPPROTO_SCTP;
            type     = SOCK_STREAM;
         }
         else if(!(strcmp(argv[n],"-sctp"))) {
            protocol = IPPROTO_SCTP;
            type     = SOCK_STREAM;
         }
         else if(!(strcmp(argv[n],"-udp"))) {
            protocol = IPPROTO_UDP;
            type     = SOCK_DGRAM;
         }
         else if(!(strcmp(argv[n],"-tcp"))) {
            protocol = IPPROTO_TCP;
            type     = SOCK_STREAM;
         }
         else if(!(strncmp(argv[n],"-load=",6))) {
            load = atol((char*)&argv[n][6]);
         }
         else if(!(strncmp(argv[n],"-weight=",8))) {
            weight = atol((char*)&argv[n][8]);
         }
         else if(!(strcmp(argv[n],"-rd"))) {
            policy = PPT_RANDOM;
         }
         else if(!(strcmp(argv[n],"-wrd"))) {
            policy = PPT_WEIGHTED_RANDOM;
         }
         else if(!(strcmp(argv[n],"-rr"))) {
            policy = PPT_ROUNDROBIN;
         }
         else if(!(strcmp(argv[n],"-wrr"))) {
            policy = PPT_WEIGHTED_ROUNDROBIN;
         }
         else if(!(strcmp(argv[n],"-lu"))) {
            policy = PPT_LEASTUSED;
         }
         else if(!(strcmp(argv[n],"-lud"))) {
            policy = PPT_LEASTUSED_DEGRADATION;
         }
         else if(!(strncmp(argv[n],"-stop=",6))) {
            stop = start + (card64)atol((char*)&argv[n][6]);
         }
         else if(!(strncmp(argv[n],"-log",4))) {
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
   if(localAddresses == 0) {
      puts("ERROR: No local addresses given!");
      exit(1);
   }
   beginLogging();

   epe = epeNew(argv[1],(const char**)&localAddressArray,localAddresses,type,protocol,policy,load,weight);
   if(epe) {

      puts("Example Pool Element - Version 1.0");
      puts("----------------------------------\n");

      while(!breakDetected()) {
         dispatcherGetSelectParameters(epe->StateMachine, &n, &readfdset, &writefdset, &exceptfdset, &timeout);
         timeout.tv_sec  = min(0,timeout.tv_sec);
         timeout.tv_usec = min(500000,timeout.tv_usec);
         result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
         if(errno == EINTR) {
            break;
         }
         if((stop > 0) && (getMicroTime() >= stop)) {
            puts("Auto-stop time reached -> exiting!");
            break;
         }
         if(result < 0) {
            perror("select() failed");
            epeDelete(epe);
         }
         dispatcherHandleSelectResult(epe->StateMachine, result, &readfdset, &writefdset, &exceptfdset);
      }

      epeDelete(epe);
      puts("\nTerminated!");
   }

   finishLogging();
   return(0);
}
