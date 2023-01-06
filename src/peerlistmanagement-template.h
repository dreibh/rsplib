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
 * Copyright (C) 2003-2023 by Thomas Dreibholz
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
#error Do not include this file directly, use poolhandlespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PeerListManagement)
{
   struct ST_CLASS(PeerList)                   List;
   struct ST_CLASS(PeerListNode)*              NewPeerListNode;
   struct ST_CLASS(PoolHandlespaceManagement)* Handlespace;

   void (*PeerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                        void*                          userData);
   void* DisposerUserData;
};


void ST_CLASS(peerListManagementNew)(
        struct ST_CLASS(PeerListManagement)*        peerListManagement,
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType        ownRegistrarIdentifier,
        void (*peerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                             void*                          userData),
        void* disposerUserData);
void ST_CLASS(peerListManagementDelete)(
        struct ST_CLASS(PeerListManagement)* peerListManagement);
void ST_CLASS(peerListManagementPrint)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        FILE*                                fd,
        const unsigned int                   fields);
void ST_CLASS(peerListManagementGetDescription)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        char*                                buffer,
        const size_t                         bufferSize);

void ST_CLASS(peerListManagementVerify)(
        struct ST_CLASS(PeerListManagement)* peerListManagement);
void ST_CLASS(peerListManagementClear)(
        struct ST_CLASS(PeerListManagement)* peerListManagement);
size_t ST_CLASS(peerListManagementGetPeers)(
          const struct ST_CLASS(PeerListManagement)* peerListManagement);

void ST_CLASS(peerListManagementActivateTimer)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const unsigned int                   timerCode,
        const unsigned long long             timerTimeStamp);
void ST_CLASS(peerListManagementDeactivateTimer)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        struct ST_CLASS(PeerListNode)*       peerListNode);

struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetFirstPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetLastPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetNextPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  struct ST_CLASS(PeerListNode)*       peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetPrevPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  struct ST_CLASS(PeerListNode)*       peerListNode);

struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetFirstPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetLastPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetNextPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  struct ST_CLASS(PeerListNode)*       peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetPrevPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  struct ST_CLASS(PeerListNode)*       peerListNode);

unsigned int ST_CLASS(peerListManagementRegisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const RegistrarIdentifierType        registrarIdentifier,
                const unsigned int                   flags,
                const struct TransportAddressBlock*  transportAddressBlock,
                const unsigned long long             currentTimeStamp,
                struct ST_CLASS(PeerListNode)**      peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestPrevPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestNextPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const RegistrarIdentifierType        registrarIdentifier,
                                  const struct TransportAddressBlock*  transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetRandomPeerListNode)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement);
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                struct ST_CLASS(PeerListNode)*       peerListNode);
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const RegistrarIdentifierType        registrarIdentifier,
                const struct TransportAddressBlock*  transportAddressBlock);
unsigned long long ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                      struct ST_CLASS(PeerListManagement)* peerListManagement);
void ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const unsigned long long             expiryTimeout);
size_t ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
          struct ST_CLASS(PeerListManagement)* peerListManagement,
          const unsigned long long             currentTimeStamp);
void ST_CLASS(peerListManagementVerifyChecksumsInHandlespace)(
        struct ST_CLASS(PeerListManagement)*        peerListManagement,
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementGetUsefulPeerForPE)(
                                  struct ST_CLASS(PeerListManagement)* peerListManagement,
                                  const PoolElementIdentifierType      peIdentifier);


#ifdef __cplusplus
}
#endif
