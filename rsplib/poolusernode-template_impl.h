/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2011 by Thomas Dreibholz
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
void ST_CLASS(poolUserStorageNodePrint)(const void* nodePtr, FILE* fd)
{
   const struct ST_CLASS(PoolUserNode)* poolUserNode = (struct ST_CLASS(PoolUserNode)*)nodePtr;
   ST_CLASS(poolUserNodePrint)(poolUserNode, fd, 0);
}


/* ###### Comparison ##################################################### */
int ST_CLASS(poolUserStorageNodeComparison)(const void* nodePtr1,
                                            const void* nodePtr2)
{
   const struct ST_CLASS(PoolUserNode)* node1 = (struct ST_CLASS(PoolUserNode)*)nodePtr1;
   const struct ST_CLASS(PoolUserNode)* node2 = (struct ST_CLASS(PoolUserNode)*)nodePtr2;

   /* Compare assoc IDs */
   if(node1->ConnectionAssocID < node2->ConnectionAssocID) {
      return(-1);
   }
   else if(node1->ConnectionAssocID > node2->ConnectionAssocID) {
      return(1);
   }

   /* Compare socket descriptors */
   if(node1->ConnectionSocketDescriptor < node2->ConnectionSocketDescriptor) {
      return(-1);
   }
   else if(node1->ConnectionSocketDescriptor > node2->ConnectionSocketDescriptor) {
      return(1);
   }

   return(0);
}


/* ###### Initialize ##################################################### */
void ST_CLASS(poolUserNodeNew)(struct ST_CLASS(PoolUserNode)* poolUserNode,
                               const int                      connectionSocketDescriptor,
                               const sctp_assoc_t             connectionAssocID)
{
   STN_METHOD(New)(&poolUserNode->PoolUserListStorageNode);

   poolUserNode->ConnectionSocketDescriptor = connectionSocketDescriptor;
   poolUserNode->ConnectionAssocID          = connectionAssocID;
   poolUserNode->HandleResolutionHash       = NULL;
   poolUserNode->EndpointUnreachableHash    = NULL;
}


/* ###### Invalidate ##################################################### */
void ST_CLASS(poolUserNodeDelete)(struct ST_CLASS(PoolUserNode)* poolUserNode)
{
   CHECK(!STN_METHOD(IsLinked)(&poolUserNode->PoolUserListStorageNode));

   poolUserNode->ConnectionSocketDescriptor = -1;
   poolUserNode->ConnectionAssocID          = 0;

   if(poolUserNode->HandleResolutionHash) {
      timeStampHashTableDelete(poolUserNode->HandleResolutionHash);
      poolUserNode->HandleResolutionHash = NULL;
   }
   if(poolUserNode->EndpointUnreachableHash) {
      timeStampHashTableDelete(poolUserNode->EndpointUnreachableHash);
      poolUserNode->EndpointUnreachableHash = NULL;
   }
}


/* ###### Get PoolUserNode from given Index Node ######################### */
struct ST_CLASS(PoolUserNode)* ST_CLASS(getPoolUserNodeFromPoolUserListStorageNode)(void* node)
{
   const struct ST_CLASS(PoolUserNode)* dummy = (struct ST_CLASS(PoolUserNode)*)node;
   long n = (long)node - ((long)&dummy->PoolUserListStorageNode - (long)dummy);
   return((struct ST_CLASS(PoolUserNode)*)n);
}


/* ###### Get textual description ######################################## */
void ST_CLASS(poolUserNodeGetDescription)(
        const struct ST_CLASS(PoolUserNode)* poolUserNode,
        char*                                buffer,
        const size_t                         bufferSize,
        const unsigned int                   fields)
{
   snprintf(buffer, bufferSize,
            "PU %d/%u",
            poolUserNode->ConnectionSocketDescriptor,
            (unsigned int)poolUserNode->ConnectionAssocID);
}


/* ###### Print ########################################################## */
void ST_CLASS(poolUserNodePrint)(const struct ST_CLASS(PoolUserNode)* poolUserNode,
                                 FILE*                                fd,
                                 const unsigned int                   fields)
{
   char poolUserNodeDescription[128];
   ST_CLASS(poolUserNodeGetDescription)(poolUserNode,
                                        (char*)&poolUserNodeDescription,
                                        sizeof(poolUserNodeDescription),
                                        fields);
   fputs(poolUserNodeDescription, fd);
}


/* ###### Note a handle resolution into the hash table ################### */
double ST_CLASS(poolUserNodeNoteHandleResolution)(struct ST_CLASS(PoolUserNode)* poolUserNode,
                                                  const struct PoolHandle*       poolHandle,
                                                  const unsigned long long       now,
                                                  const size_t                   buckets,
                                                  const size_t                   maxEntries)
{
   unsigned int hash;

   if(poolUserNode->HandleResolutionHash == NULL) {
      poolUserNode->HandleResolutionHash = timeStampHashTableNew(buckets, maxEntries);
      if(poolUserNode->HandleResolutionHash == NULL) {
         return(+9e9);
      }
   }

   hash = computePHPEHash(poolHandle, 0);
   timeStampHashTableAddTimeStamp(poolUserNode->HandleResolutionHash, hash, now);
   /* timeStampHashTablePrint(poolUserNode->HandleResolutionHash, stdout); */
   return(timeStampHashTableGetRate(poolUserNode->HandleResolutionHash, hash));
}


/* ###### Note a handle resolution into the hash table ################### */
double ST_CLASS(poolUserNodeNoteEndpointUnreachable)(struct ST_CLASS(PoolUserNode)*  poolUserNode,
                                                     const struct PoolHandle*        poolHandle,
                                                     const PoolElementIdentifierType peIdentifier,
                                                     const unsigned long long        now,
                                                     const size_t                    buckets,
                                                     const size_t                    maxEntries)
{
   unsigned int hash;

   if(poolUserNode->EndpointUnreachableHash == NULL) {
      poolUserNode->EndpointUnreachableHash = timeStampHashTableNew(buckets, maxEntries);
      if(poolUserNode->EndpointUnreachableHash == NULL) {
         return(+9e9);
      }
   }

   hash = computePHPEHash(poolHandle, peIdentifier);
   timeStampHashTableAddTimeStamp(poolUserNode->EndpointUnreachableHash, hash, now);
   /* timeStampHashTablePrint(poolUserNode->EndpointUnreachableHash, stdout); */
   return(timeStampHashTableGetRate(poolUserNode->EndpointUnreachableHash, hash));
}
