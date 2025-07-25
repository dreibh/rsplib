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

#ifndef TDTYPES_H
#define TDTYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#if defined(__FreeBSD__)
#include <machine/endian.h>
#endif
#if defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifndef __cplusplus
#ifndef max
#define max(a, b) ((a) < (b) ? (b) : (a))
#endif
#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

#endif

#endif
