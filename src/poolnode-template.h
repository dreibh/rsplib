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


struct ST_CLASS(PoolHandlespaceNode);


#define PNF_CONTROLCHANNEL (1 << 0)

struct ST_CLASS(PoolNode)
{
   struct STN_CLASSNAME                  PoolIndexStorageNode;
   struct ST_CLASSNAME                   PoolElementSelectionStorage;
   struct ST_CLASSNAME                   PoolElementIndexStorage;
   struct ST_CLASS(PoolHandlespaceNode)* OwnerPoolHandlespaceNode;

   struct PoolHandle                     Handle;
   const struct ST_CLASS(PoolPolicy)*    Policy;
   int                                   Protocol;
   int                                   Flags;
   PoolElementSeqNumberType              GlobalSeqNumber;

   void*                                 UserData;
};


void ST_CLASS(poolIndexStorageNodePrint)(const void* nodePtr, FILE* fd);
int ST_CLASS(poolIndexStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2);


void ST_CLASS(poolNodeNew)(struct ST_CLASS(PoolNode)*         poolNode,
                           const struct PoolHandle*           poolHandle,
                           const struct ST_CLASS(PoolPolicy)* poolPolicy,
                           const int                          protocol,
                           const int                          flags);
void ST_CLASS(poolNodeDelete)(struct ST_CLASS(PoolNode)* poolNode);

void ST_CLASS(poolNodeResequence)(struct ST_CLASS(PoolNode)* poolNode);
size_t ST_CLASS(poolNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolNode)* poolNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(
                                     struct ST_CLASS(PoolNode)* poolNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)* poolNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetLastPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)* poolNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetPrevPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode);
void ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(
        struct ST_CLASS(PoolNode)*        poolNode,
        struct ST_CLASS(PoolElementNode)* poolElementNode);
void ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(
        struct ST_CLASS(PoolNode)*        poolNode,
        struct ST_CLASS(PoolElementNode)* poolElementNode);
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
void ST_CLASS(poolNodeGetDescription)(const struct ST_CLASS(PoolNode)* poolNode,
                                      char*                            buffer,
                                      const size_t                     bufferSize);
void ST_CLASS(poolNodePrint)(const struct ST_CLASS(PoolNode)* poolNode,
                             FILE*                            fd,
                             const unsigned int               fields);
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
