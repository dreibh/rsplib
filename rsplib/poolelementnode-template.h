/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef INTERNAL_POOLTEMPLATE
#error Do not include this file directly, use poolnamespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


#include <ext_socket.h>


/* Timer Codes */
#define PENT_EXPIRY                 1000
#define PENT_KEEPALIVE_TRANSMISSION 1001
#define PENT_KEEPALIVE_TIMEOUT      1002


/* ====== Pool Element Node ============================================== */
struct ST_CLASS(PoolNode);

struct ST_CLASS(PoolElementNode)
{
   struct STN_CLASSNAME          PoolElementSelectionStorageNode;
   struct STN_CLASSNAME          PoolElementIndexStorageNode;
   struct STN_CLASSNAME          PoolElementTimerStorageNode;
   struct STN_CLASSNAME          PoolElementConnectionStorageNode;
   struct STN_CLASSNAME          PoolElementOwnershipStorageNode;
   struct ST_CLASS(PoolNode)*    OwnerPoolNode;

   PoolElementIdentifierType     Identifier;
   ENRPIdentifierType            HomeNSIdentifier;
   unsigned int                  RegistrationLife;
   struct PoolPolicySettings     PolicySettings;

   PoolElementSeqNumberType      SeqNumber;
   PoolElementSeqNumberType      RoundCounter;
   unsigned int                  VirtualCounter;
   unsigned int                  Degradation;
   unsigned int                  UnreachabilityReports;
   unsigned long long            SelectionCounter;
   unsigned long long            LastUpdateTimeStamp;

   unsigned int                  TimerCode;
   unsigned long long            TimerTimeStamp;

   int                           ConnectionSocketDescriptor;
   sctp_assoc_t                  ConnectionAssocID;

   struct TransportAddressBlock* AddressBlock;
   void*                         UserData;
};


void ST_CLASS(poolElementNodeNew)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const PoolElementIdentifierType   identifier,
                                  const ENRPIdentifierType          homeNSIdentifier,
                                  const unsigned int                registrationLife,
                                  const struct PoolPolicySettings*  pps,
                                  struct TransportAddressBlock*     transportAddressBlock,
                                  const int                         registratorSocketDescriptor,
                                  const sctp_assoc_t                registratorAssocID);
void ST_CLASS(poolElementNodeDelete)(struct ST_CLASS(PoolElementNode)* poolElementNode);
void ST_CLASS(poolElementNodeGetDescription)(
        struct ST_CLASS(PoolElementNode)* poolElementNode,
        char*                             buffer,
        const size_t                      bufferSize,
        const unsigned int                fields);
void ST_CLASS(poolElementNodePrint)(
        struct ST_CLASS(PoolElementNode)* poolElementNode,
        FILE*                             fd,
        const unsigned int                fields);


/* ###### Update ######################################################### */
int ST_CLASS(poolElementNodeUpdate)(struct ST_CLASS(PoolElementNode)*       poolElementNode,
                                    const struct ST_CLASS(PoolElementNode)* source);


/* ###### Get PoolElementNode from given Selection Node ################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementSelectionStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Index Node ###################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementIndexStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Timer Node ###################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromTimerStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementTimerStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Ownership Node ################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementOwnershipStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Connection Node ################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementConnectionStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


void ST_CLASS(poolElementTimerStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolElementTimerStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);
void ST_CLASS(poolElementOwnershipStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolElementOwnershipStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);


#ifdef __cplusplus
}
#endif
