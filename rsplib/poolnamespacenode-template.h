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


struct ST_CLASS(PoolNamespaceNode)
{
   struct ST_CLASSNAME PoolIndexStorage;             /* Pools                          */
   struct ST_CLASSNAME PoolElementTimerStorage;      /* PEs with timer event scheduled */
   struct ST_CLASSNAME PoolElementConnectionStorage; /* PEs by connection              */
   struct ST_CLASSNAME PoolElementOwnershipStorage;  /* PEs by ownership               */
   ENRPIdentifierType  HomeNSIdentifier;             /* This NS's Identifier           */
   size_t              PoolElements;                 /* Number of Pool Elements        */
};


void ST_CLASS(poolNamespaceNodeNew)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                    const ENRPIdentifierType            homeNSIdentifier);
void ST_CLASS(poolNamespaceNodeDelete)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode);


/* ###### Get number of timers ########################################### */
inline size_t ST_CLASS(poolNamespaceNodeGetTimerNodes)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementTimerStorage));
}


/* ###### Get first timer ################################################ */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last timer ################################################# */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementTimerNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementTimerStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get next timer ################################################# */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementTimerNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous timer ############################################# */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementTimerNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementTimerStorage,
                                                   &poolElementNode->PoolElementTimerStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromTimerStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get number of ownership nodes ################################## */
inline size_t ST_CLASS(poolNamespaceNodeGetOwnershipNodes)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementOwnershipStorage));
}


size_t ST_CLASS(poolNamespaceNodeGetOwnershipNodesForIdentifier)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                 const ENRPIdentifierType            homeNSIdentifier);


/* ###### Get first ownership ############################################ */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last ownership ############################################# */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementOwnershipStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier);


/* ###### Get next ownership ############################################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous ownership ######################################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                                   &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const ENRPIdentifierType            homeNSIdentifier);


/* ###### Get prev ownership of same home NS identifier ################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementOwnershipNodeForSameIdentifier)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      prevPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(prevPoolElementNode->HomeNSIdentifier == poolElementNode->HomeNSIdentifier) {
         return(prevPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get next ownership of same home NS identifier ################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementOwnershipStorage,
                                               &poolElementNode->PoolElementOwnershipStorageNode);
   if(node != NULL) {
      nextPoolElementNode = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(node);
      if(nextPoolElementNode->HomeNSIdentifier == poolElementNode->HomeNSIdentifier) {
         return(nextPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get number of connection nodes ################################## */
inline size_t ST_CLASS(poolNamespaceNodeGetConnectionNodes)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolElementConnectionStorage));
}


size_t ST_CLASS(poolNamespaceNodeGetConnectionNodesForConnection)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
          const int                           registratorSocketDescriptor,
          const sctp_assoc_t                  assocID);


/* ###### Get first connection ############################################ */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNamespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get last connection ############################################# */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementConnectionNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNamespaceNode->PoolElementConnectionStorage);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           registratorSocketDescriptor,
                                     const sctp_assoc_t                  assocID,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           registratorSocketDescriptor,
                                     const sctp_assoc_t                  assocID,
                                     const struct PoolHandle*            poolHandle,
                                     const PoolElementIdentifierType     poolElementIdentifier);


/* ###### Get next connection ############################################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous connection ######################################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementConnectionStorage,
                                                   &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      return(ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node));
   }
   return(NULL);
}


struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           registratorSocketDescriptor,
                                     const sctp_assoc_t                  assocID);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetLastPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     const int                           registratorSocketDescriptor,
                                     const sctp_assoc_t                  assocID);


/* ###### Get prev connection of same home NS identifier ################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolElementConnectionNodeForSameConnection)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* prevPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetPrev)(&poolNamespaceNode->PoolElementConnectionStorage,
                                               &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      prevPoolElementNode = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node);
      if((prevPoolElementNode->ConnectionSocketDescriptor == poolElementNode->ConnectionSocketDescriptor) &&
         (prevPoolElementNode->ConnectionAssocID          == poolElementNode->ConnectionAssocID)) {
         return(prevPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get next connection of same home NS identifier ################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct ST_CLASS(PoolElementNode)* nextPoolElementNode;
   struct STN_CLASSNAME*             node = ST_METHOD(GetNext)(&poolNamespaceNode->PoolElementConnectionStorage,
                                               &poolElementNode->PoolElementConnectionStorageNode);
   if(node != NULL) {
      nextPoolElementNode = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(node);
      if((nextPoolElementNode->ConnectionSocketDescriptor == poolElementNode->ConnectionSocketDescriptor) &&
         (nextPoolElementNode->ConnectionAssocID          == poolElementNode->ConnectionAssocID)) {
         return(nextPoolElementNode);
      }
   }
   return(NULL);
}


/* ###### Get number of pools ############################################ */
inline size_t ST_CLASS(poolNamespaceNodeGetPoolElementNodes)(
                 const struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(poolNamespaceNode->PoolElements);
}


/* ###### Get number of pools ############################################ */
inline size_t ST_CLASS(poolNamespaceNodeGetPoolNodes)(
                 const struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return(ST_METHOD(GetElements)(&poolNamespaceNode->PoolIndexStorage));
}


size_t ST_CLASS(poolNamespaceNodeGetPoolElementNodesOfPool)(
          struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
          const struct PoolHandle*            poolHandle);


/* ###### Get first PoolNode ############################################# */
inline struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetFirstPoolNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetFirst)(&poolNamespaceNode->PoolIndexStorage));
}


/* ###### Get last PoolNode ############################################## */
inline struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetLastPoolNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetLast)(&poolNamespaceNode->PoolIndexStorage));
}


/* ###### Get next PoolNode ############################################## */
inline struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetNextPoolNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolNode)*          poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetNext)(&poolNamespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


/* ###### Get previous PoolNode ########################################## */
inline struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeGetPrevPoolNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolNode)*          poolNode)
{
   return((struct ST_CLASS(PoolNode)*)ST_METHOD(GetPrev)(&poolNamespaceNode->PoolIndexStorage, &poolNode->PoolIndexStorageNode));
}


struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeAddPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode);
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindNearestPrevPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeFindNearestNextPoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              const struct PoolHandle*            poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolNamespaceNodeRemovePoolNode)(
                              struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                              struct ST_CLASS(PoolNode)*          poolNode);

struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeAddPoolElementNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolNode)*          poolNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode,
                                     unsigned int*                       errorCode);

inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeFindPoolElementNode)(
                                            struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                            const struct PoolHandle*            poolHandle,
                                            const PoolElementIdentifierType     poolElementIdentifier)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(
                                            poolNamespaceNode,
                                            poolHandle);
   if(poolNode != NULL) {
      return(ST_CLASS(poolNodeFindPoolElementNode)(poolNode, poolElementIdentifier));
   }
   return(NULL);
}

void ST_CLASS(poolNamespaceNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolNamespaceNode)*     poolNamespaceNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeAddOrUpdatePoolElementNode)(
                                    struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                    struct ST_CLASS(PoolNode)**         poolNode,
                                    struct ST_CLASS(PoolElementNode)**  poolElementNode,
                                    unsigned int*                       errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                     struct ST_CLASS(PoolElementNode)*   poolElementNode);
void ST_CLASS(poolNamespaceNodeGetDescription)(
        struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
        char*                               buffer,
        const size_t                        bufferSize);
void ST_CLASS(poolNamespaceNodePrint)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                      FILE*                               fd,
                                      const unsigned int                  fields);


/* ###### Insert PoolElementNode into timer storage ###################### */
inline void ST_CLASS(poolNamespaceNodeActivateTimer)(
               struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
               struct ST_CLASS(PoolElementNode)*   poolElementNode,
               const unsigned int                  timerCode,
               const unsigned long long            timerTimeStamp)
{
   struct STN_CLASSNAME* result;

   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   poolElementNode->TimerCode      = timerCode;
   poolElementNode->TimerTimeStamp = timerTimeStamp;
   result = ST_METHOD(Insert)(&poolNamespaceNode->PoolElementTimerStorage,
                              &poolElementNode->PoolElementTimerStorageNode);
   CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
}


/* ###### Remove PoolElementNode from timer storage ###################### */
inline void ST_CLASS(poolNamespaceNodeDeactivateTimer)(
               struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
               struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   struct STN_CLASSNAME* result;

   if(STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode)) {
      result = ST_METHOD(Remove)(&poolNamespaceNode->PoolElementTimerStorage,
                                 &poolElementNode->PoolElementTimerStorageNode);
      CHECK(result == &poolElementNode->PoolElementTimerStorageNode);
   }
}


void ST_CLASS(poolNamespaceNodeVerify)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode);
void ST_CLASS(poolNamespaceNodeClear)(struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                                      void                                (*poolNodeDisposer)(void* poolNode, void* userData),
                                      void                                (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                                      void*                               userData);


/* ###### Select PoolElementNodes by Pool Policy ######################### */
inline size_t ST_CLASS(poolNamespaceNodeSelectPoolElementNodesByPolicy)(
                 struct ST_CLASS(PoolNamespaceNode)* poolNamespaceNode,
                 const struct PoolHandle*            poolHandle,
                 struct ST_CLASS(PoolElementNode)**  poolElementNodeArray,
                 const size_t                        maxPoolElementNodes,
                 const size_t                        maxIncrement,
                 unsigned int*                       errorCode)
{
   struct ST_CLASS(PoolNode)* poolNode = ST_CLASS(poolNamespaceNodeFindPoolNode)(poolNamespaceNode, poolHandle);
   size_t                     count    = 0;
   if(poolNode != NULL) {
      *errorCode = RSPERR_OKAY;
      count = poolNode->Policy->SelectionFunction(poolNode, poolElementNodeArray, maxPoolElementNodes, maxIncrement);
#ifdef VERIFY
      ST_CLASS(poolNamespaceNodeVerify)(poolNamespaceNode);
#endif
   }
   else {
      *errorCode = RSPERR_NOT_FOUND;
   }
   return(count);
}


#ifdef __cplusplus
}
#endif
