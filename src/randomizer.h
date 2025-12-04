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

#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include "tdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
  * Get 8-bit random value.
  *
  * @return Random value.
  */
uint8_t random8();

/**
  * Get162-bit random value.
  *
  * @return Random value.
  */
uint16_t random16();

/**
  * Get 32-bit random value.
  *
  * @return Random value.
  */
uint32_t random32();

/**
  * Get 64-bit random value.
  *
  * @return Random value.
  */
uint64_t random64();

/**
  * Get double random value out of interval [0,1).
  *
  * @return Random value.
  */
double randomDouble();

/**
  * Get exponential-distributed random value.
  *
  * @param p Average value.
  * @return Random value.
  */
double randomExpDouble(const double p);


#ifdef __cplusplus
}
#endif


#endif
