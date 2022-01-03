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
 * Copyright (C) 2003-2022 by Thomas Dreibholz
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


struct ST_CLASS(PoolHandlespaceManagement)
{
   struct ST_CLASS(PoolHandlespaceNode) Handlespace;
   struct ST_CLASS(PoolNode)*           NewPoolNode;
   struct ST_CLASS(PoolElementNode)*    NewPoolElementNode;

   void (*PoolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolNode,
                                    void*                      userData);
   void (*PoolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                           void*                             userData);
   void* DisposerUserData;

   void (*PoolNodeUpdateNotification)(struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                      struct ST_CLASS(PoolElementNode)*           poolElementNode,
                                      enum PoolNodeUpdateAction                   updateAction,
                                      HandlespaceChecksumAccumulatorType          preUpdateChecksum,
                                      RegistrarIdentifierType                     preUpdateHomeRegistrar,
                                      void*                                       userData);
   void* NotificationUserData;
};


void ST_CLASS(poolHandlespaceManagementNew)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType               homeRegistrarIdentifier,
        void (*poolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolElementNode,
                                         void*                      userData),
        void (*poolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                void*                             userData),
        void* disposerUserData);
void ST_CLASS(poolHandlespaceManagementDelete)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
void ST_CLASS(poolHandlespaceManagementGetDescription)(
        const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        char*                                             buffer,
        const size_t                                      bufferSize);
void ST_CLASS(poolHandlespaceManagementPrint)(
        const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        FILE*                                             fd,
        const unsigned int                                fields);
void ST_CLASS(poolHandlespaceManagementVerify)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
void ST_CLASS(poolHandlespaceManagementClear)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);

HandlespaceChecksumType ST_CLASS(poolHandlespaceManagementGetHandlespaceChecksum)(
                           const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
HandlespaceChecksumType ST_CLASS(poolHandlespaceManagementGetOwnershipChecksum)(
                           const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPools)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElements)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetOwnedPoolElements)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfPool)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const struct PoolHandle*                    poolHandle);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfConnection)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const int                                   socketDescriptor,
          const sctp_assoc_t                          assocID);

struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolNode)(
                              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
struct ST_CLASS(PoolNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolNode)(
                              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                              struct ST_CLASS(PoolNode)*                  poolNode);

struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementTimerNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode);

struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceNodeGetFirstPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceNode)* poolHandlespaceNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementConnectionNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode);

struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementOwnershipNode)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetFirstPoolElementOwnershipNodeForIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     const RegistrarIdentifierType               homeRegistrarIdentifier);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementGetNextPoolElementOwnershipNodeForSameIdentifier)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     struct ST_CLASS(PoolElementNode)*           poolElementNode);

unsigned int ST_CLASS(poolHandlespaceManagementRegisterPoolElement)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const RegistrarIdentifierType               homeRegistrarIdentifier,
                const PoolElementIdentifierType             poolElementIdentifier,
                const unsigned int                          registrationLife,
                const struct PoolPolicySettings*            poolPolicySettings,
                const struct TransportAddressBlock*         userTransport,
                const struct TransportAddressBlock*         registratorTransport,
                const int                                   connectionSocketDescriptor,
                const sctp_assoc_t                          connectionAssocID,
                const unsigned long long                    currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**          poolElementNode);
unsigned int ST_CLASS(poolHandlespaceManagementRegisterPoolElementByPtr)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const struct ST_CLASS(PoolElementNode)*     originalPoolElementNode,
                const unsigned long long                    currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**          poolElementNode);

void ST_CLASS(poolHandlespaceManagementUpdateOwnershipOfPoolElementNode)(
              struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
              struct ST_CLASS(PoolElementNode)*           poolElementNode,
              const RegistrarIdentifierType               newHomeRegistrarIdentifier);
void ST_CLASS(poolHandlespaceManagementUpdateConnectionOfPoolElementNode)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        struct ST_CLASS(PoolElementNode)*           poolElementNode,
        const int                                   connectionSocketDescriptor,
        const sctp_assoc_t                          connectionAssocID);

unsigned long long ST_CLASS(poolHandlespaceManagementGetNextTimerTimeStamp)(
                      struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
void ST_CLASS(poolHandlespaceManagementRestartPoolElementExpiryTimer)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        struct ST_CLASS(PoolElementNode)*           poolElementNode,
        const unsigned long long                    expiryTimeout);
size_t ST_CLASS(poolHandlespaceManagementPurgeExpiredPoolElements)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const unsigned long long                    currentTimeStamp);
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolHandlespaceManagementFindPoolElement)(
                                     struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                                     const struct PoolHandle*                    poolHandle,
                                     const PoolElementIdentifierType             poolElementIdentifier);
unsigned int ST_CLASS(poolHandlespaceManagementDeregisterPoolElementByPtr)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                struct ST_CLASS(PoolElementNode)*           poolElementNode);

unsigned int ST_CLASS(poolHandlespaceManagementDeregisterPoolElement)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                const PoolElementIdentifierType             poolElementIdentifier);

unsigned int ST_CLASS(poolHandlespaceManagementHandleResolution)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                struct ST_CLASS(PoolElementNode)**          poolElementNodeArray,
                size_t*                                     poolElementNodes,
                const size_t                                maxHandleResolutionItems,
                const size_t                                maxIncrement);


/*
   Maximum amount of Pool Element Parameters in ASAP/ENRP message:
   Per PE Transport Parameter:
   - 20 for Header + PE-ID + Home-PR-ID + Reg.Life
   - 8+8 for User Transport Parameter with single IPv4 address
   - 8+8 for ASAP Transport Parameter with single IPv4 address
   - 8 for Policy Parameter
   => 60 Bytes.
   => About ~1090 PE Parameters per message
      (depending on other overhead, e.g. PH Parameter)
*/
#define NTE_MAX_POOL_ELEMENT_NODES 1024
#define HTEF_START                 (1 << 0)
#define HTEF_OWNCHILDSONLY         (1 << 1)

struct ST_CLASS(HandleTableExtract)
{
   struct PoolHandle                 LastPoolHandle;
   PoolElementIdentifierType         LastPoolElementIdentifier;
   size_t                            PoolElementNodes;
   struct ST_CLASS(PoolElementNode)* PoolElementNodeArray[NTE_MAX_POOL_ELEMENT_NODES];
};


int ST_CLASS(poolHandlespaceManagementGetHandleTable)(
       struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
       const RegistrarIdentifierType               homeRegistrarIdentifier,
       struct ST_CLASS(HandleTableExtract)*        handleTableExtract,
       const unsigned int                          flags,
       size_t                                      maxElements);


void ST_CLASS(poolHandlespaceManagementMarkPoolElementNodes)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        const RegistrarIdentifierType ownerID);
size_t ST_CLASS(poolHandlespaceManagementPurgeMarkedPoolElementNodes)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const RegistrarIdentifierType ownerID);


#ifdef __cplusplus
}
#endif
