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
 * Copyright (C) 2002-2015 by Thomas Dreibholz
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
#define PORT_ENRP 9901  /* old value: 3864 */

#define PPID_ASAP 11    /* old value: 0xFAEEB5D1 */
#define PPID_ENRP 12    /* old value: 0xFAEEB5D2 */


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
#define AHT_ERROR                      (0x0e | AHT_ASAP_MODIFIER)


#define AHF_REGISTRATION_REJECT        (1 << 0)
#define AHF_HANDLE_RESOLUTION_REJECT   (1 << 0)
#define AHF_ENDPOINT_KEEP_ALIVE_HOME   (1 << 0)


struct rserpool_header
{
   uint8_t  ah_type;
   uint8_t  ah_flags;
   uint16_t ah_length;
} __attribute__((packed));


struct rserpool_tlv_header
{
   uint16_t atlv_type;
   uint16_t atlv_length;
} __attribute__((packed));


#define ATT_ACTION_MASK                0xc000
#define ATT_ACTION_STOP                0x0000
#define ATT_ACTION_STOP_AND_REPORT     0x4000
#define ATT_ACTION_CONTINUE            0x8000
#define ATT_ACTION_CONTINUE_AND_REPORT 0xc000
#define PURE_ATT_TYPE(type)            (type & (~ATT_ACTION_MASK))

#define ATT_IPv4_ADDRESS               0x0001
#define ATT_IPv6_ADDRESS               0x0002
#define ATT_DCCP_TRANSPORT             0x0003
#define ATT_SCTP_TRANSPORT             0x0004
#define ATT_TCP_TRANSPORT              0x0005
#define ATT_UDP_TRANSPORT              0x0006
#define ATT_UDPLITE_TRANSPORT          0x0007
#define ATT_POOL_POLICY                0x0008
#define ATT_POOL_HANDLE                0x0009
#define ATT_POOL_ELEMENT               0x000a
#define ATT_SERVER_INFORMATION         0x000b
#define ATT_OPERATION_ERROR            0x000c
#define ATT_COOKIE                     0x000d
#define ATT_POOL_ELEMENT_IDENTIFIER    0x000e
#define ATT_POOL_ELEMENT_CHECKSUM      0x000f
#define ATT_HANDLE_RESOLUTION          0x003f   /* Custom */

struct rserpool_poolelementparameter
{
   uint32_t pep_identifier;
   uint32_t pep_homeserverid;
   uint32_t pep_reg_life;
} __attribute__((packed));


#define UTP_DATA_ONLY         0x0000
#define UTP_DATA_PLUS_CONTROL 0x0001

struct rserpool_sctptransportparameter
{
   uint16_t stp_port;
   uint16_t stp_transport_use;
} __attribute__((packed));

struct rserpool_tcptransportparameter
{
   uint16_t ttp_port;
   uint16_t ttp_transport_use;
} __attribute__((packed));

struct rserpool_udptransportparameter
{
   uint16_t utp_port;
   uint16_t utp_reserved;
} __attribute__((packed));

struct rserpool_dccptransportparameter
{
   uint16_t dtp_port;
   uint16_t dtp_reserved;
   uint32_t dtp_scode;
} __attribute__((packed));


struct rserpool_policy_roundrobin
{
   uint32_t pp_rr_policy;
} __attribute__((packed));

struct rserpool_policy_weighted_roundrobin
{
   uint32_t pp_wrr_policy;
   uint32_t pp_wrr_weight;
} __attribute__((packed));

struct rserpool_policy_priority
{
   uint32_t pp_p_policy;
   uint32_t pp_p_priority;
} __attribute__((packed));

struct rserpool_policy_leastused
{
   uint32_t  pp_lu_policy;
   uint32_t pp_lu_load;
} __attribute__((packed));

struct rserpool_policy_leastused_dpf
{
   uint32_t pp_ludpf_policy;
   uint32_t pp_ludpf_load;
   uint32_t pp_ludpf_load_dpf;
   uint32_t pp_ludpf_distance;
} __attribute__((packed));

struct rserpool_policy_leastused_degradation
{
   uint32_t pp_lud_policy;
   uint32_t pp_lud_load;
   uint32_t pp_lud_loaddeg;
} __attribute__((packed));

struct rserpool_policy_leastused_degradation_dpf
{
   uint32_t pp_luddpf_policy;
   uint32_t pp_luddpf_load;
   uint32_t pp_luddpf_loaddeg;
   uint32_t pp_luddpf_load_dpf;
   uint32_t pp_luddpf_distance;
} __attribute__((packed));

struct rserpool_policy_priority_leastused
{
   uint32_t pp_plu_policy;
   uint32_t pp_plu_load;
} __attribute__((packed));

struct rserpool_policy_priority_leastused_degradation
{
   uint32_t pp_plud_policy;
   uint32_t pp_plud_load;
   uint32_t pp_plud_loaddeg;
} __attribute__((packed));

struct rserpool_policy_random
{
   uint32_t pp_rd_policy;
} __attribute__((packed));

struct rserpool_policy_weighted_random
{
   uint32_t pp_wrd_policy;
   uint32_t pp_wrd_weight;
} __attribute__((packed));

struct rserpool_policy_weighted_random_dpf
{
   uint32_t pp_wrddpf_policy;
   uint32_t pp_wrddpf_weight;
   uint32_t pp_wrddpf_weight_dpf;
   uint32_t pp_wrddpf_distance;
} __attribute__((packed));

struct rserpool_policy_randomized_leastused
{
   uint32_t pp_rlu_policy;
   uint32_t pp_rlu_load;
} __attribute__((packed));

struct rserpool_policy_randomized_leastused_degradation
{
   uint32_t pp_rlud_policy;
   uint32_t pp_rlud_load;
   uint32_t pp_rlud_loaddeg;
} __attribute__((packed));

struct rserpool_policy_randomized_priority_leastused
{
   uint32_t pp_rplu_policy;
   uint32_t pp_rplu_load;
} __attribute__((packed));

struct rserpool_policy_randomized_priority_leastused_degradation
{
   uint32_t pp_rplud_policy;
   uint32_t pp_rplud_load;
   uint32_t pp_rplud_loaddeg;
} __attribute__((packed));


struct rserpool_errorcause
{
   uint16_t aec_cause;
   uint16_t aec_length;
   char     aec_data[0];
} __attribute__((packed));


struct rserpool_handleresolutionparameter
{
   uint32_t hrp_items;
} __attribute__((packed));


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
#define EHT_ERROR                 (0x0a | EHT_ENRP_MODIFIER)


struct rserpool_peerpresenceparameter
{
   uint32_t ppp_sender_id;
   uint32_t ppp_receiver_id;
   uint32_t ppp_checksum;
} __attribute__((packed));

struct rserpool_handleupdateparameter
{
   uint32_t pnup_sender_id;
   uint32_t pnup_receiver_id;
   uint16_t pnup_update_action;
   uint16_t pnup_pad;
} __attribute__((packed));

#define PNUP_ADD_PE 0x0000
#define PNUP_DEL_PE 0x0001


struct rserpool_serverinfoparameter
{
   uint32_t sip_server_id;
} __attribute__((packed));

#define EHF_SERVERINFO_MULTICAST (1 << 31)


struct rserpool_serverparameter
{
   uint32_t sp_sender_id;
   uint32_t sp_receiver_id;
} __attribute__((packed));

struct rserpool_targetparameter
{
   uint32_t tp_sender_id;
   uint32_t tp_receiver_id;
   uint32_t tp_target_id;
} __attribute__((packed));


#define EHF_PRESENCE_REPLY_REQUIRED                (1 << 0)
#define EHF_HANDLE_TABLE_REQUEST_OWN_CHILDREN_ONLY (1 << 0)
#define EHF_LIST_RESPONSE_REJECT                   (1 << 0)
#define EHF_HANDLE_TABLE_RESPONSE_REJECT           (1 << 0)
#define EHF_HANDLE_TABLE_RESPONSE_MORE_TO_SEND     (1 << 1)
#define EHF_TAKEOVER_SUGGESTED                     (1 << 0)   /* draft-dreibholz-rserpool-enrpupdate */


struct RSerPoolMessage
{
   unsigned int                                Type;
   uint16_t                                    Error;
   uint8_t                                     Flags;
   uint16_t                                    Action;
   union sockaddr_union*                       AddressArray;
   size_t                                      Addresses;

   uint16_t                                    OperationErrorCode;
   char*                                       OperationErrorData;
   size_t                                      OperationErrorLength;

   char*                                       OffendingParameterTLV;
   size_t                                      OffendingParameterTLVLength;

   char*                                       ErrorCauseParameterTLV;
   size_t                                      ErrorCauseParameterTLVLength;
   bool                                        ErrorCauseParameterTLVAutoDelete;

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
   struct ST_CLASS(PeerListManagement)*        PeerListPtr;
   bool                                        PeerListPtrAutoDelete;

   struct ST_CLASS(PoolHandlespaceManagement)* HandlespacePtr;
   bool                                        HandlespacePtrAutoDelete;
   size_t                                      MaxElementsPerHTRequest;

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
  * @param sctpFlags SCTP flags.
  * @param timeout Timeout in microseconds.
  * @param message RSerPoolMessage.
  * @return true in case of success; false otherwise.
  */
bool rserpoolMessageSend(int                      protocol,
                         int                      fd,
                         const sctp_assoc_t       assocID,
                         const int                flags,
                         const uint16_t           sctpFlags,
                         const unsigned long long timeout,
                         struct RSerPoolMessage*  message);

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
