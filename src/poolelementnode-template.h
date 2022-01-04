/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2022 by Thomas Dreibholz
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

#ifndef INTERNAL_POOLTEMPLATE
#error Do not include this file directly, use poolhandlespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


#include <ext_socket.h>


/* Timer Codes */
#define PENT_EXPIRY                 1000
#define PENT_KEEPALIVE_TRANSMISSION 1001
#define PENT_KEEPALIVE_TIMEOUT      1002

/* Pool Element flags */
#define PENF_MARKED  (1 << 0)
#define PENF_UPDATED (1 << 14)   /* Indicates that reregistration updated entry */
#define PENF_NEW     (1 << 15)   /* Indicates that registration added new node  */


/* ====== Pool Element Node ============================================== */
struct ST_CLASS(PoolNode);

struct ST_CLASS(PoolElementNode)
{
   struct STN_CLASSNAME               PoolElementSelectionStorageNode;
   struct STN_CLASSNAME               PoolElementIndexStorageNode;
   struct STN_CLASSNAME               PoolElementTimerStorageNode;
   struct STN_CLASSNAME               PoolElementConnectionStorageNode;
   struct STN_CLASSNAME               PoolElementOwnershipStorageNode;
   struct ST_CLASS(PoolNode)*         OwnerPoolNode;

   PoolElementIdentifierType          Identifier;
   HandlespaceChecksumAccumulatorType Checksum;
   RegistrarIdentifierType            HomeRegistrarIdentifier;
   unsigned int                       RegistrationLife;
   struct PoolPolicySettings          PolicySettings;
   unsigned int                       Flags;

   PoolElementSeqNumberType           SeqNumber;
   PoolElementSeqNumberType           RoundCounter;
   unsigned int                       VirtualCounter;
   unsigned int                       Degradation;
   unsigned int                       UnreachabilityReports;
   unsigned long long                 SelectionCounter;
   unsigned long long                 LastUpdateTimeStamp;

   unsigned int                       TimerCode;
   unsigned long long                 TimerTimeStamp;

   int                                ConnectionSocketDescriptor;
   sctp_assoc_t                       ConnectionAssocID;

   struct TransportAddressBlock*      UserTransport;
   struct TransportAddressBlock*      RegistratorTransport;

   void*                              UserData;
   unsigned long long                 LastKeepAliveTransmission;
};


void ST_CLASS(poolElementNodeNew)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const PoolElementIdentifierType   identifier,
                                  const RegistrarIdentifierType     homeRegistrarIdentifier,
                                  const unsigned int                registrationLife,
                                  const struct PoolPolicySettings*  pps,
                                  struct TransportAddressBlock*     userTransport,
                                  struct TransportAddressBlock*     registratorTransport,
                                  const int                         connectionSocketDescriptor,
                                  const sctp_assoc_t                connectionAssocID);
void ST_CLASS(poolElementNodeDelete)(struct ST_CLASS(PoolElementNode)* poolElementNode);
void ST_CLASS(poolElementNodeGetDescription)(
        const struct ST_CLASS(PoolElementNode)* poolElementNode,
        char*                                   buffer,
        const size_t                            bufferSize,
        const unsigned int                      fields);
void ST_CLASS(poolElementNodePrint)(
        const struct ST_CLASS(PoolElementNode)* poolElementNode,
        FILE*                                   fd,
        const unsigned int                      fields);
HandlespaceChecksumAccumulatorType ST_CLASS(poolElementNodeComputeChecksum)(
                                      const struct ST_CLASS(PoolElementNode)* poolElementNode);
int ST_CLASS(poolElementNodeUpdate)(struct ST_CLASS(PoolElementNode)*       poolElementNode,
                                    const struct ST_CLASS(PoolElementNode)* source);
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(void* node);
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(void* node);
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromTimerStorageNode)(void* node);
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(void* node);
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(void* node);
void ST_CLASS(poolElementTimerStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolElementTimerStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);
void ST_CLASS(poolElementOwnershipStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolElementOwnershipStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);


#ifdef __cplusplus
}
#endif
