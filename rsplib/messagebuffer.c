/*
 *  $Id: messagebuffer.c,v 1.5 2004/11/11 22:44:20 dreibh Exp $
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
 * Purpose: Message Buffer
 *
 */

#include "tdtypes.h"
#include "messagebuffer.h"
#include "netutilities.h"
#include "loglevel.h"

#include "rsplib-tags.h"


/* ###### Constructor ##################################################### */
struct MessageBuffer* messageBufferNew(const size_t size)
{
   struct MessageBuffer* mb = (struct MessageBuffer*)malloc(sizeof(struct MessageBuffer) + size);
   if(mb) {
      mb->Size     = size;
      mb->Position = 0;
      mb->ToGo     = 0;
   }
   return(mb);
}


/* ###### Destructor ###################################################### */
void messageBufferDelete(struct MessageBuffer* mb)
{
   if(mb) {
      mb->Size   = 0;
      free(mb);
   }
}


/* ###### Read from buffer ################################################ */
ssize_t messageBufferRead(struct MessageBuffer* mb,
                          int                   sd,
                          const uint32_t        requiredPPID,
                          const card64          peekTimeout,
                          const card64          totalTimeout)
{
   struct TLVHeader header;
   ssize_t          received=0;
   size_t           tlvLength;
   uint32_t         ppid;
   sctp_assoc_t     assocID;
   unsigned short   streamID;
   int64            timeout;
   int              flags;

   if(mb->Position == 0) {
      LOG_VERBOSE4
      fprintf(stdlog, "Trying to peek TLV header from socket %d, peek timeout %lld [µs], total timeout %lld [µs]\n",
              sd, peekTimeout, totalTimeout);
      LOG_END
      mb->StartTimeStamp = getMicroTime();
      flags = MSG_PEEK;
      received = recvfromplus(sd, (char*)&header, sizeof(header), &flags,
                              NULL, 0, &ppid, &assocID, &streamID, peekTimeout);
      if(received > 0) {
         if(ppid == requiredPPID) {
            if(received == sizeof(header)) {
               tlvLength = (size_t)ntohs(header.Length);
               if(tlvLength < mb->Size) {
                  mb->ToGo = tlvLength;
               }
               else {
                  LOG_ERROR
                  fprintf(stdlog, "Message buffer size %d of socket %d is too small to read TLV of length %d\n",
                          (int)mb->Size, sd, (int)tlvLength);
                  LOG_END
                  return(RspRead_TooBig);
               }
            }
         }
         else {
            LOG_VERBOSE4
            fprintf(stdlog, "Data on socket %d has PPID $%08x, but required is $%08x\n",
                    sd, ppid, requiredPPID);
            LOG_END
            return(RspRead_WrongPPID);
         }
      }
      else if(errno == EAGAIN) {
         LOG_VERBOSE3
         fputs("Timeout while trying to read data\n", stdlog);
         LOG_END
         return(RspRead_Timeout);
      }
      else {
         LOG_VERBOSE3
         logerror("Unable to read data");
         LOG_END
         return(RspRead_ReadError);
      }
   }

   if(mb->ToGo > 0) {
      timeout = (int64)totalTimeout - ((int64)getMicroTime() - (int64)mb->StartTimeStamp);
      if(timeout < 0) {
         timeout = 0;
      }
      LOG_VERBOSE4
      fprintf(stdlog, "Trying to read remaining %d bytes from message of length %d from socket %d, timeout %lld [µs]\n",
              (int)mb->ToGo, (int)(mb->Position + mb->ToGo), sd, timeout);
      LOG_END
      flags    = 0;
      received = recvfromplus(sd, (char*)&mb->Buffer[mb->Position], mb->ToGo, &flags,
                              NULL, 0, &ppid, &assocID, &streamID, (card64)timeout);
      if(received > 0) {
         mb->ToGo     -= received;
         mb->Position += received;
         LOG_VERBOSE4
         fprintf(stdlog, "Successfully read %d bytes from socket %d, %d bytes to go\n",
                 received, sd, (int)mb->ToGo);
         LOG_END
         if(mb->ToGo == 0) {
            LOG_VERBOSE3
            fprintf(stdlog, "Successfully read message of %d bytes from socket %d\n",
                    (int)mb->Position, sd);
            LOG_END
            received      = mb->Position;
            mb->Position = 0;
         }
         else {
            return(RspRead_PartialRead);
         }
      }
   }

   if(received < 0) {
      received = RspRead_ReadError;
   }
   return(received);
}
