/*
 *  $Id: rserpoolmessagecreator.c,v 1.22 2005/03/08 12:51:03 dreibh Exp $
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
 * Purpose: RSerPool Message Creator
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rserpoolmessagecreator.h"

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
   size_t               tlvPosition = 0;
   char*                output;
   struct sockaddr_in*  in;
   struct sockaddr_in6* in6;

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
         if(beginTLV(message, &tlvPosition, ATT_IPv6_ADDRESS) == false) {
            return(false);
         }
         in6 = (struct sockaddr_in6*)address;
         output = (char*)getSpace(message, 16);
         if(output == NULL) {
            return(false);
         }
         memcpy(output, &in6->sin6_addr, 16);
       break;
      default:
         LOG_ERROR
         fputs("Unknown address family\n", stdlog);
         LOG_END_FATAL
         return(false);
       break;
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
      default:
         LOG_ERROR
         fprintf(stdlog,"Unknown protocol #%d\n", transportAddressBlock->Protocol);
         LOG_END_FATAL
         return(false);
       break;
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
   struct rserpool_policy_leastused*                                 lu;
   struct rserpool_policy_leastused_degradation*                     lud;
   struct rserpool_policy_priority_leastused*                        plu;
   struct rserpool_policy_priority_leastused_degradation*            plud;
   struct rserpool_policy_random*                                    rd;
   struct rserpool_policy_weighted_random*                           wrd;
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
          rd->pp_rd_policy = poolPolicySettings->PolicyType;
          rd->pp_rd_pad    = 0;
       break;
      case PPT_WEIGHTED_RANDOM:
          wrd = (struct rserpool_policy_weighted_random*)getSpace(message, sizeof(struct rserpool_policy_weighted_random));
          if(wrd == NULL) {
             return(false);
          }
          wrd->pp_wrd_policy = poolPolicySettings->PolicyType;
          wrd->pp_wrd_weight = hton24(poolPolicySettings->Weight);
       break;
      case PPT_ROUNDROBIN:
          rr = (struct rserpool_policy_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_roundrobin));
          if(rr == NULL) {
             return(false);
          }
          rr->pp_rr_policy = poolPolicySettings->PolicyType;
          rr->pp_rr_pad    = 0;
       break;
      case PPT_WEIGHTED_ROUNDROBIN:
          wrr = (struct rserpool_policy_weighted_roundrobin*)getSpace(message, sizeof(struct rserpool_policy_weighted_roundrobin));
          if(wrr == NULL) {
             return(false);
          }
          wrr->pp_wrr_policy = poolPolicySettings->PolicyType;
          wrr->pp_wrr_weight = hton24(poolPolicySettings->Weight);
       break;
      case PPT_LEASTUSED:
          lu = (struct rserpool_policy_leastused*)getSpace(message, sizeof(struct rserpool_policy_leastused));
          if(lu == NULL) {
             return(false);
          }
          lu->pp_lu_policy = poolPolicySettings->PolicyType;
          lu->pp_lu_load   = hton24(poolPolicySettings->Load);
       break;
      case PPT_LEASTUSED_DEGRADATION:
          lud = (struct rserpool_policy_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_leastused_degradation));
          if(lud == NULL) {
             return(false);
          }
          lud->pp_lud_policy  = poolPolicySettings->PolicyType;
          lud->pp_lud_load    = hton24(poolPolicySettings->Load);
          lud->pp_lud_pad     = 0x00;
          lud->pp_lud_loaddeg = hton24(poolPolicySettings->LoadDegradation);
       break;
      case PPT_PRIORITY_LEASTUSED:
          plu = (struct rserpool_policy_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused));
          if(plu == NULL) {
             return(false);
          }
          plu->pp_plu_policy = poolPolicySettings->PolicyType;
          plu->pp_plu_load   = hton24(poolPolicySettings->Load);
       break;
      case PPT_PRIORITY_LEASTUSED_DEGRADATION:
          plud = (struct rserpool_policy_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_priority_leastused_degradation));
          if(plud == NULL) {
             return(false);
          }
          plud->pp_plud_policy  = poolPolicySettings->PolicyType;
          plud->pp_plud_load    = hton24(poolPolicySettings->Load);
          plud->pp_plud_pad     = 0x00;
          plud->pp_plud_loaddeg = hton24(poolPolicySettings->LoadDegradation);
       break;
      case PPT_RANDOMIZED_LEASTUSED:
          rlu = (struct rserpool_policy_randomized_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused));
          if(rlu == NULL) {
             return(false);
          }
          rlu->pp_rlu_policy = poolPolicySettings->PolicyType;
          rlu->pp_rlu_load   = hton24(poolPolicySettings->Load);
       break;
      case PPT_RANDOMIZED_LEASTUSED_DEGRADATION:
          rlud = (struct rserpool_policy_randomized_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_leastused_degradation));
          if(rlud == NULL) {
             return(false);
          }
          rlud->pp_rlud_policy  = poolPolicySettings->PolicyType;
          rlud->pp_rlud_load    = hton24(poolPolicySettings->Load);
          rlud->pp_rlud_pad     = 0x00;
          rlud->pp_rlud_loaddeg = hton24(poolPolicySettings->LoadDegradation);
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED:
          rplu = (struct rserpool_policy_randomized_priority_leastused*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused));
          if(rplu == NULL) {
             return(false);
          }
          rplu->pp_rplu_policy = poolPolicySettings->PolicyType;
          rplu->pp_rplu_load   = hton24(poolPolicySettings->Load);
       break;
      case PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION:
          rplud = (struct rserpool_policy_randomized_priority_leastused_degradation*)getSpace(message, sizeof(struct rserpool_policy_randomized_priority_leastused_degradation));
          if(rplud == NULL) {
             return(false);
          }
          rplud->pp_rplud_policy  = poolPolicySettings->PolicyType;
          rplud->pp_rplud_load    = hton24(poolPolicySettings->Load);
          rplud->pp_rplud_pad     = 0x00;
          rplud->pp_rplud_loaddeg = hton24(poolPolicySettings->LoadDegradation);
       break;
      default:
         LOG_ERROR
         fprintf(stdlog,"Unknown policy #$%02x\n",poolPolicySettings->PolicyType);
         LOG_END_FATAL
         return(false);
       break;
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create pool element parameter ################################## */
static bool createPoolElementParameter(
               struct RSerPoolMessage*                 message,
               const struct ST_CLASS(PoolElementNode)* poolElement)
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

   if(poolElement->RegistratorTransport) {
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


/* ###### Create PR identifier parameter ################################# */
static bool createRegistrarIdentifierParameter(
               struct RSerPoolMessage*       message,
               const RegistrarIdentifierType registrarIdentifier)
{
   uint32_t* identifier;
   size_t    tlvPosition;

   /* ATT_REGISTRAR_IDENTIFIER is optional => Also set ATT_ACTION_CONTINUE! */
   if(beginTLV(message, &tlvPosition, ATT_REGISTRAR_IDENTIFIER|ATT_ACTION_CONTINUE) == false) {
      return(false);
   }

   identifier = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(identifier == NULL) {
      return(false);
   }
   *identifier = htonl(registrarIdentifier);

   return(finishTLV(message, tlvPosition));
}


/* ###### Create pool element checksum parameter ####################### */
static bool createPoolElementChecksumParameter(
               struct RSerPoolMessage*       message,
               const PoolElementChecksumType poolElementChecksum)
{
   uint32_t* checksum;
   size_t    tlvPosition = 0;

   if(beginTLV(message, &tlvPosition, ATT_POOL_ELEMENT_CHECKSUM) == false) {
      return(false);
   }

   checksum = (uint32_t*)getSpace(message, sizeof(uint32_t));
   if(checksum == NULL) {
      return(false);
   }
   *checksum = htonl(poolElementChecksum);

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

   cause = message->Error;
   switch(cause) {
      case RSPERR_UNRECOGNIZED_PARAMETER:
         data       = message->OffendingParameterTLV;
         dataLength = message->OffendingParameterTLVLength;
       break;
      case RSPERR_UNRECOGNIZED_MESSAGE:
      case RSPERR_INVALID_VALUES:
         data       = message->OffendingMessageTLV;
         dataLength = message->OffendingMessageTLVLength;
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
   memcpy((char*)&aec->aec_data, data, dataLength);

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
   sip->sip_flags     = peerListNode->Flags;

   if(createTransportParameter(message, peerListNode->AddressBlock) == false) {
      return(false);
   }

   return(finishTLV(message, tlvPosition));
}


/* ###### Create endpoint keepalive message ############################## */
static bool createEndpointKeepAliveMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_ENDPOINT_KEEP_ALIVE, message->Flags & 0x00, PPID_ASAP) == NULL) {
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
                   message->Flags & AHF_REGISTRATION_REJECT, PPID_ASAP) == NULL) {
      return(false);
   }

   if(createPoolHandleParameter(message, &message->Handle) == false) {
      return(false);
   }
   if(createPoolElementParameter(message, message->PoolElementPtr) == false) {
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
                   message->Flags & AHF_DEREGISTRATION_REJECT, PPID_ASAP) == NULL) {
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
   return(finishMessage(message));
}


/* ###### Create handle resolution response message ######################## */
static bool createHandleResolutionResponseMessage(struct RSerPoolMessage* message)
{
   size_t i;

   CHECK(message->PoolElementPtrArraySize < MAX_MAX_HANDLE_RESOLUTION_ITEMS);
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
         if(createPoolElementParameter(message, message->PoolElementPtrArray[i]) == false) {
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
      if(createPoolElementParameter(message, message->PoolElementPtrArray[i]) == false) {
         return(false);
      }
   }

   return(finishMessage(message));
}


/* ###### Create server announce message ################################## */
static bool createServerAnnounceMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_SERVER_ANNOUNCE, message->Flags & 0x00, PPID_ASAP) == NULL) {
      return(false);
   }
   if(createRegistrarIdentifierParameter(message, message->RegistrarIdentifier) == false) {
      return(false);
   }
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
static bool createPeerPresenceMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_PEER_PRESENCE,
                   message->Flags & EHF_PEER_PRESENCE_REPLY_REQUIRED,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   sp = (struct rserpool_serverparameter*)getSpace(message, sizeof(struct rserpool_serverparameter));
   if(sp == NULL) {
      return(false);
   }
   sp->sp_sender_id   = htonl(message->SenderID);
   sp->sp_receiver_id = htonl(message->ReceiverID);

   if(createPoolElementChecksumParameter(message, message->Checksum) == false) {
      return(false);
   }
   if(createServerInformationParameter(message, message->PeerListNodePtr) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create peer list request ####################################### */
static bool createPeerListRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_PEER_LIST_REQUEST,
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
static bool createPeerListResponseMessage(struct RSerPoolMessage* message)
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

   if(beginMessage(message, EHT_PEER_LIST_RESPONSE,
                   message->Flags & EHT_PEER_LIST_RESPONSE_REJECT,
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
                     message->PeerListPtr);
   while(peerListNode != NULL) {
      /* The draft does not say what to do when the amount of peers exceeds the
         message size -> We fill as much entries as possible and reply this
         partial list. This is better than to do nothing! */
      oldPosition = message->Position;
      if(createServerInformationParameter(message, peerListNode) == false) {
         message->Position = oldPosition;
         break;
      }
      peerListNode = ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                        message->PeerListPtr,
                        peerListNode);
   }

   return(finishMessage(message));
}


/* ###### Create peer handle table request ################################# */
static bool createPeerHandleTableRequestMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_PEER_NAME_TABLE_REQUEST,
                   message->Flags & EHF_PEER_NAME_TABLE_REQUEST_OWN_CHILDREN_ONLY,
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
static bool createPeerHandleTableResponseMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter*   sp;
   struct ST_CLASS(HandleTableExtract)* nte;
   struct PoolHandle*                 lastPoolHandle;
   unsigned int                       flags;
   int                                result;
   size_t                             i;
   size_t                             oldPosition;
   struct rserpool_header*            header;

   header = beginMessage(message, EHT_PEER_NAME_TABLE_RESPONSE,
                         message->Flags & (EHT_PEER_NAME_TABLE_RESPONSE_REJECT|EHT_PEER_NAME_TABLE_RESPONSE_MORE_TO_SEND),
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
      flags = (message->Action & EHF_PEER_NAME_TABLE_REQUEST_OWN_CHILDREN_ONLY) ? NTEF_OWNCHILDSONLY : 0;
      nte = (struct ST_CLASS(HandleTableExtract)*)message->PeerListNodePtr->UserData;
      if(nte == NULL) {
         flags |= NTEF_START;
         nte = (struct ST_CLASS(HandleTableExtract)*)malloc(sizeof(struct ST_CLASS(HandleTableExtract)));
         if(nte == NULL) {
            return(false);
         }
         message->PeerListNodePtr->UserData = nte;
      }

      oldPosition = message->Position;
      result = ST_CLASS(poolHandlespaceManagementGetHandleTable)(
                  message->HandlespacePtr,
                  message->HandlespacePtr->Handlespace.HomeRegistrarIdentifier,
                  nte,
                  flags);
      if(result > 0) {
         lastPoolHandle = NULL;
         for(i = 0;i < nte->PoolElementNodes;i++) {
            LOG_NOTE
            ST_CLASS(poolElementNodePrint)(nte->PoolElementNodeArray[i], stdlog, ~0);
            fputs("\n", stdlog);
            LOG_END

            if(lastPoolHandle != &nte->PoolElementNodeArray[i]->OwnerPoolNode->Handle) {
               lastPoolHandle = &nte->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
               oldPosition = message->Position;
               if(createPoolHandleParameter(message, lastPoolHandle) == false) {
                  if(i < 1) {
                     return(false);
                  }
                  message->Position = oldPosition;
                  break;
               }
            }

            CHECK(nte->PoolElementNodeArray[i]->RegistratorTransport != NULL);
            if(createPoolElementParameter(message, nte->PoolElementNodeArray[i]) == false) {
               if(i < 1) {
                  return(false);
               }
               message->Position = oldPosition;
               break;
            }
            oldPosition = message->Position;

            nte->LastPoolHandle            = nte->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
            nte->LastPoolElementIdentifier = nte->PoolElementNodeArray[i]->Identifier;
         }
         if((nte->PoolElementNodes == NTE_MAX_POOL_ELEMENT_NODES) ||
            (i != nte->PoolElementNodes)) {
            header->ah_flags |= EHT_PEER_NAME_TABLE_RESPONSE_MORE_TO_SEND;
         }
      }
   }

   return(finishMessage(message));
}


/* ###### Create peer name update ######################################## */
static bool createPeerNameUpdateMessage(struct RSerPoolMessage* message)
{
   struct rserpool_peernameupdateparameter* pnup;

   CHECK(message->PoolElementPtr->RegistratorTransport != NULL);

   if(beginMessage(message, EHT_PEER_NAME_UPDATE,
                   message->Flags & 0x00,
                   PPID_ENRP) == NULL) {
      return(false);
   }

   pnup = (struct rserpool_peernameupdateparameter*)getSpace(message, sizeof(struct rserpool_peernameupdateparameter));
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

   if(createPoolElementParameter(message, message->PoolElementPtr) == false) {
      return(false);
   }

   return(finishMessage(message));
}


/* ###### Create peer init takeover ###################################### */
static bool createPeerInitTakeoverMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_PEER_INIT_TAKEOVER,
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
static bool createPeerInitTakeoverAckMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_PEER_INIT_TAKEOVER_ACK,
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
static bool createPeerTakeoverServerMessage(struct RSerPoolMessage* message)
{
   struct rserpool_targetparameter* tp;

   if(beginMessage(message, EHT_PEER_TAKEOVER_SERVER,
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


/* ###### Create peer ownership change ################################### */
static bool createPeerOwnershipChangeMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;
   struct PoolHandle*               lastPoolHandle;
   unsigned int                     flags;
   unsigned int                     result;
   size_t                           oldPosition;
   size_t                           i;

   if(beginMessage(message, EHT_PEER_OWNERSHIP_CHANGE,
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

   flags = NTEF_OWNCHILDSONLY;
   if(message->ExtractContinuation->LastPoolElementIdentifier == 0) {
      flags |= NTEF_START;
   }

   oldPosition = message->Position;
   result = ST_CLASS(poolHandlespaceManagementGetHandleTable)(
               message->HandlespacePtr,
               message->RegistrarIdentifier,
               message->ExtractContinuation,
               flags);
   if(result > 0) {
      lastPoolHandle = NULL;
      for(i = 0;i < message->ExtractContinuation->PoolElementNodes;i++) {
         LOG_NOTE
         ST_CLASS(poolElementNodePrint)(message->ExtractContinuation->PoolElementNodeArray[i], stdlog, ~0);
         fputs("\n", stdlog);
         LOG_END

         if(lastPoolHandle != &message->ExtractContinuation->PoolElementNodeArray[i]->OwnerPoolNode->Handle) {
            lastPoolHandle = &message->ExtractContinuation->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
            oldPosition = message->Position;
            if(createPoolHandleParameter(message, lastPoolHandle) == false) {
               if(i < 1) {
                  return(false);
               }
               message->Position = oldPosition;
               break;
            }
         }

         if(createPoolElementIdentifierParameter(message, message->ExtractContinuation->PoolElementNodeArray[i]->Identifier) == false) {
            if(i < 1) {
               return(false);
            }
            message->Position = oldPosition;
            break;
         }
         oldPosition = message->Position;

         message->ExtractContinuation->LastPoolHandle            = message->ExtractContinuation->PoolElementNodeArray[i]->OwnerPoolNode->Handle;
         message->ExtractContinuation->LastPoolElementIdentifier = message->ExtractContinuation->PoolElementNodeArray[i]->Identifier;
      }
      if((message->ExtractContinuation->PoolElementNodes < NTE_MAX_POOL_ELEMENT_NODES) ||
         (i >= message->ExtractContinuation->PoolElementNodes)) {
         message->ExtractContinuation->LastPoolElementIdentifier = 0;
      }
   }

   return(finishMessage(message));
}


/* ###### Create peer error ############################################## */
static bool createENRPPeerErrorMessage(struct RSerPoolMessage* message)
{
   struct rserpool_serverparameter* sp;

   if(beginMessage(message, EHT_PEER_ERROR,
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
static bool createASAPPeerErrorMessage(struct RSerPoolMessage* message)
{
   if(beginMessage(message, AHT_PEER_ERROR,
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
       case AHT_PEER_ERROR:
          LOG_VERBOSE2
          fputs("Creating PeerError (ASAP) message...\n", stdlog);
          LOG_END
          if(createASAPPeerErrorMessage(message) == true) {
             return(message->Position);
          }
        break;

       /* ====== ENRP ==================================================== */
       case EHT_PEER_PRESENCE:
          LOG_VERBOSE2
          fputs("Creating PeerPresence message...\n", stdlog);
          LOG_END
          if(createPeerPresenceMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_NAME_TABLE_REQUEST:
          LOG_VERBOSE2
          fputs("Creating PeerHandleTableRequest message...\n", stdlog);
          LOG_END
          if(createPeerHandleTableRequestMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_NAME_TABLE_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating PeerHandleTableResponse message...\n", stdlog);
          LOG_END
          if(createPeerHandleTableResponseMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_NAME_UPDATE:
          LOG_VERBOSE2
          fputs("Creating PeerNameUpdate message...\n", stdlog);
          LOG_END
          if(createPeerNameUpdateMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_LIST_REQUEST:
          LOG_VERBOSE2
          fputs("Creating PeerListRequest message...\n", stdlog);
          LOG_END
          if(createPeerListRequestMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_LIST_RESPONSE:
          LOG_VERBOSE2
          fputs("Creating PeerListResponse message...\n", stdlog);
          LOG_END
          if(createPeerListResponseMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_INIT_TAKEOVER:
          LOG_VERBOSE2
          fputs("Creating PeerInitTakeover message...\n", stdlog);
          LOG_END
          if(createPeerInitTakeoverMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_INIT_TAKEOVER_ACK:
          LOG_VERBOSE2
          fputs("Creating PeerInitTakeoverAck message...\n", stdlog);
          LOG_END
          if(createPeerInitTakeoverAckMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_TAKEOVER_SERVER:
          LOG_VERBOSE2
          fputs("Creating PeerTakeoverServer message...\n", stdlog);
          LOG_END
          if(createPeerTakeoverServerMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_OWNERSHIP_CHANGE:
          LOG_VERBOSE2
          fputs("Creating PeerOwnershipChange message...\n", stdlog);
          LOG_END
          if(createPeerOwnershipChangeMessage(message) == true) {
             return(message->Position);
          }
        break;
       case EHT_PEER_ERROR:
          LOG_VERBOSE2
          fputs("Creating PeerError (ENRP) message...\n", stdlog);
          LOG_END
          if(createENRPPeerErrorMessage(message) == true) {
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
