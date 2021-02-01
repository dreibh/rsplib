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
#error Do not include this file directly, use poolhandlespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PeerList)
{
   struct ST_CLASSNAME     PeerListIndexStorage;
   struct ST_CLASSNAME     PeerListTimerStorage;

   RegistrarIdentifierType OwnIdentifier;

   void*                   UserData;
};


void ST_CLASS(peerListNew)(struct ST_CLASS(PeerList)* peerList,
                           const RegistrarIdentifierType   ownIdentifier);
void ST_CLASS(peerListDelete)(struct ST_CLASS(PeerList)* peerList);
size_t ST_CLASS(peerListGetPeerListNodes)(
          const struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromIndexStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromTimerStorage)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListAddPeerListNode)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode,
                                  unsigned int*                  errorCode);
void ST_CLASS(peerListUpdatePeerListNode)(
        struct ST_CLASS(PeerList)*           peerList,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const struct ST_CLASS(PeerListNode)* source,
        unsigned int*                        errorCode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListAddOrUpdatePeerListNode)(
                                  struct ST_CLASS(PeerList)*      peerList,
                                  struct ST_CLASS(PeerListNode)** peerListNode,
                                  unsigned int*                   errorCode);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetRandomPeerListNode)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestPrevPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestNextPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const RegistrarIdentifierType       identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListRemovePeerListNode)(
                                  struct ST_CLASS(PeerList)*     peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
unsigned int ST_CLASS(peerListCheckPeerListNodeCompatibility)(
                const struct ST_CLASS(PeerList)*     peerList,
                const struct ST_CLASS(PeerListNode)* peerListNode);
void ST_CLASS(peerListGetDescription)(struct ST_CLASS(PeerList)* peerList,
                                      char*                      buffer,
                                      const size_t               bufferSize);
void ST_CLASS(peerListVerify)(struct ST_CLASS(PeerList)* peerList);
void ST_CLASS(peerListPrint)(struct ST_CLASS(PeerList)* peerList,
                             FILE*                      fd,
                             const unsigned int         fields);
void ST_CLASS(peerListClear)(struct ST_CLASS(PeerList)* peerList,
                             void                       (*peerListNodeDisposer)(void* peerListNode, void* userData),
                             void*                      userData);
void ST_CLASS(peerListActivateTimer)(
               struct ST_CLASS(PeerList)*     peerList,
               struct ST_CLASS(PeerListNode)* peerListNode,
               const unsigned int             timerCode,
               const unsigned long long       timerTimeStamp);
void ST_CLASS(peerListDeactivateTimer)(
        struct ST_CLASS(PeerList)*     peerList,
        struct ST_CLASS(PeerListNode)* peerListNode);
size_t ST_CLASS(peerListPurgeExpiredPeerListNodes)(
          struct ST_CLASS(PeerList)* peerList,
          const unsigned long long   currentTimeStamp);


#ifdef __cplusplus
}
#endif
