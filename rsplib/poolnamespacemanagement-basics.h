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

#ifndef POOLMANAGEMENT_BASICS_H
#define POOLMANAGEMENT_BASICS_H


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_POOLHANDLESIZE 32

typedef uint32_t ENRPIdentifierType;
typedef uint32_t PoolElementIdentifierType;
typedef uint32_t PoolElementSeqNumberType;


#define PENC_OKAY                           0
#define PENC_NO_RESOURCES                   1
#define PENC_NOT_FOUND                      2
#define PENC_INVALID_ID                     3
#define PENC_DUPLICATE_ID                   4
#define PENC_WRONG_PROTOCOL                 5
#define PENC_WRONG_CONTROLCHANNEL_HANDLING  6
#define PENC_INCOMPATIBLE_POOL_POLICY       7
#define PENC_INVALID_POOL_POLICY            8
#define PENC_INVALID_POOL_HANDLE            9

#define _PENC_LAST_CODE                    10


#define PENPO_POLICYINFO      (1 << 0)   /* constants set by PE      */
#define PENPO_POLICYSTATE     (1 << 1)   /* current policy state     */
#define PENPO_HOME_NS         (1 << 2)   /* Home NS identifier       */
#define PENPO_REGLIFE         (1 << 3)   /* Registration lifetime    */
#define PENPO_UR_REPORTS      (1 << 4)   /* Unreachability reports   */
#define PENPO_LASTUPDATE      (1 << 5)   /* Last update time stamp   */
#define PENPO_USERTRANSPORT   (1 << 6)   /* User transport info      */

#define PNPO_INDEX            (1 << 8)   /* PEs of pool by index     */
#define PNPO_SELECTION        (1 << 9)   /* PEs of pool by selection */

#define PNNPO_POOLS_INDEX     (1 << 10)
#define PNNPO_POOLS_SELECTION (1 << 11)
#define PNNPO_POOLS_PROPERTY  (1 << 12)
#define PNNPO_POOLS_TIMER     (1 << 13)


#define PENPO_FULL          (~0)
#define PENPO_ONLY_ID       0
#define PENPO_ONLY_POLICY   (PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PNNPO_POOLS_SELECTION)



#define PLNPO_USERTRANSPORT (1 << 0)

#define PLPO_PEERS_INDEX    (1 << 10)
#define PLPO_PEERS_TIMER    (1 << 11)

#define PLPO_FULL           (~0)
#define PLPO_ONLY_INDEX     (PLPO_PEERS_INDEX)


const char* poolNamespaceManagementGetErrorDescription(const unsigned int errorCode);

void poolHandleGetDescription(const unsigned char* poolHandle,
                              const size_t         poolHandleSize,
                              char*                buffer,
                              const size_t         bufferSize);
void poolHandlePrint(const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     FILE*                fd);
int poolHandleComparison(const unsigned char* poolHandle1,
                         const size_t         poolHandleSize1,
                         const unsigned char* poolHandle2,
                         const size_t         poolHandleSize2);

PoolElementIdentifierType getPoolElementIdentifier();

unsigned long long random64();


/*
 Starting value for seqence numbers. Set it to (~0) ^ 0xf to test
 sequence number warp.
 */
extern PoolElementSeqNumberType SeqNumberStart;


#ifdef __cplusplus
}
#endif

#endif
