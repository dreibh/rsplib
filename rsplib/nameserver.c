/*
 *  $Id: nameserver.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Name Server
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "utilities.h"
#include "netutilities.h"
#include "dispatcher.h"
#include "timer.h"
#include "messagebuffer.h"
#include "rsplib-tags.h"

#include "pool.h"
#include "poolelement.h"
#include "poolnamespace.h"
#include "asapmessage.h"
#include "asapcreator.h"
#include "asapparser.h"
#include "asaperror.h"
#include "randomizer.h"
#include "breakdetector.h"

#include <ext_socket.h>
#include <net/if.h>
#include <sys/ioctl.h>



/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */


struct NameServerInterface
{
   int                      FD;
   int                      Protocol;
   struct TransportAddress* Address;
};


struct NameServerUser
{
   int                   FD;
   int                   Protocol;
   char                  Identifier[128];

   size_t                Registrations;
   struct NameServer*    Owner;
   GList*                PoolElementList;
   struct Timer*         KeepAliveTimer;
   struct Timer*         UserTimeoutTimer;
   struct MessageBuffer* Buffer;
};


struct NameServer
{
   GTree*                UserTree;
   GTree*                InterfaceTree;
   GList*                AnnounceList;
   struct PoolNamespace* Namespace;
   struct Dispatcher*    StateMachine;
   struct Timer*         AnnounceTimer;
   uint32_t              ServerID;

   card64                AnnounceInterval;
   card64                KeepAliveInterval;
   card64                KeepAliveTimeout;
   card64                MessageReceptionTimeout;

   int                   IPv4AnnounceSocket;
#ifdef HAVE_IPV6
   int                   IPv6AnnounceSocket;
#endif
};


#define NAMESERVER_DEFAULT_ANNOUNCE_INTERVAL          2000000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_INTERVAL        5000000
#define NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT        12000000
#define NAMESERVER_DEFAULT_MESSAGE_RECEPTION_TIMEOUT  3000000


struct NameServer* nameServerNew();

void nameServerDelete(struct NameServer* nameServer);

static void keepAliveTimerCallback(struct Dispatcher* dispatcher,
                                   struct Timer*      timer,
                                   void*              userData);

static void userTimeoutTimerCallback(struct Dispatcher* dispatcher,
                                     struct Timer*      timer,
                                     void*              userData);

static void announceTimerCallback(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData);

static void socketHandler(struct Dispatcher* dispatcher,
                                    int                fd,
                                    int                eventMask,
                                    void*              userData);



/* ###### Add announce address ################################################### */
static bool addAnnounceAddress(struct NameServer*             nameServer,
                               const struct sockaddr_storage* address)
{
   struct sockaddr_storage* copy = (struct sockaddr_storage*)memdup(address,sizeof(struct sockaddr_storage));
   if(copy) {
      nameServer->AnnounceList = g_list_append(nameServer->AnnounceList,copy);
      return(true);
   }
   return(false);
}


/* ###### Add interface ################################################### */
static bool addInterface(struct NameServer*       nameServer,
                         int                      interfaceFD,
                         struct TransportAddress* transportAddress)
{
   if(ext_listen(interfaceFD,10) < 0) {
      LOG_ERROR
      logerror("listen() call failed");
      LOG_END
      return(false);
   }

   if(nameServer != NULL) {
      struct NameServerInterface* interface =
         (struct NameServerInterface*)malloc(sizeof(struct NameServerInterface));
      if(interface != NULL) {
         interface->FD       = interfaceFD;
         interface->Address  = transportAddress;
         interface->Protocol = transportAddress->Protocol;

         g_tree_insert(nameServer->InterfaceTree,(gpointer)interface->FD,interface);

         dispatcherAddFDCallback(nameServer->StateMachine,
                                 interfaceFD,
                                 FDCE_Read|FDCE_Exception,
                                 socketHandler,
                                 (void*)nameServer);
         return(true);
      }
   }
   return(false);
}


/* ###### Remove interface ################################################## */
void removeInterface(struct NameServer* nameServer,
                     int                interfaceFD)
{
   struct NameServerInterface* interface =
      (struct NameServerInterface*)g_tree_lookup(nameServer->InterfaceTree,
                                                 (gpointer)interfaceFD);
   if(interface != NULL) {
      dispatcherRemoveFDCallback(nameServer->StateMachine, interfaceFD);
      if(interface->Address) {
         transportAddressDelete(interface->Address);
      }
      g_tree_remove(nameServer->InterfaceTree,(gpointer)interfaceFD);
      free(interface);
   }
}


/* ###### Remove pool element ############################################ */
static void removePoolElement(struct NameServer*     nameServer,
                              struct NameServerUser* user,
                              struct PoolElement*    poolElement)
{
   struct Pool* pool = poolElement->OwnerPool;

   user->PoolElementList = g_list_remove(user->PoolElementList,poolElement);
   poolRemovePoolElement(pool,poolElement);
   poolElementDelete(poolElement);
   poolElement = NULL;

   if(poolIsEmpty(pool)) {
      poolNamespaceRemovePool(nameServer->Namespace,pool);
      poolDelete(pool);
      pool = NULL;
   }
}


/* ###### Add user ######################################################## */
static void addUser(struct NameServer* nameServer,
                    int                fd,
                    int                protocol)
{
   struct sockaddr_storage name;
   socklen_t               namelen;

   struct NameServerUser* user = (struct NameServerUser*)malloc(sizeof(struct NameServerUser));
   if(user != NULL) {
      /* ====== Add user ================================================== */
      user->FD               = fd;
      user->Protocol         = protocol;
      user->PoolElementList  = NULL;
      user->Owner            = nameServer;
      user->Registrations    = 0;
      user->KeepAliveTimer   = timerNew(nameServer->StateMachine,keepAliveTimerCallback,(void*)user);
      user->UserTimeoutTimer = timerNew(nameServer->StateMachine,userTimeoutTimerCallback,(void*)user);
      user->Buffer           = messageBufferNew(65536);
      if((user->KeepAliveTimer != NULL) && (user->UserTimeoutTimer != NULL) && (user->Buffer != NULL)) {
         g_tree_insert(nameServer->UserTree, (gpointer)user->FD, user);
         dispatcherAddFDCallback(nameServer->StateMachine,
                                 fd, FDCE_Read|FDCE_Exception,
                                 socketHandler,
                                 (void*)nameServer);

         /* ====== Get information ========================================== */
         namelen = sizeof(name);
         if(ext_getpeername(fd, (struct sockaddr*)&name, &namelen) == 0) {
            if(address2string((struct sockaddr*)&name,
                              (char*)&user->Identifier,
                              sizeof(user->Identifier),
                              true) == false) {
               strcpy((char*)&user->Identifier,"(bad)");
            }
         }
         else {
            strcpy((char*)&user->Identifier,"?");
         }

         printTimeStamp(stdout);
         printf("Adding new user %s, socket #%d.\n", user->Identifier, user->FD);
      }
      else {
         free(user);
         ext_close(fd);
      }
   }
   else {
      ext_close(fd);
   }
}


/* ###### Remove user ################################################## */
static void removeUser(struct NameServer*     nameServer,
                       struct NameServerUser* user)
{
   GList*              list;
   struct PoolElement* poolElement;

   if(user != NULL) {
      printTimeStamp(stdout);
      printf("Removing user %s, socket #%d.\n", user->Identifier, user->FD);

      if(user->KeepAliveTimer) {
         timerDelete(user->KeepAliveTimer);
         user->KeepAliveTimer = NULL;
      }
      if(user->UserTimeoutTimer) {
         timerDelete(user->UserTimeoutTimer);
         user->UserTimeoutTimer = NULL;
      }

      list = g_list_first(user->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;

         printf("Pool element $%08x still registered in pool ",
                poolElement->Identifier);
         poolHandlePrint(poolElement->OwnerPool->Handle, stdout);
         puts(" -> removing");

         removePoolElement(nameServer,user,poolElement);
         list = g_list_first(user->PoolElementList);
      }

      if(user->Buffer) {
         messageBufferDelete(user->Buffer);
         user->Buffer = NULL;
      }

      dispatcherRemoveFDCallback(nameServer->StateMachine,user->FD);
      g_tree_remove(nameServer->UserTree,(gpointer)user->FD);
      ext_close(user->FD);
      free(user);
   }
}




/* ###### Constructor #################################################### */
struct NameServer* nameServerNew()
{
   struct NameServer* nameServer = (struct NameServer*)malloc(sizeof(struct NameServer));
   if(nameServer != NULL) {
      nameServer->KeepAliveInterval       = NAMESERVER_DEFAULT_KEEP_ALIVE_INTERVAL;
      nameServer->KeepAliveTimeout        = NAMESERVER_DEFAULT_KEEP_ALIVE_TIMEOUT;
      nameServer->AnnounceInterval        = NAMESERVER_DEFAULT_ANNOUNCE_INTERVAL;
      nameServer->MessageReceptionTimeout = NAMESERVER_DEFAULT_MESSAGE_RECEPTION_TIMEOUT;
      nameServer->AnnounceTimer      = NULL;
      nameServer->AnnounceList       = NULL;
      nameServer->ServerID           = random32();
      nameServer->UserTree           = g_tree_new(fdCompareFunc);
      nameServer->InterfaceTree      = g_tree_new(fdCompareFunc);
      nameServer->Namespace          = poolNamespaceNew();
      nameServer->StateMachine       = dispatcherNew(dispatcherDefaultLock,dispatcherDefaultUnlock,NULL);
      nameServer->IPv4AnnounceSocket = ext_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if(nameServer->IPv4AnnounceSocket >= 0) {
         setNonBlocking(nameServer->IPv4AnnounceSocket);
      }
#ifdef HAVE_IPV6
      nameServer->IPv6AnnounceSocket = ext_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      if(nameServer->IPv6AnnounceSocket >= 0) {
         setNonBlocking(nameServer->IPv6AnnounceSocket);
      }
#endif
      if((nameServer->UserTree      == NULL) ||
         (nameServer->InterfaceTree == NULL) ||
         (nameServer->Namespace     == NULL) ||
         (nameServer->StateMachine  == NULL)) {
         nameServerDelete(nameServer);
         nameServer = NULL;
      }
      nameServer->AnnounceTimer = timerNew(nameServer->StateMachine,
                                           announceTimerCallback,
                                           (void*)nameServer);
      if(nameServer->AnnounceTimer == NULL) {
         nameServerDelete(nameServer);
         nameServer = NULL;
      }
      timerStart(nameServer->AnnounceTimer,0);
   }
   return(nameServer);
}


/* ###### Destructor ###################################################### */
void nameServerDelete(struct NameServer* nameServer)
{
   gpointer key;
   gpointer value;

   if(nameServer) {
      if(nameServer->AnnounceTimer) {
         timerDelete(nameServer->AnnounceTimer);
         nameServer->AnnounceTimer = NULL;
      }
      if(nameServer->UserTree) {
         while(getFirstTreeElement(nameServer->UserTree,&key,&value)) {
            removeUser(nameServer,(struct NameServerUser*)value);
         }
         g_tree_destroy(nameServer->UserTree);
      }
      if(nameServer->InterfaceTree) {
         while(getFirstTreeElement(nameServer->InterfaceTree,&key,&value)) {
            removeInterface(nameServer,(int)key);
         }
         g_tree_destroy(nameServer->InterfaceTree);
      }
      if(nameServer->Namespace) {
         poolNamespaceDelete(nameServer->Namespace);
         nameServer->Namespace = NULL;
      }
      if(nameServer->IPv4AnnounceSocket >= 0) {
         ext_close(nameServer->IPv4AnnounceSocket);
         nameServer->IPv4AnnounceSocket = -1;
      }
#ifdef HAVE_IPV6
      if(nameServer->IPv6AnnounceSocket >= 0) {
         ext_close(nameServer->IPv6AnnounceSocket);
         nameServer->IPv6AnnounceSocket = -1;
      }
#endif
      if(nameServer->StateMachine) {
         dispatcherDelete(nameServer->StateMachine);
         nameServer->StateMachine = NULL;
      }
      free(nameServer);
   }
}


/* ###### Dump namespace ################################################# */
void nameServerDump(struct NameServer* nameServer)
{
   fputs("*************** Namespace Dump ***************\n",stdlog);
   printTimeStamp(stdlog);
   fputs("\n",stdlog);
   poolNamespacePrint(nameServer->Namespace,stdlog);
   fputs("**********************************************\n",stdlog);
}


/* ###### Main loop ###################################################### */
void nameServerMainLoop(struct NameServer* nameServer)
{
   if(nameServer) {
      for(;;) {
         dispatcherEventLoop(nameServer->StateMachine);
      }
   }
}


/* ###### Filter address array ########################################### */
static bool filterAddresses(struct PoolElement*            poolElement,
                            const struct sockaddr_storage* addressArray,
                            const size_t                   addresses)
{
   struct TransportAddress* transportAddress;
   GList*                   list;
   bool                     found;
   size_t                   i, j;
   bool                     valid;

   valid = false;
   list = poolElement->TransportAddressList;
   while(list != NULL) {
      transportAddress = (struct TransportAddress*)list->data;
      for(i = 0;i < transportAddress->Addresses;i++) {
         found = false;
         for(j = 0;j < addresses;j++) {
            if(addresscmp((const struct sockaddr*)&transportAddress->AddressArray[i],
                          (const struct sockaddr*)&addressArray[j],
                          false) == 0) {
               found = true;
               break;
            }
         }

         if(!found) {
            LOG_VERBOSE3
            fputs("Rejected address ",stdlog);
            fputaddress((struct sockaddr*)&transportAddress->AddressArray[i],
                        false, stdlog);
            fputs("\n",stdlog);
            LOG_END

            if(i + 1 < transportAddress->Addresses) {
               for(j = i;j < transportAddress->Addresses - 1;j++) {
                  transportAddress->AddressArray[j] = transportAddress->AddressArray[j + 1];
               }
               i--;
            }
            transportAddress->Addresses--;
         }
         else {
            LOG_VERBOSE2
            fputs("Accepted address ",stdlog);
            fputaddress((struct sockaddr*)&transportAddress->AddressArray[i],
                        false, stdlog);
            fputs("\n",stdlog);
            LOG_END
         }
      }

      list = g_list_next(list);
      if(transportAddress->Addresses < 1) {
         LOG_VERBOSE3
         fputs("Removed empty TransportAddress structure\n",stdlog);
         LOG_END
         poolElementRemoveTransportAddress(poolElement,transportAddress);
         transportAddressDelete(transportAddress);
      }
      else {
         valid = true;
      }
   }

   return(valid);
}


/* ###### Handle registration request #################################### */
static bool registrationRequest(struct NameServer*     nameServer,
                                struct NameServerUser* user,
                                int                    fd,
                                struct ASAPMessage*    message)
{
   struct PoolElement*      existing;
   struct Pool*             pool;
   bool                     poolAdded;
   struct sockaddr_storage* addressArray;
   int                      addresses;
   int                      i;
   bool                     addressesValid;

   LOG_ACTION
   fprintf(stdlog,"Registration Request: ");
   poolElementPrint(message->PoolElementPtr,stdlog);
   LOG_END


   message->Type       = AHT_REGISTRATION_RESPONSE;
   message->Error      = AEC_OKAY;
   message->Identifier = message->PoolElementPtr->Identifier;


   /* ====== Filter addresses ========================================= */
   addresses = getpaddrsplus(fd, 0, &addressArray);
   if(addresses > 0) {
      LOG_VERBOSE
      fputs("SCTP association local addresses:\n",stdlog);
      for(i = 0;i < addresses;i++) {
         fprintf(stdlog,"%d/%d: ",i + 1,addresses);
         fputaddress((struct sockaddr*)&addressArray[i],false,stdlog);
         fputs("\n",stdlog);
      }
      LOG_END

      addressesValid = filterAddresses(message->PoolElementPtr,addressArray,addresses);

      LOG_VERBOSE
      fputs("Filtered Registration Request: ",stdlog);
      poolElementPrint(message->PoolElementPtr,stdlog);
      LOG_END

      free(addressArray);
   }
   else {
      LOG_WARNING
      fputs("Cannot obtain peer addresses. Assuming addresses to be okay\n", stdlog);
      LOG_END
      addressesValid = true;
   }

   if(addressesValid == false) {
      LOG_ERROR
      fputs("No valid addresses for registration\n",stdlog);
      LOG_END
      message->Error = AEC_AUTHORIZATION_FAILURE;
      return(true);
   }


   /* ====== Check, if pool element already exists ======================= */
   existing = poolNamespaceFindPoolElement(
                  nameServer->Namespace,
                  message->PoolHandlePtr,
                  message->PoolElementPtr->Identifier);
   if(existing == NULL) {
      /* ====== Check, if pool already exists ============================ */
      pool = poolNamespaceFindPool(nameServer->Namespace,message->PoolHandlePtr);
      if(pool != NULL) {
         poolAdded = false;

         /* Try to adapt requested policy to pool's policy. */
         if(poolPolicyAdapt(message->PoolElementPtr->Policy,pool->Policy) == false) {
            LOG_ACTION
            fputs("Pool policy inconsistent -> Registration rejected.\n",stdlog);
            LOG_END
            message->Error = AEC_POLICY_INCONSISTENT;
            pool = NULL;
         }
      }

      /* ====== Pool does not exist -> add it. =========================== */
      else {
         poolAdded = true;

         /* Create pool. */
         pool = poolNew(message->PoolHandlePtr,message->PoolElementPtr->Policy);
         if(pool != NULL) {
            poolNamespaceAddPool(nameServer->Namespace,pool);
         }
         else {
            message->Error = AEC_OUT_OF_MEMORY;
            pool           = NULL;
         }
      }


      /* ====== Add pool element to pool ================================= */
      if(pool != NULL) {
         /* Take pool element from ASAPMessage structure. */
         message->PoolElementPtrAutoDelete = false;

         message->PoolElementPtr->HomeENRPServerID = nameServer->ServerID;
         message->PoolElementPtr->UserData         = (void*)user;
         poolAddPoolElement(pool,message->PoolElementPtr);
         user->PoolElementList = g_list_append(user->PoolElementList,message->PoolElementPtr);

         user->Registrations++;
         if(user->Registrations == 1) {
            LOG_ACTION
            fputs("Starting KeepAlive timers\n",stdlog);
            LOG_END
            timerStart(user->KeepAliveTimer,nameServer->KeepAliveInterval);
            timerStart(user->UserTimeoutTimer,nameServer->KeepAliveTimeout);
         }


         LOG_ACTION
         fputs("Registration successful!\n",stdlog);
         LOG_END
      }
   }


   /* ====== Pool element already exists -> try re-registration ========== */
   else {
      pool = existing->OwnerPool;

      /* Existing pool element has been created by another user.
         Probably non-unique PE-ID. */
      if(existing->UserData != user) {
         message->Error = AEC_NONUNIQUE_PE_ID;

         LOG_ACTION
         fputs("Non-unique pool element identifier -> Registration rejected.\n",stdlog);
         LOG_END
      }

      /* Re-registration */
      else {
         /* Try to adapt requested policy to pool's policy. */
         if(poolPolicyAdapt(message->PoolElementPtr->Policy,pool->Policy) == false) {
            message->Error = AEC_POLICY_INCONSISTENT;
            LOG_ACTION
            fputs("Pool policy for re-registration inconsistent -> Registration rejected.\n",stdlog);
            LOG_END
         }

         /* Update pool element's settings. */
         else {
            message->Error = poolElementAdapt(existing,message->PoolElementPtr);
            LOG_ACTION
            fputs("Re-registration successful!\n",stdlog);
            nameServerDump(nameServer);
            LOG_END
         }
      }
   }

   return(true);
}


/* ###### Handle deregistration request ################################## */
static bool deregistrationRequest(struct NameServer*      nameServer,
                                  struct NameServerUser*  user,
                                  int                     fd,
                                  struct ASAPMessage*     message)
{
   struct PoolElement* poolElement;

   LOG_ACTION
   fprintf(stdlog,"Deregistration Request for pool element $%08x of pool ",message->Identifier);
   poolHandlePrint(message->PoolHandlePtr, stdlog);
   fputs("\n",stdlog);
   LOG_END

   message->Type  = AHT_DEREGISTRATION_RESPONSE;
   message->Error = AEC_OKAY;


   /* ====== Check, if pool element still exists ========================= */
   poolElement = poolNamespaceFindPoolElement(
                    nameServer->Namespace, message->PoolHandlePtr, message->Identifier);
   if(poolElement != NULL) {

      /* ====== Pool element may only be removed by its creator ========== */
      if(poolElement->UserData == user) {
         message->PoolElementPtr = poolElementDuplicate(poolElement);
         removePoolElement(nameServer,user,poolElement);

         user->Registrations--;
         if(user->Registrations == 0) {
            LOG_ACTION
            fputs("Stopping KeepAlive timers\n",stdlog);
            LOG_END
            timerStop(user->KeepAliveTimer);
            timerStop(user->UserTimeoutTimer);
         }

         LOG_ACTION
         fputs("Deregistration successful!\n",stdlog);
         nameServerDump(nameServer);
         LOG_END
      }

      else {
         message->Error = AEC_AUTHORIZATION_FAILURE;
         LOG_ACTION
         fputs("Deregistration not by creator -> Deregistration rejected!\n",stdlog);
         nameServerDump(nameServer);
         LOG_END
      }
   }

   /* ====== Pool element does not exist -> okay. ======================== */
   else {
      LOG_ACTION
      fputs("Pool element to deregister is not in the namespace!\n",stdlog);
      LOG_END
   }

   return(true);
}


/* ###### Handle name resolution request ################################# */
static bool nameResolutionRequest(struct NameServer*  nameServer,
                                  struct NameServerUser*  user,
                                  int                 fd,
                                  struct ASAPMessage* message)
{
   struct Pool* pool;

   LOG_ACTION
   fprintf(stdlog,"Name Resolution Request: ");
   poolHandlePrint(message->PoolHandlePtr,stdlog);
   fputs("\n",stdlog);
   LOG_END

   pool = poolNamespaceFindPool(nameServer->Namespace,message->PoolHandlePtr);
   if(pool != NULL) {
      message->Type              = AHT_NAME_RESOLUTION_RESPONSE;
      message->Error             = 0;
      message->PoolPtrAutoDelete = false;
      message->PoolPtr           = pool;

      LOG_ACTION
      fprintf(stdlog,"Result: ");
      poolPrint(message->PoolPtr,stdlog);
      LOG_END
   }
   else {
      LOG_ACTION
      fputs("Pool is not in the namespace!\n",stdlog);
      LOG_END
      message->Type  = AHT_NAME_RESOLUTION_RESPONSE;
      message->Error = AEC_NOT_FOUND;
   }

   return(true);
}


/* ###### Handle endpoint keepalive acknowledgement ###################### */
static void endpointKeepAliveAck(struct NameServer*     nameServer,
                                 struct NameServerUser* user,
                                 int                    fd,
                                 struct ASAPMessage*    message)
{
   struct PoolElement* poolElement;

   LOG_VERBOSE
   fprintf(stdlog,"Endpoint KeepAliveAck for $%08x in ",message->Identifier);
   poolHandlePrint(message->PoolHandlePtr,stdlog);
   fputs("\n",stdlog);
   LOG_END

   poolElement = poolNamespaceFindPoolElement(nameServer->Namespace,
                                              message->PoolHandlePtr,
                                              message->Identifier);
   if(poolElement != NULL) {
      poolElement->TimeStamp            = getMicroTime();
      poolElement->OwnerPool->TimeStamp = poolElement->TimeStamp;

      timerRestart(user->UserTimeoutTimer,nameServer->KeepAliveTimeout);
   }
   else {
      LOG_VERBOSE
      fputs("Pool element not found!\n",stdlog);
      LOG_END
   }
}


/* ###### Handle events on unicast sockets ############################### */
static void eventHandler(struct NameServer*     nameServer,
                         struct NameServerUser* user,
                         int                    fd)
{
   struct ASAPMessage* message;
   ssize_t             received;
   bool                reply;

   LOG_ACTION
   fputs("Entering Unicast Message Handler\n",stdlog);
   LOG_END

   received = messageBufferRead(user->Buffer, fd,
                                (user->Protocol == IPPROTO_SCTP) ? PPID_ASAP : 0,
                                0, nameServer->MessageReceptionTimeout);
   if(received > 0) {
      message = asapPacket2Message((char*)&user->Buffer->Buffer, received, user->Buffer->Size);
      if(message != NULL) {
         reply = false;
         switch(message->Type) {
            case AHT_ENDPOINT_KEEP_ALIVE:
               puts("KeepAlive request received!");
             break;
            case AHT_ENDPOINT_KEEP_ALIVE_ACK:
               endpointKeepAliveAck(nameServer,user,fd,message);
             break;
            case AHT_NAME_RESOLUTION:
               reply = nameResolutionRequest(nameServer,user,fd,message);
             break;
            case AHT_NAME_RESOLUTION_RESPONSE:
             break;
            case AHT_REGISTRATION:
               reply = registrationRequest(nameServer,user,fd,message);
             break;
            case AHT_DEREGISTRATION:
               reply = deregistrationRequest(nameServer,user,fd,message);
             break;
            default:
               LOG_WARNING
               fprintf(stdlog,"Unsupported message type $%02x!\n",
                       message->Type);
               LOG_END
             break;
         }

         if(reply) {
            if(asapMessageSend(fd, 0, message) == false) {
               LOG_WARNING
               logerror("Sending reply failed. Closing connection");
               LOG_END
               removeUser(nameServer,user);
            }
         }

         asapMessageDelete(message);
      }
   }
   else if(received == RspRead_PartialRead) {
      LOG_VERBOSE2
      fputs("Partially read message\n", stdlog);
      LOG_END
   }
   else {
      LOG_WARNING
      fprintf(stdlog, "Error reading messasge: received=%d\n", received);
      LOG_END
      removeUser(nameServer,user);
   }

   LOG_ACTION
   fputs("Leaving Unicast Message Handler\n",stdlog);
   LOG_END
}


/* ###### Handle events on sockets ####################################### */
static void socketHandler(struct Dispatcher* dispatcher,
                          int                fd,
                          int                eventMask,
                          void*              userData)
{
   struct NameServer*          nameServer = (struct NameServer*)userData;
   struct NameServerUser*      user;
   struct NameServerInterface* interface;
   int                         newfd;

   user = (struct NameServerUser*)g_tree_lookup(nameServer->UserTree,(gpointer)fd);
   if(user != NULL) {
      eventHandler(nameServer,user,fd);
   }
   else {
      interface = (struct NameServerInterface*)g_tree_lookup(nameServer->InterfaceTree,(gpointer)fd);
      if(interface != NULL) {
         newfd = ext_accept(fd,NULL,0);
         if(newfd >= 0) {
            if(setNonBlocking(fd)) {
               addUser(nameServer, newfd, interface->Protocol);
            }
            else {
               ext_close(newfd);
            }
         }
      }
   }
}


/* ###### Send endpoint keepalive messages ############################### */
static void keepAliveTimerCallback(struct Dispatcher* dispatcher,
                                   struct Timer*      timer,
                                   void*              userData)
{
   struct NameServerUser* user       = (struct NameServerUser*)userData;
   struct NameServer*     nameServer = user->Owner;
   GList*                 list;
   struct PoolElement*    poolElement;
   struct ASAPMessage*    message;

   LOG_VERBOSE
   fprintf(stdlog,"KeepAlive-Timer for socket #%d.\n",user->FD);
   LOG_END

   message = asapMessageNew(NULL,1024);
   if(message != NULL) {
      list = g_list_first(user->PoolElementList);
      while(list != NULL) {
         poolElement = (struct PoolElement*)list->data;

         LOG_VERBOSE2
         fprintf(stdlog,"KeepAlive for pool element $%08x in pool ",poolElement->Identifier);
         poolHandlePrint(poolElement->OwnerPool->Handle,stdlog);
         fputs("\n",stdlog);
         LOG_END

         message->PoolElementPtr = poolElement;
         message->PoolHandlePtr  = poolElement->OwnerPool->Handle;
         message->Type           = AHT_ENDPOINT_KEEP_ALIVE;
         if(asapMessageSend(user->FD,0,message) == false) {
            LOG_ERROR
            fputs("Sending KeepAlive failed!\n",stdlog);
            LOG_END
            return;
         }

         asapMessageClearAll(message);

         list = g_list_next(list);
      }
      asapMessageDelete(message);
   }

   timerStart(timer,nameServer->KeepAliveInterval);
}


/* ###### Handle user timeout ########################################## */
static void userTimeoutTimerCallback(struct Dispatcher* dispatcher,
                                     struct Timer*      timer,
                                     void*              userData)
{
   struct NameServerUser* user       = (struct NameServerUser*)userData;
   struct NameServer*     nameServer = user->Owner;

   LOG_ACTION
   fprintf(stdlog,"Timeout-Timer for socket #%d -> removing user\n",user->FD);
   LOG_END

   removeUser(nameServer,user);
}


/* ###### Traverse function for interface list to collect addresses ####### */
static gint announceAddressListCollector(gpointer key,
                                         gpointer value,
                                         gpointer data)
{
   struct NameServerInterface* interface = (struct NameServerInterface*)value;
   GList**                     list      = (GList**)data;
   *list = g_list_append(*list,interface->Address);

   LOG_VERBOSE5
   fputs("Adding address ",stdlog);
   transportAddressPrint(interface->Address,stdlog);
   fputs("\n",stdlog);
   LOG_END
   return(false);
}


/* ###### Send announcement messages ###################################### */
static void announceTimerCallback(struct Dispatcher* dispatcher,
                                  struct Timer*      timer,
                                  void*              userData)
{
   struct NameServer*  nameServer = (struct NameServer*)userData;
   GList*              announceList;
   GList*              transportAddressList;
   struct ASAPMessage* message;
   size_t              messageLength;

   if(nameServer->AnnounceList) {
      transportAddressList = NULL;
      g_tree_traverse(nameServer->InterfaceTree,
                      announceAddressListCollector,
                      G_PRE_ORDER,
                      (gpointer)&transportAddressList);
      if(transportAddressList) {
         message = asapMessageNew(NULL,65536);
         if(message) {
             message->Type                    = AHT_SERVER_ANNOUNCE;
             message->TransportAddressListPtr = transportAddressList;
             messageLength = asapMessage2Packet(message);
             if(messageLength > 0) {

                announceList = g_list_first(nameServer->AnnounceList);
                while(announceList) {
                   switch(((struct sockaddr*)announceList->data)->sa_family) {
                      case AF_INET:
                          if(nameServer->IPv4AnnounceSocket) {
                             LOG_VERBOSE2
                             fputs("Sending announce via IPv4 socket to ",stdlog);
                             fputaddress((struct sockaddr*)announceList->data,true,stdlog);
                             fputs("\n",stdlog);
                             LOG_END
                             if(ext_sendto(nameServer->IPv4AnnounceSocket,
                                           message->Buffer,
                                           messageLength,
                                           0,
                                           (struct sockaddr*)announceList->data,
                                           getSocklen((struct sockaddr*)announceList->data)) < (ssize_t)messageLength) {
                                LOG_WARNING
                                logerror("Unable to send to IPv4 socket");
                                LOG_END
                             }
                          }
                       break;
#ifdef HAVE_IPV6
                      case AF_INET6:
                          if(nameServer->IPv6AnnounceSocket) {
                             LOG_VERBOSE2
                             fputs("Sending announce via IPv6 socket to ",stdlog);
                             fputaddress((struct sockaddr*)announceList->data,true,stdlog);
                             fputs("\n",stdlog);
                             LOG_END
                             if(ext_sendto(nameServer->IPv6AnnounceSocket,
                                           message->Buffer,
                                           messageLength,
                                           0,
                                           (struct sockaddr*)announceList->data,
                                           getSocklen((struct sockaddr*)announceList->data)) < (ssize_t)messageLength) {
                                LOG_WARNING
                                logerror("Unable to send to IPv6 socket");
                                LOG_END
                             }
                          }
                       break;
#endif
                   }
                   announceList = g_list_next(announceList);
                }
             }
             else {
                LOG_ERROR
                fputs("Unable to create ServerAnnounce message\n",stdlog);
                LOG_END
             }
            asapMessageDelete(message);
         }
         g_list_free(transportAddressList);
      }
   }
   timerStart(timer,nameServer->AnnounceInterval);
}



/* ###### Main program ################################################### */
#define MAX_ADDRESSES 128

int main(int argc, char** argv)
{
   struct NameServer*       nameServer;
   struct TransportAddress* transportAddress;
   bool                     hasSCTPInterface = false;
   bool                     autoSCTP         = false;
   size_t                   localAddresses;
   struct sockaddr_storage  localAddressArray[MAX_ADDRESSES];
   int                      result;
   struct timeval           timeout;
   fd_set                   readfdset;
   fd_set                   writefdset;
   fd_set                   exceptfdset;
   fd_set                   testfdset;
   card64                   testTS;
   int                      sockFD;
   int                      i;
   int                      n;
   char*                    address=NULL;
   char*                    idx;
   unsigned int             interfaces;


   nameServer = nameServerNew();
   if(nameServer == NULL) {
      puts("ERROR: Out of memory!");
      exit(1);
   }

   puts("The rsplib Name Server - Version 2.0");
   puts("------------------------------------\n");

   interfaces = 0;
   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i],"-tcp=",5))) {
         address = (char*)&argv[i][5];
         if(!string2address(address,&localAddressArray[0])) {
            printf("ERROR: Bad local address %s! Use format <address:port>.\n",address);
            exit(1);
         }
         transportAddress = transportAddressNew(IPPROTO_TCP,
                                                getPort((struct sockaddr*)&localAddressArray[0]),
                                                (struct sockaddr_storage*)&localAddressArray,
                                                1);
         if(transportAddress == NULL) {
            puts("ERROR: Unable to handle socket address(es)!");
            exit(1);
         }
         printf("Adding TCP interface ");
         transportAddressPrint(transportAddress,stdout);
         puts("");

         sockFD = ext_socket(((struct sockaddr*)&localAddressArray[0])->sa_family,
                             SOCK_STREAM,
                             IPPROTO_TCP);
         if(sockFD >= 0) {
            if(ext_bind(sockFD,
                        (struct sockaddr*)&localAddressArray,
                        getSocklen((struct sockaddr*)&localAddressArray)) != 0) {
               perror("ERROR: Unable to bind socket");
               printf("Used local address %s.\n",address);
               exit(1);
            }
            if(!addInterface(nameServer, sockFD, transportAddress)) {
               printf("ERROR: Unable to use local address %s.\n",address);
               exit(1);
            }
            interfaces++;
         }
         else {
            puts("ERROR: Unable to create TCP socket!");
         }
      }
      else if(!(strncasecmp(argv[i],"-sctp=",6))) {
         localAddresses = 0;
         address = (char*)&argv[i][6];
         while(localAddresses < MAX_ADDRESSES) {
            idx = index(address,',');
            if(idx) {
               *idx = 0x00;
            }
            if(!string2address(address,&localAddressArray[localAddresses])) {
               printf("ERROR: Bad local address %s! Use format <address:port>.\n",address);
               exit(1);
            }
            localAddresses++;
            if(idx) {
               address = idx;
               address++;
            }
            else {
               break;
            }
         }
         if(localAddresses < 1) {
            puts("ERROR: At least one SCTP address must be specified!\n");
            exit(1);
         }
         transportAddress = transportAddressNew(IPPROTO_SCTP,
                                                getPort((struct sockaddr*)&localAddressArray[0]),
                                                (struct sockaddr_storage*)&localAddressArray,
                                                localAddresses);
         if(transportAddress == NULL) {
            puts("ERROR: Unable to handle socket address(es)!");
            exit(1);
         }
         printf("Adding SCTP interface ");
         transportAddressPrint(transportAddress,stdout);
         puts("");

         sockFD = ext_socket(AF_INET,SOCK_STREAM,IPPROTO_SCTP);
         if(sockFD >= 0) {
            if(bindplus(sockFD,
                        (struct sockaddr_storage*)&localAddressArray,
                        localAddresses) == false) {
               perror("ERROR: Unable to bind socket");
               exit(1);
            }
            if(!addInterface(nameServer, sockFD, transportAddress)) {
               printf("ERROR: Unable to use local address %s.\n",address);
               exit(1);
            }
            interfaces++;
            hasSCTPInterface = true;
         }
         else {
            puts("ERROR: Unable to create SCTP socket!");
         }
      }
      else if(!(strncasecmp(argv[i],"-announce=",10))) {
         address = (char*)&argv[i][10];
         if(!string2address(address,&localAddressArray[0])) {
            printf("ERROR: Bad local address %s! Use format <address:port>.\n",address);
            exit(1);
         }
         addAnnounceAddress(nameServer,(struct sockaddr_storage*)&localAddressArray[0]);
      }
      else if(!(strcmp(argv[i],"-autosctp"))) {
         autoSCTP = true;
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else {
         printf("Usage: %s {-autosctp} {-sctp=address:port{,address}...} {{[-tcp=address:port}} {[-announce=address:port}]} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n",argv[0]);
         exit(1);
      }
   }
   if((!hasSCTPInterface) && (autoSCTP)) {
      struct sockaddr_storage* socketAddressArray;
      sockFD = ext_socket(checkIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_SCTP);
      if(sockFD >= 0) {
         if(bindplus(sockFD, NULL, 0) == false) {
            puts("ERROR: Unable to bind SCTP socket!");
            exit(1);
         }
         localAddresses = getladdrsplus(sockFD, 0, (struct sockaddr_storage**)&socketAddressArray);
         if(localAddresses < 1) {
            puts("ERROR: Unable to find out local addresses!");
            exit(1);
         }
         transportAddress = transportAddressNew(IPPROTO_SCTP, PORT_ASAP,
                                                socketAddressArray, localAddresses);
         if(transportAddress == NULL) {
            puts("ERROR: Unable to handle socket address(es)!");
            exit(1);
         }
         printf("Adding SCTP interface ");
         transportAddressPrint(transportAddress,stdout);
         puts("");
         if(!addInterface(nameServer, sockFD, transportAddress)) {
            printf("ERROR: Unable to use local address %s.\n",address);
            exit(1);
         }
         interfaces++;
         hasSCTPInterface = true;
         free(socketAddressArray);
      }
      else {
         puts("ERROR: Unable to create SCTP socket!");
         exit(1);
      }
   }
   if(interfaces == 0) {
      puts("ERROR: No interfaces given. Exiting!");
      exit(1);
   }
   puts("\n");
#ifndef FAST_BREAK
   installBreakDetector();
#endif


   beginLogging();


   // ====== Main loop =======================================================
   while(!breakDetected()) {
      dispatcherGetSelectParameters(nameServer->StateMachine, &n, &readfdset, &writefdset, &exceptfdset, &testfdset, &testTS, &timeout);
      timeout.tv_sec  = min(0,timeout.tv_sec);
      timeout.tv_usec = min(250000,timeout.tv_usec);
      result = ext_select(n, &readfdset, &writefdset, &exceptfdset, &timeout);
      dispatcherHandleSelectResult(nameServer->StateMachine, result, &readfdset, &writefdset, &exceptfdset, &testfdset, testTS);
      if(errno == EINTR) {
         break;
      }
      if(result < 0) {
         perror("select() failed");
         break;
      }
   }


   // ====== Clean up ========================================================
   nameServerDelete(nameServer);
   finishLogging();
   puts("\nTerminated!");
   return(0);
}
