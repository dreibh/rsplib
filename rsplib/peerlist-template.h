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


struct ST_CLASS(PeerList)
{
   struct ST_CLASSNAME PeerListIndexStorage;
   struct ST_CLASSNAME PeerListTimerStorage;

   ENRPIdentifierType  OwnIdentifier;

   void*               UserData;
};


void ST_CLASS(peerListNew)(struct ST_CLASS(PeerList)* peerList,
                           const ENRPIdentifierType   ownIdentifier);
void ST_CLASS(peerListDelete)(struct ST_CLASS(PeerList)* peerList);


/* ###### Get number of pool elements #################################### */
inline size_t ST_CLASS(peerListGetPeerListNodes)(
                 const struct ST_CLASS(PeerList)* peerList)
{
   return(ST_METHOD(GetElements)(&peerList->PeerListIndexStorage));
}


/* ###### Get first PeerListNode from Index ############################## */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromIndexStorage)(
                                         struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&peerList->PeerListIndexStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get last PeerListNode from Index ############################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromIndexStorage)(
                                         struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&peerList->PeerListIndexStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PeerListNode from Index ############################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromIndexStorage)(
                                         struct ST_CLASS(PeerList)*     peerList,
                                         struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&peerList->PeerListIndexStorage,
                                                   &peerListNode->PeerListIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PeerListNode from Index ########################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromIndexStorage)(
                                         struct ST_CLASS(PeerList)*     peerList,
                                         struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&peerList->PeerListIndexStorage,
                                                   &peerListNode->PeerListIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get first PeerListNode from Timer ############################## */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetFirstPeerListNodeFromTimerStorage)(
                                            struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&peerList->PeerListTimerStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get last PeerListNode from Timer ############################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetLastPeerListNodeFromTimerStorage)(
                                            struct ST_CLASS(PeerList)* peerList)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&peerList->PeerListTimerStorage);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PeerListNode from Timer ############################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetNextPeerListNodeFromTimerStorage)(
                                         struct ST_CLASS(PeerList)*     peerList,
                                         struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&peerList->PeerListTimerStorage,
                                                   &peerListNode->PeerListTimerStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PeerListNode from Timer ########################### */
inline struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetPrevPeerListNodeFromTimerStorage)(
                                         struct ST_CLASS(PeerList)*     peerList,
                                         struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&peerList->PeerListTimerStorage,
                                                   &peerListNode->PeerListTimerStorageNode);
   if(node) {
      return(ST_CLASS(getPeerListNodeFromPeerListTimerStorageNode)(node));
   }
   return(NULL);
}


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
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListGetRandomPeerNode)(
                                  struct ST_CLASS(PeerList)* peerList);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const ENRPIdentifierType            identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestPrevPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const ENRPIdentifierType            identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListFindNearestNextPeerListNode)(
                                  struct ST_CLASS(PeerList)*          peerList,
                                  const ENRPIdentifierType            identifier,
                                  const struct TransportAddressBlock* transportAddressBlock);
struct ST_CLASS(PeerListNode)* ST_CLASS(peerListRemovePeerListNode)(
                                  struct ST_CLASS(PeerList)*        peerList,
                                  struct ST_CLASS(PeerListNode)* peerListNode);
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

/* ###### Insert PeerListNode into timer storage ######################### */
inline void ST_CLASS(peerListActivateTimer)(
               struct ST_CLASS(PeerList)*     peerList,
               struct ST_CLASS(PeerListNode)* peerListNode,
               const unsigned int             timerCode,
               const unsigned long long       timerTimeStamp)
{
   struct STN_CLASSNAME* result;

   CHECK(!STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode));
   peerListNode->TimerCode      = timerCode;
   peerListNode->TimerTimeStamp = timerTimeStamp;
   result = ST_METHOD(Insert)(&peerList->PeerListTimerStorage,
                              &peerListNode->PeerListTimerStorageNode);
   CHECK(result == &peerListNode->PeerListTimerStorageNode);
}


/* ###### Remove PeerListNode from timer storage ######################### */
inline void ST_CLASS(peerListDeactivateTimer)(
               struct ST_CLASS(PeerList)*     peerList,
               struct ST_CLASS(PeerListNode)* peerListNode)
{
   struct STN_CLASSNAME* result;

   if(STN_METHOD(IsLinked)(&peerListNode->PeerListTimerStorageNode)) {
      result = ST_METHOD(Remove)(&peerList->PeerListTimerStorage,
                                 &peerListNode->PeerListTimerStorageNode);
      CHECK(result == &peerListNode->PeerListTimerStorageNode);
   }
}


size_t ST_CLASS(peerListPurgeExpiredPeerListNodes)(
          struct ST_CLASS(PeerList)* peerList,
          const unsigned long long   currentTimeStamp);


#ifdef __cplusplus
}
#endif
