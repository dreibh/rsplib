/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2008 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rserpoolmessageparser.h"
#include "rserpool.h"

#include <netinet/in.h>
#include <ext_socket.h>


/* ###### Peek next TLV header type ###################################### */
static uint16_t peekNextTLVType(struct RSerPoolMessage* message)
{
   const struct rserpool_tlv_header* header;

   header = (const struct rserpool_tlv_header*)getSpace(message, sizeof(struct rserpool_tlv_header));
   if(header) {
      message->Position -= sizeof(struct rserpool_tlv_header);
      return(ntohs(header->atlv_type));
   }
   return(0x00);
}


/* ###### Scan next TLV header ########################################### */
static bool getNextTLV(struct RSerPoolMessage*      message,
                       size_t*                      tlvPosition,
                       struct rserpool_tlv_header** header,
                       uint16_t*                    tlvType,
                       size_t*                      tlvLength)
{
   *tlvPosition = message->Position;
   message->OffendingParameterTLV       = (char*)&message->Buffer[*tlvPosition];
   message->OffendingParameterTLVLength = 0;

   *header = (struct rserpool_tlv_header*)getSpace(message, sizeof(struct rserpool_tlv_header));
   if(*header == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   *tlvType   = ntohs((*header)->atlv_type);
   *tlvLength = (size_t)ntohs((*header)->atlv_length);

   LOG_VERBOSE5
   fprintf(stdlog, "TLV: Type $%04x, length %u at position %u\n",
           *tlvType, (unsigned int)*tlvLength, (unsigned int)(message->Position - sizeof(struct rserpool_tlv_header)));
   LOG_END

   if(message->Position + *tlvLength - sizeof(struct rserpool_tlv_header) > message->BufferSize) {
      LOG_WARNING
      fprintf(stdlog, "TLV length exceeds message size!\n"
             "p=%u + l=%u > size=%u   type=$%02x\n",
             (unsigned int)(message->Position - sizeof(struct rserpool_tlv_header)), (unsigned int)*tlvLength, (unsigned int)message->BufferSize, *tlvType);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   if(*tlvLength < sizeof(struct rserpool_tlv_header)) {
      LOG_WARNING
      fputs("TLV length too low!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   message->OffendingParameterTLVLength = *tlvLength;
   return(*tlvLength > 0);
}


/* ###### Handle unknown TLV ############################################# */
static bool handleUnknownTLV(struct RSerPoolMessage* message,
                             const uint16_t          tlvType,
                             const size_t            tlvLength)
{
   void* ptr;

   if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_CONTINUE) {
      ptr = getSpace(message, tlvLength - sizeof(struct rserpool_tlv_header));
      if(ptr != NULL) {
         LOG_VERBOSE3
         fprintf(stdlog, "Silently skipping TLV type $%02x at position %u\n",
                 tlvType, (unsigned int)(message->Position - sizeof(struct rserpool_tlv_header)));
         LOG_END
         return(true);
      }
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_CONTINUE_AND_REPORT) {
      ptr = getSpace(message, tlvLength - sizeof(struct rserpool_tlv_header));
      if(ptr != NULL) {
         LOG_VERBOSE3
         fprintf(stdlog, "Skipping TLV type $%02x at position %u\n",
                 tlvType, (unsigned int)(message->Position - sizeof(struct rserpool_tlv_header)));
         LOG_END

         /*
            Implement Me: Build error table!
         */

         return(true);
      }
      return(false);
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_STOP) {
      LOG_VERBOSE1
      fprintf(stdlog, "Silently stop processing for type $%02x at position %u\n",
              tlvType, (unsigned int)message->Position);
      LOG_END
      message->Position -= sizeof(struct rserpool_tlv_header);
      message->Error     = RSPERR_UNRECOGNIZED_PARAMETER_SILENT;
      return(false);
   }
   else if((tlvType & ATT_ACTION_MASK) == ATT_ACTION_STOP_AND_REPORT) {
      LOG_VERBOSE1
      fprintf(stdlog, "Stop processing for type $%02x at position %u\n",
              tlvType, (unsigned int)message->Position);
      LOG_END
      message->Position -= sizeof(struct rserpool_tlv_header);
      message->Error     = RSPERR_UNRECOGNIZED_PARAMETER;
      return(false);
   }

   return(false);
}


/* ###### Check begin of message ######################################### */
static size_t checkBeginMessage(struct RSerPoolMessage* message,
                                size_t*                 startPosition)
{
   struct rserpool_header* header;
   size_t                  length;

   *startPosition                     = message->Position;
   message->OffendingMessageTLV       = (char*)&message->Buffer[*startPosition];
   message->OffendingMessageTLVLength = 0;
   message->OffendingParameterTLV     = NULL;
   message->OffendingParameterTLV     = 0;

   header = (struct rserpool_header*)getSpace(message, sizeof(struct rserpool_header));
   if(header == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }

   length = (size_t)ntohs(header->ah_length);

   if(message->Position + length - sizeof(struct rserpool_header) > message->BufferSize) {
      LOG_WARNING
      fprintf(stdlog, "Message length exceeds message size!\n"
             "p=%u + l=%u - 4 > size=%u\n",
             (unsigned int)message->Position, (unsigned int)length, (unsigned int)message->BufferSize);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }
   if(length < sizeof(struct rserpool_tlv_header)) {
      LOG_WARNING
      fputs("Message length too low!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(0);
   }

   message->Flags                     = header->ah_flags;
   message->OffendingMessageTLVLength = length;
   return(length);
}


/* ###### Check finish of message ######################################## */
static bool checkFinishMessage(struct RSerPoolMessage* message,
                               const size_t            startPosition)
{
   struct rserpool_header* header  = (struct rserpool_header*)&message->Buffer[startPosition];
   const size_t            length  = (size_t)ntohs(header->ah_length);
   const size_t            endPos  = startPosition + length;
   const size_t            padding = getPadding(endPos, 4);

   if(message->Position != endPos + padding) {
      LOG_WARNING
      fputs("Message length invalid!\n", stdlog);
      fprintf(stdlog, "position=%u expected=%u padding=%u\n",
              (unsigned int)message->Position,
              (unsigned int)endPos,
              (unsigned int)padding);
      LOG_END
      if(message->Error == RSPERR_OKAY) {
         /* Error has not been set yet (first error occured) */
         message->Error = RSPERR_INVALID_VALUES;
      }
      return(false);
   }

   message->OffendingParameterTLV       = NULL;
   message->OffendingParameterTLVLength = 0;
   message->OffendingMessageTLV         = NULL;
   message->OffendingMessageTLVLength   = 0;
   return(true);
}


/* ###### Check begin of TLV ############################################# */
static size_t checkBeginTLV(struct RSerPoolMessage* message,
                            size_t*                 tlvPosition,
                            const uint16_t          expectedType,
                            const bool              checkType)
{
   struct rserpool_tlv_header* header;
   uint16_t                    tlvType;
   size_t                      tlvLength;

   tlvLength = 0;
   tlvType   = 0;
   while(getNextTLV(message, tlvPosition, &header, &tlvType, &tlvLength)) {
      if((!checkType) || (PURE_ATT_TYPE(tlvType) == (expectedType))) {
         break;
      }

      LOG_VERBOSE4
      fprintf(stdlog, "Type $%04x, expected type $%04x!\n",
              PURE_ATT_TYPE(tlvType), PURE_ATT_TYPE(expectedType));
      LOG_END

      if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
         return(0);
      }
   }
   return(tlvLength);
}


/* ###### Check finish of TLV ############################################ */
static bool checkFinishTLV(struct RSerPoolMessage* message,
                           const size_t            tlvPosition)
{
   struct rserpool_tlv_header* header = (struct rserpool_tlv_header*)&message->Buffer[tlvPosition];
   const size_t                length = (size_t)ntohs(header->atlv_length);
   const size_t                endPos = tlvPosition + length + getPadding(length, 4);

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


/* ###### Scan address parameter ######################################### */
static bool scanAddressParameter(struct RSerPoolMessage* message,
                                 const uint16_t          port,
                                 union sockaddr_union*   address)
{
   struct sockaddr_in*  in;
   uint16_t             tlvType;
   char*                space;
   struct sockaddr_in6* in6;

   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition, 0, false);
   if(tlvLength == 0) {
      message->Error = RSPERR_BUFFERSIZE_EXCEEDED;
      return(false);
   }
   tlvLength -= sizeof(struct rserpool_tlv_header);

   memset((void*)address, 0, sizeof(union sockaddr_union));

   tlvType = ntohs(((struct rserpool_tlv_header*)&message->Buffer[tlvPosition])->atlv_type);
   switch(PURE_ATT_TYPE(tlvType)) {
      case ATT_IPv4_ADDRESS:
         if(tlvLength >= 4) {
            space = (char*)getSpace(message, 4);
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
#ifdef HAVE_SIN_LEN
            in->sin_len = sizeof(struct sockaddr_in);
#endif
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
      case ATT_IPv6_ADDRESS:
         if(tlvLength >= 16) {
            space = (char*)getSpace(message, 16);
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
#ifdef HAVE_SIN6_LEN
            in6->sin6_len = sizeof(struct sockaddr_in6);
#endif
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
      default:
         if(handleUnknownTLV(message, tlvType, tlvLength) == false) {
            return(false);
         }
       break;
   }

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan user transport parameter ################################## */
static bool scanTransportParameter(struct RSerPoolMessage*       message,
                                   struct TransportAddressBlock* transportAddressBlock)
{
   size_t                                  addresses;
   union sockaddr_union                    addressArray[MAX_PE_TRANSPORTADDRESSES];
   struct rserpool_udptransportparameter*  utp;
   struct rserpool_tcptransportparameter*  ttp;
   struct rserpool_sctptransportparameter* stp;
   size_t                                  tlvEndPos;
   uint16_t                                tlvType;
   uint16_t                                port              = 0;
   int                                     protocol          = 0;
   bool                                    hasControlChannel = false;

   size_t  tlvPosition = 0;
   size_t  tlvLength   = checkBeginTLV(message,  &tlvPosition, 0, false);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   tlvLength -= sizeof(struct rserpool_tlv_header);
   tlvEndPos = message->Position + tlvLength;

   tlvType = ntohs(((struct rserpool_tlv_header*)&message->Buffer[tlvPosition])->atlv_type);
   switch(PURE_ATT_TYPE(tlvType)) {

      case ATT_SCTP_TRANSPORT:
          stp = (struct rserpool_sctptransportparameter*)getSpace(message, sizeof(struct rserpool_sctptransportparameter));
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
          ttp = (struct rserpool_tcptransportparameter*)getSpace(message, sizeof(struct rserpool_tcptransportparameter));
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
          utp = (struct rserpool_udptransportparameter*)getSpace(message, sizeof(struct rserpool_udptransportparameter));
          if(utp == NULL) {
             message->Error = RSPERR_INVALID_VALUES;
             return(false);
          }
          port              = ntohs(utp->utp_port);
          protocol          = IPPROTO_UDP;
          hasControlChannel = false;
       break;

#if 0
      case ATT_DCCP_TRANSPORT:
          dtp = (struct rserpool_dccptransportparameter*)getSpace(message, sizeof(struct rserpool_dccptransportparameter));
          if(dtp == NULL) {
             message->Error = RSPERR_INVALID_VALUES;
             return(false);
          }
          port              = ntohs(dtp->dtp_port);
          protocol          = IPPROTO_DCCP;
          hasControlChannel = false;
       break;
#endif

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
      /* Link-local scoped addresses will be skipped => it is not possible to
         determine the right network interface! */
      if(getScope(&addressArray[addresses].sa) > AS_UNICAST_LINKLOCAL) {
         addresses++;
      }
      else {
         LOG_VERBOSE3
         fputs("Skipping link-local address in Transport Parameter\n", stdlog);
         LOG_END
      }

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
                            (union sockaddr_union*)&addressArray,
                            addresses, MAX_PE_TRANSPORTADDRESSES);

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


/* ###### Scan Pool Policy parameter ##################################### */
static bool scanPolicyParameter(struct RSerPoolMessage*    message,
                                struct PoolPolicySettings* poolPolicySettings)
{
   const struct rserpool_policy_roundrobin*                          rr;
   struct rserpool_policy_weighted_roundrobin*                       wrr;
   struct rserpool_policy_priority*                                  p;
   struct rserpool_policy_leastused*                                 lu;
   struct rserpool_policy_leastused_dpf*                             ludpf;
   struct rserpool_policy_leastused_degradation*                     lud;
   struct rserpool_policy_leastused_degradation_dpf*                 luddpf;
   struct rserpool_policy_priority_leastused*                        plu;
   struct rserpool_policy_priority_leastused_degradation*            plud;
   struct rserpool_policy_random*                                    rd;
   struct rserpool_policy_weighted_random*                           wrd;
   struct rserpool_policy_weighted_random_dpf*                       wrddpf;
   struct rserpool_policy_randomized_leastused*                      rlu;
   struct rserpool_policy_randomized_leastused_degradation*          rlud;
   struct rserpool_policy_randomized_priority_leastused*             rplu;
   struct rserpool_policy_randomized_priority_leastused_degradation* rplud;
   uint32_t                                                          policyType;

   size_t  tlvPosition = 0;
   size_t  tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_POLICY, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   tlvLength -= sizeof(struct rserpool_tlv_header);

   if(tlvLength < sizeof(policyType)) {
      LOG_WARNING
      fputs("TLV too short\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   policyType = ntohl(*((uint32_t*)&message->Buffer[message->Position]));

   poolPolicySettingsNew(poolPolicySettings);
   switch(policyType) {
      case PPT_LEASTUSED:
         if(tlvLength >= sizeof(struct rserpool_policy_leastused)) {
            lu = (struct rserpool_policy_leastused*)getSpace(message, sizeof(struct rserpool_policy_leastused));
            if(lu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(lu->pp_lu_policy);
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntohl(lu->pp_lu_load);
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
      case PPT_LEASTUSED_DPF:
         if(tlvLength >= sizeof(struct rserpool_policy_leastused_dpf)) {
            ludpf = (struct rserpool_policy_leastused_dpf*)getSpace(message, sizeof(struct rserpool_policy_leastused_dpf));
            if(ludpf == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(ludpf->pp_ludpf_policy);
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntohl(ludpf->pp_ludpf_load);
            poolPolicySettings->LoadDPF    = ntohl(ludpf->pp_ludpf_load_dpf);
            poolPolicySettings->Distance   = ntohl(ludpf->pp_ludpf_distance);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy LU-DPF, load=$%06x, ldpf=%1.3f%%, distance=%u\n",
                    poolPolicySettings->Load,
                    100.0 * ((double)poolPolicySettings->LoadDPF / (double)PPV_MAX_LOADDPF),
                    poolPolicySettings->Distance);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("LU-DPF TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_LEASTUSED_DEGRADATION:
         if(tlvLength >= sizeof(struct rserpool_policy_leastused_degradation)) {
            lud = (struct rserpool_policy_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_leastused_degradation));
            if(lud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = ntohl(lud->pp_lud_policy);
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntohl(lud->pp_lud_load);
            poolPolicySettings->LoadDegradation = ntohl(lud->pp_lud_loaddeg);
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
      case PPT_LEASTUSED_DEGRADATION_DPF:
         if(tlvLength >= sizeof(struct rserpool_policy_leastused_degradation_dpf)) {
            luddpf = (struct rserpool_policy_leastused_degradation_dpf*)getSpace(message, sizeof(struct rserpool_policy_leastused_degradation_dpf));
            if(luddpf == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = ntohl(luddpf->pp_luddpf_policy);
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntohl(luddpf->pp_luddpf_load);
            poolPolicySettings->LoadDegradation = ntohl(luddpf->pp_luddpf_loaddeg);
            poolPolicySettings->LoadDPF         = ntohl(luddpf->pp_luddpf_load_dpf);
            poolPolicySettings->Distance        = ntohl(luddpf->pp_luddpf_distance);
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy LUD-DPF, load=$%06x, loaddeg=$%06x, ldpf=%1.3f%%, distance=%u\n",
                    poolPolicySettings->Load,
                    poolPolicySettings->LoadDegradation,
                    100.0 * ((double)poolPolicySettings->LoadDPF / (double)PPV_MAX_LOADDPF),
                    poolPolicySettings->Distance);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("LUD-DPF TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_PRIORITY_LEASTUSED:
         if(tlvLength >= sizeof(struct rserpool_policy_priority_leastused)) {
            plu = (struct rserpool_policy_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused));
            if(plu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(plu->pp_plu_policy);
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntohl(plu->pp_plu_load);
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
         if(tlvLength >= sizeof(struct rserpool_policy_priority_leastused_degradation)) {
            plud = (struct rserpool_policy_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused_degradation));
            if(plud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = ntohl(plud->pp_plud_policy);
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntohl(plud->pp_plud_load);
            poolPolicySettings->LoadDegradation = ntohl(plud->pp_plud_loaddeg);
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
         if(tlvLength >= sizeof(struct rserpool_policy_random)) {
            rd = (struct rserpool_policy_random*)getSpace(message, sizeof(struct rserpool_policy_random));
            if(rd == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(rd->pp_rd_policy);
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
         if(tlvLength >= sizeof(struct rserpool_policy_weighted_random)) {
            wrd = (struct rserpool_policy_weighted_random*)getSpace(message, sizeof(struct rserpool_policy_weighted_random));
            if(wrd == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(wrd->pp_wrd_policy);
            poolPolicySettings->Weight     = ntohl(wrd->pp_wrd_weight);
            poolPolicySettings->Load       = 0;

            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy WRAND, weight=%u\n",
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
      case PPT_WEIGHTED_RANDOM_DPF:
         if(tlvLength >= sizeof(struct rserpool_policy_weighted_random_dpf)) {
            wrddpf = (struct rserpool_policy_weighted_random_dpf*)getSpace(message, sizeof(struct rserpool_policy_weighted_random_dpf));
            if(wrddpf == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(wrddpf->pp_wrddpf_policy);
            poolPolicySettings->Weight     = ntohl(wrddpf->pp_wrddpf_weight);
            poolPolicySettings->Load       = 0;
            poolPolicySettings->WeightDPF  = ntohl(wrddpf->pp_wrddpf_weight_dpf);
            poolPolicySettings->Distance   = ntohl(wrddpf->pp_wrddpf_distance);

            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy WRAND-DPF, weight=%u, wdpf=%1.3f%%, distance=%u\n",
                    poolPolicySettings->Weight,
                    100.0 * ((double)poolPolicySettings->WeightDPF / (double)PPV_MAX_WEIGHTDPF),
                    poolPolicySettings->Distance);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("WRAND-DPF TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_ROUNDROBIN:
         if(tlvLength >= sizeof(struct rserpool_policy_roundrobin)) {
            rr = (struct rserpool_policy_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_roundrobin));
            if(rr == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(rr->pp_rr_policy);
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
         if(tlvLength >= sizeof(struct rserpool_policy_weighted_roundrobin)) {
            wrr = (struct rserpool_policy_weighted_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_weighted_roundrobin));
            if(wrr == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(wrr->pp_wrr_policy);
            poolPolicySettings->Weight     = ntohl(wrr->pp_wrr_weight);
            poolPolicySettings->Load       = 0;
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy WRR, weight=%u\n",poolPolicySettings->Weight);
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
      case PPT_PRIORITY:
         if(tlvLength >= sizeof(struct rserpool_policy_priority)) {
            p = (struct rserpool_policy_priority*)getSpace(message, sizeof(struct rserpool_policy_priority));
            if(p == NULL) {
               message->Error = RSPERR_INVALID_VALUES;
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(p->pp_p_policy);
            poolPolicySettings->Weight     = ntohl(p->pp_p_priority);
            poolPolicySettings->Load       = 0;
            LOG_VERBOSE3
            fprintf(stdlog, "Scanned policy PRI, priority=%u\n",poolPolicySettings->Weight);
            LOG_END
         }
         else {
            LOG_WARNING
            fputs("PRI TLV too short\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }
       break;
      case PPT_RANDOMIZED_LEASTUSED:
         if(tlvLength >= sizeof(struct rserpool_policy_randomized_leastused)) {
            rlu = (struct rserpool_policy_randomized_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused));
            if(rlu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(rlu->pp_rlu_policy);
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntohl(rlu->pp_rlu_load);
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
         if(tlvLength >= sizeof(struct rserpool_policy_randomized_leastused_degradation)) {
            rlud = (struct rserpool_policy_randomized_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused_degradation));
            if(rlud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = ntohl(rlud->pp_rlud_policy);
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntohl(rlud->pp_rlud_load);
            poolPolicySettings->LoadDegradation = ntohl(rlud->pp_rlud_loaddeg);
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
         if(tlvLength >= sizeof(struct rserpool_policy_randomized_priority_leastused)) {
            rplu = (struct rserpool_policy_randomized_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused));
            if(rplu == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType = ntohl(rplu->pp_rplu_policy);
            poolPolicySettings->Weight     = 0;
            poolPolicySettings->Load       = ntohl(rplu->pp_rplu_load);
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
         if(tlvLength >= sizeof(struct rserpool_policy_randomized_priority_leastused_degradation)) {
            rplud = (struct rserpool_policy_randomized_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused_degradation));
            if(rplud == NULL) {
               return(false);
            }
            poolPolicySettings->PolicyType      = ntohl(rplud->pp_rplud_policy);
            poolPolicySettings->Weight          = 0;
            poolPolicySettings->Load            = ntohl(rplud->pp_rplud_load);
            poolPolicySettings->LoadDegradation = ntohl(rplud->pp_rplud_loaddeg);
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
         LOG_WARNING
         fprintf(stdlog, "Unsupported policy $%08x\n", policyType);
         LOG_END
         message->Error = RSPERR_UNSUPPORTED_POOL_POLICY;
         return(false);
       break;
   }

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan pool element paramter ##################################### */
static struct ST_CLASS(PoolElementNode)* scanPoolElementParameter(
                                            struct RSerPoolMessage* message,
                                            const bool              registratorTransportRequired,
                                            const bool              mustHaveHomeRegistrar)
{
   struct rserpool_poolelementparameter* pep;
   char                                  userTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*         userTransportAddressBlock = (struct TransportAddressBlock*)&userTransportAddressBlockBuffer;
   struct TransportAddressBlock*         newUserTransportAddressBlock;
   char                                  registratorTransportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*         registratorTransportAddressBlock = (struct TransportAddressBlock*)&registratorTransportAddressBlockBuffer;
   struct TransportAddressBlock*         newRegistratorTransportAddressBlock;
   bool                                  hasRegistratorTransportAddressBlock = false;
   struct PoolPolicySettings             poolPolicySettings;
   struct ST_CLASS(PoolElementNode)*     poolElementNode;

   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_ELEMENT, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      message->Error = RSPERR_INVALID_VALUES;
      return(NULL);
   }

   pep = (struct rserpool_poolelementparameter*)getSpace(message, sizeof(struct rserpool_poolelementparameter));
   if(pep == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(NULL);
   }

   if( (mustHaveHomeRegistrar) && (pep->pep_homeserverid == UNDEFINED_REGISTRAR_IDENTIFIER) ) {
      message->Error = RSPERR_INVALID_VALUES;
      return(NULL);
   }

   if(scanTransportParameter(message, userTransportAddressBlock) == false) {
      return(NULL);
   }

   if(scanPolicyParameter(message, &poolPolicySettings) == false) {
      return(NULL);
   }

   if(registratorTransportRequired) {
      if(scanTransportParameter(message, registratorTransportAddressBlock)) {
         hasRegistratorTransportAddressBlock = true;
      }
   }

   if(checkFinishTLV(message, tlvPosition) == false) {
      return(NULL);
   }

   poolElementNode = (struct ST_CLASS(PoolElementNode)*)malloc(sizeof(struct ST_CLASS(PoolElementNode)));
   if(poolElementNode == NULL) {
      message->Error = RSPERR_OUT_OF_MEMORY;
      return(NULL);
   }
   newUserTransportAddressBlock = transportAddressBlockDuplicate(userTransportAddressBlock);
   if(newUserTransportAddressBlock == NULL) {
      free(poolElementNode);
      message->Error = RSPERR_OUT_OF_MEMORY;
      return(NULL);
   }
   if(hasRegistratorTransportAddressBlock) {
      newRegistratorTransportAddressBlock = transportAddressBlockDuplicate(registratorTransportAddressBlock);
      if(registratorTransportAddressBlock == NULL) {
         free(newUserTransportAddressBlock);
         free(poolElementNode);
         message->Error = RSPERR_OUT_OF_MEMORY;
         return(NULL);
      }
   }
   else {
      newRegistratorTransportAddressBlock = NULL;
   }
   ST_CLASS(poolElementNodeNew)(poolElementNode,
                                ntohl(pep->pep_identifier),
                                ntohl(pep->pep_homeserverid),
                                ntohl(pep->pep_reg_life),
                                &poolPolicySettings,
                                newUserTransportAddressBlock,
                                newRegistratorTransportAddressBlock,
                                -1, 0);

   LOG_VERBOSE5
   fputs("Successfully scanned pool element parameter: ", stdlog);
   ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
   LOG_END

   return(poolElementNode);
}


/* ###### Scan pool handle paramter ###################################### */
static bool scanPoolHandleParameter(struct RSerPoolMessage* message,
                                    struct PoolHandle*      poolHandlePtr)
{
   unsigned char* poolHandle;
   size_t tlvPosition = 0;
   size_t tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_HANDLE, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
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
   fprintf(stdlog, ", length=%u.\n", (unsigned int)poolHandlePtr->Size);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan pool element identifier parameter ######################### */
static bool scanPoolElementIdentifierParameter(struct RSerPoolMessage* message)
{
   uint32_t* identifier;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_IDENTIFIER, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
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
   fprintf(stdlog, "Scanned pool element identifier $%08x\n", message->Identifier);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan pool element checksum parameter ########################### */
static bool scanHandlespaceChecksumParameter(struct RSerPoolMessage* message)
{
   uint16_t* checksum;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_CHECKSUM, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
   if(tlvLength != sizeof(uint16_t)) {
      LOG_WARNING
      fputs("Pool element checksum too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   checksum = (uint16_t*)getSpace(message, tlvLength);
   if(checksum == NULL) {
      return(false);
   }
   message->Checksum = *checksum;

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned pool element checksum $%08x\n", message->Checksum);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan error parameter ########################################### */
static bool scanErrorParameter(struct RSerPoolMessage* message)
{
   struct rserpool_errorcause* aec;
   char*                       data;
   size_t                      dataLength;
   size_t                      tlvPosition = 0;
   size_t                      tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_OPERATION_ERROR, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
   if(tlvLength < sizeof(struct rserpool_errorcause)) {
      LOG_WARNING
      fputs("Error parameter TLV too short\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   aec = (struct rserpool_errorcause*)&message->Buffer[message->Position];
   message->OperationErrorCode = ntohs(aec->aec_cause);
   dataLength                  = ntohs(aec->aec_length);

   if(dataLength < sizeof(struct rserpool_errorcause)) {
      LOG_WARNING
      fputs("Cause length too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   dataLength -= sizeof(struct rserpool_errorcause);
   data = (char*)getSpace(message,dataLength);
   if(data == NULL) {
      return(false);
   }

   message->OperationErrorData   = data;
   message->OperationErrorLength = dataLength;

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan cookie parameter ########################################## */
static bool scanCookieParameter(struct RSerPoolMessage* message)
{
   void*     cookie;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_COOKIE, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
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
   memcpy(message->CookiePtr,cookie, message->CookieSize);

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned cookie, length=%d\n",(int)message->CookieSize);
   LOG_END

   return(true);
}


/* ###### Scan server information paramter ############################### */
static struct ST_CLASS(PeerListNode)* scanServerInformationParameter(struct RSerPoolMessage* message)
{
   char                                 transportAddressBlockBuffer[transportAddressBlockGetSize(MAX_PE_TRANSPORTADDRESSES)];
   struct TransportAddressBlock*        transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;
   struct TransportAddressBlock*        newTransportAddressBlock;
   struct ST_CLASS(PeerListNode)*       peerListNode;
   struct rserpool_serverinfoparameter* sip;
   size_t                               tlvPosition = 0;
   size_t                               tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_SERVER_INFORMATION, true);

   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
   if(tlvLength < 1) {
      LOG_WARNING
      fputs("Server information too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   sip = (struct rserpool_serverinfoparameter*)getSpace(message, sizeof(struct rserpool_serverinfoparameter));
   if(sip == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   if(scanTransportParameter(message, transportAddressBlock) == false) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   peerListNode = (struct ST_CLASS(PeerListNode)*)malloc(sizeof(struct ST_CLASS(PeerListNode)));
   if(peerListNode == NULL) {
      message->Error = RSPERR_OUT_OF_MEMORY;
      return(false);
   }

   newTransportAddressBlock = transportAddressBlockDuplicate(transportAddressBlock);
   if(newTransportAddressBlock == NULL) {
      message->Error = RSPERR_OUT_OF_MEMORY;
      free(peerListNode);
      return(false);
   }

   ST_CLASS(peerListNodeNew)(peerListNode,
                             ntohl(sip->sip_server_id),
                             0,
                             newTransportAddressBlock);

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned server information parameter for registrar $%08x, flags=$%08x, address=",
           peerListNode->Identifier, peerListNode->Flags);
   transportAddressBlockPrint(transportAddressBlock, stdlog);
   fputs("\n", stdlog);
   LOG_END

   if(checkFinishTLV(message, tlvPosition) == false) {
      free(newTransportAddressBlock);
      free(peerListNode);
      return(false);
   }

   return(peerListNode);
}


/* ###### Scan pool element checksum parameter ########################### */
static bool scanHandleResolutionParameter(struct RSerPoolMessage* message)
{
   struct rserpool_handleresolutionparameter* hrp;
   size_t    tlvPosition = 0;
   size_t    tlvLength   = checkBeginTLV(message, &tlvPosition, ATT_HANDLE_RESOLUTION, true);
   if(tlvLength < sizeof(struct rserpool_tlv_header)) {
      return(false);
   }

   tlvLength -= sizeof(struct rserpool_tlv_header);
   if(tlvLength < sizeof(struct rserpool_handleresolutionparameter)) {
      LOG_WARNING
      fputs("Handle resolution parameter too short!\n", stdlog);
      LOG_END
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }

   hrp = (struct rserpool_handleresolutionparameter*)getSpace(message, sizeof(struct rserpool_handleresolutionparameter));
   if(hrp == NULL) {
      return(false);
   }
   if(hrp->hrp_items == 0xffffffff) {
      message->Addresses = RSPGETADDRS_MAX;
   }
   else {
      message->Addresses = ntohl(hrp->hrp_items);
   }

   LOG_VERBOSE3
   fprintf(stdlog, "Scanned handle resolution parameter, items=%u\n",
           (unsigned int)message->Addresses);
   LOG_END

   return(checkFinishTLV(message, tlvPosition));
}


/* ###### Scan endpoint keepalive message ################################ */
static bool scanEndpointKeepAliveMessage(struct RSerPoolMessage* message)
{
   uint32_t* registrarIdentifier;

   registrarIdentifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(registrarIdentifier == NULL) {
      return(false);
   }
   message->RegistrarIdentifier = ntohl(*registrarIdentifier);

   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   /* ------ Custom parameter ------ */
   if(scanPoolElementIdentifierParameter(message) == false) {
      message->Identifier = 0;
   }
   /* ------------------------------ */

   return(true);
}


/* ###### Scan endpoint keepalive acknowledgement message ################ */
static bool scanEndpointKeepAliveAckMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan endpoint unreachable message ############################## */
static bool scanEndpointUnreachableMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan registration message ###################################### */
static bool scanRegistrationMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   message->PoolElementPtr = scanPoolElementParameter(message, false, false);
   if(message->PoolElementPtr == NULL) {
      return(false);
   }
   if(message->PoolElementPtr->RegistratorTransport != NULL) {
      message->Error = RSPERR_INVALID_REGISTRATOR;
      return(false);
   }

   return(true);
}


/* ###### Scan deregistration message #################################### */
static bool scanDeregistrationMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanPoolElementIdentifierParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan registration response message ############################# */
static bool scanRegistrationResponseMessage(struct RSerPoolMessage* message)
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


/* ###### Scan deregistration response message ########################### */
static bool scanDeregistrationResponseMessage(struct RSerPoolMessage* message)
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


/* ###### Scan handle resolution message ################################# */
static bool scanHandleResolutionMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(scanHandleResolutionParameter(message) == false) {
      message->Addresses = 0;
   }
   return(true);
}


/* ###### Scan handle resolution response message ######################## */
static bool scanHandleResolutionResponseMessage(struct RSerPoolMessage* message)
{
   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   if(!scanErrorParameter(message)) {
      if(scanPolicyParameter(message, &message->PolicySettings) == false) {
         return(false);
      }

      message->PoolElementPtrArraySize = 0;
      while(message->Position < message->BufferSize) {
         if(message->PoolElementPtrArraySize >= MAX_MAX_HANDLE_RESOLUTION_ITEMS) {
            LOG_WARNING
            fputs("Too many Pool Element Parameters in Handle Resolution Response\n", stdlog);
            LOG_END
            return(false);
         }
         message->PoolElementPtrArray[message->PoolElementPtrArraySize] =
            scanPoolElementParameter(message, false, false);
         if(message->PoolElementPtrArray[message->PoolElementPtrArraySize] == false) {
            break;
         }
         message->PoolElementPtrArraySize++;
      }
   }

   return(true);
}


/* ###### Scan handle resolution response message ######################## */
static bool scanServerAnnounceMessage(struct RSerPoolMessage* message)
{
   uint32_t*                     registrarIdentifier;
   char                          transportAddressBlockBuffer[transportAddressBlockGetSize(1)];
   struct TransportAddressBlock* transportAddressBlock = (struct TransportAddressBlock*)&transportAddressBlockBuffer;

   registrarIdentifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(registrarIdentifier == NULL) {
      return(false);
   }
   message->RegistrarIdentifier = ntohl(*registrarIdentifier);

   /* ====== Try to read Server Information Parameter ==================== */
   message->PeerListNodePtr = scanServerInformationParameter(message);
   if(message->PeerListNodePtr == NULL) {
      /* ====== Take address and port from UDP message =================== */
      message->PeerListNodePtr = (struct ST_CLASS(PeerListNode)*)malloc(sizeof(struct ST_CLASS(PeerListNode)));
      if(message->PeerListNodePtr == NULL) {
         message->Error = RSPERR_OUT_OF_MEMORY;
         return(false);
      }
      transportAddressBlockNew(&transportAddressBlock[0],
                               IPPROTO_SCTP,
                               getPort(&message->SourceAddress.sa),
                               0,
                               &message->SourceAddress, 1, 1);
      transportAddressBlock = transportAddressBlockDuplicate(transportAddressBlock);
      if(transportAddressBlock == NULL) {
         message->Error = RSPERR_OUT_OF_MEMORY;
         free(message->PeerListNodePtr);
         message->PeerListNodePtr = NULL;
         return(false);
      }
      ST_CLASS(peerListNodeNew)(message->PeerListNodePtr,
                                message->RegistrarIdentifier,
                                PLNF_DYNAMIC,
                                transportAddressBlock);
   }

   return(true);
}


/* ###### Scan cookie message ################################################ */
static bool scanCookieMessage(struct RSerPoolMessage* message)
{
   if(scanCookieParameter(message) == false) {
      return(false);
   }
   return(true);
}


/* ###### Scan cookie echo message ########################################### */
static bool scanCookieEchoMessage(struct RSerPoolMessage* message)
{
   if(scanCookieParameter(message) == false) {
      return(false);
   }

   return(true);
}


/* ###### Scan business card message ######################################### */
static bool scanBusinessCardMessage(struct RSerPoolMessage* message)
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
         if(message->PoolElementPtrArraySize >= MAX_MAX_HANDLE_RESOLUTION_ITEMS) {
            LOG_WARNING
            fputs("Too many Pool Element Parameters in Business Card\n", stdlog);
            LOG_END
            return(false);
         }
         message->PoolElementPtrArray[message->PoolElementPtrArraySize] =
            scanPoolElementParameter(message, false, false);
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


/* ###### Scan peer presence message ##################################### */
static bool scanPresenceMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   if(scanHandlespaceChecksumParameter(message) == false) {
      return(false);
   }

   message->PeerListNodePtr = scanServerInformationParameter(message);
   if(message->PeerListNodePtr == NULL) {
      return(false);
   }

   return(true);
}


/* ###### Scan peer list request message ################################# */
static bool scanListRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   return(true);
}


/* ###### Scan peer list response message ################################ */
static bool scanListResponseMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;
   struct ST_CLASS(PeerListNode)*   peerListNode;
   struct ST_CLASS(PeerListNode)*   newPeerListNode;
   unsigned int                     errorCode;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   if(!(message->Flags & EHF_LIST_RESPONSE_REJECT)) {
      while(message->Position < message->BufferSize) {
         peerListNode = scanServerInformationParameter(message);
         if(peerListNode == NULL) {
            break;
         }

         if(message->PeerListPtr == NULL) {
            message->PeerListPtr = (struct ST_CLASS(PeerListManagement)*)malloc(sizeof(struct ST_CLASS(PeerListManagement)));
            if(message->PeerListPtr == NULL) {
               message->Error = RSPERR_OUT_OF_MEMORY;
               free(peerListNode);
               return(false);
            }
            ST_CLASS(peerListManagementNew)(message->PeerListPtr, 0, 0, NULL, NULL);
         }

         peerListNode->Flags |= PLNF_FROM_PEER;
         if(peerListNode->Identifier != UNDEFINED_REGISTRAR_IDENTIFIER) {
            peerListNode->Flags |= PLNF_DYNAMIC;
         }

         errorCode = ST_CLASS(peerListManagementRegisterPeerListNode)(
                        message->PeerListPtr,
                        peerListNode->Identifier,
                        peerListNode->Flags,
                        peerListNode->AddressBlock,
                        0,
                        &newPeerListNode);
         if(errorCode != RSPERR_OKAY) {
            LOG_WARNING
            fputs("ListResponse contains bad peer ", stdlog);
            ST_CLASS(peerListNodePrint)(peerListNode, stdlog, PLPO_FULL);
            fputs(". Unable to add it: ", stdlog);
            rserpoolErrorPrint(errorCode, stdlog);
            fputs("\n", stdlog);
            LOG_END
            ST_CLASS(peerListNodeDelete)(peerListNode);
            free(peerListNode);
            message->Error = (uint16_t)errorCode;
            return(false);
         }

         free(peerListNode->AddressBlock);
         peerListNode->AddressBlock = NULL;
         ST_CLASS(peerListNodeDelete)(peerListNode);
         free(peerListNode);
      }
   }

   return(true);
}


/* ###### Scan peer handle table request message ########################### */
static bool scanHandleTableRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   return(true);
}


/* ###### Scan peer handle table response message ########################## */
static bool scanHandleTableResponseMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter*  sp;
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   struct ST_CLASS(PoolElementNode)* newPoolElementNode;
   size_t                            scannedPoolElementParameters;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   if(!(message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT)) {
      message->HandlespacePtr = (struct ST_CLASS(PoolHandlespaceManagement)*)malloc(sizeof(struct ST_CLASS(PoolHandlespaceManagement)));
      if(message->HandlespacePtr == NULL) {
         message->Error = RSPERR_OUT_OF_MEMORY;
         return(false);
      }
      ST_CLASS(poolHandlespaceManagementNew)(message->HandlespacePtr, 0, NULL, NULL, NULL);

      while( (message->Error == RSPERR_OKAY) &&
             (peekNextTLVType(message) == ATT_POOL_HANDLE) &&
             ( ((scanPoolHandleParameter(message, &message->Handle)) == true) ) ) {
         scannedPoolElementParameters = 0;

         while( (message->Error == RSPERR_OKAY) &&
                (peekNextTLVType(message) == ATT_POOL_ELEMENT) &&
                ( (poolElementNode = scanPoolElementParameter(message, true, true)) != NULL ) ) {
            if(poolElementNode->RegistratorTransport == NULL) {
               free(poolElementNode->UserTransport);
               free(poolElementNode);
               message->Error = RSPERR_INVALID_REGISTRATOR;
               return(false);
            }
            scannedPoolElementParameters++;

            message->Error = ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                                message->HandlespacePtr,
                                &message->Handle,
                                poolElementNode->HomeRegistrarIdentifier,
                                poolElementNode->Identifier,
                                poolElementNode->RegistrationLife,
                                &poolElementNode->PolicySettings,
                                poolElementNode->UserTransport,
                                poolElementNode->RegistratorTransport,
                                -1, 0,
                                0,
                                &newPoolElementNode);

            /* These structures were used temporarily only */
            free(poolElementNode->UserTransport);
            free(poolElementNode->RegistratorTransport);
            free(poolElementNode);

            if(message->Error != RSPERR_OKAY) {
               LOG_WARNING
               fputs("HandleTableResponse contains bad/inconsistent entry: ", stdlog);
               ST_CLASS(poolElementNodePrint)(poolElementNode, stdlog, PENPO_FULL);
               fputs(" - Unable to use it: ", stdlog);
               rserpoolErrorPrint(message->Error, stdlog);
               fputs("\n", stdlog);
               LOG_END
               return(false);
            }
         }
         if(message->Error != RSPERR_OKAY) {
            return(false);
         }

         if(scannedPoolElementParameters == 0) {
            LOG_WARNING
            fputs("HandleTableResponse contains empty pool\n", stdlog);
            LOG_END
            message->Error = RSPERR_INVALID_VALUES;
            return(false);
         }

         LOG_VERBOSE5
         fputs("Successfully scanned data from HandleTableResponse:\n", stdlog);
         ST_CLASS(poolHandlespaceManagementPrint)(message->HandlespacePtr, stdlog, PENPO_FULL);
         LOG_END
      }
   }

   return(true);
}


/* ###### Scan peer handle table response message ########################## */
static bool scanHandleUpdateMessage(struct RSerPoolMessage* message)
{
   struct rserpool_handleupdateparameter* pnup;

   pnup = (struct rserpool_handleupdateparameter*)getSpace(message, sizeof(struct rserpool_handleupdateparameter));
   if(pnup == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(pnup->pnup_sender_id);
   message->ReceiverID = ntohl(pnup->pnup_receiver_id);
   message->Action     = ntohs(pnup->pnup_update_action);

   if(scanPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   message->PoolElementPtr = scanPoolElementParameter(message, true, true);
   if(message->PoolElementPtr == NULL) {
      return(false);
   }
   if(message->PoolElementPtr->RegistratorTransport == NULL) {
      message->Error = RSPERR_INVALID_REGISTRATOR;
      return(false);
   }

   return(true);
}


/* ###### Scan peer init takeover message ################################ */
static bool scanInitTakeoverMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID            = ntohl(tp->tp_sender_id);
   message->ReceiverID          = ntohl(tp->tp_receiver_id);
   message->RegistrarIdentifier = ntohl(tp->tp_target_id);

   return(true);
}


/* ###### Scan peer init takeover acknowledgement message ################ */
static bool scanInitTakeoverAckMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID            = ntohl(tp->tp_sender_id);
   message->ReceiverID          = ntohl(tp->tp_receiver_id);
   message->RegistrarIdentifier = ntohl(tp->tp_target_id);

   return(true);
}


/* ###### Scan peer takeover server message ############################## */
static bool scanTakeoverServerMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID            = ntohl(tp->tp_sender_id);
   message->ReceiverID          = ntohl(tp->tp_receiver_id);
   message->RegistrarIdentifier = ntohl(tp->tp_target_id);

   return(true);
}


/* ###### Scan ENRP error message ######################################## */
static bool scanENRPErrorMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      message->Error = RSPERR_INVALID_VALUES;
      return(false);
   }
   message->SenderID   = ntohl(sp->sp_sender_id);
   message->ReceiverID = ntohl(sp->sp_receiver_id);

   if(!scanErrorParameter(message)) {
      return(false);
   }

   return(true);
}


/* ###### Scan ASAP error message ######################################## */
static bool scanASAPErrorMessage(struct RSerPoolMessage* message)
{
   if(!scanErrorParameter(message)) {
      return(false);
   }
   return(true);
}


/* ###### Scan message ################################################### */
static bool scanMessage(struct RSerPoolMessage* message)
{
   struct rserpool_header* header;
   size_t                  startPosition;
   size_t                  length;
   size_t                  i, j;

   LOG_VERBOSE5
   fprintf(stdlog, "Incoming message (%u bytes):\n", (unsigned int)message->BufferSize);
   for(i = 0;i < message->BufferSize;i += 32) {
      fprintf(stdlog, "%4u: ", (unsigned int)i);
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
   if(length < sizeof(struct rserpool_header)) {
      return(false);
   }

   header        = (struct rserpool_header*)&message->Buffer[startPosition];
   message->Type = header->ah_type;
   if(message->PPID == PPID_ASAP) {
      message->Type |= AHT_ASAP_MODIFIER;
   }
   else if(message->PPID == PPID_ENRP) {
      message->Type |= EHT_ENRP_MODIFIER;
   }
   else {
      LOG_ERROR
      fprintf(stdlog, "Wrong PPID ($%08x), expected ASAP or ENRP\n", message->PPID);
      LOG_END
      return(false);
   }

   switch(message->Type) {
      /* ====== ASAP ===================================================== */
      case AHT_HANDLE_RESOLUTION:
         LOG_VERBOSE2
         fputs("Scanning HandleResolution message...\n", stdlog);
         LOG_END
         if(scanHandleResolutionMessage(message) == false) {
            return(false);
         }
       break;
      case AHT_HANDLE_RESOLUTION_RESPONSE:
         LOG_VERBOSE2
         fputs("Scanning HandleResolutionResponse message...\n", stdlog);
         LOG_END
         if(scanHandleResolutionResponseMessage(message) == false) {
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
      case AHT_ERROR:
          LOG_VERBOSE2
          fputs("Scanning Error (ASAP) message...\n", stdlog);
          LOG_END
          if(scanASAPErrorMessage(message) == false) {
             return(false);
          }
       break;

       /* ====== ENRP ==================================================== */
      case EHT_PRESENCE:
          LOG_VERBOSE2
          fputs("Scanning Presence message...\n", stdlog);
          LOG_END
          if(scanPresenceMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_HANDLE_TABLE_REQUEST:
          LOG_VERBOSE2
          fputs("Scanning HandleTableRequest message...\n", stdlog);
          LOG_END
          if(scanHandleTableRequestMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_HANDLE_TABLE_RESPONSE:
          LOG_VERBOSE2
          fputs("Scanning HandleTableResponse message...\n", stdlog);
          LOG_END
          if(scanHandleTableResponseMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_HANDLE_UPDATE:
          LOG_VERBOSE2
          fputs("Scanning HandleUpdate message...\n", stdlog);
          LOG_END
          if(scanHandleUpdateMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_LIST_REQUEST:
          LOG_VERBOSE2
          fputs("Scanning ListRequest message...\n", stdlog);
          LOG_END
          if(scanListRequestMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_LIST_RESPONSE:
          LOG_VERBOSE2
          fputs("Scanning ListResponse message...\n", stdlog);
          LOG_END
          if(scanListResponseMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_INIT_TAKEOVER:
          LOG_VERBOSE2
          fputs("Scanning InitTakeover message...\n", stdlog);
          LOG_END
          if(scanInitTakeoverMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_INIT_TAKEOVER_ACK:
          LOG_VERBOSE2
          fputs("Scanning InitTakeoverAck message...\n", stdlog);
          LOG_END
          if(scanInitTakeoverAckMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_TAKEOVER_SERVER:
          LOG_VERBOSE2
          fputs("Scanning TakeoverServer message...\n", stdlog);
          LOG_END
          if(scanTakeoverServerMessage(message) == false) {
             return(false);
          }
       break;
      case EHT_ERROR:
          LOG_VERBOSE2
          fputs("Scanning Error (ENRP) message...\n", stdlog);
          LOG_END
          if(scanENRPErrorMessage(message) == false) {
             return(false);
          }
       break;

       /* ====== Unknown ================================================= */
      default:
         LOG_WARNING
         fprintf(stdlog, "Unknown message type $%04x!\n", message->Type);
         LOG_END
         return(false);
       break;
   }

   return(checkFinishMessage(message,startPosition));
}


/* ###### Convert packet to RSerPoolMessage ############################## */
unsigned int rserpoolPacket2Message(char*                       packet,
                                    const union sockaddr_union* sourceAddress,
                                    const sctp_assoc_t          assocID,
                                    const uint32_t              ppid,
                                    const size_t                packetSize,
                                    const size_t                minBufferSize,
                                    struct RSerPoolMessage**    message)
{
   *message = rserpoolMessageNew(packet, packetSize);
   if(*message != NULL) {
      if(sourceAddress) {
         memcpy(&(*message)->SourceAddress, sourceAddress, sizeof(*sourceAddress));
      }
      else {
         memset(&(*message)->SourceAddress, 0, sizeof(*sourceAddress));
      }
      (*message)->AssocID                                = assocID;
      (*message)->PPID                                   = ppid;
      (*message)->OriginalBufferSize                     = max(packetSize, minBufferSize);
      (*message)->Position                               = 0;
      (*message)->PoolElementPtrAutoDelete               = true;
      (*message)->CookiePtrAutoDelete                    = true;
      (*message)->PoolElementPtrArrayAutoDelete          = true;
      (*message)->TransportAddressBlockListPtrAutoDelete = true;
      (*message)->HandlespacePtrAutoDelete               = true;
      (*message)->PeerListNodePtrAutoDelete              = true;
      (*message)->PeerListPtrAutoDelete                  = true;

      LOG_VERBOSE3
      fprintf(stdlog, "Scanning message, size=%u...\n",
              (unsigned int)packetSize);
      LOG_END

      if(scanMessage(*message) == true) {
         LOG_VERBOSE3
         fputs("Message successfully scanned!\n", stdlog);
         LOG_END
         (*message)->Error = (*message)->OperationErrorCode;
         return(RSPERR_OKAY);
      }

      LOG_WARNING
      fprintf(stdlog, "Error while parsing message at byte %u (TLV at position %lu, length %lu): ",
              (unsigned int)(*message)->Position,
              ((*message)->OffendingParameterTLV != NULL) ? ((unsigned long)(*message)->OffendingParameterTLV - (unsigned long)(*message)->Buffer) : 0,
              (unsigned long)(*message)->OffendingParameterTLVLength);
      rserpoolErrorPrint((*message)->Error, stdlog);
      fputs("\n", stdlog);
      LOG_END
      return(RSPERR_OKAY);
   }
   return(RSPERR_OUT_OF_MEMORY);
}
