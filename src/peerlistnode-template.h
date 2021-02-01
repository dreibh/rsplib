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
 * Copyright (C) 2003-2021 by Thomas Dreibholz
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
#error Do not include this file directly, use peerListmanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PeerList);
struct TakeoverProcess;


/* Flags */
#define PLNF_STATIC    0
#define PLNF_DYNAMIC   (1 << 0)
#define PLNF_FROM_PEER (1 << 1)
#define PLNF_NEW       (1 << 15)  /* Indicates that registration added new node */

/* Status */
#define PLNS_LISTSYNC  (1 << 0)   /* Peer List synchronization in progress      */
#define PLNS_HTSYNC    (1 << 1)   /* Handle Table synchronization in progress   */
#define PLNS_MENTOR    (1 << 2)   /* Synchronization with mentor PR             */

/* Timer Codes */
#define PLNT_MAX_TIME_LAST_HEARD  3000
#define PLNT_MAX_TIME_NO_RESPONSE 3001
#define PLNT_TAKEOVER_EXPIRY      3002


struct ST_CLASS(PeerListNode)
{
   struct STN_CLASSNAME               PeerListIndexStorageNode;
   struct STN_CLASSNAME               PeerListTimerStorageNode;
   struct ST_CLASS(PeerList)*         OwnerPeerList;

   RegistrarIdentifierType            Identifier;
   unsigned int                       Flags;
   unsigned long long                 LastUpdateTimeStamp;

   unsigned int                       TimerCode;
   unsigned long long                 TimerTimeStamp;

   HandlespaceChecksumAccumulatorType OwnershipChecksum;

   unsigned int                       Status;
   RegistrarIdentifierType            TakeoverRegistrarID;
   struct TakeoverProcess*            TakeoverProcess;

   struct TransportAddressBlock*      AddressBlock;
   void*                              UserData;
};


void ST_CLASS(peerListIndexStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(peerListIndexStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);

void ST_CLASS(peerListTimerStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(peerListTimerStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);

struct ST_CLASS(PeerListNode)* ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(void* node);
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
void ST_CLASS(peerListNodePrint)(const struct ST_CLASS(PeerListNode)* peerListNode,
                                 FILE*                                fd,
                                 const unsigned int                   fields);


#ifdef __cplusplus
}
#endif
