/*
 *  $Id: rserpoolmessage.c,v 1.12 2004/11/13 03:24:13 dreibh Exp $
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
 * Purpose: RSerPool Message Definitions
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rserpoolmessage.h"
#include "rserpoolmessagecreator.h"
#include "rserpoolmessageparser.h"

#include <ext_socket.h>


/* ###### Constructor #################################################### */
struct RSerPoolMessage* rserpoolMessageNew(char* buffer, const size_t bufferSize)
{
   struct RSerPoolMessage* message;

   if(buffer == NULL) {
      message = (struct RSerPoolMessage*)malloc(sizeof(struct RSerPoolMessage) + bufferSize);
      if(message != NULL) {
         memset(message, 0, sizeof(struct RSerPoolMessage));
         message->Buffer             = (char*)((long)message + (long)sizeof(struct RSerPoolMessage));
         message->BufferSize         = bufferSize;
         message->OriginalBufferSize = bufferSize;
      }
   }
   else {
      message = (struct RSerPoolMessage*)malloc(sizeof(struct RSerPoolMessage));
      if(message != NULL) {
         memset(message, 0, sizeof(struct RSerPoolMessage));
         message->Buffer             = buffer;
         message->BufferSize         = bufferSize;
         message->OriginalBufferSize = bufferSize;
      }
   }

   return(message);
}


/* ###### Destructor ##################################################### */
void rserpoolMessageDelete(struct RSerPoolMessage* message)
{
   if(message != NULL) {
      rserpoolMessageClearAll(message);
      if((message->BufferAutoDelete) && (message->Buffer)) {
         free(message->Buffer);
      }
      message->Buffer     = NULL;
      message->BufferSize = 0;
      free(message);
   }
}


/* ###### Clear RSerPoolMessage ########################################## */
void rserpoolMessageClearAll(struct RSerPoolMessage* message)
{
   struct TransportAddressBlock*  transportAddressBlock;
   struct TransportAddressBlock*  nextTransportAddressBlock;
   struct ST_CLASS(PeerListNode)* peerListNode;
   char*                          buffer;
   size_t                         originalBufferSize;
   bool                           bufferAutoDelete;
   size_t                         i;

   if(message != NULL) {
      if((message->PoolElementPtr) && (message->PoolElementPtrAutoDelete)) {
         ST_CLASS(poolElementNodeDelete)(message->PoolElementPtr);
         free(message->PoolElementPtr);
      }
      if((message->CookiePtr) && (message->CookiePtrAutoDelete)) {
         free(message->CookiePtr);
      }
      if(message->TransportAddressBlockListPtrAutoDelete) {
         transportAddressBlock = message->TransportAddressBlockListPtr;
         while(transportAddressBlock) {
            nextTransportAddressBlock = transportAddressBlock->Next;
            transportAddressBlockDelete(transportAddressBlock);
            free(transportAddressBlock);
            transportAddressBlock = nextTransportAddressBlock;
         }
      }
      message->TransportAddressBlockListPtr = NULL;
      if(message->PoolElementPtrArrayAutoDelete) {
         CHECK(message->PoolElementPtrArraySize < MAX_MAX_NAME_RESOLUTION_ITEMS);
         for(i = 0;i < message->PoolElementPtrArraySize;i++) {
            if(message->PoolElementPtrArray[i]) {
               ST_CLASS(poolElementNodeDelete)(message->PoolElementPtrArray[i]);
               transportAddressBlockDelete(message->PoolElementPtrArray[i]->UserTransport);
               free(message->PoolElementPtrArray[i]->UserTransport);
               free(message->PoolElementPtrArray[i]);
               message->PoolElementPtrArray[i] = NULL;
            }
         }
      }
      if((message->PeerListNodePtrAutoDelete) && (message->PeerListNodePtr)) {
         ST_CLASS(peerListNodeDelete)(message->PeerListNodePtr);
         free(message->PeerListNodePtr);
      }
      if((message->PeerListPtrAutoDelete) && (message->PeerListPtr)) {
         peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                           message->PeerListPtr);
         while(peerListNode != NULL) {
            ST_CLASS(peerListRemovePeerListNode)(message->PeerListPtr, peerListNode);
            ST_CLASS(peerListNodeDelete)(peerListNode);
            transportAddressBlockDelete(peerListNode->AddressBlock);
            free(peerListNode->AddressBlock);
            free(message->PeerListNodePtr);
            peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                              message->PeerListPtr);
         }
      }
      if((message->NamespacePtrAutoDelete) && (message->NamespacePtr)) {
         ST_CLASS(poolNamespaceManagementClear)(message->NamespacePtr);
         ST_CLASS(poolNamespaceManagementDelete)(message->NamespacePtr);
         free(message->NamespacePtr);
         message->NamespacePtr = NULL;
      }
      if((message->OffendingParameterTLV) && (message->OffendingParameterTLVAutoDelete)) {
         free(message->OffendingParameterTLV);
      }
      if((message->OffendingMessageTLV) && (message->OffendingMessageTLVAutoDelete)) {
         free(message->OffendingMessageTLV);
      }

      buffer             = message->Buffer;
      originalBufferSize = message->OriginalBufferSize;
      bufferAutoDelete   = message->BufferAutoDelete;
      memset(message,0,sizeof(struct RSerPoolMessage));
      message->BufferAutoDelete   = bufferAutoDelete;
      message->OriginalBufferSize = originalBufferSize;
      message->BufferSize         = originalBufferSize;
      message->Buffer             = buffer;
   }
}


/* ###### Reset buffer size ############################################## */
void rserpoolMessageClearBuffer(struct RSerPoolMessage* message)
{
   if(message != NULL) {
      message->BufferSize = message->OriginalBufferSize;
      message->Position   = 0;
   }
}


/* ###### Send RSerPoolMessage ########################################### */
bool rserpoolMessageSend(int                     protocol,
                         int                     fd,
                         sctp_assoc_t            assocID,
                         int                     flags,
                         const card64            timeout,
                         struct RSerPoolMessage* message)
{
   size_t   messageLength;
   ssize_t  sent;
   uint32_t myPPID;

   messageLength = rserpoolMessage2Packet(message);
   if(messageLength > 0) {
      myPPID = (protocol == IPPROTO_SCTP) ? message->PPID : 0;
      sent = sendtoplus(fd,
                        message->Buffer, messageLength, flags,
                        message->AddressArray, message->Addresses,
                        myPPID,
                        assocID,
                        0, 0, timeout);
      if(sent == (ssize_t)messageLength) {
         LOG_VERBOSE2
         fprintf(stdlog, "Successfully sent ASAP message: "
                 "assoc=%u PPID=$%08x, Type=$%02x\n",
                 (unsigned int)assocID,
                 myPPID,
                 message->Type);
         LOG_END
         return(true);
      }
      LOG_ERROR
      logerror("Write error");
      LOG_END
   }
   else {
      LOG_ERROR
      fputs("Unable to create packet for message\n",stdlog);
      LOG_END
   }
   return(false);
}


/* ###### Try to get space in RSerPoolMessage's buffer ####################### */
void* getSpace(struct RSerPoolMessage* message,
               const size_t            headerSize)
{
   void* header;

   if(message->Position + headerSize <= message->BufferSize) {
      header = (void*)&message->Buffer[message->Position];
      message->Position += headerSize;
      return(header);
   }

   if(message->Position == message->BufferSize) {
      LOG_VERBOSE4
      fputs("End of message\n", stdlog);
      LOG_END
   }
   else {
      LOG_VERBOSE3
      fprintf(stdlog,
            "Buffer size too low!\np=%u + h=%u > size=%u\n",
            (unsigned int)message->Position, (unsigned int)headerSize,
            (unsigned int)message->BufferSize);
      LOG_END
   }

   return(NULL);
}
