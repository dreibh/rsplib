/*
 *  $Id: asapmessage.c,v 1.5 2004/07/20 08:47:38 dreibh Exp $
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
 * Purpose: ASAP Message
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "asapmessage.h"
#include "asapcreator.h"
#include "asapparser.h"

#include <glib.h>
#include <ext_socket.h>



/* ###### Constructor #################################################### */
struct ASAPMessage* asapMessageNew(char* buffer, const size_t bufferSize)
{
   struct ASAPMessage* message;

   if(buffer == NULL) {
      message = (struct ASAPMessage*)malloc(sizeof(struct ASAPMessage) + bufferSize);
      if(message != NULL) {
         memset(message,0,sizeof(struct ASAPMessage));
         message->Buffer             = (char*)((long)message + (long)sizeof(struct ASAPMessage));
         message->BufferSize         = bufferSize;
         message->OriginalBufferSize = bufferSize;
      }
   }
   else {
      message = (struct ASAPMessage*)malloc(sizeof(struct ASAPMessage));
      if(message != NULL) {
         memset(message,0,sizeof(struct ASAPMessage));
         message->Buffer             = buffer;
         message->BufferSize         = bufferSize;
         message->OriginalBufferSize = bufferSize;
      }
   }

   return(message);
}


/* ###### Destructor ##################################################### */
void asapMessageDelete(struct ASAPMessage* message)
{
   if(message != NULL) {
      asapMessageClearAll(message);
      if((message->BufferAutoDelete) && (message->Buffer)) {
         free(message->Buffer);
      }
      message->Buffer     = NULL;
      message->BufferSize = 0;
      free(message);
   }
}


/* ###### Clear ASAPMessage ############################################## */
void asapMessageClearAll(struct ASAPMessage* message)
{
   struct TransportAddressBlock* transportAddressBlock;
   struct TransportAddressBlock* nextTransportAddressBlock;
   char*                         buffer;
   size_t                        originalBufferSize;
   bool                          bufferAutoDelete;
   size_t                        i;

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
               free(message->PoolElementPtrArray[i]);
               message->PoolElementPtrArray[i] = NULL;
            }
         }
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
      memset(message,0,sizeof(struct ASAPMessage));
      message->BufferAutoDelete   = bufferAutoDelete;
      message->OriginalBufferSize = originalBufferSize;
      message->BufferSize         = originalBufferSize;
      message->Buffer             = buffer;
   }
}


/* ###### Reset buffer size ############################################## */
void asapMessageClearBuffer(struct ASAPMessage* message)
{
   if(message != NULL) {
      message->BufferSize = message->OriginalBufferSize;
      message->Position   = 0;
   }
}


/* ###### Peek message size from socket ################################## */
static size_t asapMessagePeekSize(int fd, const card64 timeout)
{
   struct asap_header header;
   ssize_t            received;
   size_t             asapLength;

   received = recvfromplus(fd,
                           (char*)&header, sizeof(header), MSG_PEEK, NULL, 0,
                           NULL, NULL, NULL,
                           timeout);
   if(received == sizeof(header)) {
      asapLength = ntohs(header.ah_length);
      LOG_VERBOSE3
      fprintf(stdlog,"Incoming ASAP message: type=$%02x flags=$%02x length=%u\n",
              header.ah_type, header.ah_flags, (unsigned int)asapLength);
      LOG_END
      return(asapLength);
   }

   return(0);
}


/* ###### Send ASAPMessage ############################################### */
bool asapMessageSend(int                 fd,
                     sctp_assoc_t        assocID,
                     const card64        timeout,
                     struct ASAPMessage* message)
{
   size_t  messageLength;
   ssize_t sent;

   messageLength = asapMessage2Packet(message);
   if(messageLength > 0) {
      sent = sendtoplus(fd,
                        message->Buffer, messageLength, 0, NULL, 0,
                        message->PPID,
                        assocID,
                        0, 0, timeout);
      if(sent == (ssize_t)messageLength) {
         LOG_ACTION
         fprintf(stdlog, "Successfully sent ASAP message: "
                 "AssocID=%u PPID=$%08x, Type=$%02x\n",
                 (unsigned int)assocID,
                 message->PPID,
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


/* ###### Receive ASAPMessage ############################################ */
struct ASAPMessage* asapMessageReceive(int              fd,
                                       const card64     peekTimeout,
                                       const card64     totalTimeout,
                                       const size_t     minBufferSize,
                                       struct sockaddr* senderAddress,
                                       socklen_t*       senderAddressLength)
{
   struct ASAPMessage* message;
   char*               buffer;
   size_t              length;
   ssize_t             received;
   uint32_t            ppid;
   sctp_assoc_t        assocID;
   unsigned short      streamID;
   card64              peekStart;
   card64              peekTime;
   card64              timeout;

   peekStart = getMicroTime();
   length = asapMessagePeekSize(fd,peekTimeout);
   if(length > 0) {
      peekTime = getMicroTime() - peekStart;
      buffer = (char*)malloc(max(length, minBufferSize));
      if(buffer != NULL) {
         timeout = totalTimeout;
         if(timeout < peekTime) {
            timeout = 0;
         }
         else {
            timeout -= peekTime;
         }
         received = recvfromplus(fd,
                                 buffer, length, 0,
                                 senderAddress, senderAddressLength,
                                 &ppid, &assocID, &streamID,
                                 timeout);
         if(received == (ssize_t)length) {
            message = asapPacket2Message(buffer, length, max(length, minBufferSize));
            if(message != NULL) {
               message->BufferAutoDelete = true;
               message->PPID             = ppid;
               message->AssocID          = assocID;
               message->StreamID         = streamID;

               LOG_ACTION
               fprintf(stdlog,"Successfully received ASAP message\n"
                       "PPID=$%08x AssocID=%d StreamID=%d, ASAP Type = $%02x\n",
                       ppid, (unsigned int)assocID, streamID,
                       message->Type);
               LOG_END
               return(message);
            }
            else {
               LOG_WARNING
               fputs("Received bad packet\n",stdlog);
               LOG_END
            }
         }
         else {
            LOG_WARNING
            fprintf(stdlog,"Read error, read=%d expected=%u\n!",
                    received, (unsigned int)length);
            LOG_END
         }
         free(buffer);
      }
      else {
         LOG_WARNING
         fputs("Response too long\n",stdlog);
         LOG_END
      }
   }
   return(NULL);
}


/* ###### Try to get space in ASAPMessage's buffer ####################### */
void* getSpace(struct ASAPMessage* message,
               const size_t        headerSize)
{
   void* header;

   if(message->Position + headerSize <= message->BufferSize) {
      header = (void*)&message->Buffer[message->Position];
      message->Position += headerSize;
      return(header);
   }

   LOG_VERBOSE2
   fprintf(stdlog,
           "Buffer size too low!\np=%u + h=%u > size=%u\n",
           (unsigned int)message->Position, (unsigned int)headerSize, (unsigned int)message->BufferSize);
   LOG_END_FATAL
   return(NULL);
}
