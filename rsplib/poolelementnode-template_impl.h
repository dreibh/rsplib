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
                                  struct TransportAddressBlock*     userTransport,
                                  struct TransportAddressBlock*     registratorTransport,
                                  const int                         connectionSocketDescriptor,
                                  const sctp_assoc_t                connectionAssocID)
{
   STN_METHOD(New)(&poolElementNode->PoolElementSelectionStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementIndexStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementTimerStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementConnectionStorageNode);
   STN_METHOD(New)(&poolElementNode->PoolElementOwnershipStorageNode);

   poolElementNode->OwnerPoolNode               = NULL;

   poolElementNode->Identifier                  = identifier;
   poolElementNode->HomeNSIdentifier            = homeNSIdentifier;
   poolElementNode->RegistrationLife            = registrationLife;
   poolElementNode->PolicySettings              = *pps;

   poolElementNode->SeqNumber                   = 0;
   poolElementNode->RoundCounter                = 0;
   poolElementNode->VirtualCounter              = 0;
   poolElementNode->SelectionCounter            = 0;
   poolElementNode->Degradation                 = 0;
   poolElementNode->UnreachabilityReports       = 0;

   poolElementNode->LastUpdateTimeStamp         = 0;

   poolElementNode->TimerTimeStamp              = 0;
   poolElementNode->TimerCode                   = 0;

   poolElementNode->ConnectionSocketDescriptor  = connectionSocketDescriptor;
   poolElementNode->ConnectionAssocID           = connectionAssocID;

   poolElementNode->UserTransport               = userTransport;
   poolElementNode->RegistratorTransport        = registratorTransport;

   poolElementNode->UserData                    = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolElementNodeDelete)(struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementSelectionStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementIndexStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode));

   poolElementNode->RegistrationLife            = 0;
   poolElementNode->OwnerPoolNode               = NULL;
   poolElementNode->SeqNumber                   = 0;
   poolElementNode->RoundCounter                = 0;
   poolElementNode->VirtualCounter              = 0;
   poolElementNode->SelectionCounter            = 0;
   poolElementNode->Degradation                 = 0;
   poolElementNode->UnreachabilityReports       = 0;
   poolElementNode->LastUpdateTimeStamp         = 0;
   poolElementNode->TimerTimeStamp              = 0;
   poolElementNode->TimerCode                   = 0;
   poolElementNode->ConnectionSocketDescriptor = -1;
   poolElementNode->ConnectionAssocID          = 0;
   /* Do not clear UserTransport, RegistratorTransport, UserData,
      Identifier and HomeNSIdentifier yet -
      they may be necessary for user-specific dispose function! */

   STN_METHOD(Delete)(&poolElementNode->PoolElementConnectionStorageNode);
   STN_METHOD(Delete)(&poolElementNode->PoolElementOwnershipStorageNode);
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
   if(fields & PENPO_CONNECTION) {
      snprintf((char*)&tmp, sizeof(tmp), " c=(S%d,A%u)",
               poolElementNode->ConnectionSocketDescriptor,
               poolElementNode->ConnectionAssocID);
      safestrcat(buffer, tmp, bufferSize);
   }
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
      (poolElementNode->UserTransport->Addresses > 0)) {
      transportAddressBlockGetDescription(poolElementNode->UserTransport,
                                          (char*)&transportAddressDescription,
                                          sizeof(transportAddressDescription));
      safestrcat(buffer, "\n     userT: ", bufferSize);
      safestrcat(buffer, transportAddressDescription, bufferSize);
   }
   if((fields & PENPO_REGISTRATORTRANSPORT) &&
      (poolElementNode->RegistratorTransport != NULL) &&
      (poolElementNode->RegistratorTransport->Addresses > 0)) {
      transportAddressBlockGetDescription(poolElementNode->RegistratorTransport,
                                          (char*)&transportAddressDescription,
                                          sizeof(transportAddressDescription));
      safestrcat(buffer, "\n     regT:  ", bufferSize);
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
      /* ====== Update policy information ================================ */
      poolElementNode->PolicySettings = source->PolicySettings;

      /* ====== Reduce virtual nodes counter, if necessary =============== */
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
   int cmpResult;

   if(node1->TimerTimeStamp < node2->TimerTimeStamp) {
      return(-1);
   }
   else if(node1->TimerTimeStamp > node2->TimerTimeStamp) {
      return(1);
   }

   cmpResult = ST_CLASS(poolIndexStorageNodeComparison)(node1->OwnerPoolNode, node2->OwnerPoolNode);
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


/* ###### Ownership Print ################################################# */
void ST_CLASS(poolElementOwnershipStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x", node->Identifier);
}


/* ###### Ownership Comparison ############################################ */
int ST_CLASS(poolElementOwnershipStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)((void*)nodePtr2);
   int                                     cmpResult;

   if(node1->HomeNSIdentifier < node2->HomeNSIdentifier) {
      return(-1);
   }
   else if(node1->HomeNSIdentifier > node2->HomeNSIdentifier) {
      return(1);
   }
   cmpResult = ST_CLASS(poolIndexStorageNodeComparison)(node1->OwnerPoolNode, node2->OwnerPoolNode);
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


/* ###### Connection Print ################################################# */
void ST_CLASS(poolElementConnectionStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x (S%d,A%u)   ",
           node->Identifier, node->ConnectionSocketDescriptor, node->ConnectionAssocID);
}


/* ###### Connection Comparison ############################################ */
int ST_CLASS(poolElementConnectionStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr2);
   int                                     cmpResult;

   if(node1->ConnectionSocketDescriptor < node2->ConnectionSocketDescriptor) {
      return(-1);
   }
   else if(node1->ConnectionSocketDescriptor > node2->ConnectionSocketDescriptor) {
      return(1);
   }
   if(node1->ConnectionAssocID < node2->ConnectionAssocID) {
      return(-1);
   }
   else if(node1->ConnectionAssocID > node2->ConnectionAssocID) {
      return(1);
   }
   cmpResult = ST_CLASS(poolIndexStorageNodeComparison)(node1->OwnerPoolNode, node2->OwnerPoolNode);
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


/* ###### Get PoolElementNode from given Selection Node ################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementSelectionStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Index Node ###################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementIndexStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Timer Node ###################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromTimerStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementTimerStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Ownership Node ################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromOwnershipStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementOwnershipStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}


/* ###### Get PoolElementNode from given Connection Node ################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(getPoolElementNodeFromConnectionStorageNode)(void* node)
{
   const struct ST_CLASS(PoolElementNode)* dummy = (struct ST_CLASS(PoolElementNode)*)node;
   long n = (long)node - ((long)&dummy->PoolElementConnectionStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolElementNode)*)n);
}
