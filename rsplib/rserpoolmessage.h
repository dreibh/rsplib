/*
 *  $Id: rserpoolmessage.h,v 1.17 2005/07/27 10:26:18 dreibh Exp $
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


#ifndef RSERPOOLMESSAGE_H
#define RSERPOOLMESSAGE_H


#include "tdtypes.h"
#include "poolhandlespacemanagement.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Set internal limit */
#define MAX_MAX_HANDLE_RESOLUTION_ITEMS 128


#define PORT_ASAP 3863
#define PORT_ENRP 3864

#define PPID_ASAP 11 /* old value: 0xFAEEB5D1 */
#define PPID_ENRP 12 /* old value: 0xFAEEB5D2 */


#define AHT_ASAP_MODIFIER              0xaa00
#define AHT_REGISTRATION               (0x01 | AHT_ASAP_MODIFIER)
#define AHT_DEREGISTRATION             (0x02 | AHT_ASAP_MODIFIER)
#define AHT_REGISTRATION_RESPONSE      (0x03 | AHT_ASAP_MODIFIER)
#define AHT_DEREGISTRATION_RESPONSE    (0x04 | AHT_ASAP_MODIFIER)
#define AHT_HANDLE_RESOLUTION          (0x05 | AHT_ASAP_MODIFIER)
#define AHT_HANDLE_RESOLUTION_RESPONSE (0x06 | AHT_ASAP_MODIFIER)
#define AHT_ENDPOINT_KEEP_ALIVE        (0x07 | AHT_ASAP_MODIFIER)
#define AHT_ENDPOINT_KEEP_ALIVE_ACK    (0x08 | AHT_ASAP_MODIFIER)
#define AHT_ENDPOINT_UNREACHABLE       (0x09 | AHT_ASAP_MODIFIER)
#define AHT_SERVER_ANNOUNCE            (0x0a | AHT_ASAP_MODIFIER)
#define AHT_COOKIE                     (0x0b | AHT_ASAP_MODIFIER)
#define AHT_COOKIE_ECHO                (0x0c | AHT_ASAP_MODIFIER)
#define AHT_BUSINESS_CARD              (0x0d | AHT_ASAP_MODIFIER)
#define AHT_PEER_ERROR                 (0x0e | AHT_ASAP_MODIFIER)


#define AHF_ENDPOINT_KEEP_ALIVE_HOME   (1 << 0)


struct rserpool_header
{
   uint8_t  ah_type;
   uint8_t  ah_flags;
   uint16_t ah_length;
};


struct rserpool_tlv_header
{
   uint16_t atlv_type;
   uint16_t atlv_length;
};


#define ATT_ACTION_MASK                0xc0
#define ATT_ACTION_STOP                0x00
#define ATT_ACTION_STOP_AND_REPORT     0x40
#define ATT_ACTION_CONTINUE            0x80
#define ATT_ACTION_CONTINUE_AND_REPORT 0xc0
#define PURE_ATT_TYPE(type)            (type & (~ATT_ACTION_MASK))

#define ATT_IPv4_ADDRESS            0x01
#define ATT_IPv6_ADDRESS            0x02
#define ATT_SCTP_TRANSPORT          0x03
#define ATT_TCP_TRANSPORT           0x04
#define ATT_UDP_TRANSPORT           0x05
#define ATT_POOL_POLICY             0x06
#define ATT_POOL_HANDLE             0x07
#define ATT_POOL_ELEMENT            0x08
#define ATT_SERVER_INFORMATION      0x09
#define ATT_OPERATION_ERROR         0x0a
#define ATT_COOKIE                  0x0b
#define ATT_POOL_ELEMENT_IDENTIFIER 0x0c
#define ATT_POOL_ELEMENT_CHECKSUM   0x0d
#define ATT_REGISTRAR_IDENTIFIER    0x3f


struct rserpool_poolelementparameter
{
   uint32_t pep_identifier;
   uint32_t pep_homeserverid;
   uint32_t pep_reg_life;
};


#define UTP_DATA_ONLY         0x0000
#define UTP_DATA_PLUS_CONTROL 0x0001

struct rserpool_sctptransportparameter
{
   uint16_t stp_port;
   uint16_t stp_transport_use;
};

struct rserpool_tcptransportparameter
{
   uint16_t ttp_port;
   uint16_t ttp_transport_use;
};

struct rserpool_udptransportparameter
{
   uint16_t utp_port;
   uint16_t utp_reserved;
};


struct rserpool_policy_roundrobin
{
   uint8_t  pp_rr_policy:8;
   uint32_t pp_rr_pad:24;
};

struct rserpool_policy_weighted_roundrobin
{
   uint8_t  pp_wrr_policy:8;
   uint32_t pp_wrr_weight:24;
};

struct rserpool_policy_leastused
{
   uint8_t  pp_lu_policy:8;
   uint32_t pp_lu_load:24;
};

struct rserpool_policy_leastused_degradation
{
   uint8_t  pp_lud_policy:8;
   uint32_t pp_lud_load:24;
   uint8_t  pp_lud_pad:8;
   uint32_t pp_lud_loaddeg:24;
};

struct rserpool_policy_priority_leastused
{
   uint8_t  pp_plu_policy:8;
   uint32_t pp_plu_load:24;
};

struct rserpool_policy_priority_leastused_degradation
{
   uint8_t  pp_plud_policy:8;
   uint32_t pp_plud_load:24;
   uint8_t  pp_plud_pad:8;
   uint32_t pp_plud_loaddeg:24;
};

struct rserpool_policy_random
{
   uint8_t  pp_rd_policy:8;
   uint32_t pp_rd_pad:24;
};

struct rserpool_policy_weighted_random
{
   uint8_t  pp_wrd_policy:8;
   uint32_t pp_wrd_weight:24;
};

struct rserpool_policy_randomized_leastused
{
   uint8_t  pp_rlu_policy:8;
   uint32_t pp_rlu_load:24;
};

struct rserpool_policy_randomized_leastused_degradation
{
   uint8_t  pp_rlud_policy:8;
   uint32_t pp_rlud_load:24;
   uint8_t  pp_rlud_pad:8;
   uint32_t pp_rlud_loaddeg:24;
};

struct rserpool_policy_randomized_priority_leastused
{
   uint8_t  pp_rplu_policy:8;
   uint32_t pp_rplu_load:24;
};

struct rserpool_policy_randomized_priority_leastused_degradation
{
   uint8_t  pp_rplud_policy:8;
   uint32_t pp_rplud_load:24;
   uint8_t  pp_rplud_pad:8;
   uint32_t pp_rplud_loaddeg:24;
};


struct rserpool_errorcause
{
   uint16_t aec_cause;
   uint16_t aec_length;
   char     aec_data[0];
};



#define EHT_ENRP_MODIFIER         0xee00
#define EHT_PRESENCE              (0x01 | EHT_ENRP_MODIFIER)
#define EHT_HANDLE_TABLE_REQUEST  (0x02 | EHT_ENRP_MODIFIER)
#define EHT_HANDLE_TABLE_RESPONSE (0x03 | EHT_ENRP_MODIFIER)
#define EHT_HANDLE_UPDATE         (0x04 | EHT_ENRP_MODIFIER)
#define EHT_LIST_REQUEST          (0x05 | EHT_ENRP_MODIFIER)
#define EHT_LIST_RESPONSE         (0x06 | EHT_ENRP_MODIFIER)
#define EHT_INIT_TAKEOVER         (0x07 | EHT_ENRP_MODIFIER)
#define EHT_INIT_TAKEOVER_ACK     (0x08 | EHT_ENRP_MODIFIER)
#define EHT_TAKEOVER_SERVER       (0x09 | EHT_ENRP_MODIFIER)
#define EHT_ERROR                 (0x0b | EHT_ENRP_MODIFIER)


struct rserpool_peerpresenceparameter
{
   uint32_t ppp_sender_id;
   uint32_t ppp_receiver_id;
   uint32_t ppp_checksum;
};

struct rserpool_peernameupdateparameter
{
   uint32_t pnup_sender_id;
   uint32_t pnup_receiver_id;
   uint16_t pnup_update_action;
   uint16_t pnup_pad;
};

#define PNUP_ADD_PE 0x0000
#define PNUP_DEL_PE 0x0001


struct rserpool_serverinfoparameter
{
   uint32_t sip_server_id;
   uint32_t sip_flags;
};

#define EHF_SERVERINFO_MULTICAST (1 << 31)


struct rserpool_serverparameter
{
   uint32_t sp_sender_id;
   uint32_t sp_receiver_id;
};

struct rserpool_targetparameter
{
   uint32_t tp_sender_id;
   uint32_t tp_receiver_id;
   uint32_t tp_target_id;
};


#define EHF_PRESENCE_REPLY_REQUIRED                (1 << 0)
#define EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY (1 << 0)
#define EHF_LIST_RESPONSE_REJECT                   (1 << 0)
#define EHF_HANDLE_TABLE_RESPONSE_REJECT           (1 << 0)
#define EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND     (1 << 1)


struct RSerPoolMessage
{
   unsigned int                                Type;
   uint16_t                                    Error;
   uint8_t                                     Flags;
   uint16_t                                    Action;
   union sockaddr_union*                       AddressArray;
   size_t                                      Addresses;

   char*                                       OperationErrorData;
   size_t                                      OperationErrorLength;

   char*                                       OffendingParameterTLV;
   size_t                                      OffendingParameterTLVLength;
   bool                                        OffendingParameterTLVAutoDelete;
   char*                                       OffendingMessageTLV;
   size_t                                      OffendingMessageTLVLength;
   bool                                        OffendingMessageTLVAutoDelete;

   char*                                       Buffer;
   bool                                        BufferAutoDelete;
   size_t                                      BufferSize;
   size_t                                      OriginalBufferSize;
   size_t                                      Position;

   PoolElementIdentifierType                   Identifier;
   HandlespaceChecksumType                     Checksum;
   struct PoolPolicySettings                   PolicySettings;
   struct PoolHandle                           Handle;

   RegistrarIdentifierType                     RegistrarIdentifier;
   RegistrarIdentifierType                     SenderID;
   RegistrarIdentifierType                     ReceiverID;

   struct ST_CLASS(PoolElementNode)*           PoolElementPtr;
   bool                                        PoolElementPtrAutoDelete;

   void*                                       CookiePtr;
   bool                                        CookiePtrAutoDelete;
   size_t                                      CookieSize;

   struct TransportAddressBlock*               TransportAddressBlockListPtr;
   bool                                        TransportAddressBlockListPtrAutoDelete;

   struct ST_CLASS(PoolElementNode)*           PoolElementPtrArray[MAX_MAX_HANDLE_RESOLUTION_ITEMS];
   size_t                                      PoolElementPtrArraySize;
   bool                                        PoolElementPtrArrayAutoDelete;

   struct ST_CLASS(PeerListNode)*              PeerListNodePtr;
   bool                                        PeerListNodePtrAutoDelete;
   struct ST_CLASS(PeerList)*                  PeerListPtr;
   bool                                        PeerListPtrAutoDelete;

   struct ST_CLASS(PoolHandlespaceManagement)* HandlespacePtr;
   bool                                        HandlespacePtrAutoDelete;

   struct ST_CLASS(HandleTableExtract)*        ExtractContinuation;

   sctp_assoc_t                                AssocID;
   uint32_t                                    PPID;
   union sockaddr_union                        SourceAddress;
};



/**
  * Constructor.
  *
  * @param buffer Buffer or NULL if buffer of given bufferSize should be allocated.
  * @param bufferSize Size of buffer.
  * @return RSerPoolMessage or NULL in case of error.
  */
struct RSerPoolMessage* rserpoolMessageNew(char* buffer, const size_t bufferSize);

/**
  * Destructor.
  *
  * @param message RSerPoolMessage.
  */
void rserpoolMessageDelete(struct RSerPoolMessage* message);

/**
  * Clear all fields of the RSerPoolMessage.
  *
  * @param message RSerPoolMessage.
  */
void rserpoolMessageClearAll(struct RSerPoolMessage* message);

/**
  * Reset buffer size to original value.
  *
  * @param message RSerPoolMessage.
  */
void rserpoolMessageClearBuffer(struct RSerPoolMessage* message);

/**
  * Convert RSerPoolMessage to packet and send it to file descriptor
  * with given timeout.
  *
  * @param protocol Protocol (e.g. IPPROTO_SCTP).
  * @param fd File descriptor to write packet to.
  * @param assocID Association ID.
  * @param flags Flags for sendmsg().
  * @param timeout Timeout in microseconds.
  * @param message RSerPoolMessage.
  * @return true in case of success; false otherwise.
  */
bool rserpoolMessageSend(int                     protocol,
                         int                     fd,
                         sctp_assoc_t            assocID,
                         int                     flags,
                         const card64            timeout,
                         struct RSerPoolMessage* message);

/**
  * For internal usage only!
  */
void* getSpace(struct RSerPoolMessage* message,
               const size_t        headerSize);



#ifdef __cplusplus
}
#endif


#include "rserpoolmessagecreator.h"
#include "rserpoolmessageparser.h"


#endif
