#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "netutilities.h"
#include "registrar.h"


ST_CLASS(PoolUserList)  PoolUsers;
ST_CLASS(PoolUserNode)* NextPoolUserNode = NULL;


double MaxHandleResolutionRate    = 5.0;
double MaxEndpointUnreachableRate = 2.5;


#define TSHT_BUCKETS 64
#define TSHT_ENTRIES 16


/* ###### Check PU's permission for given operation using thresholds ##### */
bool poolUserHasPermissionFor(const int                       fd,
                              const sctp_assoc_t              assocID,
                              const unsigned int              action,
                              const PoolHandle*               poolHandle,
                              const PoolElementIdentifierType peIdentifier)
{
   ST_CLASS(PoolUserNode)* poolUserNode;
   double                  rate;
   double                  threshold;
   unsigned long long      now;

   if(NextPoolUserNode == NULL) {
      NextPoolUserNode = (ST_CLASS(PoolUserNode)*)malloc(sizeof(ST_CLASS(PoolUserNode)));
      if(NextPoolUserNode == NULL) {
         /* Giving permission here seems to be useful in case of trying to
            avoid DoS when - for some reason - the PR's memory is full. */
         return(true);
      }

/*
      NextPoolUserNode->HandleResolutionHash    = timeStampHashTableNew(TSHT_BUCKETS, TSHT_ENTRIES);
      NextPoolUserNode->EndpointUnreachableHash = timeStampHashTableNew(TSHT_BUCKETS, TSHT_ENTRIES);
      if( (NextPoolUserNode->HandleResolutionHash == NULL) ||
          (NextPoolUserNode->EndpointUnreachableHash == NULL) ) {
         if(NextPoolUserNode->HandleResolutionHash) free(NextPoolUserNode->HandleResolutionHash);
         if(NextPoolUserNode->EndpointUnreachableHash) free(NextPoolUserNode->EndpointUnreachableHash);
         free(NextPoolUserNode);
         NextPoolUserNode = NULL;
         return(false);
      }
*/
   }

   ST_CLASS(poolUserNodeNew)(NextPoolUserNode, fd, assocID);
   poolUserNode = ST_CLASS(poolUserListAddOrUpdatePoolUserNode)(&PoolUsers, &NextPoolUserNode);
   CHECK(poolUserNode != NULL);

   now  = getMicroTime();
   rate = threshold = -1.0;
   switch(action) {
      case AHT_HANDLE_RESOLUTION:
         rate = ST_CLASS(poolUserNodeNoteHandleResolution)(poolUserNode, poolHandle, now, TSHT_BUCKETS, TSHT_ENTRIES);
         threshold = MaxHandleResolutionRate;
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         rate = ST_CLASS(poolUserNodeNoteEndpointUnreachable)(poolUserNode, poolHandle, peIdentifier, now, TSHT_BUCKETS, TSHT_ENTRIES);
         threshold = MaxEndpointUnreachableRate;
       break;
      default:
         CHECK(false);
       break;
   }

   printf("rate=%1.6f   thres=%1.6f\n", rate, threshold);
   if(rate > threshold) {
      puts("   EXCEED!");
      return(false);
   }
   return(true);
}


int main(int argc, char** argv)
{
   ST_CLASS(poolUserListNew)(&PoolUsers);

   PoolHandle ph1;
   PoolHandle ph2;
   poolHandleNew(&ph1, (const unsigned char*)"TES", 3);
   poolHandleNew(&ph2, (const unsigned char*)"Delta", 5);

   for(int i=0;i<20;i++) {
      poolUserHasPermissionFor(1, 1, AHT_HANDLE_RESOLUTION, (const PoolHandle*)&ph1, 0);
      poolUserHasPermissionFor(1, 1, AHT_ENDPOINT_UNREACHABLE, (const PoolHandle*)&ph1, 0xabcd1234);
      usleep(1000000);
   }

   ST_CLASS(poolUserListDelete)(&PoolUsers);
}
