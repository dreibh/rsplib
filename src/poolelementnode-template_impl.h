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

/* ###### Initialize ##################################################### */
void ST_CLASS(poolElementNodeNew)(struct ST_CLASS(PoolElementNode)* poolElementNode,
                                  const PoolElementIdentifierType   identifier,
                                  const RegistrarIdentifierType     homeRegistrarIdentifier,
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

   poolElementNode->OwnerPoolNode              = NULL;

   poolElementNode->Checksum                   = INITIAL_HANDLESPACE_CHECKSUM;
   poolElementNode->Identifier                 = identifier;
   poolElementNode->HomeRegistrarIdentifier    = homeRegistrarIdentifier;
   poolElementNode->RegistrationLife           = registrationLife;
   poolElementNode->PolicySettings             = *pps;
   poolElementNode->Flags                      = 0;

   poolElementNode->SeqNumber                  = 0;
   poolElementNode->RoundCounter               = 0;
   poolElementNode->VirtualCounter             = 0;
   poolElementNode->SelectionCounter           = 0;
   poolElementNode->Degradation                = 0;
   poolElementNode->UnreachabilityReports      = 0;

   poolElementNode->LastUpdateTimeStamp        = 0;

   poolElementNode->TimerTimeStamp             = 0;
   poolElementNode->TimerCode                  = 0;

   poolElementNode->ConnectionSocketDescriptor = connectionSocketDescriptor;
   poolElementNode->ConnectionAssocID          = connectionAssocID;

   poolElementNode->UserTransport              = userTransport;
   poolElementNode->RegistratorTransport       = registratorTransport;

   poolElementNode->UserData                   = 0;
   poolElementNode->LastKeepAliveTransmission  = 0;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolElementNodeDelete)(struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementSelectionStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementIndexStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementTimerStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementOwnershipStorageNode));
   CHECK(!STN_METHOD(IsLinked)(&poolElementNode->PoolElementConnectionStorageNode));

   poolElementNode->Checksum                    = 0;
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
      Identifier and HomeRegistrarIdentifier yet -
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
        const struct ST_CLASS(PoolElementNode)* poolElementNode,
        char*                                   buffer,
        const size_t                            bufferSize,
        const unsigned int                      fields)
{
   char tmp[536];
   char poolPolicySettingsDescription[512];
   char transportAddressDescription[1024];

   snprintf(buffer, bufferSize, "$%08x flags=", poolElementNode->Identifier);
   if(poolElementNode->Flags & PENF_NEW) {
      safestrcat(buffer, "[new]", bufferSize);
   }
   if(poolElementNode->Flags & PENF_UPDATED) {
      safestrcat(buffer, "[updated]", bufferSize);
   }
   if(poolElementNode->Flags & PENF_MARKED) {
      safestrcat(buffer, "[marked]", bufferSize);
   }
   if(fields & (PENPO_CONNECTION|PENPO_CHECKSUM|PENPO_HOME_PR|PENPO_REGLIFE|PENPO_UR_REPORTS|PENPO_LASTUPDATE)) {
      safestrcat(buffer, "\n     ", bufferSize);
   }
   if(fields & PENPO_CONNECTION) {
      snprintf((char*)&tmp, sizeof(tmp), "c=(S%d,A%u) ",
               poolElementNode->ConnectionSocketDescriptor,
               (unsigned int)poolElementNode->ConnectionAssocID);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_CHECKSUM) {
      snprintf((char*)&tmp, sizeof(tmp), "chsum=$%08x ",
               (unsigned int)handlespaceChecksumFinish(poolElementNode->Checksum));
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_HOME_PR) {
      snprintf((char*)&tmp, sizeof(tmp), "home=$%08x ",
               poolElementNode->HomeRegistrarIdentifier);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_REGLIFE) {
      snprintf((char*)&tmp, sizeof(tmp), "life=%ums ",
               poolElementNode->RegistrationLife);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_UR_REPORTS) {
      snprintf((char*)&tmp, sizeof(tmp), "ur=%u ",
               poolElementNode->UnreachabilityReports);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_LASTUPDATE) {
      snprintf((char*)&tmp, sizeof(tmp), "upd=%llu ",
               poolElementNode->LastUpdateTimeStamp);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_POLICYINFO) {
      poolPolicySettingsGetDescription(&poolElementNode->PolicySettings,
                                       (char*)&poolPolicySettingsDescription,
                                       sizeof(poolPolicySettingsDescription));
      snprintf((char*)&tmp, sizeof(tmp), "\n     %s",
               poolPolicySettingsDescription);
      safestrcat(buffer, tmp, bufferSize);
   }
   if(fields & PENPO_POLICYSTATE) {
      snprintf((char*)&tmp, sizeof(tmp), "\n     seq=%llu val=%llu rd=%u vrt=%u deg=$%x {sel=%llu s/w=%1.1f}",
               (unsigned long long)poolElementNode->SeqNumber,
               poolElementNode->PoolElementSelectionStorageNode.Value,
               poolElementNode->RoundCounter,
               poolElementNode->VirtualCounter,
               poolElementNode->Degradation,
               poolElementNode->SelectionCounter,
               (double)poolElementNode->SelectionCounter / (double)poolElementNode->PolicySettings.Weight);
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
        const struct ST_CLASS(PoolElementNode)* poolElementNode,
        FILE*                                   fd,
        const unsigned int                      fields)
{
   char buffer[4096];
   ST_CLASS(poolElementNodeGetDescription)(poolElementNode,
                                           (char*)&buffer, sizeof(buffer),
                                           fields);
   fputs(buffer, fd);
}


/* ###### Compute PE checksum ############################################ */
HandlespaceChecksumAccumulatorType ST_CLASS(poolElementNodeComputeChecksum)(
                                      const struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   HandlespaceChecksumAccumulatorType checksum   = INITIAL_HANDLESPACE_CHECKSUM;
   PoolElementIdentifierType          identifier = htonl(poolElementNode->Identifier);

   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&poolElementNode->OwnerPoolNode->Handle.Handle,
                                         poolElementNode->OwnerPoolNode->Handle.Size);
   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&identifier,
                                         sizeof(identifier));

/*
   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&poolElementNode->HomeRegistrarIdentifier,
                                         sizeof(poolElementNode->HomeRegistrarIdentifier));

   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&poolElementNode->PolicySettings.Weight,
                                         sizeof(poolElementNode->PolicySettings.Weight));
   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&poolElementNode->PolicySettings.Load,
                                         sizeof(poolElementNode->PolicySettings.Load));
   checksum = handlespaceChecksumCompute(checksum,
                                         (const char*)&poolElementNode->PolicySettings.LoadDegradation,
                                         sizeof(poolElementNode->PolicySettings.LoadDegradation));
*/
   return(checksum);
}


/* ###### Update ######################################################### */
int ST_CLASS(poolElementNodeUpdate)(struct ST_CLASS(PoolElementNode)*       poolElementNode,
                                    const struct ST_CLASS(PoolElementNode)* source)
{
   poolElementNode->Flags &= ~PENF_MARKED;
   if( (poolPolicySettingsComparison(&poolElementNode->PolicySettings, &source->PolicySettings) != 0) ||
       (poolElementNode->Degradation != 0) ) {
      /* ====== Update policy information ================================ */
      poolElementNode->PolicySettings = source->PolicySettings;

      /* ====== Reset of degradation ===================================== */
      poolElementNode->Degradation = 0;

      /* ====== Reduce virtual nodes counter, if necessary =============== */
      if(poolElementNode->VirtualCounter > poolElementNode->PolicySettings.Weight) {
         poolElementNode->VirtualCounter = poolElementNode->PolicySettings.Weight;
      }
      poolElementNode->Flags |= PENF_UPDATED;
      return(1);
   }
   poolElementNode->Flags &= ~PENF_UPDATED;
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

   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }

   return(poolHandleComparison(&node1->OwnerPoolNode->Handle, &node2->OwnerPoolNode->Handle));
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

   if(node1->HomeRegistrarIdentifier < node2->HomeRegistrarIdentifier) {
      return(-1);
   }
   else if(node1->HomeRegistrarIdentifier > node2->HomeRegistrarIdentifier) {
      return(1);
   }

   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }

   return(poolHandleComparison(&node1->OwnerPoolNode->Handle, &node2->OwnerPoolNode->Handle));
}


/* ###### Connection Print ################################################# */
void ST_CLASS(poolElementConnectionStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolElementNode)* node = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr);
   fprintf(fd, "#$%08x (S%d,A%u)   ",
           node->Identifier,
           node->ConnectionSocketDescriptor,
           (unsigned int)node->ConnectionAssocID);
}


/* ###### Connection Comparison ############################################ */
int ST_CLASS(poolElementConnectionStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolElementNode)* node1 = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr1);
   const struct ST_CLASS(PoolElementNode)* node2 = ST_CLASS(getPoolElementNodeFromConnectionStorageNode)((void*)nodePtr2);

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

   if(node1->Identifier < node2->Identifier) {
      return(-1);
   }
   else if(node1->Identifier > node2->Identifier) {
      return(1);
   }

   return(poolHandleComparison(&node1->OwnerPoolNode->Handle, &node2->OwnerPoolNode->Handle));
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
