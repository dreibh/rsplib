/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
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
   void*                                DisposerUserData;
   void (*PoolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolNode,
                                    void*                      userData);
   void (*PoolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                           void*                             userData);
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
void ST_CLASS(poolHandlespaceManagementPrint)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
        FILE*                                       fd,
        const unsigned int                          fields);
void ST_CLASS(poolHandlespaceManagementVerify)(
               struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
void ST_CLASS(poolHandlespaceManagementClear)(
        struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPools)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElements)(
          const struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfPool)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const struct PoolHandle*                    poolHandle);
size_t ST_CLASS(poolHandlespaceManagementGetPoolElementsOfConnection)(
          struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
          const int                                   socketDescriptor,
          const sctp_assoc_t                          assocID);
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

unsigned int ST_CLASS(poolHandlespaceManagementNameResolution)(
                struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
                const struct PoolHandle*                    poolHandle,
                struct ST_CLASS(PoolElementNode)**          poolElementNodeArray,
                size_t*                                     poolElementNodes,
                const size_t                                maxNameResolutionItems,
                const size_t                                maxIncrement);


#define NTE_MAX_POOL_ELEMENT_NODES 128
#define NTEF_START                 (1 << 0)
#define NTEF_OWNCHILDSONLY         (1 << 1)

struct ST_CLASS(NameTableExtract)
{
   struct PoolHandle                 LastPoolHandle;
   PoolElementIdentifierType         LastPoolElementIdentifier;
   size_t                            PoolElementNodes;
   struct ST_CLASS(PoolElementNode)* PoolElementNodeArray[NTE_MAX_POOL_ELEMENT_NODES];
};


int ST_CLASS(poolHandlespaceManagementGetNameTable)(
       struct ST_CLASS(PoolHandlespaceManagement)* poolHandlespaceManagement,
       const RegistrarIdentifierType               homeRegistrarIdentifier,
       struct ST_CLASS(NameTableExtract)*          nameTableExtract,
       const unsigned int                          flags);


#ifdef __cplusplus
}
#endif