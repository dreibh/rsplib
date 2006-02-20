/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id$
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
#error Do not include this file directly, use peerListmanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PeerList);


#define PLNF_STATIC    0
#define PLNF_DYNAMIC   (1 << 0)
#define PLNF_FROM_PEER (1 << 1)
#define PLNF_MULTICAST (1 << 2)

#define PLNT_MAX_TIME_LAST_HEARD    3000
#define PLNT_MAX_TIME_NO_RESPONSE   3001


struct ST_CLASS(PeerListNode)
{
   struct STN_CLASSNAME               PeerListIndexStorageNode;
   struct STN_CLASSNAME               PeerListAddressStorageNode;
   struct STN_CLASSNAME               PeerListTimerStorageNode;
   struct ST_CLASS(PeerList)*         OwnerPeerList;

   RegistrarIdentifierType            Identifier;
   unsigned int                       Flags;
   unsigned long long                 LastUpdateTimeStamp;

   unsigned int                       TimerCode;
   unsigned long long                 TimerTimeStamp;

   HandlespaceChecksumAccumulatorType OwnershipChecksum;

   struct TransportAddressBlock*      AddressBlock;
   void*                              UserData;
};


void ST_CLASS(peerListIndexStorageNodePrint)(const void *nodePtr, FILE* fd);
int ST_CLASS(peerListIndexStorageNodeComparison)(const void *nodePtr1, const void *nodePtr2);

void ST_CLASS(peerListAddressStorageNodePrint)(const void *nodePtr, FILE* fd);
int ST_CLASS(peerListAddressStorageNodeComparison)(const void *nodePtr1, const void *nodePtr2);

void ST_CLASS(peerListTimerStorageNodePrint)(const void *nodePtr, FILE* fd);
int ST_CLASS(peerListTimerStorageNodeComparison)(const void *nodePtr1, const void *nodePtr2);

struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(void* node);
struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListAddressStorageNode)(void* node);
struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(void* node);

void ST_CLASS(peerListNodeNew)(struct ST_CLASS(PeerListNode)* peerListNode,
                               const RegistrarIdentifierType  identifier,
                               const unsigned int             flags,
                               struct TransportAddressBlock*  transportAddressBlock);
void ST_CLASS(peerListNodeDelete)(struct ST_CLASS(PeerListNode)* peerListNode);
int ST_CLASS(peerListNodeUpdate)(struct ST_CLASS(PeerListNode)*       peerListNode,
                                 const struct ST_CLASS(PeerListNode)* source);
HandlespaceChecksumType ST_CLASS(peerListNodeGetOwnershipChecksum)(
                           const struct ST_CLASS(PeerListNode)* peerListNode);
void ST_CLASS(peerListNodeGetDescription)(
        const struct ST_CLASS(PeerListNode)* peerListNode,
        char*                                buffer,
        const size_t                         bufferSize,
        const unsigned int                   fields);
void ST_CLASS(peerListNodeVerify)(struct ST_CLASS(PeerListNode)* peerListNode);
void ST_CLASS(peerListNodePrint)(const struct ST_CLASS(PeerListNode)* peerListNode,
                                 FILE*                                fd,
                                 const unsigned int                   fields);


#ifdef __cplusplus
}
#endif
