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


struct ST_CLASS(PoolNamespaceManagement)
{
   struct ST_CLASS(PoolNamespaceNode) Namespace;
   struct ST_CLASS(PoolNode)*         NewPoolNode;
   struct ST_CLASS(PoolElementNode)*  NewPoolElementNode;
   void (*PoolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolNode,
                                    void*                      userData);
   void (*PoolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                           void*                             userData);
};


void ST_CLASS(poolNamespaceManagementNew)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
        const ENRPIdentifierType                  homeNSIdentifier,
        void (*poolNodeUserDataDisposer)(struct ST_CLASS(PoolNode)* poolElementNode,
                                         void*                      userData),
        void (*poolElementNodeUserDataDisposer)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                                void*                             userData));

void ST_CLASS(poolNamespaceManagementDelete)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement);


inline void ST_CLASS(poolNamespaceManagementPrint)(
               struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
               FILE*                                     fd,
               const unsigned int                        fields)
{
   ST_CLASS(poolNamespaceNodePrint)(&poolNamespaceManagement->Namespace, fd, fields);
}


inline void ST_CLASS(poolNamespaceManagementVerify)(
               struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   ST_CLASS(poolNamespaceNodeVerify)(&poolNamespaceManagement->Namespace);
}


void ST_CLASS(poolNamespaceManagementClear)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement);


/* ###### Get number of pools ############################################ */
inline size_t ST_CLASS(poolNamespaceManagementGetPools)(
                 const struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   return(ST_CLASS(poolNamespaceNodeGetPoolNodes)(&poolNamespaceManagement->Namespace));
}


/* ###### Get number of pool elements #################################### */
inline size_t ST_CLASS(poolNamespaceManagementGetPoolElements)(
                 const struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   return(ST_CLASS(poolNamespaceNodeGetPoolElementNodes)(&poolNamespaceManagement->Namespace));
}


/* ###### Get number of pool elements of given pool ###################### */
inline size_t ST_CLASS(poolNamespaceManagementGetPoolElementsOfPool)(
                 struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                 const struct PoolHandle*                  poolHandle)
{
   return(ST_CLASS(poolNamespaceNodeGetPoolElementNodesOfPool)(
             &poolNamespaceManagement->Namespace,
             poolHandle));
}

unsigned int ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                const ENRPIdentifierType                  homeNSIdentifier,
                const PoolElementIdentifierType           poolElementIdentifier,
                const unsigned int                        registrationLife,
                const struct PoolPolicySettings*          poolPolicySettings,
                const struct TransportAddressBlock*       transportAddressBlock,
                const unsigned long long                  currentTimeStamp,
                struct ST_CLASS(PoolElementNode)**        poolElementNode);


/* ###### Registration ################################################### */
inline unsigned int ST_CLASS(poolNamespaceManagementRegisterPoolElementByPtr)(
                       struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                       const struct PoolHandle*                  poolHandle,
                       const struct ST_CLASS(PoolElementNode)*   originalPoolElementNode,
                       const unsigned long long                  currentTimeStamp,
                       struct ST_CLASS(PoolElementNode)**        poolElementNode)
{
   return(ST_CLASS(poolNamespaceManagementRegisterPoolElement)(
             poolNamespaceManagement,
             poolHandle,
             originalPoolElementNode->HomeNSIdentifier,
             originalPoolElementNode->Identifier,
             originalPoolElementNode->RegistrationLife,
             &originalPoolElementNode->PolicySettings,
             originalPoolElementNode->AddressBlock,
             currentTimeStamp,
             poolElementNode));
}


/* ###### Get next timer time stamp ###################################### */
inline unsigned long long ST_CLASS(poolNamespaceManagementGetNextTimerTimeStamp)(
                             struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement)
{
   const struct ST_CLASS(PoolElementNode)* nextTimer =
      ST_CLASS(poolNamespaceNodeGetFirstPoolElementTimerNode)(
         &poolNamespaceManagement->Namespace);
   if(nextTimer != NULL) {
      return(nextTimer->TimerTimeStamp);
   }
   return(~0);
}


void ST_CLASS(poolNamespaceManagementRestartPoolElementExpiryTimer)(
        struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
        struct ST_CLASS(PoolElementNode)*         poolElementNode,
        const unsigned long long                  expiryTimeout);

size_t ST_CLASS(poolNamespaceManagementPurgeExpiredPoolElements)(
          struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
          const unsigned long long                  currentTimeStamp);


inline struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNamespaceManagementFindPoolElement)(
                                            struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                                            const struct PoolHandle*                  poolHandle,
                                            const PoolElementIdentifierType           poolElementIdentifier)
{
   return(ST_CLASS(poolNamespaceNodeFindPoolElementNode)(
             &poolNamespaceManagement->Namespace,
             poolHandle,
             poolElementIdentifier));
}


unsigned int ST_CLASS(poolNamespaceManagementDeregisterPoolElementByPtr)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                struct ST_CLASS(PoolElementNode)*         poolElementNode);

unsigned int ST_CLASS(poolNamespaceManagementDeregisterPoolElement)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                const PoolElementIdentifierType           poolElementIdentifier);

unsigned int ST_CLASS(poolNamespaceManagementNameResolution)(
                struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
                const struct PoolHandle*                  poolHandle,
                struct ST_CLASS(PoolElementNode)**        poolElementNodeArray,
                size_t*                                   poolElementNodes,
                const size_t                              maxNameResolutionItems,
                const size_t                              maxIncrement);


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


int ST_CLASS(poolNamespaceManagementGetNameTable)(
       struct ST_CLASS(PoolNamespaceManagement)* poolNamespaceManagement,
       struct ST_CLASS(NameTableExtract)*        nameTableExtract,
       const unsigned int                        flags);


#ifdef __cplusplus
}
#endif
