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
 * Copyright (C) 2003-2012 by Thomas Dreibholz
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

#ifndef TIMESTAMPHASHTABLE_H
#define TIMESTAMPHASHTABLE_H

#include "tdtypes.h"

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


struct TimeStampBucket
{
   size_t             Entries;
   unsigned long long TimeStamp[0];
};

struct TimeStampHashTable
{
   size_t                  Buckets;
   size_t                  MaxEntries;
   struct TimeStampBucket* BucketArray[0];
};


struct TimeStampHashTable* timeStampHashTableNew(const size_t buckets,
                                                 const size_t maxEntries);
void timeStampHashTableDelete(struct TimeStampHashTable* timeStampHashTable);
void timeStampHashTableClear(struct TimeStampHashTable* timeStampHashTable);
void timeStampHashTablePrint(struct TimeStampHashTable* timeStampHashTable,
                             FILE*                      fd);
bool timeStampHashTableAddTimeStamp(struct TimeStampHashTable* timeStampHashTable,
                                    const unsigned long        hashValue,
                                    const unsigned long long   newTimeStamp);
double timeStampHashTableGetRate(const struct TimeStampHashTable* timeStampHashTable,
                                 const unsigned long              hashValue);


#ifdef __cplusplus
}
#endif

#endif
