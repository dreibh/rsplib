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
 * Copyright (C) 2003-2021 by Thomas Dreibholz
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

#include "poolhandle.h"
#include "debug.h"
#include "stringutilities.h"


/* ###### Initialize ##################################################### */
void poolHandleNew(struct PoolHandle*   poolHandle,
                   const unsigned char* handle,
                   const size_t         size)
{
   CHECK(size > 0);
   CHECK(size <= MAX_POOLHANDLESIZE);
   poolHandle->Size = size;
   memcpy(&poolHandle->Handle, handle, size);
}


/* ###### Invalidate ##################################################### */
void poolHandleDelete(struct PoolHandle* poolHandle)
{
   poolHandle->Size = 0;
}


/* ###### Get textual description ######################################## */
void poolHandleGetDescription(const struct PoolHandle* poolHandle,
                              char*                    buffer,
                              const size_t             bufferSize)
{
   char   result[8];
   size_t i;

   buffer[0] = 0x00;
   for(i = 0;i < ((poolHandle->Size <= MAX_POOLHANDLESIZE) ? poolHandle->Size : MAX_POOLHANDLESIZE);i++) {
      if(!iscntrl(poolHandle->Handle[i])) {
         result[0] = poolHandle->Handle[i];
         result[1] = 0x00;
         safestrcat(buffer, result, bufferSize);
      }
      else {
         snprintf((char*)&result, sizeof(result), "{%02x}", poolHandle->Handle[i]);
         safestrcat(buffer, result, bufferSize);
      }
   }
}


/* ###### Print pool handle ############################################## */
void poolHandlePrint(const struct PoolHandle* poolHandle,
                     FILE*                    fd)
{
   char poolHandleDescription[128];
   poolHandleGetDescription(poolHandle,
                            poolHandleDescription, sizeof(poolHandleDescription));
   fputs(poolHandleDescription, fd);
}


/* ###### Pool Handle comparison ######################################### */
int poolHandleComparison(const struct PoolHandle* poolHandle1,
                         const struct PoolHandle* poolHandle2)
{
   if(poolHandle1->Size < poolHandle2->Size) {
      return(-1);
   }
   if(poolHandle1->Size > poolHandle2->Size) {
      return(1);
   }
   return(memcmp(poolHandle1->Handle, poolHandle2->Handle,
                 poolHandle1->Size));
}
