/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id$
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

#include "takeoverprocess.h"


/* ###### Get takeover process from index storage node ################### */
struct TakeoverProcess* getTakeoverProcessFromIndexStorageNode(struct STN_CLASSNAME* node)
{
   const struct TakeoverProcess* dummy = (struct TakeoverProcess*)node;
   long n = (long)node - ((long)&dummy->IndexStorageNode - (long)dummy);
   return((struct TakeoverProcess*)n);
}


/* ###### Get takeover process from timer storage node ################### */
struct TakeoverProcess* getTakeoverProcessFromTimerStorageNode(struct STN_CLASSNAME* node)
{
   const struct TakeoverProcess* dummy = (struct TakeoverProcess*)node;
   long n = (long)node - ((long)&dummy->TimerStorageNode - (long)dummy);
   return((struct TakeoverProcess*)n);
}


/* ###### Print ########################################################## */
void takeoverProcessIndexPrint(const void* takeoverProcessPtr,
                               FILE*       fd)
{
   size_t i;

   struct TakeoverProcess* takeoverProcess = (struct TakeoverProcess*)takeoverProcessPtr;
   fprintf(fd, "   - Takeover for $%08x (expiry in %lldus)\n",
           takeoverProcess->TargetID,
           (long long)takeoverProcess->ExpiryTimeStamp - (long long)getMicroTime());
   for(i = 0;i < takeoverProcess->OutstandingAcknowledgements;i++) {
      fprintf(fd, "      * Ack required by $%08x\n", takeoverProcess->PeerIDArray[i]);
   }
}


/* ###### Comparison ##################################################### */
int takeoverProcessIndexComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2)
{
   const struct TakeoverProcess* takeoverProcess1 = getTakeoverProcessFromIndexStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr1);
   const struct TakeoverProcess* takeoverProcess2 = getTakeoverProcessFromIndexStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr2);

   if(takeoverProcess1->TargetID < takeoverProcess2->TargetID) {
      return(-1);
   }
   else if(takeoverProcess1->TargetID > takeoverProcess2->TargetID) {
      return(1);
   }
   return(0);
}


/* ###### Comparison ##################################################### */
int takeoverProcessTimerComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2)
{
   const struct TakeoverProcess* takeoverProcess1 = getTakeoverProcessFromTimerStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr1);
   const struct TakeoverProcess* takeoverProcess2 = getTakeoverProcessFromTimerStorageNode((struct STN_CLASSNAME*)takeoverProcessPtr2);

   if(takeoverProcess1->ExpiryTimeStamp < takeoverProcess2->ExpiryTimeStamp) {
      return(-1);
   }
   else if(takeoverProcess1->ExpiryTimeStamp > takeoverProcess2->ExpiryTimeStamp) {
      return(1);
   }
   return(0);
}
