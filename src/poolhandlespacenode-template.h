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


struct ST_CLASS(PoolHandlespaceNode)
{
   struct ST_CLASSNAME                 PoolIndexStorage;             /* Pools                          */
   struct ST_CLASSNAME                 PoolElementTimerStorage;      /* PEs with timer event scheduled */
   struct ST_CLASSNAME                 PoolElementConnectionStorage; /* PEs by connection              */
   struct ST_CLASSNAME                 PoolElementOwnershipStorage;  /* PEs by ownership               */

   HandlespaceChecksumAccumulatorType  HandlespaceChecksum;          /* Handlespace checksum           */
   HandlespaceChecksumAccumulatorType  OwnershipChecksum;            /* Ownership checksum             */
   RegistrarIdentifierType             HomeRegistrarIdentifier;      /* This NS's Identifier           */
   size_t                              PoolElements;                 /* Number of Pool Elements        */
   size_t                              OwnedPoolElements;            /* Number of owned Pool Elements  */

   void* NotificationUserData;
   void (*PoolNodeUpdateNotification)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      struct ST_CLASS(PoolElementNode)*     poolElementNode,
                                      enum PoolNodeUpdateAction             updateAction,
                                      HandlespaceChecksumAccumulatorType    preUpdateChecksum,
                                      RegistrarIdentifierType               preUpdateHomeRegistrar,
                                      void*                                 userData);
};


void ST_CLASS(poolHandlespaceNodeNew)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      const RegistrarIdentifierType         homeRegistrarIdentifier,
                                      void (*poolNodeUpdateNotification)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                                                         struct ST_CLASS(PoolElementNode)*     poolElementNode,
                                                                         enum PoolNodeUpdateAction             updateAction,
                                                                         HandlespaceChecksumAccumulatorType    preUpdateChecksum,
                                                                         RegistrarIdentifierType               preUpdateHomeRegistrar,
                                                                         void*                                 userData),
                                      void* notificationUserData);
void ST_CLASS(poolHandlespaceNodeDelete)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeGetHandlespaceChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeGetOwnershipChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeComputeHandlespaceChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
HandlespaceChecksumAccumulatorType ST_CLASS(poolHandlespaceNodeComputeOwnershipChecksum)(
                                      const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                      const RegistrarIdentifierType               registrarIdentifier);
size_t ST_CLASS(poolHandlespaceNodeGetTimerNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
size_t ST_CLASS(poolHandlespaceNodeGetOwnershipNodesForIdentifier)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const RegistrarIdentifierType         homeRegistrarIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const RegistrarIdentifierType         homeRegistrarIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
size_t ST_CLASS(poolHandlespaceNodeGetConnectionNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
size_t ST_CLASS(poolHandlespaceNodeGetConnectionNodesForConnection)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const int                             connectionSocketDescriptor,
          const sctp_assoc_t                    assocID);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNodeForConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const int                             connectionSocketDescriptor,
                                     const sctp_assoc_t                    assocID);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolElementConnectionNodeForSameConnection)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
size_t ST_CLASS(poolHandlespaceNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
size_t ST_CLASS(poolHandlespaceNodeGetOwnedPoolElementNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
size_t ST_CLASS(poolHandlespaceNodeGetPoolNodes)(
          const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
size_t ST_CLASS(poolHandlespaceNodeGetPoolElementNodesOfPool)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const struct PoolHandle*              poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeGetNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeAddPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeFindPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              const struct PoolHandle*              poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeFindNearestNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              const struct PoolHandle*              poolHandle);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceNodeRemovePoolNode)(
                              struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                              struct ST_CLASS(PoolNode)*            poolNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeAddPoolElementNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolNode)*            poolNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode,
                                     unsigned int*                         errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeFindPoolElementNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     const struct PoolHandle*              poolHandle,
                                     const PoolElementIdentifierType       poolElementIdentifier);
void ST_CLASS(poolHandlespaceNodeUpdateOwnershipOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const RegistrarIdentifierType         newHomeRegistrarIdentifier);
void ST_CLASS(poolHandlespaceNodeUpdateConnectionOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const int                             connectionSocketDescriptor,
        const sctp_assoc_t                    connectionAssocID);
void ST_CLASS(poolHandlespaceNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolHandlespaceNode)*   poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeAddOrUpdatePoolElementNode)(
                                    struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                    struct ST_CLASS(PoolNode)**           poolNode,
                                    struct ST_CLASS(PoolElementNode)**    poolElementNode,
                                    unsigned int*                         errorCode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                     struct ST_CLASS(PoolElementNode)*     poolElementNode);
void ST_CLASS(poolHandlespaceNodeGetDescription)(
        const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        char*                                       buffer,
        const size_t                                bufferSize);
void ST_CLASS(poolHandlespaceNodePrint)(const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                        FILE*                                       fd,
                                        const unsigned int                          fields);
int ST_CLASS(poolHandlespaceNodeHasActiveTimer)(
       const struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
       const struct ST_CLASS(PoolElementNode)*     poolElementNode);
void ST_CLASS(poolHandlespaceNodeActivateTimer)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode,
        const unsigned int                    timerCode,
        const unsigned long long              timerTimeStamp);
void ST_CLASS(poolHandlespaceNodeDeactivateTimer)(
        struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
        struct ST_CLASS(PoolElementNode)*     poolElementNode);
void ST_CLASS(poolHandlespaceNodeVerify)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
void ST_CLASS(poolHandlespaceNodeClear)(struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
                                        void                                  (*poolNodeDisposer)(void* poolNode, void* userData),
                                        void                                  (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                                        void*                                 userData);
size_t ST_CLASS(poolHandlespaceNodeSelectPoolElementNodesByPolicy)(
          struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode,
          const struct PoolHandle*              poolHandle,
          struct ST_CLASS(PoolElementNode)**    poolElementNodeArray,
          const size_t                          maxPoolElementNodes,
          const size_t                          maxIncrement,
          unsigned int*                         errorCode);


#ifdef __cplusplus
}
#endif
