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

/* ###### Print ########################################################## */
void ST_CLASS(poolIndexStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolNode)* poolNode = (struct ST_CLASS(PoolNode)*)nodePtr;
   fprintf(fd, "\"");
   poolHandlePrint(&poolNode->Handle, fd);
   fprintf(fd, "\"");
}


/* ###### Comparison ##################################################### */
int ST_CLASS(poolIndexStorageNodeComparison)(const void* nodePtr1, const void* nodePtr2)
{
   const struct ST_CLASS(PoolNode)* poolNode1 = (struct ST_CLASS(PoolNode)*)nodePtr1;
   const struct ST_CLASS(PoolNode)* poolNode2 = (struct ST_CLASS(PoolNode)*)nodePtr2;
   return(poolHandleComparison(&poolNode1->Handle, &poolNode2->Handle));
}


/* ###### Initialize ##################################################### */
void ST_CLASS(poolNodeNew)(struct ST_CLASS(PoolNode)*         poolNode,
                           const struct PoolHandle*           poolHandle,
                           const struct ST_CLASS(PoolPolicy)* poolPolicy,
                           const int                          protocol,
                           const int                          flags)
{
   STN_METHOD(New)(&poolNode->PoolIndexStorageNode);
   poolHandleNew(&poolNode->Handle,
                 poolHandle->Handle,
                 poolHandle->Size);
   poolNode->Policy                 = poolPolicy;
   poolNode->Protocol               = protocol;
   poolNode->Flags                  = flags;
   poolNode->GlobalSeqNumber        = SeqNumberStart;
   poolNode->UserData               = NULL;
   poolNode->OwnerPoolHandlespaceNode = NULL;
   ST_METHOD(New)(&poolNode->PoolElementSelectionStorage, ST_CLASS(poolElementSelectionStorageNodePrint), ST_CLASS(poolElementSelectionStorageNodeComparison));
   ST_METHOD(New)(&poolNode->PoolElementIndexStorage, ST_CLASS(poolElementIndexStorageNodePrint), ST_CLASS(poolElementIndexStorageNodeComparison));
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolNodeDelete)(struct ST_CLASS(PoolNode)* poolNode)
{
   CHECK(!STN_METHOD(IsLinked)(&poolNode->PoolIndexStorageNode));
   CHECK(ST_METHOD(IsEmpty)(&poolNode->PoolElementSelectionStorage));
   poolHandleDelete(&poolNode->Handle);
   ST_METHOD(Delete)(&poolNode->PoolElementSelectionStorage);
   ST_METHOD(Delete)(&poolNode->PoolElementIndexStorage);
   poolNode->Protocol = 0;
   poolNode->UserData = NULL;
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolNodeGetDescription)(const struct ST_CLASS(PoolNode)* poolNode,
                                      char*                            buffer,
                                      const size_t                     bufferSize)
{
   char        poolHandleDescription[256];
   char        poolDescription[128];
   const char* protocol = "(unknown)";

   switch(poolNode->Protocol) {
      case IPPROTO_SCTP:
         protocol = "SCTP";
       break;
      case IPPROTO_TCP:
         protocol = "TCP";
       break;
      case IPPROTO_UDP:
         protocol = "UDP";
       break;
   }

   poolHandleGetDescription(&poolNode->Handle,
                            poolHandleDescription, sizeof(poolHandleDescription));

   safestrcpy(buffer, "Pool \"", bufferSize);
   safestrcat(buffer, poolHandleDescription, bufferSize);
   snprintf((char*)&poolDescription, sizeof(poolDescription),
            "\", Policy \"%s\", Protocol %s, CtrlCh=%s: (%u nodes)",
            poolNode->Policy->Name,
            protocol,
            ((poolNode->Flags & PNF_CONTROLCHANNEL) ? "yes" : "no"),
            (unsigned int)ST_CLASS(poolNodeGetPoolElementNodes)(poolNode));
   safestrcat(buffer, poolDescription, bufferSize);
}


/* ###### Print by Index and Selection ################################### */
void ST_CLASS(poolNodePrint)(const struct ST_CLASS(PoolNode)* poolNode,
                             FILE*                            fd,
                             const unsigned int               fields)
{
   const struct ST_CLASS(PoolElementNode)* poolElementNode;
   char                                    poolNodeDescription[512];
   size_t                                  i;

   ST_CLASS(poolNodeGetDescription)(poolNode,
                                    (char*)&poolNodeDescription,
                                    sizeof(poolNodeDescription));
   fputs(poolNodeDescription, fd);
   fputs("\n", fd);

   if(fields & PNPO_INDEX) {
      fputs(" +-- Index:\n", fd);
      i               = 1;
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)((struct ST_CLASS(PoolNode)*)poolNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - idx:#%04u: ", (unsigned int)i++);
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, fields);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)((struct ST_CLASS(PoolNode)*)poolNode, (struct ST_CLASS(PoolElementNode)*)poolElementNode);
      }
   }
   if(fields & PNPO_SELECTION) {
      fputs(" +-- Selection:\n", fd);
      i               = 1;
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)((struct ST_CLASS(PoolNode)*)poolNode);
      while(poolElementNode != NULL) {
         fprintf(fd, "   - sel:#%04u: ", (unsigned int)i++);
         ST_CLASS(poolElementNodePrint)(poolElementNode, fd, fields);
         fputs("\n", fd);
         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)((struct ST_CLASS(PoolNode)*)poolNode, (struct ST_CLASS(PoolElementNode)*)poolElementNode);
      }
   }
}


/* ###### Clear ########################################################## */
void ST_CLASS(poolNodeClear)(struct ST_CLASS(PoolNode)* poolNode,
                             void                       (*poolElementNodeDisposer)(void* poolElementNode, void* userData),
                             void*                      userData)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);
   while(poolElementNode != NULL) {
      if(poolNode->OwnerPoolHandlespaceNode) {
         ST_CLASS(poolHandlespaceNodeRemovePoolElementNode)(poolNode->OwnerPoolHandlespaceNode, poolElementNode);
      }
      else {
         ST_CLASS(poolNodeRemovePoolElementNode)(poolNode, poolElementNode);
      }
      ST_CLASS(poolElementNodeDelete)(poolElementNode);
      poolElementNodeDisposer(poolElementNode, userData);
      poolElementNode = ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);
   }
}


/* ###### Check, if node may be inserted into pool ####################### */
unsigned int ST_CLASS(poolNodeCheckPoolElementNodeCompatibility)(
                struct ST_CLASS(PoolNode)*          poolNode,
                struct ST_CLASS(PoolElementNode)*   poolElementNode)
{
   if(poolElementNode->Identifier == 0) {
      return(RSPERR_INVALID_ID);
   }

   if(poolNode->Protocol != poolElementNode->UserTransport->Protocol) {
      return(RSPERR_WRONG_PROTOCOL);
   }

   if(poolElementNode->RegistratorTransport) {
      if(poolElementNode->RegistratorTransport->Protocol != IPPROTO_SCTP) {
         return(RSPERR_INVALID_REGISTRATOR);
      }
      if(poolElementNode->RegistratorTransport->Flags & TABF_CONTROLCHANNEL) {
         return(RSPERR_INVALID_REGISTRATOR);
      }
      if((poolElementNode->RegistratorTransport->Addresses < 1) ||
         (poolElementNode->RegistratorTransport->Addresses > MAX_PE_TRANSPORTADDRESSES)) {
         return(RSPERR_INVALID_REGISTRATOR);
      }
      if(poolElementNode->RegistratorTransport->Port == 0) {
         return(RSPERR_INVALID_REGISTRATOR);
      }
   }

   if((poolElementNode->UserTransport->Addresses < 1) ||
      (poolElementNode->UserTransport->Addresses > MAX_PE_TRANSPORTADDRESSES)) {
      return(RSPERR_INVALID_ADDRESSES);
   }
   if(poolElementNode->UserTransport->Port == 0) {
      return(RSPERR_INVALID_ADDRESSES);
   }

   if(poolNode->Flags & PNF_CONTROLCHANNEL) {
      if(!(poolElementNode->UserTransport->Flags & TABF_CONTROLCHANNEL)) {
         return(RSPERR_WRONG_CONTROLCHANNEL_HANDLING);
      }
   }
   else {
      if(poolElementNode->UserTransport->Flags & TABF_CONTROLCHANNEL) {
         return(RSPERR_WRONG_CONTROLCHANNEL_HANDLING);
      }
   }

   if(!poolPolicySettingsIsValid(&poolElementNode->PolicySettings)) {
      return(RSPERR_INVALID_POOL_POLICY);
   }

   if(poolPolicySettingsAdapt(&poolElementNode->PolicySettings,
                              poolNode->Policy->Type) == 0) {
      return(RSPERR_INCOMPATIBLE_POOL_POLICY);
   }

   return(RSPERR_OKAY);
}


/* ###### Get number of pool elements #################################### */
size_t ST_CLASS(poolNodeGetPoolElementNodes)(
          const struct ST_CLASS(PoolNode)* poolNode)
{
   return(ST_METHOD(GetElements)(&poolNode->PoolElementIndexStorage));
}


/* ###### Get first PoolElementNode from Index ########################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromIndex)(
                                     struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNode->PoolElementIndexStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PoolElementNode from Index ############################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromIndex)(
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


/* ###### Get first PoolElementNode from Selection ####################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetFirst)(&poolNode->PoolElementSelectionStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get last PoolElementNode from Selection ######################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetLastPoolElementNodeFromSelection)(
                                     struct ST_CLASS(PoolNode)* poolNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(GetLast)(&poolNode->PoolElementSelectionStorage);
   if(node) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementSelectionStorageNode)(node));
   }
   return(NULL);
};


/* ###### Get next PoolElementNode from Selection ######################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(
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
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeGetPrevPoolElementNodeFromSelection)(
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
void ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(
        struct ST_CLASS(PoolNode)*        poolNode,
        struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* node = ST_METHOD(Remove)(&poolNode->PoolElementSelectionStorage,
                                                  &poolElementNode->PoolElementSelectionStorageNode);
   CHECK(node == &poolElementNode->PoolElementSelectionStorageNode);
}


/* ###### Link PoolElementNode into Selection ############################ */
void ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(
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


/* ###### Add PoolElementNode ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeAddPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode,
                                     unsigned int*                     errorCode)
{
   struct STN_CLASSNAME* result;

   *errorCode = ST_CLASS(poolNodeCheckPoolElementNodeCompatibility)(poolNode, poolElementNode);
   if(*errorCode != RSPERR_OKAY) {
      return(NULL);
   }

   result = ST_METHOD(Insert)(&poolNode->PoolElementIndexStorage,
                              &poolElementNode->PoolElementIndexStorageNode);
   if(result == &poolElementNode->PoolElementIndexStorageNode) {
      if((PoolElementSeqNumberType)(poolNode->GlobalSeqNumber + 1) <
         poolNode->GlobalSeqNumber) {
         ST_CLASS(poolNodeResequence)(poolNode);
      }
      poolElementNode->Flags |= PENF_UPDATED;
      poolElementNode->SeqNumber        = poolNode->GlobalSeqNumber++;
      poolElementNode->VirtualCounter   = 0;
      poolElementNode->RoundCounter     = 0;
      poolElementNode->SelectionCounter = 0;
      poolElementNode->Degradation      = 0;
      poolElementNode->OwnerPoolNode    = poolNode;
      if(poolNode->Policy->InitializePoolElementNodeFunction) {
         poolNode->Policy->InitializePoolElementNodeFunction(poolElementNode);
      }
      ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(poolNode, poolElementNode);
      *errorCode = RSPERR_OKAY;
      return(poolElementNode);
   }
   *errorCode = RSPERR_DUPLICATE_ID;
   return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(result));
}


/* ###### Update PoolElementNode ######################################### */
void ST_CLASS(poolNodeUpdatePoolElementNode)(
        struct ST_CLASS(PoolNode)*              poolNode,
        struct ST_CLASS(PoolElementNode)*       poolElementNode,
        const struct ST_CLASS(PoolElementNode)* source,
        unsigned int*                           errorCode)
{
   *errorCode = ST_CLASS(poolNodeCheckPoolElementNodeCompatibility)(poolNode, poolElementNode);
   if(*errorCode == RSPERR_OKAY) {
      if(ST_CLASS(poolElementNodeUpdate)(poolElementNode, source)) {
         /*
            Policy information has changed. Now, the node has to be re-inserted (Selection only).
            Currently, the node's position may be incorrect now!
         */
         ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(poolNode, poolElementNode);
         ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(poolNode, poolElementNode);
      }
   }
}


/* ###### Find PoolElementNode ########################################### */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeFindPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*      poolNode,
                                     const PoolElementIdentifierType identifier)
{
   struct ST_CLASS(PoolElementNode) cmpElement;
   struct STN_CLASSNAME*            result;

   cmpElement.Identifier = identifier;
   result = ST_METHOD(Find)(&poolNode->PoolElementIndexStorage,
                            &cmpElement.PoolElementIndexStorageNode);
   if(result) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(result));
   }
   return(NULL);
}


/* ###### Find nearest next PoolElementNode ############################## */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeFindNearestNextPoolElementNode)(
                                     struct ST_CLASS(PoolNode)*      poolNode,
                                     const PoolElementIdentifierType identifier)
{
   struct ST_CLASS(PoolElementNode) cmpElement;
   struct STN_CLASSNAME*            result;

   cmpElement.Identifier = identifier;
   result = ST_METHOD(GetNearestNext)(&poolNode->PoolElementIndexStorage,
                                      &cmpElement.PoolElementIndexStorageNode);
   if(result) {
      return(ST_CLASS(getPoolElementNodeFromPoolElementIndexStorageNode)(result));
   }
   return(NULL);
}


/* ###### Remove PoolElementNode ############################################ */
struct ST_CLASS(PoolElementNode)* ST_CLASS(poolNodeRemovePoolElementNode)(
                                     struct ST_CLASS(PoolNode)*        poolNode,
                                     struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   struct STN_CLASSNAME* result;

   result = ST_METHOD(Remove)(&poolNode->PoolElementIndexStorage,
                              &poolElementNode->PoolElementIndexStorageNode);
   CHECK(result == &poolElementNode->PoolElementIndexStorageNode);
   result = ST_METHOD(Remove)(&poolNode->PoolElementSelectionStorage,
                              &poolElementNode->PoolElementSelectionStorageNode);
   CHECK(result != NULL);
   poolElementNode->OwnerPoolNode = NULL;
   return(poolElementNode);
}


/* ###### Resequence: Avoid sequence number wrap ######################### */
void ST_CLASS(poolNodeResequence)(struct ST_CLASS(PoolNode)* poolNode)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode =
      ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);

   /*
      Reseqencing does not affect the sorting order of this storage:
      Let a < b:
      => a is stored before b
      => a gets new sequence number before b
      => new sequence number of a < new sequence number of b.
   */

   poolNode->GlobalSeqNumber = 0;
   while(poolElementNode != NULL) {
      poolElementNode->SeqNumber = poolNode->GlobalSeqNumber++;
      poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(poolNode, poolElementNode);
   }
}
