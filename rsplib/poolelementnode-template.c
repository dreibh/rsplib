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

/* ###### Initialize ##################################################### */
void ST_CLASS(poolElementNodeNew)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const PoolElementIdentifierType   identifier,
                                  const ENRPIdentifierType          homeNSIdentifier,
                                  const unsigned int                registrationLife,
                                  const struct PoolPolicySettings*  pps,
                                  struct TransportAddressBlock*     transportAddressBlock)
{
   STN_METHOD(New)(&poolElementNode->PoolElementSelectionStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementIndexStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementTimerStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementPropertyStorageNode);

   poolElementNode->OwnerPoolNode         = NULL;

   poolElementNode->Identifier            = identifier;
   poolElementNode->HomeNSIdentifier      = homeNSIdentifier;
   poolElementNode->RegistrationLife      = registrationLife;
   poolElementNode->PolicySettings        = *pps;

   poolElementNode->SeqNumber             = 0;
   poolElementNode->RoundCounter          = 0;
   poolElementNode->VirtualCounter        = 0;
   poolElementNode->SelectionCounter      = 0;
   poolElementNode->Degradation           = 0;
   poolElementNode->UnreachabilityReports = 0;

   poolElementNode->LastUpdateTimeStamp   = 0;

   poolElementNode->TimerTimeStamp        = 0;
   poolElementNode->TimerCode             = 0;

   poolElementNode->AddressBlock          = transportAddressBlock;
   poolElementNode->UserData              = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolElementNodeDelete)(struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementSelectionStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementIndexStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementPropertyStorageNode));

   poolElementNode->Identifier            = 0;
   poolElementNode->HomeNSIdentifier      = 0;
   poolElementNode->RegistrationLife      = 0;
   poolElementNode->OwnerPoolNode         = NULL;
   poolElementNode->SeqNumber             = 0;
   poolElementNode->RoundCounter          = 0;
   poolElementNode->VirtualCounter        = 0;
   poolElementNode->SelectionCounter      = 0;
   poolElementNode->Degradation           = 0;
   poolElementNode->UnreachabilityReports = 0;
   poolElementNode->LastUpdateTimeStamp   = 0;
   poolElementNode->TimerTimeStamp        = 0;
   poolElementNode->TimerCode             = 0;
   /* Do not clear AddressBlock and UserData yet, it must be used for
      user-specific dispose function! */

   STN_METHOD(Delete)(&poolElementNode->PoolElementPropertyStorageNode);
   STN_METHOD(Delete)(&poolElementNode->PoolElementTimerStorageNode);
   STN_METHOD(Delete)(&poolElementNode->PoolElementIndexStorageNode);
   STN_METHOD(Delete)(&poolElementNode->PoolElementSelectionStorageNode);
   poolPolicySettingsDelete(&poolElementNode->PolicySettings);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolElementNodeGetDescription)(
        struct ST_CLASS(PoolElementNode)* poolElementNode,
        char*                             buffer,
        const size_t                      bufferSize,
        const unsigned int                fields)
{
   char tmp[512];
   char poolPolicySettingsDescription[512];
   char transportAddressDescription[1024];

   snprintf(buffer, bufferSize, "$%08x", poolElementNode->Identifier);
   if(fields & PENPO_HOME_NS) {
      snprintf((char*)&tmp, sizeof(tmp), " home=$%08x",
               poolElementNode->HomeNSIdentifier);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_REGLIFE) {
      snprintf((char*)&tmp, sizeof(tmp), " life=%ums",
               poolElementNode->RegistrationLife);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_UR_REPORTS) {
      snprintf((char*)&tmp, sizeof(tmp), " ur=%u",
               poolElementNode->UnreachabilityReports);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_POLICYINFO) {
      poolPolicySettingsGetDescription(&poolElementNode->PolicySettings,
                                       (char*)&poolPolicySettingsDescription,
                                       sizeof(poolPolicySettingsDescription));
      snprintf((char*)&tmp, sizeof(tmp), "   [%s]",
               poolPolicySettingsDescription);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_POLICYSTATE) {
      snprintf((char*)&tmp, sizeof(tmp), "   seq=%Lu val=%Lu rd=%u vrt=%u deg=$%x {sel=%Lu s/w=%1.1f}",
               (unsigned long long)poolElementNode->SeqNumber,
               poolElementNode->PoolElementSelectionStorageNode.Value,
               poolElementNode->RoundCounter,
               poolElementNode->VirtualCounter,
               poolElementNode->Degradation,
               poolElementNode->SelectionCounter,
               (double)poolElementNode->SelectionCounter / (double)poolElementNode->PolicySettings.Weight);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_LASTUPDATE) {
      snprintf((char*)&tmp, sizeof(tmp), "   upd=%Lu",
               poolElementNode->LastUpdateTimeStamp);
      safestrcat(buffer, tmp, bufferSize);
   }
   if((fields & PENPO_USERTRANSPORT) &&
      (poolElementNode->AddressBlock->Addresses > 0)) {
      transportAddressBlockGetDescription(poolElementNode->AddressBlock,
                                          (char*)&transportAddressDescription,
                                          sizeof(transportAddressDescription));
      safestrcat(buffer, "\n     addrs: ", bufferSize);
      safestrcat(buffer, transportAddressDescription, bufferSize);
   }
}


/* ###### Print ########################################################## */
void ST_CLASS(poolElementNodePrint)(
        struct ST_CLASS(PoolElementNode)* poolElementNode,
        FILE*                             fd,
        const unsigned int                fields)
{
   char buffer[4096];
   ST_CLASS(poolElementNodeGetDescription)(poolElementNode,
                                           (char*)&buffer, sizeof(buffer),
                                           fields);
   fputs(buffer, fd);
}


/* ###### Update ######################################################### */
int ST_CLASS(poolElementNodeUpdate)(struct ST_CLASS(PoolElementNode)*       poolElementNode,
                                    const struct ST_CLASS(PoolElementNode)* source)
{
   if(poolPolicySettingsComparison(&poolElementNode->PolicySettings,
                                   &source->PolicySettings) != 0) {
      // ====== Update policy information ====================================
      poolElementNode->PolicySettings = source->PolicySettings;

      // ====== Reduce virtual nodes counter, if necessary ===================
      if(poolElementNode->VirtualCounter > poolElementNode->PolicySettings.Weight) {
         poolElementNode->VirtualCounter = poolElementNode->PolicySettings.Weight;
      }
      return(1);
   }
   return(0);
}


/* ###### Selection Print ################################################ */
void ST_CLASS(poolElementSelectionStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x", node->Identifier);
}


/* ###### Selection Comparison ########################################### */
int ST_CLASS(poolElementSelectionStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)((void*)nodePtr2);
   const struct ST_CLASS(PoolNode)* poolNode = node1->OwnerPoolNode;
   return(poolNode->Policy->ComparisonFunction(node1, node2));
}


/* ###### Index Print #################################################### */
void ST_CLASS(poolElementIndexStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x", node->Identifier);
}


/* ###### Index Comparison ############################################### */
int ST_CLASS(poolElementIndexStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)((void*)nodePtr2);
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }
   return(0);
}


/* ###### Timer Print #################################################### */
void ST_CLASS(poolElementTimerStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromTimerStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x", node->Identifier);
}


/* ###### Timer Comparison ############################################### */
int ST_CLASS(poolElementTimerStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromTimerStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromTimerStorageNode)((void*)nodePtr2);

   if(node1->TimerTimeStamp < node2->TimerTimeStamp) {
      return(-1);
   }
   else if(node1->TimerTimeStamp > node2->TimerTimeStamp) {
      return(1);
   }

   const int cmpResult = ST_CLASS(poolIndexStorageNodeComparison)(node1->OwnerPoolNode, node2->OwnerPoolNode);
   if(cmpResult != 0) {
      return(cmpResult);
   }
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }
   return(0);
}


/* ###### Property Print ################################################# */
void ST_CLASS(poolElementPropertyStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromPropertyStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x", node->Identifier);
}


/* ###### Property Comparison ############################################ */
int ST_CLASS(poolElementPropertyStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromPropertyStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromPropertyStorageNode)((void*)nodePtr2);
   const int cmpResult = ST_CLASS(poolIndexStorageNodeComparison)(node1->OwnerPoolNode, node2->OwnerPoolNode);
   if(cmpResult != 0) {
      return(cmpResult);
   }
   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }
   return(0);
}
