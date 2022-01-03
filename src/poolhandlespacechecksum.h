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

#ifndef POOLHANDLESPACECHECKSUM_H
#define POOLHANDLESPACECHECKSUM_H


#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef uint16_t HandlespaceChecksumType;
typedef uint32_t HandlespaceChecksumAccumulatorType;
#define HandlespaceChecksumBits (sizeof(HandlespaceChecksumType) * 8)
#define HandlespaceChecksumMask ((HandlespaceChecksumType)-1)

#define INITIAL_HANDLESPACE_CHECKSUM 0


HandlespaceChecksumAccumulatorType handlespaceChecksumAdd(const HandlespaceChecksumAccumulatorType a,
                                                          const HandlespaceChecksumAccumulatorType b);
HandlespaceChecksumAccumulatorType handlespaceChecksumSub(const HandlespaceChecksumAccumulatorType a,
                                                          const HandlespaceChecksumAccumulatorType b);
HandlespaceChecksumAccumulatorType handlespaceChecksumCompute(HandlespaceChecksumAccumulatorType sum,
                                                              const char*                        buffer,
                                                              size_t                             size);
HandlespaceChecksumType handlespaceChecksumFinish(HandlespaceChecksumAccumulatorType sum);


#ifdef __cplusplus
}
#endif

#endif
