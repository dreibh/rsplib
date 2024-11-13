/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2025 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rserpoolmessagecreator.h"
#include "rserpool.h"

#include <netinet/in.h>
#include <ext_socket.h>


/* ###### Begin message ################################################## */
static struct rserpool_header* beginMessage(struct RSerPoolMessage* message,
                                            const unsigned int      type,
                                            const uint8_t           flags,
                                            const uint32_t          ppid)
{
   struct rserpool_header* header = (struct rserpool_header*)getSpace(message,sizeof(struct rserpool_header));
   if(header == NULL) {
      return(NULL);
   }
   message->PPID = ppid;

   header->ah_type   = (uint8_t)(type & 0xff);
   header->ah_flags  = flags;
   header->ah_length = 0xffff;
   return(header);
}


/* ###### Finish message ################################################# */
static bool finishMessage(struct RSerPoolMessage* message)
{
   char*  pad;
   size_t padding = getPadding(message->Position,4);

   struct rserpool_header* header = (struct rserpool_header*)message->Buffer;
   if(message->BufferSize >= sizeof(struct rserpool_header)) {
      header->ah_length = htons((uint16_t)message->Position);

      pad = (char*)getSpace(message, padding);
      memset(pad, 0, padding);
      return(true);
   }
   return(false);
}


/* ###### Begin TLV ###################################################### */
static bool beginTLV(struct RSerPoolMessage* message,
                     size_t*                 tlvPosition,
                     const uint16_t          type)
{
   struct rserpool_tlv_header* header;

   *tlvPosition = message->Position;
   header = (struct rserpool_tlv_header*)getSpace(message, sizeof(struct rserpool_tlv_header));
   if(header == NULL) {
      return(false);
   }

   header->atlv_type   = htons(type);
   header->atlv_length = 0xffff;
   return(true);
}


/* ###### Finish TLV ##################################################### */
static bool finishTLV(struct RSerPoolMessage* message,
                      const size_t            tlvPosition)
{
   struct rserpool_tlv_header* header  = (struct rserpool_tlv_header*)&message->Buffer[tlvPosition];
   size_t                      length  = message->Position - tlvPosition;
   size_t                      padding = getPadding(length,4);
   char*                       pad;

   if(message->BufferSize >= sizeof(struct rserpool_tlv_header)) {
      header->atlv_length = htons(length);

      pad = (char*)getSpace(message, padding);
      memset(pad, 0, padding);
      return(true);
   }
   return(false);
}


/* ###### Create pool handle parameter ################################### */
static bool createPoolHandleParameter(struct RSerPoolMessage*  message,
                                      const struct PoolHandle* poolHandle)
{
   char*    handle;
   size_t tlvPosition = 0;

   if(poolHandle == NULL) {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   if(beginTLV(message, &tlvPosition, ATT_POOL_HANDLE) == false) {
      return(false);
   }

   handle = (char*)getSpace(message, poolHandle->Size);
   if(handle == NULL) {
      return(false);
   }
   memcpy(handle, &poolHandle->Handle, poolHandle->Size);

   return(finishTLV(message, tlvPosition));
}


/* ###### Create address parameter ####################################### */
static bool createAddressParameter(struct RSerPoolMessage* message,
                                   const struct sockaddr*  address)
{
   size_t                     tlvPosition = 0;
   char*                      output;
   const struct sockaddr_in*  in;
   const struct sockaddr_in6* in6;

   switch(address->sa_family) {
      case AF_INET:
         if(beginTLV(message, &tlvPosition, ATT_IPv4_ADDRESS) == false) {
            return(false);
         }
         in = (struct sockaddr_in*)address;
         output = (char*)getSpace(message, 4);
         if(output == NULL) {
            return(false);
         }
         memcpy(output, &in->sin_addr, 4);
       break;
      case AF_INET6:
         in6 = (const struct sockaddr_in6*)address;
         if(!IN6_IS_ADDR_V4MAPPED((const struct in6_addr*)&(in6->sin6_addr))) {
            if(beginTLV(message, &tlvPosition, ATT_IPv6_ADDRESS) == false) {
               return(false);
            }
            output = (char*)getSpace(message, 16);
            if(output == NULL) {
               return(false);
            }
            memcpy(output, &in6->sin6_addr, 16);
          }
          else {
             if(beginTLV(message, &tlvPosition, ATT_IPv4_ADDRESS) == false) {
                return(false);
             }
             output = (char*)getSpace(message, 4);
             if(output == NULL) {
                return(false);
             }
#if defined(__sun) && defined(__SVR4)
             memcpy(output, &in6->sin6_addr._S6_un._S6_u32[3], 4);
#elif defined(__linux__)
             memcpy(output, &in6->sin6_addr.s6_addr32[3], 4);
#else
             memcpy(output, &in6->sin6_addr.__u6_addr.__u6_addr32[3], 4);
#endif
          }
       break;
      default:
         LOG_ERROR
         fprintf(stdlog, "Unknown address family %u\n", address->sa_family);
         LOG_END_FATAL
         return(false);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create transport parameter ###################################### */
static bool createTransportParameter(struct RSerPoolMessage*             message,
                                     const struct TransportAddressBlock* transportAddressBlock)
{
   size_t                                  tlvPosition = 0;
   struct rserpool_udptransportparameter*  utp;
   struct rserpool_tcptransportparameter*  ttp;
   struct rserpool_sctptransportparameter* stp;
   uint16_t                                type;
   uint16_t                                transportUse;
   size_t                                  i;

   if(transportAddressBlock == NULL) {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   switch(transportAddressBlock->Protocol) {
      case IPPROTO_SCTP:
         type = ATT_SCTP_TRANSPORT;
       break;
      case IPPROTO_TCP:
         type = ATT_TCP_TRANSPORT;
       break;
      case IPPROTO_UDP:
         type = ATT_UDP_TRANSPORT;
       break;
#if 0
      case IPPROTO_DCCP:
         type = ATT_DCCP_TRANSPORT;
       break;
#endif
      default:
         LOG_ERROR
         fprintf(stdlog,"Unknown protocol #%d\n", transportAddressBlock->Protocol);
         LOG_END_FATAL
         return(false);
   }

   if(transportAddressBlock->Flags & TABF_CONTROLCHANNEL) {
      transportUse = UTP_DATA_PLUS_CONTROL;
   }
   else {
      transportUse = UTP_DATA_ONLY;
   }

   if(beginTLV(message, &tlvPosition, type) == false) {
      return(false);
   }

   switch(type) {
      case ATT_SCTP_TRANSPORT:
         stp = (struct rserpool_sctptransportparameter*)getSpace(message, sizeof(struct rserpool_sctptransportparameter));
         if(stp == NULL) {
            return(false);
         }
         stp->stp_port          = htons(transportAddressBlock->Port);
         stp->stp_transport_use = htons(transportUse);
       break;

      case ATT_TCP_TRANSPORT:
         ttp = (struct rserpool_tcptransportparameter*)getSpace(message, sizeof(struct rserpool_tcptransportparameter));
         if(ttp == NULL) {
            return(false);
         }
         ttp->ttp_port          = htons(transportAddressBlock->Port);
         ttp->ttp_transport_use = htons(transportUse);
       break;

      case ATT_UDP_TRANSPORT:
         utp = (struct rserpool_udptransportparameter*)getSpace(message, sizeof(struct rserpool_udptransportparameter));
         if(utp == NULL) {
            return(false);
         }
         utp->utp_port     = htons(transportAddressBlock->Port);
         utp->utp_reserved = 0;
       break;

#if 0
      case ATT_DCCP_TRANSPORT:
         dtp = (struct rserpool_dccptransportparameter*)getSpace(message, sizeof(struct rserpool_dccptransportparameter));
         if(dtp == NULL) {
            return(false);
         }
         dtp->dtp_port     = htons(transportAddressBlock->Port);
         dtp->dtp_scode    = 0;
         dtp->dtp_reserved = 0;
       break;
#endif
   }

   for(i = 0;i < transportAddressBlock->Addresses;i++) {
      if(createAddressParameter(message, (struct sockaddr*)&transportAddressBlock->AddressArray[i]) == false) {
         return(false);
      }
      if((i > 0) && (type != ATT_SCTP_TRANSPORT)) {
         LOG_ERROR
         fputs("Multiple addresses for non-multihomed protocol\n", stdlog);
         LOG_END_FATAL
         return(false);
      }
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create policy parameter ######################################## */
static bool createPolicyParameter(struct RSerPoolMessage*          message,
                                  const struct PoolPolicySettings* poolPolicySettings)
{
   size_t                                                            tlvPosition = 0;
   struct rserpool_policy_roundrobin*                                rr;
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

   if(beginTLV(message, &tlvPosition, ATT_POOL_POLICY) == false) {
      return(false);
   }

   if(poolPolicySettings == NULL) {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   switch(poolPolicySettings->PolicyType) {
      case PPT_RANDOM:
          rd = (struct rserpool_policy_random*)getSpace(message, sizeof(struct rserpool_policy_random));
          if(rd == NULL) {
             return(false);
          }
          rd->pp_rd_policy = htonl(poolPolicySettings->PolicyType);
       break;
      case PPT_WEIGHTED_RANDOM:
          wrd = (struct rserpool_policy_weighted_random*)getSpace(message, sizeof(struct rserpool_policy_weighted_random));
          if(wrd == NULL) {
             return(false);
          }
          wrd->pp_wrd_policy = htonl(poolPolicySettings->PolicyType);
          wrd->pp_wrd_weight = htonl(poolPolicySettings->Weight);
       break;
      case PPT_WEIGHTED_RANDOM_DPF:
          wrddpf = (struct rserpool_policy_weighted_random_dpf*)getSpace(message, sizeof(struct rserpool_policy_weighted_random_dpf));
          if(wrddpf == NULL) {
             return(false);
          }
          wrddpf->pp_wrddpf_policy     = htonl(poolPolicySettings->PolicyType);
          wrddpf->pp_wrddpf_weight     = htonl(poolPolicySettings->Weight);
          wrddpf->pp_wrddpf_weight_dpf = htonl(poolPolicySettings->WeightDPF);
          wrddpf->pp_wrddpf_distance   = htonl(poolPolicySettings->Distance);
       break;
      case PPT_ROUNDROBIN:
          rr = (struct rserpool_policy_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_roundrobin));
          if(rr == NULL) {
             return(false);
          }
          rr->pp_rr_policy = htonl(poolPolicySettings->PolicyType);
       break;
      case PPT_WEIGHTED_ROUNDROBIN:
          wrr = (struct rserpool_policy_weighted_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_weighted_roundrobin));
          if(wrr == NULL) {
             return(false);
          }
          wrr->pp_wrr_policy = htonl(poolPolicySettings->PolicyType);
          wrr->pp_wrr_weight = htonl(poolPolicySettings->Weight);
       break;
      case PPT_PRIORITY:
          p = (struct rserpool_policy_priority*)getSpace(message, sizeof(struct rserpool_policy_priority));
          if(p == NULL) {
             return(false);
          }
          p->pp_p_policy   = htonl(poolPolicySettings->PolicyType);
          p->pp_p_priority = htonl(poolPolicySettings->Weight);
       break;
      case PPT_LEASTUSED:
          lu = (struct rserpool_policy_leastused*)getSpace(message, sizeof(struct rserpool_policy_leastused));
          if(lu == NULL) {
             return(false);
          }
          lu->pp_lu_policy = htonl(poolPolicySettings->PolicyType);
          lu->pp_lu_load   = htonl(poolPolicySettings->Load);
       break;
      case PPT_LEASTUSED_DPF:
          ludpf = (struct rserpool_policy_leastused_dpf*)getSpace(message, sizeof(struct rserpool_policy_leastused_dpf));
          if(ludpf == NULL) {
             return(false);
          }
          ludpf->pp_ludpf_policy   = htonl(poolPolicySettings->PolicyType);
          ludpf->pp_ludpf_load     = htonl(poolPolicySettings->Load);
          ludpf->pp_ludpf_load_dpf = htonl(poolPolicySettings->LoadDPF);
          ludpf->pp_ludpf_distance = htonl(poolPolicySettings->Distance);
       break;
      case PPT_LEASTUSED_DEGRADATION:
          lud = (struct rserpool_policy_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_leastused_degradation));
          if(lud == NULL) {
             return(false);
          }
          lud->pp_lud_policy  = htonl(poolPolicySettings->PolicyType);
          lud->pp_lud_load    = htonl(poolPolicySettings->Load);
          lud->pp_lud_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      case PPT_LEASTUSED_DEGRADATION_DPF:
          luddpf = (struct rserpool_policy_leastused_degradation_dpf*)getSpace(message, sizeof(struct rserpool_policy_leastused_degradation_dpf));
          if(luddpf == NULL) {
             return(false);
          }
          luddpf->pp_luddpf_policy   = htonl(poolPolicySettings->PolicyType);
          luddpf->pp_luddpf_load     = htonl(poolPolicySettings->Load);
          luddpf->pp_luddpf_loaddeg  = htonl(poolPolicySettings->LoadDegradation);
          luddpf->pp_luddpf_load_dpf = htonl(poolPolicySettings->LoadDPF);
          luddpf->pp_luddpf_distance = htonl(poolPolicySettings->Distance);
       break;
      case PPT_PRIORITY_LEASTUSED:
          plu = (struct rserpool_policy_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused));
          if(plu == NULL) {
             return(false);
          }
          plu->pp_plu_policy  = htonl(poolPolicySettings->PolicyType);
          plu->pp_plu_load    = htonl(poolPolicySettings->Load);
          plu->pp_plu_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      case PPT_PRIORITY_LEASTUSED_DEGRADATION:
          plud = (struct rserpool_policy_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused_degradation));
          if(plud == NULL) {
             return(false);
          }
          plud->pp_plud_policy  = htonl(poolPolicySettings->PolicyType);
          plud->pp_plud_load    = htonl(poolPolicySettings->Load);
          plud->pp_plud_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      case PPT_RANDOMIZED_LEASTUSED:
          rlu = (struct rserpool_policy_randomized_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused));
          if(rlu == NULL) {
             return(false);
          }
          rlu->pp_rlu_policy = htonl(poolPolicySettings->PolicyType);
          rlu->pp_rlu_load   = htonl(poolPolicySettings->Load);
       break;
      case PPT_RANDOMIZED_LEASTUSED_DEGRADATION:
          rlud = (struct rserpool_policy_randomized_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused_degradation));
          if(rlud == NULL) {
             return(false);
          }
          rlud->pp_rlud_policy  = htonl(poolPolicySettings->PolicyType);
          rlud->pp_rlud_load    = htonl(poolPolicySettings->Load);
          rlud->pp_rlud_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED:
          rplu = (struct rserpool_policy_randomized_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused));
          if(rplu == NULL) {
             return(false);
          }
          rplu->pp_rplu_policy  = htonl(poolPolicySettings->PolicyType);
          rplu->pp_rplu_load    = htonl(poolPolicySettings->Load);
          rplu->pp_rplu_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION:
          rplud = (struct rserpool_policy_randomized_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused_degradation));
          if(rplud == NULL) {
             return(false);
          }
          rplud->pp_rplud_policy  = htonl(poolPolicySettings->PolicyType);
          rplud->pp_rplud_load    = htonl(poolPolicySettings->Load);
          rplud->pp_rplud_loaddeg = htonl(poolPolicySettings->LoadDegradation);
       break;
      default:
         LOG_ERROR
         fprintf(stdlog, "Unknown policy #$%02x\n", poolPolicySettings->PolicyType);
         LOG_END_FATAL
         return(false);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create pool element parameter ################################## */
static bool createPoolElementParameter(
               struct RSerPoolMessage*                 message,
               const struct ST_CLASS(PoolElementNode)* poolElement,
               const bool                              includeRegistratorTransport)
{
   size_t                                tlvPosition = 0;
   struct rserpool_poolelementparameter* pep;

   if(poolElement == NULL) {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   if(beginTLV(message, &tlvPosition, ATT_POOL_ELEMENT) == false) {
      return(false);
   }

   pep = (struct rserpool_poolelementparameter*)getSpace(message, sizeof(struct rserpool_poolelementparameter));
   if(pep == NULL) {
      return(false);
   }

   pep->pep_identifier   = htonl(poolElement->Identifier);
   pep->pep_homeserverid = htonl(poolElement->HomeRegistrarIdentifier);
   pep->pep_reg_life     = htonl(poolElement->RegistrationLife);

   if(createTransportParameter(message, poolElement->UserTransport) == false) {
      return(false);
   }

   if(createPolicyParameter(message, &poolElement->PolicySettings) == false) {
      return(false);
   }

   if(includeRegistratorTransport) {
      CHECK(poolElement->RegistratorTransport);
      if(createTransportParameter(message, poolElement->RegistratorTransport) == false) {
         return(false);
      }
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create pool element identifier parameter ####################### */
static bool createPoolElementIdentifierParameter(
               struct RSerPoolMessage*         message,
               const PoolElementIdentifierType poolElementIdentifier)
{
   uint32_t* identifier;
   size_t    tlvPosition;

   if(beginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_IDENTIFIER) == false) {
      return(false);
   }

   identifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(identifier == NULL) {
      return(false);
   }
   *identifier = htonl(poolElementIdentifier);

   return(finishTLV(message, tlvPosition));
}


/* ###### Create pool element checksum parameter ####################### */
static bool createHandlespaceChecksumParameter(
               struct RSerPoolMessage*       message,
               const HandlespaceChecksumType poolElementChecksum)
{
   uint16_t* checksum;
   size_t    tlvPosition = 0;

   if(beginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_CHECKSUM) == false) {
      return(false);
   }

   checksum = (uint16_t*)getSpace(message, sizeof(uint16_t));
   if(checksum == NULL) {
      return(false);
   }
   *checksum = poolElementChecksum;

   return(finishTLV(message, tlvPosition));
}


/* ###### Create error parameter ######################################### */
static bool createErrorParameter(struct RSerPoolMessage* message)
{
   struct rserpool_errorcause* aec;
   size_t                  tlvPosition = 0;
   uint16_t                cause;
   char*                   data;
   size_t                  dataLength;

   if(beginTLV(message, &tlvPosition, ATT_OPERATION_ERROR) == false) {
      return(false);
   }

   if(message->ErrorCauseParameterTLV == NULL) {
      CHECK(message->ErrorCauseParameterTLVLength == 0);
   }

   cause = message->Error;
   switch(cause) {
      case RSPERR_UNRECOGNIZED_PARAMETER:
      case RSPERR_INVALID_TLV:
      case RSPERR_INVALID_VALUE:
         data       = message->ErrorCauseParameterTLV;
         dataLength = message->ErrorCauseParameterTLVLength;
       break;
      default:
         data       = NULL;
         dataLength = 0;
       break;
   }
   if(data == NULL) {
      dataLength = 0;
   }

   aec = (struct rserpool_errorcause*)getSpace(message,sizeof(struct rserpool_errorcause) + dataLength);
   if(aec == NULL) {
      return(false);
   }

   aec->aec_cause  = htons(cause);
   aec->aec_length = htons(sizeof(struct rserpool_errorcause) + dataLength);
   if(dataLength > 0) {
      memcpy((char*)&aec->aec_data, data, dataLength);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create cookie parameter ######################################### */
static bool createCookieParameter(struct RSerPoolMessage* message,
                                  void*                   cookie,
                                  const size_t            cookieSize)
{
   void*  buffer;
   size_t tlvPosition = 0;
   if(beginTLV(message, &tlvPosition, ATT_COOKIE) == false) {
      return(false);
   }

   buffer = (void*)getSpace(message, cookieSize);
   if(buffer == NULL) {
      return(false);
   }

   memcpy(buffer, cookie, cookieSize);

   return(finishTLV(message, tlvPosition));
}


/* ###### Create server information parameter ############################## */
static bool createServerInformationParameter(struct RSerPoolMessage*        message,
                                             struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct rserpool_serverinfoparameter* sip;
   size_t                               tlvPosition = 0;
   if(beginTLV(message, &tlvPosition, ATT_SERVER_INFORMATION) == false) {
      return(false);
   }

   sip = (struct rserpool_serverinfoparameter*)getSpace(message, sizeof(struct rserpool_serverinfoparameter));
   if(sip == NULL) {
      return(false);
   }

   sip->sip_server_id = htonl(peerListNode->Identifier);

   if(createTransportParameter(message, peerListNode->AddressBlock) == false) {
      return(false);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create handle resolution parameter ############################# */
static bool createHandleResolutionParameter(
               struct RSerPoolMessage* message,
               const size_t            items)
{
   struct rserpool_handleresolutionparameter* hrp;
   size_t                                     tlvPosition = 0;

   if(beginTLV(message, &tlvPosition, ATT_HANDLE_RESOLUTION|ATT_ACTION_CONTINUE) == false) {
      return(false);
   }

   hrp = (struct rserpool_handleresolutionparameter*)getSpace(message, sizeof(struct rserpool_handleresolutionparameter));
   if(hrp == NULL) {
      return(false);
   }
   if(items == RSPGETADDRS_MAX) {
      hrp->hrp_items = 0xffffffff;
   }
   else {
      hrp->hrp_items = htonl((uint32_t)items);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create endpoint keepalive message ############################## */
static bool createEndpointKeepAliveMessage(struct RSerPoolMessage* message)
{
   uint32_t* identifier;

   if(beginMessage(message, AHT_ENDPOINT_KEEP_ALIVE,
                   message->Flags & AHF_ENDPOINT_KEEP_ALIVE_HOME, PPID_ASAP) == NULL) {
      return(false);
   }

   identifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(identifier == NULL) {
      return(false);
   }
   *identifier = htonl(message->RegistrarIdentifier);

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   /* ------ Custom parameter ------ */
   if(message->Identifier != 0) {
      if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
         return(false);
      }
   }
   /* ------------------------------ */

   return(finishMessage(message));
}


/* ###### Create endpoint keepalive acknowledgement message ############## */
static bool createEndpointKeepAliveAckMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_ENDPOINT_KEEP_ALIVE_ACK, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create endpoint keepalive message ############################## */
static bool createEndpointUnreachableMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_ENDPOINT_UNREACHABLE, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create registration message #################################### */
static bool createRegistrationMessage(struct RSerPoolMessage* message)
{
   CHECK(message->PoolElementPtr->RegistratorTransport == NULL);

   if(beginMessage(message, AHT_REGISTRATION,
                   message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementParameter(message, message->PoolElementPtr, false) == false) {
      return(false);
   }
   if(message->Error != 0x00) {
      if(createErrorParameter(message) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create registration response message ########################### */
static bool createRegistrationResponseMessage(struct RSerPoolMessage* message)
{
   if(message->PoolElementPtr == NULL)  {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   if(beginMessage(message, AHT_REGISTRATION_RESPONSE,
                   message->Flags & AHF_REGISTRATION_REJECT, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
      return(false);
   }
   if(message->Error != 0x00) {
      if(createErrorParameter(message) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create deregistration message ################################## */
static bool createDeregistrationMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_DEREGISTRATION, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create deregistration response message ######################### */
static bool createDeregistrationResponseMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_DEREGISTRATION_RESPONSE,
                   message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementIdentifierParameter(message, message->Identifier) == false) {
      return(false);
   }
   if(message->Error != 0x00) {
      if(createErrorParameter(message) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create handle resolution message ################################# */
static bool createHandleResolutionMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_HANDLE_RESOLUTION, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }
   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(message->Addresses != 0) {
      if(createHandleResolutionParameter(message, message->Addresses) == false) {
         return(false);
      }
   }
   return(finishMessage(message));
}


/* ###### Create handle resolution response message ######################## */
static bool createHandleResolutionResponseMessage(struct RSerPoolMessage* message)
{
   size_t i;

   CHECK(message->PoolElementPtrArraySize <= MAX_MAX_HANDLE_RESOLUTION_ITEMS);
   if(beginMessage(message, AHT_HANDLE_RESOLUTION_RESPONSE, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   if(message->Error == 0x00) {
      if(createPolicyParameter(message, &message->PolicySettings) == false) {
         return(false);
      }

      for(i = 0;i < message->PoolElementPtrArraySize;i++) {
         if(createPoolElementParameter(message, message->PoolElementPtrArray[i], false) == false) {
            return(false);
         }
      }
   }
   else {
      if(createErrorParameter(message) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create business card message #################################### */
static bool createBusinessCardMessage(struct RSerPoolMessage* message)
{
   size_t i;

   CHECK(message->PoolElementPtrArraySize > 0);
   CHECK(message->PoolElementPtrArraySize < MAX_MAX_HANDLE_RESOLUTION_ITEMS);
   if(beginMessage(message, AHT_BUSINESS_CARD, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPolicyParameter(message, &message->PolicySettings) == false) {
      return(false);
   }
   for(i = 0;i < message->PoolElementPtrArraySize;i++) {
      if(createPoolElementParameter(message, message->PoolElementPtrArray[i], false) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create server announce message ################################## */
static bool createServerAnnounceMessage(struct RSerPoolMessage* message)
{
   uint32_t* identifier;

   if(beginMessage(message, AHT_SERVER_ANNOUNCE, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }

   identifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(identifier == NULL) {
      return(false);
   }
   *identifier = htonl(message->RegistrarIdentifier);

   return(finishMessage(message));
}


/* ###### Create cookie message ########################################### */
static bool createCookieMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_COOKIE, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }
   if(createCookieParameter(message, message->CookiePtr, message->CookieSize) == false) {
      return(false);
   }
   return(finishMessage(message));
}


/* ###### Create cookie echo message ###################################### */
static bool createCookieEchoMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_COOKIE_ECHO, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }
   if(createCookieParameter(message, message->CookiePtr, message->CookieSize) == false) {
      return(false);
   }
   return(finishMessage(message));
}


/* ###### Create peer presence ########################################### */
static bool createPresenceMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_PRESENCE,
                   message->Flags & EHF_PRESENCE_REPLY_REQUIRED,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   if(createHandlespaceChecksumParameter(message, message->Checksum) == false) {
      return(false);
   }
   if(createServerInformationParameter(message, message->PeerListNodePtr) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create peer list request ####################################### */
static bool createListRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_LIST_REQUEST,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   return(finishMessage(message));
}


/* ###### Create peer list response ###################################### */
static bool createListResponseMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;
   struct ST_CLASS(PeerListNode)*   peerListNode;
   size_t                           oldPosition;

   if(message->PeerListPtr == NULL) {
      LOG_ERROR
      fputs("Invalid parameters\n", stdlog);
      LOG_END_FATAL
      return(false);
   }

   if(beginMessage(message, EHT_LIST_RESPONSE,
                   message->Flags & EHF_LIST_RESPONSE_REJECT,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   peerListNode = ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                     &message->PeerListPtr->List);
   while(peerListNode != NULL) {
      /* The draft does not say what to do when the number of peers exceeds the
         message size -> We fill as much entries as possible and reply this
         partial list. This is better than to do nothing! */
      oldPosition = message->Position;
      if(createServerInformationParameter(message, peerListNode) == false) {
         message->Position = oldPosition;
         break;
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        &message->PeerListPtr->List,
                        peerListNode);
   }

   return(finishMessage(message));
}


/* ###### Create peer handle table request ################################# */
static bool createHandleTableRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_HANDLE_TABLE_REQUEST,
                   message->Flags & EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   return(finishMessage(message));
}


/* ###### Create peer handle table response ################################ */
static bool createHandleTableResponseMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter*     sp;
   struct ST_CLASS(HandleTableExtract)* hte = NULL;
   struct PoolHandle*                   lastPoolHandle;
   unsigned int                         flags;
   int                                  result;
   size_t                               i;
   size_t                               oldPosition;
   struct rserpool_header*              header;

   header = beginMessage(message, EHT_HANDLE_TABLE_RESPONSE,
                         message->Flags & EHF_HANDLE_TABLE_RESPONSE_REJECT,
                         PPID_ENRP);
   if(header == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   if(message->PeerListNodePtr) {
      flags = (message->Action & EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY) ? HTEF_OWNCHILDSONLY : 0;
      hte = (struct ST_CLASS(HandleTableExtract)*)message->PeerListNodePtr->UserData;
      if(hte == NULL) {
         flags |= HTEF_START;
         hte = (struct ST_CLASS(HandleTableExtract)*)malloc(sizeof(struct ST_CLASS(HandleTableExtract)));
         if(hte == NULL) {
            return(false);
         }
         message->PeerListNodePtr->UserData = hte;
      }

      oldPosition = message->Position;
      result = ST_CLASS(poolHandlespaceManagementGetHandleTable)(
                  message->HandlespacePtr,
                  message->HandlespacePtr->Handlespace.HomeRegistrarIdentifier,
                  hte,flags, message->MaxElementsPerHTRequest);
      if(result > 0) {
         lastPoolHandle = NULL;
         for(i = 0;i < hte->PoolElementNodes;i++) {
            LOG_VERBOSE2
            ST_CLASS(poolElementNodePrint)(hte->PoolElementNodeArray[i], stdlog, ~0);
            fputs("\n", stdlog);
            LOG_END

            if(lastPoolHandle != &hte->PoolElementNodeArray[i]->OwnerPoolNode->Handle) {
               lastPoolHandle = &hte->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
               oldPosition = message->Position;
               if(createPoolHandleParameter(message, lastPoolHandle) == false) {
                  if(i < 1) {
                     return(false);
                  }
                  message->Position = oldPosition;
                  header->ah_flags |= EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND;
                  break;
               }
            }

            CHECK(hte->PoolElementNodeArray[i]->RegistratorTransport != NULL);
            if(createPoolElementParameter(message, hte->PoolElementNodeArray[i], true) == false) {
               if(i < 1) {
                  return(false);
               }
               message->Position = oldPosition;
               header->ah_flags |= EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND;
               break;
            }
            oldPosition = message->Position;

            hte->LastPoolHandle            = hte->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
            hte->LastPoolElementIdentifier = hte->PoolElementNodeArray[i]->Identifier;
         }
         if((hte->PoolElementNodes == min(message->MaxElementsPerHTRequest, NTE_MAX_POOL_ELEMENT_NODES)) ||
            (i != hte->PoolElementNodes)) {
            header->ah_flags |= EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND;
         }
      }

      if(!(header->ah_flags & EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND)) {
         free(message->PeerListNodePtr->UserData);
         message->PeerListNodePtr->UserData = NULL;
      }
   }

   return(finishMessage(message));
}


/* ###### Create peer name update ######################################## */
static bool createHandleUpdateMessage(struct RSerPoolMessage* message)
{
   struct rserpool_handleupdateparameter* pnup;

   CHECK(message->PoolElementPtr->RegistratorTransport != NULL);

   if(beginMessage(message, EHT_HANDLE_UPDATE,
                   message->Flags & EHF_TAKEOVER_SUGGESTED,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   pnup = (struct rserpool_handleupdateparameter*)getSpace(message, sizeof(struct rserpool_handleupdateparameter));
   if(pnup == NULL) {
      return(false);
   }
   pnup->pnup_sender_id     = htonl(message->SenderID);
   pnup->pnup_receiver_id   = htonl(message->ReceiverID);
   pnup->pnup_update_action = htons(message->Action);
   pnup->pnup_pad           = 0x0000;

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }

   if(createPoolElementParameter(message, message->PoolElementPtr, true) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create peer init takeover ###################################### */
static bool createInitTakeoverMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_INIT_TAKEOVER,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      return(false);
   }
   tp->tp_sender_id   = htonl(message->SenderID);
   tp->tp_receiver_id = htonl(message->ReceiverID);
   tp->tp_target_id   = htonl(message->RegistrarIdentifier);

   return(finishMessage(message));
}


/* ###### Create peer init takeover acknowledgement ###################### */
static bool createInitTakeoverAckMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_INIT_TAKEOVER_ACK,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      return(false);
   }
   tp->tp_sender_id   = htonl(message->SenderID);
   tp->tp_receiver_id = htonl(message->ReceiverID);
   tp->tp_target_id   = htonl(message->RegistrarIdentifier);

   return(finishMessage(message));
}


/* ###### Create peer takeover server #################################### */
static bool createTakeoverServerMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_TAKEOVER_SERVER,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   tp = (struct rserpool_targetparameter*)getSpace(message, sizeof(struct rserpool_targetparameter));
   if(tp == NULL) {
      return(false);
   }
   tp->tp_sender_id   = htonl(message->SenderID);
   tp->tp_receiver_id = htonl(message->ReceiverID);
   tp->tp_target_id   = htonl(message->RegistrarIdentifier);

   return(finishMessage(message));
}


/* ###### Create peer error ############################################## */
static bool createENRPErrorMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_ERROR,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   if(createErrorParameter(message) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create peer error ############################################## */
static bool createASAPErrorMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_ERROR,
                   message->Flags & 0x00,
                   PPID_ASAP) == NULL) {
      return(false);
   }

   if(createErrorParameter(message) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Convert RSerPoolMessage to packet ############################## */
size_t rserpoolMessage2Packet(struct RSerPoolMessage* message)
{
   rserpoolMessageClearBuffer(message);

   switch(message->Type) {
      /* ====== ASAP ===================================================== */
      case AHT_HANDLE_RESOLUTION:
          LOG_VERBOSE2
          fputs("Creating HandleResolution message...\n", stdlog);
          LOG_END
          if(createHandleResolutionMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_HANDLE_RESOLUTION_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating HandleResolutionResponse message...\n", stdlog);
          LOG_END
          if(createHandleResolutionResponseMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_ENDPOINT_KEEP_ALIVE:
          LOG_VERBOSE2
          fputs("Creating EndpointKeepAlive message...\n", stdlog);
          LOG_END
          if(createEndpointKeepAliveMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_ENDPOINT_KEEP_ALIVE_ACK:
          LOG_VERBOSE2
          fputs("Creating EndpointKeepAliveAck message...\n", stdlog);
          LOG_END
          if(createEndpointKeepAliveAckMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_SERVER_ANNOUNCE:
          LOG_VERBOSE2
          fputs("Creating ServerAnnounce message...\n", stdlog);
          LOG_END
          if(createServerAnnounceMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_REGISTRATION:
          LOG_VERBOSE2
          fputs("Creating Registration message...\n", stdlog);
          LOG_END
          if(createRegistrationMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_REGISTRATION_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating RegistrationResponse message...\n", stdlog);
          LOG_END
          if(createRegistrationResponseMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_DEREGISTRATION:
          LOG_VERBOSE2
          fputs("Creating Deregistration message...\n", stdlog);
          LOG_END
          if(createDeregistrationMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_DEREGISTRATION_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating DeregistrationResponse message...\n", stdlog);
          LOG_END
          if(createDeregistrationResponseMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_COOKIE:
          LOG_VERBOSE2
          fputs("Creating Cookie message...\n", stdlog);
          LOG_END
          if(createCookieMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_COOKIE_ECHO:
          LOG_VERBOSE2
          fputs("Creating CookieEcho message...\n", stdlog);
          LOG_END
          if(createCookieEchoMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_BUSINESS_CARD:
          LOG_VERBOSE2
          fputs("Creating BusinessCard message...\n", stdlog);
          LOG_END
          if(createBusinessCardMessage(message) == true) {
             return(message->Position);
          }
       break;
      case AHT_ENDPOINT_UNREACHABLE:
          LOG_VERBOSE2
          fputs("Creating EndpointUnreachable message...\n", stdlog);
          LOG_END
          if(createEndpointUnreachableMessage(message) == true) {
             return(message->Position);
          }
        break;
       case AHT_ERROR:
          LOG_VERBOSE2
          fputs("Creating Error (ASAP) message...\n", stdlog);
          LOG_END
          if(createASAPErrorMessage(message) == true) {
             return(message->Position);
          }
        break;

       /* ====== ENRP ==================================================== */
       case EHT_PRESENCE:
          LOG_VERBOSE2
          fputs("Creating PeerPresence message...\n", stdlog);
          LOG_END
          if(createPresenceMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_HANDLE_TABLE_REQUEST:
          LOG_VERBOSE2
          fputs("Creating HandleTableRequest message...\n", stdlog);
          LOG_END
          if(createHandleTableRequestMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_HANDLE_TABLE_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating HandleTableResponse message...\n", stdlog);
          LOG_END
          if(createHandleTableResponseMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_HANDLE_UPDATE:
          LOG_VERBOSE2
          fputs("Creating HandleUpdate message...\n", stdlog);
          LOG_END
          if(createHandleUpdateMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_LIST_REQUEST:
          LOG_VERBOSE2
          fputs("Creating ListRequest message...\n", stdlog);
          LOG_END
          if(createListRequestMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_LIST_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating ListResponse message...\n", stdlog);
          LOG_END
          if(createListResponseMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_INIT_TAKEOVER:
          LOG_VERBOSE2
          fputs("Creating PeerInitTakeover message...\n", stdlog);
          LOG_END
          if(createInitTakeoverMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_INIT_TAKEOVER_ACK:
          LOG_VERBOSE2
          fputs("Creating PeerInitTakeoverAck message...\n", stdlog);
          LOG_END
          if(createInitTakeoverAckMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_TAKEOVER_SERVER:
          LOG_VERBOSE2
          fputs("Creating PeerTakeoverServer message...\n", stdlog);
          LOG_END
          if(createTakeoverServerMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_ERROR:
          LOG_VERBOSE2
          fputs("Creating Error (ENRP) message...\n", stdlog);
          LOG_END
          if(createENRPErrorMessage(message) == true) {
             return(message->Position);
          }
        break;

       /* ====== Unknown ================================================= */
       default:
          LOG_ERROR
          fprintf(stdlog, "Unknown message type $%04x\n", message->Type);
          LOG_END_FATAL
        break;
   }
   LOG_ERROR
   fputs("Message creation failed\n", stdlog);
   LOG_END_FATAL
   return(0);
}
