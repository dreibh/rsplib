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
 * Copyright (C) 2003-2025 by Thomas Dreibholz
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

#include "timestamphashtable.h"

#include <stdlib.h>



/* ###### Constructor #################################################### */
struct TimeStampHashTable* timeStampHashTableNew(const size_t buckets,
                                                 const size_t maxEntries)
{
   struct TimeStampHashTable* timeStampHashTable;
   const size_t               totalSize =
      sizeof(struct TimeStampHashTable) +
      buckets * (sizeof(timeStampHashTable->BucketArray[0]) +
                 sizeof(struct TimeStampBucket) +
                 (maxEntries * sizeof(timeStampHashTable->BucketArray[0]->TimeStamp[0])));

   if((buckets < 1) || (maxEntries < 1)) {
      return(NULL);
   }

   timeStampHashTable = (struct TimeStampHashTable*)malloc(totalSize);
   if(timeStampHashTable) {
      timeStampHashTable->Buckets    = buckets;
      timeStampHashTable->MaxEntries = maxEntries;
      timeStampHashTableClear(timeStampHashTable);
   }
   return(timeStampHashTable);
}


/* ###### Destructor ##################################################### */
void timeStampHashTableDelete(struct TimeStampHashTable* timeStampHashTable)
{
   free(timeStampHashTable);
}


/* ###### Clear all ###################################################### */
void timeStampHashTableClear(struct TimeStampHashTable* timeStampHashTable)
{
   struct TimeStampBucket* bucket;
   size_t                  i;

   bucket = (struct TimeStampBucket*)((long)&timeStampHashTable->BucketArray +
                                       ((long)timeStampHashTable->Buckets * (long)sizeof(timeStampHashTable->BucketArray[0])));
   for(i = 0;i < timeStampHashTable->Buckets;i++) {
      bucket->Entries = 0;
      timeStampHashTable->BucketArray[i] = bucket;
      bucket = (struct TimeStampBucket*)((long)bucket + (long)sizeof(struct TimeStampBucket) + ((long)sizeof(bucket->TimeStamp[0]) * (long)timeStampHashTable->MaxEntries));
   }
}


/* ###### Print ########################################################## */
void timeStampHashTablePrint(struct TimeStampHashTable* timeStampHashTable,
                             FILE*                      fd)
{
   size_t i, j;

   fputs("TimeStampHashTable:\n", fd);
   fprintf(fd, "   - Buckets    = %u\n", (unsigned int)timeStampHashTable->Buckets);
   fprintf(fd, "   - MaxEntries = %u\n", (unsigned int)timeStampHashTable->MaxEntries);
   for(i = 0;i < timeStampHashTable->Buckets;i++) {
      fprintf(fd, "   - Bucket #%u   (%u entries)\n",
              (unsigned int)i + 1,
              (unsigned int)timeStampHashTable->BucketArray[i]->Entries);
      for(j = 0;j < timeStampHashTable->BucketArray[i]->Entries;j++) {
         fprintf(fd, "      + %llu\n", timeStampHashTable->BucketArray[i]->TimeStamp[j]);
      }
   }
}


/* ###### Append a new time stamp ######################################## */
bool timeStampHashTableAddTimeStamp(struct TimeStampHashTable* timeStampHashTable,
                                    const unsigned long        hashValue,
                                    const unsigned long long   newTimeStamp)
{
   const size_t bucket = (size_t)hashValue % timeStampHashTable->Buckets;
   size_t       i;

   if(timeStampHashTable->BucketArray[bucket]->Entries < timeStampHashTable->MaxEntries) {
      if( (timeStampHashTable->BucketArray[bucket]->Entries == 0) ||
          (timeStampHashTable->BucketArray[bucket]->TimeStamp[timeStampHashTable->BucketArray[bucket]->Entries - 1] <= newTimeStamp) ) {
         timeStampHashTable->BucketArray[bucket]->TimeStamp[timeStampHashTable->BucketArray[bucket]->Entries] = newTimeStamp;
         timeStampHashTable->BucketArray[bucket]->Entries++;
      }
      else {
         /* Non-monotonic time stamp order! */
         return(false);
      }
   }
   else {
      if(timeStampHashTable->BucketArray[bucket]->TimeStamp[timeStampHashTable->BucketArray[bucket]->Entries - 1] <= newTimeStamp) {
         for(i = 1;i < timeStampHashTable->BucketArray[bucket]->Entries;i++) {
            timeStampHashTable->BucketArray[bucket]->TimeStamp[i - 1] =
               timeStampHashTable->BucketArray[bucket]->TimeStamp[i];
         }
         timeStampHashTable->BucketArray[bucket]->TimeStamp[timeStampHashTable->BucketArray[bucket]->Entries - 1] = newTimeStamp;
      }
      else {
         /* Non-monotonic time stamp order! */
         return(false);
      }
   }
   return(true);
}


/* ###### Get time stamp rate ############################################ */
double timeStampHashTableGetRate(const struct TimeStampHashTable* timeStampHashTable,
                                 const unsigned long              hashValue)
{
   const size_t bucket = (size_t)hashValue % timeStampHashTable->Buckets;
   if(timeStampHashTable->BucketArray[bucket]->Entries > 1) {
      const unsigned long long duration =
         timeStampHashTable->BucketArray[bucket]->TimeStamp[timeStampHashTable->BucketArray[bucket]->Entries - 1] -
         timeStampHashTable->BucketArray[bucket]->TimeStamp[0];
      return((double)timeStampHashTable->BucketArray[bucket]->Entries / (duration / 1000000.0));
   }
   return(0.0);
}
