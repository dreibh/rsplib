/*
 *  $Id: asapparser.c,v 1.6 2004/07/20 08:47:38 dreibh Exp $
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
 * Purpose: ASAP Parser
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "asapparser.h"

#include <netinet/in.h>
#include <ext_socket.h>


/* ###### Scan next TLV header ############################################## */
static bool getNextTLV(struct ASAPMessage*      message,
                       size_t*                  tlvPosition,
                       struct asap_tlv_header** header,
                       uint16_t*                tlvType,
                       size_t*                  tlvLength)
{
   *tlvPosition = message->Position;
   message->OffendingParameterTLV       = (char*)&message->Buffer[*tlvPosition];
   message->OffendingParameterTLVLength = 0;

   *tlvPosition = message->Position;
   *header = (struct asap_tlv_header*)getSpace(message, sizeof(struct asap_tlv_header));
   if(*header == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   *tlvType   = ntohs((*header)->atlv_type);
   *tlvLength = (size_t)ntohs((*header)->atlv_length);

   LOG_VERBOSE5
   fprintf(stdlog, "TLV: Type $%04x, length %u at position %u\n",
           *tlvType, (unsigned int)*tlvLength, (unsigned int)message->Position);
   LOG_END

   if(message->Position + *tlvLength - sizeof(struct asap_tlv_header) > message->BufferSize) {
      LOG_WARNING
      fprintf(stdlog, "TLV length exceeds message size!\n"
             "p=%u + l=%u > size=%u   type=$%02x\n",
             (unsigned int)message->Position, (unsigned int)*tlvLength, (unsigned int)message->BufferSize, *tlvType);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   if(*tlvLength < sizeof(struct asap_tlv_header)) {
      LOG_WARNING
      fputs("TLV length too low!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   message->OffendingParameterTLVLength = *tlvLength;
   return(*tlvLength > 0);
}


/* ###### Handle unknown TLV ################################################ */
static bool handleUnknownTLV(struct ASAPMessage* message,
                             const uint16_t      tlvType,
                             const size_t        tlvLength)
{
   void* ptr;

   if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_CONTINUE) {
      ptr = getSpace(message, tlvLength - sizeof(struct asap_tlv_header));
      if(ptr != NULL) {
         LOG_VERBOSE3
         fprintf(stdlog, "Silently skipping type $%02x at position %u\n",
                 tlvType, (unsigned int)message->Position);
         LOG_END
         return(true);
      }
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_CONTINUE_AND_REPORT) {
      ptr = getSpace(message, tlvLength - sizeof(struct asap_tlv_header));
      if(ptr != NULL) {
         LOG_VERBOSE3
         fprintf(stdlog, "Skipping type $%02x at position %u\n",
                 tlvType, (unsigned int)message->Position);
         LOG_END

         /*
            Implement Me: Build error table!
         */

         return(true);
      }
      return(false);
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_STOP) {
      LOG_VERBOSE3
      fprintf(stdlog, "Silently stop processing for type $%02x at position %u\n",
              tlvType, (unsigned int)message->Position);
      LOG_END
      message->Position -= sizeof(struct asap_tlv_header);
      message->Error     = RSPERR_OKAY;
      return(false);
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_STOP_AND_REPORT) {
      LOG_VERBOSE3
      fprintf(stdlog, "Stop processing for type $%02x at position %u\n",
              tlvType, (unsigned int)message->Position);
      LOG_END
      message->Position -= sizeof(struct asap_tlv_header);
      message->Error     = RSPERR_UNRECOGNIZED_PARAMETER;
      return(false);
   }

   return(false);
}


/* ###### Check begin of message ############################################ */
static size_t checkBeginMessage(struct ASAPMessage* message,
                                size_t*             startPosition)
{
   struct asap_header* header;
   size_t              length;

   *startPosition                     = message->Position;
   message->OffendingMessageTLV       = (char*)&message->Buffer[*startPosition];
   message->OffendingMessageTLVLength = 0;
   message->OffendingParameterTLV     = NULL;
   message->OffendingParameterTLV     = 0;

   header = (struct asap_header*)getSpace(message, sizeof(struct asap_header));
   if(header == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }

   length = (size_t)ntohs(header->ah_length);

   if(message->Position + length - sizeof(struct asap_header) > message->BufferSize) {
      LOG_WARNING
      fprintf(stdlog, "Message length exceeds message size!\n"
             "p=%u + l=%u - 4 > size=%u\n",
             (unsigned int)message->Position, (unsigned int)length, (unsigned int)message->BufferSize);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }
   if(length < sizeof(struct asap_tlv_header)) {
      LOG_WARNING
      fputs("Message length too low!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }

   message->OffendingMessageTLVLength = length;
   return(length);
}


/* ###### Check finish of message ########################################### */
static bool checkFinishMessage(struct ASAPMessage* message,
                               const size_t        startPosition)
{
   struct asap_header* header = (struct asap_header*)&message->Buffer[startPosition];
   const size_t        length = (size_t)ntohs(header->ah_length);
   const size_t        endPos = startPosition + length;

   if(message->Position != endPos ) {
      LOG_WARNING
      fputs("Message length invalid!\n", stdlog);
      fprintf(stdlog, "position=%u expected=%u\n",
              (unsigned int)message->Position, (unsigned int)endPos);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   message->OffendingParameterTLV       = NULL;
   message->OffendingParameterTLVLength = 0;
   message->OffendingMessageTLV         = NULL;
   message->OffendingMessageTLVLength   = 0;
   return(true);
}


/* ###### Check begin of TLV ################################################ */
static size_t checkBeginTLV(struct ASAPMessage* message,
                            size_t*             tlvPosition,
                            const uint16_t      expectedType,
                            const bool          checkType)
{
   struct asap_tlv_header* header;
   uint16_t                tlvType;
   size_t                  tlvLength;

   while(getNextTLV(message, tlvPosition, &header, &tlvType, &tlvLength)) {
      if((!checkType) || (PURE_ATT_TYPE(tlvType) == expectedType)) {
         break;
      }

      LOG_VERBOSE4
      fprintf(stdlog, "Type $%04x, expected type $%04x!\n",PURE_ATT_TYPE(tlvType),expectedType);
      LOG_END

      if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
         return(0);
      }
   }
   return(tlvLength);
}


/* ###### Check finish of TLV ############################################### */
static bool checkFinishTLV(struct ASAPMessage* message,
                           const size_t        tlvPosition)
{
   struct asap_tlv_header* header = (struct asap_tlv_header*)&message->Buffer[tlvPosition];
   const size_t            length = (size_t)ntohs(header->atlv_length);
   const size_t            endPos = tlvPosition + length + getPadding(length,4);

   if(message->Position > endPos ) {
      LOG_WARNING
      fputs("TLV length invalid!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   else if(message->Position < endPos) {
      LOG_VERBOSE4
      fprintf(stdlog, "Skipping data: p=%u -> p=%u.\n",
              (unsigned int)message->Position, (unsigned int)endPos);
      LOG_END

      if(getSpace(message,endPos - message->Position) == NULL) {
         LOG_WARNING
         fputs("Unxpected end of message!\n", stdlog);
         LOG_END
         message->Error = RSPERR_INVALID_VALUES;
         return(false);
      }
   }

   message->OffendingParameterTLV       = NULL;
   message->OffendingParameterTLVLength = 0;
   return(true);
}


/* ###### Scan address parameter ############################################ */
static bool scanAddressParameter(struct ASAPMessage*      message,
                                 const card16             port,
                                 struct sockaddr_storage* address)
{
   struct sockaddr_in*  in;
   uint16_t             tlvType;
   char*                space;
#ifdef HAVE_IPV6
   struct sockaddr_in6* in6;
#endif

   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition,0,false);
   if(tlvLength == 0) {
      message->Error = RSPERR_BUFFERSIZE_EXCEEDED;
      return(false);
   }
   tlvLength -= sizeof(struct asap_tlv_header);

   memset((void*)address, 0, sizeof(struct sockaddr_storage));

   tlvType = ntohs(((struct asap_tlv_header*)&message->Buffer[tlvPosition])->atlv_type);
   switch(PURE_ATT_TYPE(tlvType)) {
      case ATT_IPv4_ADDRESS:
         if(tlvLength >= 4) {
            space = (char*)getSpace(message,4);
            if(space == NULL) {
               LOG_WARNING
               fputs("Unexpected end of IPv4 address TLV!\n", stdlog);
               LOG_END
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            in = (struct sockaddr_in*)address;
            in->sin_family = AF_INET;
            in->sin_port   = htons(port);
            memcpy((char*)&in->sin_addr, (char*)space, 4);
         }
         else {
            LOG_WARNING
            fputs("IPv4 address TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
#ifdef HAVE_IPV6
      case ATT_IPv6_ADDRESS:
         if(tlvLength >= 16) {
            space = (char*)getSpace(message,16);
            if(space == NULL) {
               LOG_WARNING
               fputs("Unexpected end of IPv6 address TLV!\n", stdlog);
               LOG_END
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            in6 = (struct sockaddr_in6*)address;
            in6->sin6_family   = AF_INET6;
            in6->sin6_port     = htons(port);
            in6->sin6_flowinfo = 0;
            in6->sin6_scope_id = 0;
            memcpy((char*)&in6->sin6_addr, (char*)space, 16);
         }
         else {
            LOG_WARNING
            fputs("IPv6 address TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
#endif
      default:
         if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
            return(false);
         }
       break;
   }

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan user transport parameter ##################################### */
static bool scanTransportParameter(struct ASAPMessage*           message,
                                   struct TransportAddressBlock* transportAddressBlock)
{
   size_t                              addresses;
   struct sockaddr_storage             addressArray[MAX_PE_TRANSPORTADDRESSES];
   struct asap_udptransportparameter*  utp;
   struct asap_tcptransportparameter*  ttp;
   struct asap_sctptransportparameter* stp;
   size_t                              tlvEndPos;
   uint16_t                            tlvType;
   uint16_t                            port              = 0;
   int                                 protocol          = 0;
   bool                                hasControlChannel = false;

   size_t  tlvPosition = 0;
   size_t  tlvLength   = checkBeginTLV(message,  &tlvPosition, 0, false);
   if(tlvLength < sizeof(struct asap_tlv_header)) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   tlvLength -= sizeof(struct asap_tlv_header);
   tlvEndPos = message->Position + tlvLength;

   tlvType = ntohs(((struct asap_tlv_header*)&message->Buffer[tlvPosition])->atlv_type);
   switch(PURE_ATT_TYPE(tlvType)) {
      case ATT_SCTP_TRANSPORT:
          stp = (struct asap_sctptransportparameter*)getSpace(message, sizeof(struct asap_sctptransportparameter));
          if(stp == NULL) {
             message->Error = RSPERR_INVALID_VALUES;
             return(false);
          }
          port     = ntohs(stp->stp_port);
          protocol = IPPROTO_SCTP;
          if(ntohs(stp->stp_transport_use) == UTP_DATA_PLUS_CONTROL) {
             hasControlChannel = true;
          }
       break;
      case ATT_TCP_TRANSPORT:
          ttp = (struct asap_tcptransportparameter*)getSpace(message, sizeof(struct asap_tcptransportparameter));
          if(ttp == NULL) {
             message->Error = RSPERR_INVALID_VALUES;
             return(false);
          }
          port     = ntohs(ttp->ttp_port);
          protocol = IPPROTO_TCP;
          if(ntohs(ttp->ttp_transport_use) == UTP_DATA_PLUS_CONTROL) {
             hasControlChannel = true;
          }
       break;
      case ATT_UDP_TRANSPORT:
          utp = (struct asap_udptransportparameter*)getSpace(message, sizeof(struct asap_udptransportparameter));
          if(utp == NULL) {
             message->Error = RSPERR_INVALID_VALUES;
             return(false);
          }
          port     = ntohs(utp->utp_port);
          protocol = IPPROTO_UDP;
          hasControlChannel = false;
       break;
      default:
         if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
            return(false);
         }
       break;
   }

   addresses = 0;
   while(message->Position < tlvEndPos) {
      if(addresses >= MAX_PE_TRANSPORTADDRESSES) {
         LOG_WARNING
         fputs("Too many addresses, internal limit reached!\n", stdlog);
         LOG_END
         message->Error = RSPERR_INVALID_VALUES;
         return(false);
      }
      if((tlvType != ATT_SCTP_TRANSPORT) && (addresses > 1)) {
         LOG_WARNING
         fputs("Multiple addresses for non-multihomed protocol!\n", stdlog);
         LOG_END
         message->Error = RSPERR_INVALID_VALUES;
         return(false);
      }
      if(scanAddressParameter(message, port, &addressArray[addresses]) == false) {
         message->Error = RSPERR_OKAY;
         break;
      }
      addresses++;

      LOG_VERBOSE3
      fprintf(stdlog, "Scanned address %u.\n", (unsigned int)addresses);
      LOG_END
   }
   if(addresses < 1) {
      LOG_WARNING
      fputs("No addresses given!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   transportAddressBlockNew(transportAddressBlock,
                            protocol, port,
                            (hasControlChannel == true) ? TABF_CONTROLCHANNEL : 0,
                            (struct sockaddr_storage*)&addressArray,
                            addresses);

   LOG_VERBOSE3
   fprintf(stdlog, "New TransportAddressBlock: ");
   transportAddressBlockPrint(transportAddressBlock, stdlog);
   fputs(".\n", stdlog);
   LOG_END

   if(checkFinishTLV(message, tlvPosition)) {
      return(true);
   }
   return(false);
}


/* ###### Scan poolPolicySettings parameter ############################################# */
static bool scanPolicyParameter(struct ASAPMessage*        message,
                                struct PoolPolicySettings* poolPolicySettings)
{
   uint8_t                                                       tlvType;
   struct asap_policy_roundrobin*                                rr;
   struct asap_policy_weighted_roundrobin*                       wrr;
   struct asap_policy_leastused*                                 lu;
   struct asap_policy_leastused_degradation*                     lud;
   struct asap_policy_priority_leastused*                        plu;
   struct asap_policy_priority_leastused_degradation*            plud;
   struct asap_policy_random*                                    rd;
   struct asap_policy_weighted_random*                           wrd;
   struct asap_policy_randomized_leastused*                      rlu;
   struct asap_policy_randomized_leastused_degradation*          rlud;
   struct asap_policy_randomized_priority_leastused*             rplu;
   struct asap_policy_randomized_priority_leastused_degradation* rplud;

   size_t  tlvPosition = 0;
   size_t  tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_POLICY, true);
   if(tlvLength == 0) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   tlvLength -= sizeof(struct asap_tlv_header);

   if(tlvLength < 1) {
      LOG_WARNING
      fputs("TLV too short\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   tlvType = (uint8_t)message->Buffer[message->Position];

   poolPolicySettingsNew(poolPolicySettings);
   switch(PURE_ATT_TYPE(tlvType)) {
      case PPT_LEASTUSED:
         if(tlvLength >= sizeof(struct asap_policy_leastused)) {
            lu = (struct asap_policy_leastused*)getSpace(message, sizeof(struct asap_policy_leastused));
            if(lu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = lu->pp_lu_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntoh24(lu->pp_lu_load);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy LU, load=$%06x\n",poolPolicySettings->Load);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("LU TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_LEASTUSED_DEGRADATION:
         if(tlvLength >= sizeof(struct asap_policy_leastused_degradation)) {
            lud = (struct asap_policy_leastused_degradation*)getSpace(message, sizeof(struct asap_policy_leastused_degradation));
            if(lud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = lud->pp_lud_policy;
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntoh24(lud->pp_lud_load);
            poolPolicySettings->LoadDegradation = ntoh24(lud->pp_lud_loaddeg);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy LUD, load=$%06x loaddeg=$%06x\n",
                    poolPolicySettings->Load,
                    poolPolicySettings->LoadDegradation);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("LUD TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_PRIORITY_LEASTUSED:
         if(tlvLength >= sizeof(struct asap_policy_priority_leastused)) {
            plu = (struct asap_policy_priority_leastused*)getSpace(message, sizeof(struct asap_policy_priority_leastused));
            if(plu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = plu->pp_plu_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntoh24(plu->pp_plu_load);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy PLU, load=$%06x\n",poolPolicySettings->Load);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("PLU TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_PRIORITY_LEASTUSED_DEGRADATION:
         if(tlvLength >= sizeof(struct asap_policy_priority_leastused_degradation)) {
            plud = (struct asap_policy_priority_leastused_degradation*)getSpace(message, sizeof(struct asap_policy_priority_leastused_degradation));
            if(plud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = plud->pp_plud_policy;
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntoh24(plud->pp_plud_load);
            poolPolicySettings->LoadDegradation = ntoh24(plud->pp_plud_loaddeg);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy PLUD, load=$%06x loaddeg=$%06x\n",
                    poolPolicySettings->Load,
                    poolPolicySettings->LoadDegradation);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("PLUD TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOM:
         if(tlvLength >= sizeof(struct asap_policy_random)) {
            rd = (struct asap_policy_random*)getSpace(message, sizeof(struct asap_policy_random));
            if(rd == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = rd->pp_rd_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = 0;

            LOG_VERBOSE3
            fputs("Scanned policy RAND\n", stdlog);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("RAND TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_WEIGHTED_RANDOM:
         if(tlvLength >= sizeof(struct asap_policy_weighted_random)) {
            wrd = (struct asap_policy_weighted_random*)getSpace(message, sizeof(struct asap_policy_weighted_random));
            if(wrd == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = wrd->pp_wrd_policy;
            poolPolicySettings->Weight     = ntoh24(wrd->pp_wrd_weight);
            poolPolicySettings->Load       = 0;

            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy WRAND, weight=%d\n",
                    poolPolicySettings->Weight);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("WRAND TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_ROUNDROBIN:
         if(tlvLength >= sizeof(struct asap_policy_roundrobin)) {
            rr = (struct asap_policy_roundrobin*)getSpace(message, sizeof(struct asap_policy_roundrobin));
            if(rr == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = rr->pp_rr_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = 0;
            LOG_VERBOSE3
            fputs("Scanned policy RR\n", stdlog);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("RR TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_WEIGHTED_ROUNDROBIN:
         if(tlvLength >= sizeof(struct asap_policy_weighted_roundrobin)) {
            wrr = (struct asap_policy_weighted_roundrobin*)getSpace(message, sizeof(struct asap_policy_weighted_roundrobin));
            if(wrr == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = wrr->pp_wrr_policy;
            poolPolicySettings->Weight     = ntoh24(wrr->pp_wrr_weight);
            poolPolicySettings->Load       = 0;
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy WRR, weight=%d\n",poolPolicySettings->Weight);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("WRR TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOMIZED_LEASTUSED:
         if(tlvLength >= sizeof(struct asap_policy_randomized_leastused)) {
            rlu = (struct asap_policy_randomized_leastused*)getSpace(message, sizeof(struct asap_policy_randomized_leastused));
            if(rlu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = rlu->pp_rlu_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntoh24(rlu->pp_rlu_load);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy RLU, load=$%06x\n",poolPolicySettings->Load);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("RLU TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOMIZED_LEASTUSED_DEGRADATION:
         if(tlvLength >= sizeof(struct asap_policy_randomized_leastused_degradation)) {
            rlud = (struct asap_policy_randomized_leastused_degradation*)getSpace(message, sizeof(struct asap_policy_randomized_leastused_degradation));
            if(rlud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = rlud->pp_rlud_policy;
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntoh24(rlud->pp_rlud_load);
            poolPolicySettings->LoadDegradation = ntoh24(rlud->pp_rlud_loaddeg);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy RLUD, load=$%06x loaddeg=$%06x\n",
                    poolPolicySettings->Load,
                    poolPolicySettings->LoadDegradation);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("RLUD TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED:
         if(tlvLength >= sizeof(struct asap_policy_randomized_priority_leastused)) {
            rplu = (struct asap_policy_randomized_priority_leastused*)getSpace(message, sizeof(struct asap_policy_randomized_priority_leastused));
            if(rplu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = rplu->pp_rplu_policy;
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntoh24(rplu->pp_rplu_load);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy PLU, load=$%06x\n",poolPolicySettings->Load);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("PLU TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION:
         if(tlvLength >= sizeof(struct asap_policy_randomized_priority_leastused_degradation)) {
            rplud = (struct asap_policy_randomized_priority_leastused_degradation*)getSpace(message, sizeof(struct asap_policy_randomized_priority_leastused_degradation));
            if(rplud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = rplud->pp_rplud_policy;
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntoh24(rplud->pp_rplud_load);
            poolPolicySettings->LoadDegradation = ntoh24(rplud->pp_rplud_loaddeg);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy RPLUD, load=$%06x loaddeg=$%06x\n",
                    poolPolicySettings->Load,
                    poolPolicySettings->LoadDegradation);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("RPLUD TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      default:
         if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
            return(false);
         }
       break;
   }

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan pool element paramter ######################################## */
static struct ST_CLASS(PoolElementNode)* scanPoolElementParameter(struct ASAPMessage* message)
{
   struct asap_poolelementparameter* pep;
   char                              transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*     transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   struct PoolPolicySettings         poolPolicySettings;
   struct ST_CLASS(PoolElementNode)* poolElementNode;

   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_ELEMENT, true);
   if(tlvLength == 0) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   pep = (struct asap_poolelementparameter*)getSpace(message, sizeof(struct asap_poolelementparameter));
   if(pep == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   if(scanTransportParameter(message, transportAddressBlock) == false) {
      return(false);
   }

   if(scanPolicyParameter(message, &poolPolicySettings) == false) {
      return(false);
   }

   if(checkFinishTLV(message, tlvPosition) == false) {
      return(false);
   }

   poolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
   if(poolElementNode == NULL) {
      message->Error = RSPERR_OUT_OF_MEMORY;
      return(false);
   }
   ST_CLASS(poolElementNodeNew)(poolElementNode,
                                ntohl(pep->pep_identifier),
                                ntohl(pep->pep_homeserverid),
                                ntohl(pep->pep_reg_life),
                                &poolPolicySettings,
                                transportAddressBlock);
   return(poolElementNode);
}


/* ###### Scan pool handle paramter ######################################### */
static bool scanPoolHandleParameter(struct ASAPMessage* message,
                                    struct PoolHandle*  poolHandlePtr)
{
   unsigned char* poolHandle;
   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_HANDLE, true);
   if(tlvLength == 0) {
      return(false);
   }

   tlvLength -= sizeof(struct asap_tlv_header);
   if(tlvLength < 1) {
      LOG_WARNING
      fputs("Pool handle too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   poolHandle = (unsigned char*)getSpace(message, tlvLength);
   if(poolHandle == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   if(tlvLength > MAX_POOLHANDLESIZE) {
      message->Error = RSPERR_INVALID_VALUES;
   }
   poolHandleNew(poolHandlePtr, poolHandle, tlvLength);

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned pool handle ");
   poolHandlePrint(poolHandlePtr, stdlog);
   fprintf(stdlog, ", length=%u.\n",poolHandlePtr->Size);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan pool element identifier parameter ############################ */
static bool scanPoolElementIdentifierParameter(struct ASAPMessage* message)
{
   uint32_t* identifier;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_IDENTIFIER, true);
   if(tlvLength == 0) {
      return(false);
   }

   tlvLength -= sizeof(struct asap_tlv_header);
   if(tlvLength != sizeof(uint32_t)) {
      LOG_WARNING
      fputs("Pool element identifier too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   identifier = (uint32_t*)getSpace(message, tlvLength);
   if(identifier == NULL) {
      return(false);
   }
   message->Identifier = ntohl(*identifier);

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned pool element identifier $%08x\n",message->Identifier);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan error parameter ############################################## */
static bool scanErrorParameter(struct ASAPMessage* message)
{
   struct asap_errorcause* aec;
   char*                   data;
   size_t                  dataLength;
   size_t                  tlvPosition = 0;
   size_t                  tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_OPERATION_ERROR, true);
   if(tlvLength == 0) {
      return(false);
   }

   tlvLength -= sizeof(struct asap_tlv_header);
   if(tlvLength < sizeof(struct asap_errorcause)) {
      LOG_WARNING
      fputs("Error parameter TLV too short\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   aec = (struct asap_errorcause*)&message->Buffer[message->Position];
   message->Error = ntohs(aec->aec_cause);
   dataLength     = ntohs(aec->aec_length);

   if(dataLength < sizeof(struct asap_errorcause)) {
      LOG_WARNING
      fputs("Cause length too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   dataLength -= sizeof(struct asap_errorcause);
   data = (char*)getSpace(message,dataLength);
   if(data == NULL) {
      return(false);
   }

   message->OperationErrorData   = data;
   message->OperationErrorLength = dataLength;

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan cookie parameter ############################################# */
static bool scanCookieParameter(struct ASAPMessage* message)
{
   void*     cookie;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_COOKIE, true);
   if(tlvLength == 0) {
      return(false);
   }

   tlvLength -= sizeof(struct asap_tlv_header);
   if(tlvLength < 1) {
      LOG_WARNING
      fputs("Cookie too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   cookie = (void*)getSpace(message, tlvLength);
   if(cookie == NULL) {
      return(false);
   }

   message->CookiePtr = malloc(tlvLength);
   if(message->CookiePtr == NULL) {
      message->Error = RSPERR_OUT_OF_MEMORY;
      return(false);
   }

   message->CookieSize = tlvLength;
   memcpy(message->CookiePtr,cookie,message->CookieSize);

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned cookie, length=%d\n",(int)message->CookieSize);
   LOG_END

   return(true);
}


/* ###### Scan endpoint keepalive message ################################### */
static bool scanEndpointKeepAliveMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan endpoint keepalive acknowledgement message ################### */
static bool scanEndpointKeepAliveAckMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan endpoint unreachable message ################################# */
static bool scanEndpointUnreachableMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan registration message ######################################### */
static bool scanRegistrationMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   message->PoolElementPtr = scanPoolElementParameter(message);
   if(message->PoolElementPtr == NULL) {
      return(false);
   }
   return(true);
}


/* ###### Scan deregistration message ####################################### */
static bool scanDeregistrationMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan registration response message ################################ */
static bool scanRegistrationResponseMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   if(message->Position < message->BufferSize) {
      if(!scanErrorParameter(message)) {
         return(false);
      }
   }
   return(true);
}


/* ###### Scan deregistration response message ############################## */
static bool scanDeregistrationResponseMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   if(message->Position < message->BufferSize) {
      if(!scanErrorParameter(message)) {
         return(false);
      }
   }
   return(true);
}


/* ###### Scan name resolution message ###################################### */
static bool scanNameResolutionMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan name resolution response message ############################# */
static bool scanNameResolutionResponseMessage(struct ASAPMessage* message)
{
   if(!scanErrorParameter(message)) {
      if(scanPoolHandleParameter(message, &message->Handle) == false) {
         return(false);
      }
      if(scanPolicyParameter(message, &message->PolicySettings) == false) {
         return(false);
      }

      message->PoolElementPtrArraySize = 0;
      while(message->Position < message->BufferSize) {
         if(message->PoolElementPtrArraySize >= MAX_MAX_NAME_RESOLUTION_ITEMS) {
            LOG_WARNING
            fputs("Too many Pool Element Parameters in Name Resolution Response\n", stdlog);
            LOG_END
            return(false);
         }
         message->PoolElementPtrArray[message->PoolElementPtrArraySize] =
            scanPoolElementParameter(message);
         message->PoolElementPtrArraySize++;
      }
   }

   return(true);
}


/* ###### Scan name resolution response message ############################# */
static bool scanServerAnnounceMessage(struct ASAPMessage* message)
{
   char                          transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock* transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   struct TransportAddressBlock* lastTransportAddressBlock = NULL;
   struct TransportAddressBlock* newTransportAddressBlock;
   uint32_t*                     identifier;

   identifier = (uint32_t*)getSpace(message, sizeof(identifier));
   if(identifier == NULL) {
      return(false);
   }
   message->NSIdentifier = ntohl(*identifier);

   message->TransportAddressBlockListPtr = NULL;
   while(message->Position < message->BufferSize) {
      if(scanTransportParameter(message, transportAddressBlock)) {
         newTransportAddressBlock = transportAddressBlockDuplicate(transportAddressBlock);
         if(newTransportAddressBlock == NULL) {
            message->Error = RSPERR_OUT_OF_MEMORY;
            return(false);
         }
         if(message->TransportAddressBlockListPtr == NULL) {
            message->TransportAddressBlockListPtr = newTransportAddressBlock;
         }
         else {
            lastTransportAddressBlock->Next = newTransportAddressBlock;
         }
         lastTransportAddressBlock = newTransportAddressBlock;
      }
      else {
         break;
      }
   }
   if(message->TransportAddressBlockListPtr == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   return(true);
}


/* ###### Scan cookie message ################################################ */
static bool scanCookieMessage(struct ASAPMessage* message)
{
   if(scanCookieParameter(message) == false) {
      return(false);
   }

   return(true);
}


/* ###### Scan cookie echo message ########################################### */
static bool scanCookieEchoMessage(struct ASAPMessage* message)
{
   if(scanCookieParameter(message) == false) {
      return(false);
   }

   return(true);
}


/* ###### Scan business card message ######################################### */
static bool scanBusinessCardMessage(struct ASAPMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPolicyParameter(message, &message->PolicySettings) == false) {
      return(false);
   }
   message->PoolElementPtrArraySize = 0;
   while(message->Position < message->BufferSize) {
      while(message->Position < message->BufferSize) {
         if(message->PoolElementPtrArraySize >= MAX_MAX_NAME_RESOLUTION_ITEMS) {
            LOG_WARNING
            fputs("Too many Pool Element Parameters in Business Card\n", stdlog);
            LOG_END
            return(false);
         }
         message->PoolElementPtrArray[message->PoolElementPtrArraySize] =
            scanPoolElementParameter(message);
         message->PoolElementPtrArraySize++;
      }
   }
   if(message->PoolElementPtrArraySize < 1) {
      LOG_WARNING
      fputs("Business Card contains no Pool Element Parameters\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   return(true);
}


/* ###### Scan message ###################################################### */
static bool scanMessage(struct ASAPMessage* message)
{
   struct asap_header* header;
   size_t              startPosition;
   size_t              length;
   size_t              i, j;

   LOG_VERBOSE5
   fprintf(stdlog, "Incoming message (%u bytes):\n", message->BufferSize);
   for(i = 0;i < message->BufferSize;i += 32) {
      fprintf(stdlog, "%4u: ", i);
      for(j = 0;j < 32;j++) {
         if(i + j < message->BufferSize) {
            fprintf(stdlog, "%02x", (int)((unsigned char)message->Buffer[i + j]));
         }
         else {
            fputs("  ", stdlog);
         }
         if(((j + 1) % 4) == 0) fputs(" ", stdlog);
      }
      for(j = 0;j < 32;j++) {
         if(i + j < message->BufferSize) {
            if(isprint(message->Buffer[i + j])) {
               fprintf(stdlog, "%c", message->Buffer[i + j]);
            }
            else {
               fputs(".", stdlog);
            }
         }
         else {
            fputs("  ", stdlog);
         }
      }
      fputs("\n", stdlog);
   }
   LOG_END


   length = checkBeginMessage(message, &startPosition);
   if(length < sizeof(struct asap_header)) {
      return(false);
   }

   header        = (struct asap_header*)&message->Buffer[startPosition];
   message->Type = header->ah_type;
   switch(message->Type) {
      case AHT_NAME_RESOLUTION:
         LOG_VERBOSE2
         fputs("Scanning NameResolution message...\n", stdlog);
         LOG_END
         if(scanNameResolutionMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_NAME_RESOLUTION_RESPONSE:
         LOG_VERBOSE2
         fputs("Scanning NameResolutionResponse message...\n", stdlog);
         LOG_END
         if(scanNameResolutionResponseMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_ENDPOINT_KEEP_ALIVE:
         LOG_VERBOSE2
         fputs("Scanning KeepAlive message...\n", stdlog);
         LOG_END
         if(scanEndpointKeepAliveMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_ENDPOINT_KEEP_ALIVE_ACK:
         LOG_VERBOSE2
         fputs("Scanning KeepAliveAck message...\n", stdlog);
         LOG_END
         if(scanEndpointKeepAliveAckMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_SERVER_ANNOUNCE:
         LOG_VERBOSE2
         fputs("Scanning ServerAnnounce message...\n", stdlog);
         LOG_END
         if(scanServerAnnounceMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_COOKIE:
         LOG_VERBOSE2
         fputs("Scanning Cookie message...\n", stdlog);
         LOG_END
         if(scanCookieMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_COOKIE_ECHO:
         LOG_VERBOSE2
         fputs("Scanning CookieEcho message...\n", stdlog);
         LOG_END
         if(scanCookieEchoMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_BUSINESS_CARD:
         LOG_VERBOSE2
         fputs("Scanning BusinessCard message...\n", stdlog);
         LOG_END
         if(scanBusinessCardMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         LOG_VERBOSE2
         fputs("Scanning EndpointUnreachable message...\n", stdlog);
         LOG_END
         if(scanEndpointUnreachableMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_REGISTRATION:
         LOG_VERBOSE2
         fputs("Scanning Registration message...\n", stdlog);
         LOG_END
         if(scanRegistrationMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_DEREGISTRATION:
         LOG_VERBOSE2
         fputs("Scanning Deregistration message...\n", stdlog);
         LOG_END
         if(scanDeregistrationMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_REGISTRATION_RESPONSE:
         LOG_VERBOSE2
         fputs("Scanning RegistrationResponse message...\n", stdlog);
         LOG_END
         if(scanRegistrationResponseMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_DEREGISTRATION_RESPONSE:
         LOG_VERBOSE2
         fputs("Scanning DeregistrationResponse message...\n", stdlog);
         LOG_END
         if(scanDeregistrationResponseMessage(message) == false) {
            return(false);
         }
       break;
      default:
         LOG_WARNING
         fprintf(stdlog, "Unknown type #$%02x!\n",header->ah_type);
         LOG_END
         return(false);
       break;
   }

   return(checkFinishMessage(message,startPosition));
}


/* ###### Convert packet to ASAPMessage ##################################### */
struct ASAPMessage* asapPacket2Message(char* packet, const size_t packetSize, const size_t minBufferSize)
{
   struct ASAPMessage* message = asapMessageNew(packet, packetSize);
   if(message != NULL) {
      message->OriginalBufferSize                     = max(packetSize, minBufferSize);
      message->Position                               = 0;
      message->PoolElementPtrAutoDelete               = true;
      message->CookiePtrAutoDelete                    = true;
      message->PoolElementPtrArrayAutoDelete          = true;
      message->TransportAddressBlockListPtrAutoDelete = true;

      LOG_VERBOSE3
      fprintf(stdlog, "Scanning message, size=%u...\n",
              (unsigned int)packetSize);
      LOG_END

      if(scanMessage(message) == true) {
         LOG_VERBOSE3
         fputs("Message successfully scanned!\n", stdlog);
         LOG_END
         return(message);
      }

      LOG_WARNING
      fprintf(stdlog, "Format error in message at byte %u!\n",
              (unsigned int)message->Position);
      LOG_END
      asapMessageDelete(message);
      message = NULL;
   }
   return(message);
}
