/*
 *  $Id: asapmessage.h,v 1.3 2004/07/16 15:35:40 dreibh Exp $
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
 * Purpose: ASAP Message Definitions
 *
 */


#ifndef ASAPMESSAGE_H
#define ASAPMESSAGE_H


#include "tdtypes.h"
#include "poolnamespacemanagement.h"
#include <ext_socket.h>
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif


#define PORT_ASAP 3863
#define PORT_ENRP 3864

#define PPID_ASAP 11 /* old value: 0xFAEEB5D1 */
#define PPID_ENRP 12 /* old value: 0xFAEEB5D2 */


#define AHT_REGISTRATION             0x01
#define AHT_DEREGISTRATION           0x02
#define AHT_REGISTRATION_RESPONSE    0x03
#define AHT_DEREGISTRATION_RESPONSE  0x04
#define AHT_NAME_RESOLUTION          0x05
#define AHT_NAME_RESOLUTION_RESPONSE 0x06
#define AHT_ENDPOINT_KEEP_ALIVE      0x07
#define AHT_ENDPOINT_KEEP_ALIVE_ACK  0x08
#define AHT_ENDPOINT_UNREACHABLE     0x09
#define AHT_SERVER_ANNOUNCE          0x0a
#define AHT_COOKIE                   0x0b
#define AHT_COOKIE_ECHO              0x0c
#define AHT_BUSINESS_CARD            0x0d
#define AHT_PEER_ERROR               0x0e


struct asap_header
{
   uint8_t  ah_type;
   uint8_t  ah_flags;
   uint16_t ah_length;
};

#define AHF_REGISTRATION_REJECT   (1<<0)
#define AHF_DEREGISTRATION_REJECT (1<<0)


struct asap_tlv_header
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




struct asap_poolelementparameter
{
   uint32_t pep_identifier;
   uint32_t pep_homeserverid;
   uint32_t pep_reg_life;
};


#define UTP_DATA_ONLY         0x0000
#define UTP_DATA_PLUS_CONTROL 0x0001

struct asap_sctptransportparameter
{
   uint16_t stp_port;
   uint16_t stp_transport_use;
};

struct asap_tcptransportparameter
{
   uint16_t ttp_port;
   uint16_t ttp_transport_use;
};

struct asap_udptransportparameter
{
   uint16_t utp_port;
   uint16_t utp_reserved;
};


struct asap_policy_roundrobin
{
   uint8_t  pp_rr_policy:8;
   uint32_t pp_rr_pad:24;
};

struct asap_policy_weighted_roundrobin
{
   uint8_t  pp_wrr_policy:8;
   uint32_t pp_wrr_weight:24;
};

struct asap_policy_leastused
{
   uint8_t  pp_lu_policy:8;
   uint32_t pp_lu_load:24;
};

struct asap_policy_leastused_degradation
{
   uint8_t  pp_lud_policy:8;
   uint32_t pp_lud_load:24;
};

struct asap_policy_random
{
   uint8_t  pp_rd_policy:8;
   uint32_t pp_rd_pad:24;
};

struct asap_policy_weighted_random
{
   uint8_t  pp_wrd_policy:8;
   uint32_t pp_wrd_weight:24;
};



/* ASAP-specific error causes */
#define AEC_OKAY                         0x00
#define AEC_UNRECOGNIZED_PARAMETER       0x01
#define AEC_UNRECOGNIZED_MESSAGE         0x02
#define AEC_AUTHORIZATION_FAILURE        0x03
#define AEC_INVALID_VALUES               0x04
#define AEC_NONUNIQUE_PE_ID              0x05
#define AEC_POLICY_INCONSISTENT          0x06

/* Implementation-specific error causes */
#define AEC_BUFFERSIZE_EXCEEDED          0x1000
#define AEC_OUT_OF_MEMORY                0x1001
#define AEC_READ_ERROR                   0x1010
#define AEC_WRITE_ERROR                  0x1011
#define AEC_CONNECTION_FAILURE_UNUSABLE  0x1012
#define AEC_CONNECTION_FAILURE_SOCKET    0x1013
#define AEC_CONNECTION_FAILURE_CONNECT   0x1014
#define AEC_NOT_FOUND                    0x1020
#define AEC_NO_NAME_SERVER_FOUND         0x1021


struct asap_errorcause
{
   uint16_t aec_cause;
   uint16_t aec_length;
   char     aec_data[0];
};



struct ASAPMessage
{
   uint8_t                             Type;
   uint16_t                            Error;

   char*                               OperationErrorData;
   size_t                              OperationErrorLength;

   char*                               OffendingParameterTLV;
   size_t                              OffendingParameterTLVLength;
   bool                                OffendingParameterTLVAutoDelete;
   char*                               OffendingMessageTLV;
   size_t                              OffendingMessageTLVLength;
   bool                                OffendingMessageTLVAutoDelete;

   char*                               Buffer;
   bool                                BufferAutoDelete;
   size_t                              BufferSize;
   size_t                              OriginalBufferSize;
   size_t                              Position;

   PoolElementIdentifierType           Identifier;
   struct PoolPolicySettings           PolicySettings;
   struct PoolHandle                   Handle;

   struct ST_CLASS(PoolElementNode)*   PoolElementPtr;
   bool                                PoolElementPtrAutoDelete;

   void*                               CookiePtr;
   bool                                CookiePtrAutoDelete;
   size_t                              CookieSize;

   GList*                              TransportAddressBlockListPtr;
   bool                                TransportAddressBlockListPtrAutoDelete;

   GList*                              PoolElementListPtr;
   bool                                PoolElementListPtrAutoDelete;

   sctp_assoc_t                        AssocID;
   unsigned short                      StreamID;
   uint32_t                            PPID;
};



/**
  * Constructor.
  *
  * @param buffer Buffer or NULL if buffer of given bufferSize should be allocated.
  * @param bufferSize Size of buffer.
  * @return ASAPMessage or NULL in case of error.
  */
struct ASAPMessage* asapMessageNew(char* buffer, const size_t bufferSize);

/**
  * Destructor.
  *
  * @param message ASAPMessage.
  */
void asapMessageDelete(struct ASAPMessage* message);

/**
  * Clear all fields of the ASAPMessage.
  *
  * @param message ASAPMessage.
  */
void asapMessageClearAll(struct ASAPMessage* message);

/**
  * Reset buffer size to original value.
  *
  * @param message ASAPMessage.
  */
void asapMessageClearBuffer(struct ASAPMessage* message);

/**
  * Convert ASAPMessage to packet and send it to file descriptor
  * with given timeout.
  *
  * @param fd File descriptor to write packet to.
  * @param timeout Timeout in microseconds.
  * @param message ASAPMessage.
  * @return true in case of success; false otherwise.
  */
bool asapMessageSend(int                 fd,
                     const card64        timeout,
                     struct ASAPMessage* message);

/**
  * Read ASAP packet from given file descriptor with given timeouts
  * and convert it to ASAPMessage.
  *
  * @param fd File descriptor to read packet from.
  * @param peekTimeout Timeout for reading the first 4 bytes (length + type).
  * @param totalTimeout Total timeout for reading the complete packet.
  * @param minBufferSize Minimum buffer size for the ASAPMessage to be created (e.g. for reply message).
  * @param senderAddress Socket address structure to store the sender's address to (NULL if not required).
  * @param senderAddressLength Reference to store length of sender address to (NULL if not required).
  * @return ASAPMessage in case of success; NULL otherwise.
  */
struct ASAPMessage* asapMessageReceive(int              fd,
                                       const card64     peekTimeout,
                                       const card64     totalTimeout,
                                       const size_t     minBufferSize,
                                       struct sockaddr* senderAddress,
                                       socklen_t*       senderAddressLength);

/**
  * For internal usage only!
  */
void* getSpace(struct ASAPMessage* message,
               const size_t        headerSize);



#ifdef __cplusplus
}
#endif


#include "asapcreator.h"
#include "asapparser.h"


#endif
