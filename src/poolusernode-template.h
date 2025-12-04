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
 * Copyright (C) 2003-2026 by Thomas Dreibholz
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

#ifndef INTERNAL_POOLTEMPLATE
#error Do not include this file directly, use poolUsermanagement.h
#endif

#include "timestamphashtable.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PoolUserNode)
{
   struct STN_CLASSNAME       PoolUserListStorageNode;

   int                        ConnectionSocketDescriptor;
   sctp_assoc_t               ConnectionAssocID;
   unsigned long long         LastUpdateTimeStamp;

   struct TimeStampHashTable* HandleResolutionHash;
   struct TimeStampHashTable* EndpointUnreachableHash;
};


void ST_CLASS(poolUserStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolUserStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);

struct ST_CLASS(PoolUserNode)* ST_CLASS(getPoolUserNodeFromPoolUserListStorageNode)(void* node);

void ST_CLASS(poolUserNodeNew)(struct ST_CLASS(PoolUserNode)* poolUserNode,
                               const int                      connectionSocketDescriptor,
                               const sctp_assoc_t             connectionAssocID);
void ST_CLASS(poolUserNodeDelete)(struct ST_CLASS(PoolUserNode)* poolUserNode);

void ST_CLASS(poolUserNodeGetDescription)(
        const struct ST_CLASS(PoolUserNode)* poolUserNode,
        char*                                buffer,
        const size_t                         bufferSize,
        const unsigned int                   fields);
void ST_CLASS(poolUserNodePrint)(const struct ST_CLASS(PoolUserNode)* poolUserNode,
                                 FILE*                                fd,
                                 const unsigned int                   fields);

double ST_CLASS(poolUserNodeNoteHandleResolution)(struct ST_CLASS(PoolUserNode)* poolUserNode,
                                                  const struct PoolHandle*       poolHandle,
                                                  const unsigned long long       now,
                                                  const size_t                   buckets,
                                                  const size_t                   maxEntries);
double ST_CLASS(poolUserNodeNoteEndpointUnreachable)(struct ST_CLASS(PoolUserNode)*  poolUserNode,
                                                     const struct PoolHandle*        poolHandle,
                                                     const PoolElementIdentifierType peIdentifier,
                                                     const unsigned long long        now,
                                                     const size_t                    buckets,
                                                     const size_t                    maxEntries);


#ifdef __cplusplus
}
#endif
