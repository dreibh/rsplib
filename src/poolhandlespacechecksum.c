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
 * Copyright (C) 2003-2026 by Thomas Dreibholz
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

#include "poolhandlespacechecksum.h"
#include "debug.h"


/* ###### Add checksums a and b ########################################## */
HandlespaceChecksumAccumulatorType handlespaceChecksumAdd(const HandlespaceChecksumAccumulatorType a,
                                                          const HandlespaceChecksumAccumulatorType b)
{
   HandlespaceChecksumAccumulatorType sum = a + b;
   return(sum);
}


/* ###### Subtract checksum b from a ##################################### */
HandlespaceChecksumAccumulatorType handlespaceChecksumSub(const HandlespaceChecksumAccumulatorType a,
                                                          const HandlespaceChecksumAccumulatorType b)
{
   HandlespaceChecksumAccumulatorType sum = a - b;
   return(sum);
}


/* ###### Compute handlespace checksum ################################### */
HandlespaceChecksumAccumulatorType handlespaceChecksumCompute(HandlespaceChecksumAccumulatorType sum,
                                                              const char*                        buffer,
                                                              size_t                             size)
{
   HandlespaceChecksumType* addr = (HandlespaceChecksumType*)buffer;
   HandlespaceChecksumType  tmp;

   while(size >= sizeof(*addr)) {
      sum += *addr++;
      size -= sizeof(*addr);
   }

   if(size > 0) {
      tmp = 0;
      memcpy(&tmp, addr, size);
      sum += tmp;
   }

   return(sum);
}


/* ###### Strip off carry part of the checksum ########################### */
HandlespaceChecksumType handlespaceChecksumFinish(HandlespaceChecksumAccumulatorType sum)
{
   while(sum >> HandlespaceChecksumBits) {
      sum = (sum & HandlespaceChecksumMask) + (sum >> HandlespaceChecksumBits);
   }
   return((HandlespaceChecksumType)~sum);
}
