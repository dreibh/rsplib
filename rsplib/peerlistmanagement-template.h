/*
 * An Efficient RSerPeerList PeerList List Management Implementation
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


struct ST_CLASS(PeerListManagement)
{
   struct ST_CLASS(PeerList)      List;
   struct ST_CLASS(PeerListNode)* NewPeerListNode;

   void (*PeerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                        void*                          userData);
   void* DisposerUserData;
};


void ST_CLASS(peerListManagementNew)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        const ENRPIdentifierType             ownNSIdentifier,
        void (*peerListNodeUserDataDisposer)(struct ST_CLASS(PeerListNode)* peerListNode,
                                             void*                          userData),
        void* disposerUserData);
void ST_CLASS(peerListManagementDelete)(
        struct ST_CLASS(PeerListManagement)* peerListManagement);


inline void ST_CLASS(peerListManagementPrint)(
               struct ST_CLASS(PeerListManagement)* peerListManagement,
               FILE*                                fd,
               const unsigned int                   fields)
{
   ST_CLASS(peerListPrint)(&peerListManagement->List, fd, fields);
}


inline void ST_CLASS(peerListManagementVerify)(
               struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   ST_CLASS(peerListVerify)(&peerListManagement->List);
}


void ST_CLASS(peerListManagementClear)(
        struct ST_CLASS(PeerListManagement)* peerListManagement);


/* ###### Get number of pools ############################################ */
inline size_t ST_CLASS(peerListManagementGetPeerListNodes)(
                 const struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   return(ST_CLASS(peerListGetPeerListNodes)(&peerListManagement->List));
}


unsigned int ST_CLASS(peerListManagementRegisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const ENRPIdentifierType             nsIdentifier,
                const unsigned int                   flags,
                const struct TransportAddressBlock*  transportAddressBlock,
                const unsigned long long             currentTimeStamp,
                struct ST_CLASS(PeerListNode)**      peerListNode);


/* ###### Find PeerListNode ############################################## */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindPeerListNode)(
                                         struct ST_CLASS(PeerListManagement)* peerListManagement,
                                         const ENRPIdentifierType             nsIdentifier,
                                         const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindPeerListNode)(
             &peerListManagement->List,
             nsIdentifier,
             transportAddressBlock));
}


/* ###### Find nearest prev PeerListNode ################################# */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestPrevPeerListNode)(
                                         struct ST_CLASS(PeerListManagement)* peerListManagement,
                                         const ENRPIdentifierType             nsIdentifier,
                                         const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindNearestPrevPeerListNode)(
             &peerListManagement->List,
             nsIdentifier,
             transportAddressBlock));
}


/* ###### Find nearest next PeerListNode ################################# */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListManagementFindNearestNextPeerListNode)(
                                         struct ST_CLASS(PeerListManagement)* peerListManagement,
                                         const ENRPIdentifierType             nsIdentifier,
                                         const struct TransportAddressBlock*  transportAddressBlock)
{
   return(ST_CLASS(peerListFindNearestNextPeerListNode)(
             &peerListManagement->List,
             nsIdentifier,
             transportAddressBlock));
}


unsigned int ST_CLASS(peerListManagementDeregisterPeerListNodeByPtr)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                struct ST_CLASS(PeerListNode)*       peerListNode);
unsigned int ST_CLASS(peerListManagementDeregisterPeerListNode)(
                struct ST_CLASS(PeerListManagement)* peerListManagement,
                const ENRPIdentifierType             nsIdentifier,
                const struct TransportAddressBlock*  transportAddressBlock);

/* ###### Get next timer time stamp ###################################### */
inline unsigned long long ST_CLASS(peerListManagementGetNextTimerTimeStamp)(
                             struct ST_CLASS(PeerListManagement)* peerListManagement)
{
   const struct ST_CLASS(PeerListNode)* nextTimer =
      ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
         &peerListManagement->List);
   if(nextTimer != NULL) {
      return(nextTimer->TimerTimeStamp);
   }
   return(~0);
}

void ST_CLASS(peerListManagementRestartPeerListNodeExpiryTimer)(
        struct ST_CLASS(PeerListManagement)* peerListManagement,
        struct ST_CLASS(PeerListNode)*       peerListNode,
        const unsigned long long             expiryTimeout);
size_t ST_CLASS(peerListManagementPurgeExpiredPeerListNodes)(
          struct ST_CLASS(PeerListManagement)* peerListManagement,
          const unsigned long long             currentTimeStamp);


#ifdef __cplusplus
}
#endif
