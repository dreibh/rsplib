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


struct ST_CLASS(PoolNamespaceNode);


#define PNF_CONTROLCHANNEL (1 << 0)

struct ST_CLASS(PoolNode)
{
   struct STN_CLASSNAME                PoolIndexStorageNode;
   struct ST_CLASSNAME                 PoolElementSelectionStorage;
   struct ST_CLASSNAME                 PoolElementIndexStorage;
   struct ST_CLASS(PoolNamespaceNode)* OwnerPoolNamespaceNode;

   size_t                              PoolHandleSize;
   unsigned char                       PoolHandle[MAX_POOLHANDLESIZE];
   const struct ST_CLASS(PoolPolicy)*  Policy;
   int                                 Protocol;
   int                                 Flags;
   PoolElementSeqNumberType            GlobalSeqNumber;

   void*                               UserData;
};


void ST_CLASS(poolIndexStorageNodePrint)(const void *nodePtr, FILE* fd);
int ST_CLASS(poolIndexStorageNodeComparison)(const void *nodePtr1, const void *nodePtr2);


void ST_CLASS(poolNodeNew)(struct ST_CLASS(PoolNode)*         poolNode,
                           const unsigned char*               poolHandle,
                           const size_t                       poolHandleSize,
                           const struct ST_CLASS(PoolPolicy)* poolPolicy,
                           const int                          protocol,
                           const int                          flags);
void ST_CLASS(poolNodeDelete)(struct ST_CLASS(PoolNode)* poolNode);

void ST_CLASS(poolNodeResequence)(struct ST_CLASS(PoolNode)* poolNode);


/* ###### Get number of pool elements #################################### */
inline size_t ST_CLASS(poolNodeGetPoolElementNodes)(
                 const struct ST_CLASS(PoolNode)* poolNode)
{
   return(ST_METHOD(GetElements)(&poolNode->PoolElementIndexStorage));
}


/* ###### Get first PoolElementNode from Index ########################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(
                                            struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNode->PoolElementIndexStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get last PoolElementNode from Index ############################ */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetLastPoolElementNodeFromIndex)(
                                            struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNode->PoolElementIndexStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PoolElementNode from Index ############################ */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(
                                            struct ST_CLASS(PoolNode)*        poolNode,
                                            struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNode->PoolElementIndexStorage,
                                                   &poolElementNode->PoolElementIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PoolElementNode from Index ######################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetPrevPoolElementNodeFromIndex)(
                                            struct ST_CLASS(PoolNode)*        poolNode,
                                            struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNode->PoolElementIndexStorage,
                                                   &poolElementNode->PoolElementIndexStorageNode);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get first PoolElementNode from Selection ####################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(
                                            struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNode->PoolElementSelectionStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get last PoolElementNode from Selection ######################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetLastPoolElementNodeFromSelection)(
                                            struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNode->PoolElementSelectionStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PoolElementNode from Selection ######################## */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(
                                            struct ST_CLASS(PoolNode)*        poolNode,
                                            struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetNext)(&poolNode->PoolElementSelectionStorage,
                                                   &poolElementNode->PoolElementSelectionStorageNode);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Get previous PoolElementNode from Selection #################### */
inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetPrevPoolElementNodeFromSelection)(
                                            struct ST_CLASS(PoolNode)*        poolNode,
                                            struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetPrev)(&poolNode->PoolElementSelectionStorage,
                                                   &poolElementNode->PoolElementSelectionStorageNode);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
}


/* ###### Unlink PoolElementNode from Selection ########################## */
inline void ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(
               struct ST_CLASS(PoolNode)*        poolNode,
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(Remove)(&poolNode->PoolElementSelectionStorage,
                                                  &poolElementNode->PoolElementSelectionStorageNode);
   CHECK(node == &poolElementNode->PoolElementSelectionStorageNode);
}


/* ###### Link PoolElementNode into Selection ############################ */
inline void ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(
               struct ST_CLASS(PoolNode)*        poolNode,
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node;

   CHECK(poolPolicySettingsIsValid(&poolElementNode->PolicySettings));

   if(poolNode->Policy->UpdatePoolElementNodeFunction) {
      (*poolNode->Policy->UpdatePoolElementNodeFunction)(poolElementNode);
   }

   node = ST_METHOD(Insert)(&poolNode->PoolElementSelectionStorage,
                            &poolElementNode->PoolElementSelectionStorageNode);
   CHECK(node == &poolElementNode->PoolElementSelectionStorageNode);
}


unsigned int ST_CLASS(poolNodeCheckPoolElementNodeCompatibility)(
                struct ST_CLASS(PoolNode)*          poolNode,
                struct ST_CLASS(PoolElementNode)*   poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeAddPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode,
                                     unsigned int*                     errorCode);
void ST_CLASS(poolNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolNode)*              poolNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeFindPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*      poolNode,
                                     const PoolElementIdentifierType identifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeFindNearestNextPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*      poolNode,
                                     const PoolElementIdentifierType identifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode);
void ST_CLASS(poolNodeGetDescription)(struct ST_CLASS(PoolNode)* poolNode,
                                      char*                      buffer,
                                      const size_t               bufferSize);
void ST_CLASS(poolNodePrint)(struct ST_CLASS(PoolNode)* poolNode,
                             FILE*                      fd,
                             const unsigned int         fields);
void ST_CLASS(poolNodeClear)(struct ST_CLASS(PoolNode)* poolNode,
                             void                       (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                             void*                      userData);
size_t ST_CLASS(poolNodeSelectPoolElementNodesBySelectionStorageOrder)(
          struct ST_CLASS(PoolNode)*         poolNode,
          struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
          const size_t                       maxPoolElementNodes,
          const size_t                       maxIncrement);
size_t ST_CLASS(poolNodeSelectPoolElementNodesByValueTree)(
          struct ST_CLASS(PoolNode)*         poolNode,
          struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
          const size_t                       maxPoolElementNodes,
          const size_t                       maxIncrement);


#ifdef __cplusplus
}
#endif
